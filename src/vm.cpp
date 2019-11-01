#include "vm.h"

#include "compiler.h"
#include "debug.h"
#include "object_t.h"

namespace Clox {

#define BINARY_OP(op) \
do{\
		if(!peek(0).is_number() || !peek(1).is_number()){\
			runtime_error("Operands must be numbers.");\
			return InterpretResult::RuntimeError;\
		}\
		double b = pop().as<double>(); \
		double a = pop().as<double>(); \
		push(a op b); \
} while (false)

constexpr bool is_falsey(const Value& value)
{
	return value.is_nil() ||
		(value.is_bool() && !value.as<bool>());
}

InterpretResult VM::interpret(std::string_view source)
{
	if (!Compilation::compile(source, *this))
		return InterpretResult::CompileError;
	return run();
}

InterpretResult VM::run()
{
	while (true)
	{
#ifdef _DEBUG
		std::cout << "          ";
		for (size_t slot = 0; slot < stacktop; ++slot)
		{
			std::cout << "[ " << stack.at(slot) << " ]";
		}
		std::cout << '\n';
		disassemble_instruction(chunk, ip);
#endif // _DEBUG

		auto instruction = read_byte();
		switch (instruction)
		{
			case OpCode::Constant:
			{
				auto&& constant = read_constant();
				push(constant);
				break;
			}
			case OpCode::Nil: push(Value()); break;
			case OpCode::True: push(true); break;
			case OpCode::False: push(false); break;
			case OpCode::Pop: pop(); break;
			case OpCode::GetLocal:
			{
				auto slot = static_cast<size_t>(read_byte());
				push(stack.at(slot));
				break;
			}
			case OpCode::SetLocal:
			{
				auto slot = static_cast<size_t>(read_byte());
				stack.at(slot) = peek(0);
				break;
			}
			case OpCode::GetGlobal:
			{
				auto name = read_string();
				try
				{
					auto& value = globals.at(name);
					push(value);
				} catch (const std::out_of_range&)
				{
					runtime_error("Undefined variable ", name->to_string());
					return InterpretResult::RuntimeError;
				}
				break;
			}
			case OpCode::DefineGlobal:
			{
				auto name = read_string();
				globals.insert_or_assign(name, peek(0));
				pop();
				break;
			}
			case OpCode::SetGlobal:
			{
				auto name = read_string();
				try
				{
					globals.at(name) = peek(0);
				} catch (const std::out_of_range&)
				{
					runtime_error("Undefined variable ", name->to_string());
					return InterpretResult::RuntimeError;
				}
				break;
			}
			case OpCode::Equal:
			{
				auto b = pop();
				auto a = pop();
				push(a == b);
				break;
			}
			case OpCode::Greater: BINARY_OP(> ); break;
			case OpCode::Less: BINARY_OP(< ); break;
			case OpCode::Add:
			{
				if (peek(0).is_string() && peek(1).is_string())
				{
					auto b = pop().as_string();
					auto a = pop().as_string();
					push(create_obj_string((*a) + (*b), *this));
				} else if (peek(0).is_number() && peek(1).is_number())
				{
					auto b = pop().as<double>();
					auto a = pop().as<double>();
					push(a + b);
				} else
				{
					runtime_error("Operands must be two numbers or two strings.");
					return InterpretResult::RuntimeError;
				}
				break;
			}
			case OpCode::Subtract: BINARY_OP(-); break;
			case OpCode::Multiply: BINARY_OP(*); break;
			case OpCode::Divide: BINARY_OP(/ ); break;
			case OpCode::Not:push(is_falsey(pop())); break;
			case OpCode::Negate:
				if (!peek(0).is_number())
				{
					runtime_error("Operand must be a number.");
					return InterpretResult::RuntimeError;
				}
				push(-pop().as<double>());
				break;
			case OpCode::Print:
				std::cout << "~$ " << pop() << '\n';
				break;
			case OpCode::Jump:
			{
				auto offset = read_short();
				ip += offset;
				break;
			}
			case OpCode::JumpIfFalse:
			{
				auto offset = read_short();
				if (is_falsey(peek(0)))
					ip += offset;
				break;
			}
			case OpCode::Loop:
			{
				auto offset = read_short();
				ip -= offset;
				break;
			}
			case OpCode::Return:
				return InterpretResult::Ok;
			default:
				break;
		}
	}
}

const Value& VM::peek(size_t distance) const
{
	return stack.at(stacktop - 1 - distance);
}

Value VM::pop()
{
	if (stacktop > 0)
		stacktop--;
	return stack.at(stacktop);
}

void VM::push(Value value)
{
	stack.at(stacktop) = std::move(value);
	stacktop++;
}

OpCode VM::read_byte()
{
	auto code = chunk.code.at(ip);
	ip++;
	return code;
}

Value VM::read_constant()
{
	return chunk.constants.values.at(static_cast<size_t>(read_byte()));
}

uint16_t VM::read_short()
{
	ip += 2;
	auto a = static_cast<uint8_t>(chunk.code.at(ip - 2)) << 8;
	auto b = static_cast<uint8_t>(chunk.code.at(ip - 1));
	return static_cast<uint16_t>(a | b);
}

ObjString* VM::read_string()
{
	return read_constant().as_string();
}

} //Clox