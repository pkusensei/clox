#include "debug.h"

#include <iomanip>
#include <iostream>

#include "object.h"

namespace Clox {

void disassemble_chunk(const Chunk& chunk, std::string_view name)
{
	std::cout << "== " << name << " ==\n";
	for (size_t offset = 0; offset < chunk.count();)
	{
		offset = disassemble_instruction(chunk, offset);
	}
}

[[nodiscard]] size_t byte_instruction(std::string_view name, const Chunk& chunk, size_t offset)
{
	auto slot = chunk.code.at(offset + 1);
	std::cout << std::setfill(' ') << std::left << std::setw(16) << name << ' ';
	std::cout << std::setw(4) << static_cast<unsigned>(slot) << '\n';
	return offset + 2;
}

[[nodiscard]] size_t constant_instruction(std::string_view name, const Chunk& chunk, size_t offset)
{
	auto constant = chunk.code.at(offset + 1);
	std::cout << std::setfill(' ') << std::left << std::setw(16) << name << ' ';
	std::cout << std::setw(4) << static_cast<unsigned>(constant) << ' ';
	std::cout << chunk.constants.values.at(static_cast<size_t>(constant)) << '\n';
	return offset + 2;
}

[[nodiscard]] size_t jump_instruction(std::string_view name, int sign, const Chunk& chunk, size_t offset)
{
	auto jump = static_cast<uint16_t>(chunk.code.at(offset + 1) << 8);
	jump |= chunk.code.at(offset + 2);
	std::cout << std::setfill(' ') << std::left << std::setw(16) << name << ' ';
	std::cout << std::setw(4) << offset << " -> ";
	std::cout << offset + 3 + sign * jump << '\n';
	return offset + 3;
}

[[nodiscard]] size_t simple_instruction(std::string_view name, size_t offset)
{
	std::cout << name << '\n';
	return offset + 1;
}

size_t disassemble_instruction(const Chunk& chunk, size_t offset)
{
	std::cout << std::setfill('0') << std::right << std::setw(4) << offset << ' ';
	if (offset >= chunk.count())return offset - 1;
	if (offset > 0 && chunk.lines.at(offset) == chunk.lines.at(offset - 1))
	{
		std::cout << "   | ";
	} else
	{
		std::cout << std::setfill(' ') << std::setw(4) << chunk.lines.at(offset) << ' ';
	}

	auto instruction = static_cast<OpCode>(chunk.code.at(offset));
	switch (instruction)
	{
		case OpCode::Call:
		case OpCode::GetLocal:
		case OpCode::SetLocal:
		case OpCode::GetUpvalue:
		case OpCode::SetUpvalue:
			return byte_instruction(nameof(instruction), chunk, offset);
		case OpCode::Constant:
		case OpCode::GetGlobal:
		case OpCode::DefineGlobal:
		case OpCode::SetGlobal:
			return constant_instruction(nameof(instruction), chunk, offset);
		case OpCode::Jump:
		case OpCode::JumpIfFalse:
			return jump_instruction(nameof(instruction), 1, chunk, offset);
		case OpCode::Loop:
			return jump_instruction(nameof(instruction), -1, chunk, offset);
		case OpCode::Nil:
		case OpCode::True:
		case OpCode::False:
		case OpCode::Pop:
		case OpCode::Equal:
		case OpCode::Greater:
		case OpCode::Less:
		case OpCode::Add:
		case OpCode::Subtract:
		case OpCode::Multiply:
		case OpCode::Divide:
		case OpCode::Not:
		case OpCode::Negate:
		case OpCode::Print:
		case OpCode::CloseUpvalue:
		case OpCode::Return:
			return simple_instruction(nameof(instruction), offset);
		case OpCode::Closure:
		{
			offset++;
			auto constant = chunk.code.at(offset++);
			std::cout << std::setfill(' ') << std::left << std::setw(16) << nameof(OpCode::Closure) << ' ';
			std::cout << std::setw(4) << static_cast<unsigned>(constant) << ' ';
			std::cout << chunk.constants.values.at(constant) << '\n';

			auto function = chunk.constants.values.at(constant).as_function();
			for (size_t j = 0; j < function->upvalue_count; j++)
			{
				auto is_local = chunk.code.at(offset++);
				auto index = chunk.code.at(offset++);
				std::cout << std::setfill('0') << std::right << std::setw(4) << offset - 2;
				std::cout << "      |                     ";
				std::cout << (is_local > 0 ? "local" : "upvalue");
				std::cout << ' ' << static_cast<unsigned>(index) << '\n';
			}
			return offset;
		}
		default:
			std::cout << "Unknown OpCode " << instruction;
			return offset + 1;
	}
}

} //Clox