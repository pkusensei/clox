#pragma once

#include <iostream>
#include <variant>
#include <vector>

#define NAN_BOXING

namespace Clox {

struct Obj;

template<typename T>
struct Allocator;

#ifdef NAN_BOXING

constexpr uint64_t SIGN_BIT = 0x8000000000000000;
constexpr uint64_t QNAN = 0x7ffc000000000000;

constexpr uint64_t TAG_NIL = 1;
constexpr uint64_t TAG_FALSE = 2;
constexpr uint64_t TAG_TRUE = 3;

constexpr uint64_t NIL_VAL = (QNAN | TAG_NIL);
constexpr uint64_t FALSE_VAL = (QNAN | TAG_FALSE);
constexpr uint64_t TRUE_VAL = (QNAN | TAG_TRUE);

#endif // NAN_BOXING

struct Value
{
#ifdef NAN_BOXING

	uint64_t value;

	union DoubleUnion
	{
		uint64_t bits;
		double num;
	};

	constexpr Value(bool b) noexcept
		: value(b ? TRUE_VAL : FALSE_VAL)
	{
	}
	constexpr Value() noexcept : value(QNAN | TAG_NIL) {}
	Value(double num) noexcept;
	Value(Obj* obj) noexcept
		: value(SIGN_BIT | QNAN | static_cast<uint64_t>(reinterpret_cast<uintptr_t>(obj)))
	{
	}

	[[nodiscard]] constexpr bool is_bool()const noexcept
	{
		return (value & FALSE_VAL) == FALSE_VAL;
	}

	[[nodiscard]] constexpr bool is_nil()const noexcept
	{
		return value == (QNAN | TAG_NIL);
	}

	[[nodiscard]] constexpr bool is_number()const noexcept
	{
		return (value & QNAN) != QNAN;
	}

	[[nodiscard]] constexpr bool is_obj()const noexcept
	{
		return (value & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT);
	}

	template<typename T>
	[[nodiscard]] constexpr T as()const
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return value == TRUE_VAL;
		} else if constexpr (std::is_same_v<T, double>)
		{
			DoubleUnion data;
			data.bits = value;
			return data.num;
		} else if constexpr (std::is_same_v<T, Obj*>)
		{
			return reinterpret_cast<Obj*>(static_cast<uintptr_t>(value & ~(SIGN_BIT | QNAN)));
		}
	}

#else

	using var_t = std::variant<bool, std::monostate, double, Obj*>;

	var_t value;

	constexpr Value(bool value) noexcept : value(value) {}
	constexpr Value() noexcept : value(std::monostate()) {}
	constexpr Value(double value) noexcept : value(value) {}
	constexpr Value(Obj* obj) noexcept : value(obj) {}

	[[nodiscard]] constexpr bool is_bool()const noexcept { return std::holds_alternative<bool>(value); }
	[[nodiscard]] constexpr bool is_nil()const noexcept { return std::holds_alternative<std::monostate>(value); }
	[[nodiscard]] constexpr bool is_number()const noexcept { return std::holds_alternative<double>(value); }
	[[nodiscard]] constexpr bool is_obj()const noexcept { return std::holds_alternative<Obj*>(value); }

	template<typename T>
	[[nodiscard]] constexpr T as()const
	{
		return std::get<T>(value);
	}

#endif //NAN_BOXING

	template<typename U>
	[[nodiscard]] auto is_obj_type()const
		->typename std::enable_if_t<std::is_base_of_v<Obj, U> && !std::is_same_v<Obj, U>, bool>
	{
		if (is_obj())
		{
			auto obj = as<Obj*>();
			return obj->is_type(objtype_of<U>());
		}
		return false;
	}

	template<typename U>
	[[nodiscard]] auto as_obj()const
		->typename std::enable_if_t<std::is_base_of_v<Obj, U> && !std::is_same_v<Obj, U>, U*>
	{
		try
		{
			if (is_obj_type<U>())
				return static_cast<U*>(as<Obj*>());
			else throw;
		} catch (...)
		{
			throw std::invalid_argument(std::string("Value is not a(n) ")
				+ nameof<U>().data());
		}
	}

};

[[nodiscard]] constexpr bool operator==(const Value& v1, const Value& v2)noexcept
{
#ifdef NAN_BOXING
	if (v1.is_number() && v2.is_number())
		return v1.as<double>() == v2.as<double>();
#endif // NAN_BOXING

	return v1.value == v2.value;
}
[[nodiscard]] constexpr bool operator!=(const Value& v1, const Value& v2)noexcept
{
	return !(v1 == v2);
}


std::ostream& operator<<(std::ostream& out, const Value& value);

template<template<typename>typename Alloc = Allocator>
struct ValueArray
{
	std::vector<Value, Alloc<Value>> values;

	[[nodiscard]] constexpr size_t count()const noexcept { return values.size(); }

	template<typename T>
	typename std::enable_if_t<std::is_convertible_v<T, Value>, void>
		write(T&& value)
	{
		values.emplace_back(std::forward<T>(value));
	}
};

} //Clox