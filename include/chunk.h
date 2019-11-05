#pragma once

#include <iostream>
#include <string_view>
#include <vector>

#include "value.h"

namespace Clox {

constexpr auto UINT8_COUNT = UINT8_MAX + 1;

enum class OpCode :uint8_t
{
	Constant,
	Nil,
	True,
	False,
	Pop,
	GetLocal,
	SetLocal,
	GetGlobal,
	DefineGlobal,
	SetGlobal,
	Equal,
	Greater,
	Less,
	Add,
	Subtract,
	Multiply,
	Divide,
	Not,
	Negate,
	Print,
	Jump,
	JumpIfFalse,
	Loop,
	Call,
	Return
};

std::string_view nameof(OpCode code);
std::ostream& operator<<(std::ostream& out, OpCode code);

template<typename T>
constexpr bool opcode_trait = std::is_same_v<uint8_t, T> || std::is_same_v<OpCode, T>;

struct Chunk
{
	std::vector<OpCode> code;
	std::vector<size_t> lines;
	ValueArray constants;

	constexpr size_t count()const noexcept { return code.size(); }

	template<typename T>
	typename std::enable_if_t<std::is_convertible_v<T, Value>, size_t>
		add_constant(T&& value)
	{
		constants.write(std::forward<T>(value));
		return constants.count() - 1;
	}

	template<typename T>
	typename std::enable_if_t<opcode_trait<T>, void>
		write(T byte, size_t line)
	{
		code.push_back(static_cast<OpCode>(byte));
		lines.push_back(line);
	}
};

} // Clox