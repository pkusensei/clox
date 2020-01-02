#pragma once

#include <iostream>
#include <string_view>
#include <vector>

#include "memory.h"
#include "value.h"

namespace Clox {

constexpr auto UINT8_COUNT = std::numeric_limits<uint8_t>::max() + 1;

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
	GetUpvalue,
	SetUpvalue,
	GetProperty,
	SetProperty,
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
	Closure,
	CloseUpvalue,
	Return,
	Class
};

std::string_view nameof(OpCode code);
std::ostream& operator<<(std::ostream& out, OpCode code);

namespace {
template<typename T>
using opcode_trait_helper = std::disjunction<std::is_same<uint8_t, T>, std::is_same<OpCode, T>>;
}

template<typename... Ts>
using opcode_trait = std::conjunction<opcode_trait_helper<Ts>...>;

template<typename... Ts>
constexpr bool opcode_trait_v = opcode_trait<Ts...>::value;

namespace {

template<template<typename>typename Alloc = Allocator>
struct ChunkT
{
	std::vector<uint8_t, Alloc<uint8_t>> code;
	std::vector<size_t, Alloc<size_t>> lines;
	ValueArray<Alloc> constants;

	[[nodiscard]] constexpr size_t count()const noexcept { return code.size(); }

	template<typename T>
	[[nodiscard]] typename std::enable_if_t<std::is_convertible_v<T, Value>, size_t>
		add_constant(T&& value)
	{
		constants.write(std::forward<T>(value));
		return constants.count() - 1;
	}

	template<typename T>
	typename std::enable_if_t<opcode_trait_v<T>, void>
		write(T byte, size_t line)
	{
		code.push_back(static_cast<uint8_t>(byte));
		lines.push_back(line);
	}
};

}

struct Chunk final :public ChunkT<>
{
};

} // Clox