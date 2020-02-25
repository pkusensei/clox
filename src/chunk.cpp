#include "chunk.h"

namespace Clox {

std::string_view nameof(OpCode code)
{
	using namespace std::literals;

	switch (code)
	{
		case OpCode::Constant: return "OpConstant"sv;
		case OpCode::Nil: return "OpNil"sv;
		case OpCode::True: return "OpTrue"sv;
		case OpCode::False: return "OpFalse"sv;
		case OpCode::Pop: return "OpPop"sv;
		case OpCode::GetLocal: return "OpGetLocal"sv;
		case OpCode::SetLocal: return "OpSetLocal"sv;
		case OpCode::GetGlobal: return "OpGetGlobal"sv;
		case OpCode::DefineGlobal: return "OpDefineGlobal"sv;
		case OpCode::SetGlobal: return "OpSetGlobal"sv;
		case OpCode::GetUpvalue: return "OpGetUpvalue"sv;
		case OpCode::SetUpvalue: return "OpSetUpvalue"sv;
		case OpCode::GetProperty: return "OpGetProperty"sv;
		case OpCode::SetProperty: return "OpSetProperty"sv;
		case OpCode::Equal: return "OpEqual"sv;
		case OpCode::Greater: return "OpGreater"sv;
		case OpCode::Less: return "OpLess"sv;
		case OpCode::Add: return "OpAdd"sv;
		case OpCode::Subtract: return "OpSubtract"sv;
		case OpCode::Multiply: return "OpMultiply"sv;
		case OpCode::Divide: return "OpDivide"sv;
		case OpCode::Not: return "OpNot"sv;
		case OpCode::Negate: return "OpNegate"sv;
		case OpCode::Print: return "OpPrint"sv;
		case OpCode::Jump: return "OpJump"sv;
		case OpCode::JumpIfFalse: return "OpJumpIfFalse"sv;
		case OpCode::Loop: return "OpLoop"sv;
		case OpCode::Call: return "OpCall"sv;
		case OpCode::Closure: return "OpClosure"sv;
		case OpCode::CloseUpvalue: return "OpCloseUpvalue"sv;
		case OpCode::Return: return "OpReturn"sv;
		case OpCode::Class: return "OpClass"sv;
		case OpCode::Method: return "OpMethod"sv;
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