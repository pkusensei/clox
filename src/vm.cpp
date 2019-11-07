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
	auto function = Compilation::compile(source, *this);
	if (function == nullptr)
		return InterpretResult::CompileError;

	push(function);
	call(function, 0);
	return run();
}

VM::VM()
{
	reset_stack();
}

InterpretResult VM::run()
{
	auto* frame = &frames.at(frame_count - 1);

	while (true)
	{
		//#ifdef _DEBUG
		//		std::cout << "          ";
		//		for (auto slot = stack.data(); slot < stacktop; ++slot)
		//		{
		//			std::cout << "[ " << *slot << " ]";
		//		}
		//		std::cout << '\n';
		//		disassemble_instruction(frame.chunk(), frame.ip);
		//#endif // _DEBUG

		auto instruction = static_cast<OpCode>(frame->read_byte());
		switch (instruction)
		{
			case OpCode::Constant:
			{
				auto&& constant = frame->read_constant();
				push(constant);
				break;
			}
			case OpCode::Nil: push(Value()); break;
			case OpCode::True: push(true); break;
			case OpCode::False: push(false); break;
			case OpCode::Pop: pop(); break;
			case OpCode::GetLocal:
			{
				auto slot = static_cast<size_t>(frame->read_byte());
				push(frame->slots[slot]);
				break;
			}
			case OpCode::SetLocal:
			{
				auto slot = static_cast<size_t>(frame->read_byte());
				frame->slots[slot] = peek(0);
				break;
			}
			case OpCode::GetGlobal:
			{
				auto name = frame->read_string();
				try
				{
					auto& value = globals.at(name);
					push(value);
				} catch (const std::out_of_range&)
				{
					runtime_error("Undefined variable ", name->text());
					return InterpretResult::RuntimeError;
				}
				break;
			}
			case OpCode::DefineGlobal:
			{
				auto name = frame->read_string();
				globals.insert_or_assign(name, peek(0));
				pop();
				break;
			}
			case OpCode::SetGlobal:
			{
				auto name = frame->read_string();
				try
				{
					globals.at(name) = peek(0);
				} catch (const std::out_of_range&)
				{
					runtime_error("Undefined variable ", name->text());
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
				auto offset = frame->read_short();
				frame->ip += offset;
				break;
			}
			case OpCode::JumpIfFalse:
			{
				auto offset = frame->read_short();
				if (is_falsey(peek(0)))
					frame->ip += offset;
				break;
			}
			case OpCode::Loop:
			{
				auto offset = frame->read_short();
				frame->ip -= offset;
				break;
			}
			case OpCode::Call:
			{
				auto arg_count = static_cast<uint8_t>(frame->read_byte());
				if (!call_value(peek(arg_count), arg_count))
					return InterpretResult::RuntimeError;
				frame = &frames.at(frame_count - 1);
				break;
			}
			case OpCode::Return:
			{
				auto result = pop();
				frame_count--;
				if (frame_count == 0)
				{
					pop();
					return InterpretResult::Ok;
				}
				stacktop = frame->slots;
				push(result);
				frame = &frames.at(frame_count - 1);
				break;
			}
			default:
				break;
		}
	}
}

bool VM::call(const ObjFunction* function, uint8_t arg_count)
{
	if (arg_count != function->arity)
	{
		runtime_error("Expected ", function->arity, " arguments but got ",
			static_cast<unsigned>(arg_count));
		return false;
	}
	if (frame_count == FRAME_MAX)
	{
		runtime_error("Stack overflow");
		return false;
	}
	auto& frame = frames.at(frame_count++);
	frame.function = function;
	frame.ip = 0;
	frame.slots = stacktop - arg_count - 1;
	return true;
}

bool VM::call_value(const Value& callee, uint8_t arg_count)
{
	if (callee.is_obj())
	{
		switch (callee.as<Obj*>()->type)
		{
			case ObjType::Function:
				return call(callee.as_function(), arg_count);
			default:
				break;
		}
	}
	runtime_error("Can only call functions and classes.");
	return false;
}

const Value& VM::peek(size_t distance) const
{
	return stacktop[-1 - distance];
}

Value VM::pop()
{
	stacktop--;
	return *stacktop;
}

void VM::push(Value value)
{
	*stacktop = std::move(value);
	stacktop++;
}

void VM::reset_stack() noexcept
{
	stacktop = stack.data();
	frame_count = 0;
}

uint8_t CallFrame::read_byte()
{
	auto code = chunk().code.at(ip);
	ip++;
	return code;
}

Value CallFrame::read_constant()
{
	return chunk().constants.values.at(read_byte());
}

uint16_t CallFrame::read_short()
{
	ip += 2;
	auto a = static_cast<uint8_t>(chunk().code.at(ip - 2)) << 8;
	auto b = static_cast<uint8_t>(chunk().code.at(ip - 1));
	return static_cast<uint16_t>(a | b);
}

ObjString* CallFrame::read_string()
{
	return read_constant().as_string();
}

} //Clox