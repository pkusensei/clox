#include "chunk.h"

namespace Clox {

std::string_view nameof(OpCode code)
{
	switch (code)
	{
		case OpCode::Constant: return "OpConstant";
		case OpCode::Nil: return "OpNil";
		case OpCode::True: return "OpTrue";
		case OpCode::False: return "OpFalse";
		case OpCode::Pop: return "OpPop";
		case OpCode::GetLocal: return "OpGetLocal";
		case OpCode::SetLocal: return "OpSetLocal";
		case OpCode::GetGlobal: return "OpGetGlobal";
		case OpCode::DefineGlobal: return "OpDefineGlobal";
		case OpCode::SetGlobal: return "OpSetGlobal";
		case OpCode::GetUpvalue: return "OpGetUpvalue";
		case OpCode::SetUpvalue: return "OpSetUpvalue";
		case OpCode::GetProperty: return "OpGetProperty";
		case OpCode::SetProperty: return "OpSetProperty";
		case OpCode::GetSuper: return "OpGetSuper";
		case OpCode::Equal: return "OpEqual";
		case OpCode::Greater: return "OpGreater";
		case OpCode::Less: return "OpLess";
		case OpCode::Add: return "OpAdd";
		case OpCode::Subtract: return "OpSubtract";
		case OpCode::Multiply: return "OpMultiply";
		case OpCode::Divide: return "OpDivide";
		case OpCode::Not: return "OpNot";
		case OpCode::Negate: return "OpNegate";
		case OpCode::Print: return "OpPrint";
		case OpCode::Jump: return "OpJump";
		case OpCode::JumpIfFalse: return "OpJumpIfFalse";
		case OpCode::Loop: return "OpLoop";
		case OpCode::Call: return "OpCall";
		case OpCode::Invoke: return "OpInvoke";
		case OpCode::SuperInvoke:return "OpSuperInvoke";
		case OpCode::Closure: return "OpClosure";
		case OpCode::CloseUpvalue: return "OpCloseUpvalue";
		case OpCode::Return: return "OpReturn";
		case OpCode::Class: return "OpClass";
		case OpCode::Inherit: return "OpInherit";
		case OpCode::Method: return "OpMethod";
		default:
			throw std::invalid_argument("Unexpected OpCode: nameof");
	}
}

std::ostream& operator<<(std::ostream& out, OpCode code)
{
	out << nameof(code);
	return out;
}

} //Clox