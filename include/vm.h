#pragma once

#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <set>

#include "chunk.h"

namespace Clox {

constexpr auto STACK_MAX = 256;

template<typename T, typename... Args>
void err_print(T&& t, Args&&... args)
{
	std::cerr << std::forward<T>(t);
	err_print(std::forward<Args>(args)...);
}

template<typename T>
void err_print(T&& t)
{
	std::cerr << std::forward<T>(t);
}

enum class InterpretResult
{
	Ok,
	CompileError,
	RuntimeError
};

struct VM
{
	Chunk chunk;
	size_t ip = 0;
	std::unique_ptr<Obj, ObjDeleter> objects = nullptr;
	std::array<Value, STACK_MAX> stack = {};
	size_t stacktop = 0;
	std::map<ObjString*, Value> globals;
	std::set<ObjString*> strings;

	InterpretResult interpret(std::string_view source);

private:
	InterpretResult run();

	const Value& peek(size_t distance)const;
	Value pop();
	void push(Value value);

	void reset_stack()noexcept { stacktop = 0; }
	OpCode read_byte();
	Value read_constant();
	uint16_t read_short();
	ObjString* read_string();

	template<typename... Args>
	void runtime_error(Args&&... args)
	{
		err_print(std::forward<Args>(args)...);
		std::cerr << '\n';
		auto line = chunk.lines.at(ip);
		std::cerr << "[line " << line << "] in script\n";
		reset_stack();
	}

public:
	template<typename T>
	ObjString* find_string(const T& str)const
	{
		if (strings.empty()) return nullptr;

		auto res = std::find_if(strings.cbegin(), strings.cend(),
			[&str](const auto& it) { return it->content == str; });
		if (res != strings.end())
			return *res;
		return nullptr;
	}
};

} //Clox