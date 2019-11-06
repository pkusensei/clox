#pragma once

#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <set>

#include "chunk.h"
#include "object.h"

namespace Clox {

constexpr auto FRAME_MAX = 64;
constexpr auto STACK_MAX = FRAME_MAX * UINT8_COUNT;

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

struct CallFrame
{
	const ObjFunction* function = nullptr;
	size_t ip = 0; // index at function->chunk.code
	Value* slots = nullptr;  // pointer to VM::stack

	OpCode read_byte();
	Value read_constant();
	uint16_t read_short();
	ObjString* read_string();

	const Chunk& chunk()const noexcept { return function->chunk; }
};

struct VM
{
	std::array<CallFrame, FRAME_MAX> frames = {};
	size_t frame_count = 0;
	std::unique_ptr<Obj, ObjDeleter> objects = nullptr;
	std::array<Value, STACK_MAX> stack = {};
	Value* stacktop = nullptr;
	std::map<ObjString*, Value> globals;
	std::set<ObjString*> strings;

	InterpretResult interpret(std::string_view source);
	VM();

private:
	InterpretResult run();

	bool call(const ObjFunction* function, uint8_t arg_count);
	bool call_value(const Value& callee, uint8_t arg_count);

	const Value& peek(size_t distance)const;
	Value pop();
	void push(Value value);

	void reset_stack()noexcept;

	template<typename... Args>
	void runtime_error(Args&&... args)
	{
		err_print(std::forward<Args>(args)...);
		std::cerr << '\n';
		for (int i = frame_count - 1; i >= 0; i--)
		{
			const auto& frame = frames.at(i);
			auto function = frame.function;
			auto instruction = frame.ip - 1;
			std::cerr << "[line " << function->chunk.lines.at(instruction);
			std::cerr << "] in ";
			if (function->name == nullptr)
				std::cerr << "script\n";
			else
				std::cerr << function->name->text() << "()\n";
		}
		//const auto& frame = frames.at(frame_count - 1);
		//auto line = frame.chunk().lines.at(frame.ip);
		//std::cerr << "[line " << line << "] in script\n";
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