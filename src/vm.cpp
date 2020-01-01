#include "vm.h"

#include <chrono>

#include "obj_string.h"

#ifdef _DEBUG
//#define DEBUG_TRACE_EXECUTION
#endif // _DEBUG

#ifdef DEBUG_TRACE_EXECUTION
#include "debug.h"
#endif // DEBUG_TRACE_EXECUTION

namespace Clox {

Value clock_native([[maybe_unused]] uint8_t arg_count, [[maybe_unused]] Value* args)noexcept
{
	auto tp = std::chrono::high_resolution_clock::now().time_since_epoch();
	return std::chrono::duration<double>(tp).count();
}

[[nodiscard]] constexpr bool is_falsey(const Value& value)
{
	return value.is_nil() ||
		(value.is_bool() && !value.as<bool>());
}

InterpretResult VM::interpret(std::string_view source)
{
	auto function = cu.compile(source);
	if (function == nullptr)
		return InterpretResult::CompileError;

	push(function);
	auto closure = create_obj<ObjClosure>(gc, function);
	pop();
	push(closure);
	static_cast<void>(call_value(closure, 0));
	return run();
}

VM::VM()
	:cu(*this), gc(*this)
{
	AllocBase::init(&gc);

	reset_stack();

	define_native("clock", clock_native);
}

InterpretResult VM::run()
{

#define BINARY_OP(op) \
do{\
		if(!peek(0).is_number() || !peek(1).is_number()){\
			runtime_error("Operands must be numbers.");\
			return InterpretResult::RuntimeError;\
		}\
		double b = pop().as<double>(); \
		double a = pop().as<double>(); \
		push(a op b); \
} while (false);

	auto* frame = &frames.at(frame_count - 1);

	while (true)
	{
#ifdef DEBUG_TRACE_EXECUTION
		std::cout << "          ";
		for (auto slot = stack.data(); slot < stacktop; ++slot)
		{
			std::cout << "[ " << *slot << " ]";
		}
		std::cout << '\n';
		static_cast<void>(disassemble_instruction(frame->chunk(), frame->ip));
#endif // DEBUG_TRACE_EXECUTION

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
			case OpCode::GetUpvalue:
			{
				auto slot = frame->read_byte();
				push(*frame->closure->upvalues.at(slot)->location);
				break;
			}
			case OpCode::SetUpvalue:
			{
				auto slot = frame->read_byte();
				*frame->closure->upvalues.at(slot)->location = peek(0);
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
					auto b = peek(0).as_string();
					auto a = peek(1).as_string();
					auto res = create_obj_string((*a) + (*b), *this);
					pop();
					pop();
					push(res);
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
			case OpCode::Closure:
			{
				auto function = frame->read_constant().as_function();
				auto closure = create_obj<ObjClosure>(gc, function);
				push(closure);
				for (size_t i = 0; i < closure->upvalue_count(); i++)
				{
					auto is_local = frame->read_byte();
					auto index = frame->read_byte();
					if (is_local > 0)
						closure->upvalues.at(i) = captured_upvalue(frame->slots + index);
					else
						closure->upvalues.at(i) = frame->closure->upvalues.at(index);
				}
				break;
			}
			case OpCode::CloseUpvalue:
				close_upvalues(stacktop - 1);
				pop();
				break;
			case OpCode::Return:
			{
				auto result = pop();
				close_upvalues(frame->slots);
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
			case OpCode::Class:
				push(create_obj<ObjClass>(gc, frame->read_string()));
				break;
			default:
				break;
		}
	}

#undef BINARY_OP
}

ObjUpvalue* VM::captured_upvalue(Value* local)
{
	ObjUpvalue* prev_upvalue = nullptr;
	ObjUpvalue* upvalue = open_upvalues;

	while (upvalue != nullptr && upvalue->location > local)
	{
		prev_upvalue = upvalue;
		upvalue = upvalue->next;
	}
	if (upvalue != nullptr && upvalue->location == local)
		return upvalue;

	auto created = create_obj<ObjUpvalue>(gc, local);
	created->next = upvalue;
	if (prev_upvalue == nullptr)
		open_upvalues = created;
	else
		prev_upvalue->next = created;

	return created;
}

void VM::close_upvalues(Value* last)
{
	while (open_upvalues != nullptr && open_upvalues->location >= last)
	{
		auto upvalue = open_upvalues;
		upvalue->closed = *upvalue->location;
		upvalue->location = &upvalue->closed;
		open_upvalues = upvalue->next;
	}
}

bool VM::call(const ObjClosure* closure, uint8_t arg_count)
{
	if (arg_count != closure->function->arity)
	{
		runtime_error("Expected ", closure->function->arity, " arguments but got ",
			static_cast<unsigned>(arg_count));
		return false;
	}
	if (frame_count == FRAME_MAX)
	{
		runtime_error("Stack overflow");
		return false;
	}
	auto& frame = frames.at(frame_count++);
	frame.closure = closure;
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
			case ObjType::Closure:
				return call(callee.as_closure(), arg_count);
			case ObjType::Native:
			{
				auto native = callee.as_native()->function;
				auto result = native(arg_count, stacktop - arg_count);
				stacktop -= arg_count + 1;
				push(result);
				return true;
			}
			default:
				break;
		}
	}
	runtime_error("Can only call functions and classes.");
	return false;
}

void VM::define_native(std::string_view name, NativeFn function)
{
	push(create_obj_string(name, *this));
	push(create_obj<ObjNative>(gc, function));
	globals.insert_or_assign(stack.at(0).as_string(), stack.at(1));
	pop();
	pop();
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

const Chunk& CallFrame::chunk() const noexcept
{
	return closure->function->chunk;
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
	auto a = chunk().code.at(ip - 2) << 8;
	auto b = chunk().code.at(ip - 1);
	return static_cast<uint16_t>(a | b);
}

ObjString* CallFrame::read_string()
{
	return read_constant().as_string();
}

} //Clox