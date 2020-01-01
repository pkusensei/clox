#pragma once

#include <array>

#include "compiler.h"
#include "memory.h"
#include "object.h"

namespace Clox {

constexpr auto FRAME_MAX = 64;
constexpr auto STACK_MAX = FRAME_MAX * UINT8_COUNT;

template<typename T, typename... Args>
void err_print(T&& t, Args&&... args)
{
	std::cerr << std::forward<T>(t);
	if constexpr (sizeof...(Args) > 0)
		err_print(std::forward<Args>(args)...);
}

enum class InterpretResult
{
	Ok,
	CompileError,
	RuntimeError
};

struct CallFrame
{
	const ObjClosure* closure = nullptr;
	size_t ip = 0; // index at function->chunk.code
	Value* slots = nullptr;  // pointer to VM::stack

	[[nodiscard]] uint8_t read_byte();
	[[nodiscard]] Value read_constant();
	[[nodiscard]] uint16_t read_short();
	[[nodiscard]] ObjString* read_string();

	[[nodiscard]] const Chunk& chunk()const noexcept;
};

struct VM
{
	std::array<CallFrame, FRAME_MAX> frames;
	size_t frame_count = 0;
	std::array<Value, STACK_MAX> stack;
	Value* stacktop = nullptr;
	table globals;
	ObjUpvalue* open_upvalues = nullptr;

	Compilation cu;
	GC gc;

	InterpretResult interpret(std::string_view source);
	VM();

	Value pop();
	void push(Value value);

private:
	InterpretResult run();

	[[nodiscard]] ObjUpvalue* captured_upvalue(Value* local);
	void close_upvalues(Value* last);
	[[nodiscard]] bool call(const ObjClosure* closure, uint8_t arg_count);
	[[nodiscard]] bool call_value(const Value& callee, uint8_t arg_count);
	void define_native(std::string_view name, NativeFn function);

	[[nodiscard]] const Value& peek(size_t distance)const;

	void reset_stack()noexcept;

	template<typename... Args>
	void runtime_error(Args&&... args)
	{
		err_print(std::forward<Args>(args)...);
		std::cerr << '\n';
		for (int i = frame_count - 1; i >= 0; i--)
		{
			const auto& frame = frames.at(i);
			auto function = frame.closure->function;
			auto instruction = frame.ip - 1;
			auto line = function->chunk.lines.at(instruction);
			std::cerr << "[line " << line << "] in ";
			if (function->name == nullptr)
				std::cerr << "script\n";
			else
				std::cerr << function->name->text() << "()\n";
		}
		reset_stack();
	}
};

} //Clox