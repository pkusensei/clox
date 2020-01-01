#pragma once

#include <iostream>
#include <variant>
#include <vector>

namespace Clox {

enum class ObjType;
struct Obj;
struct ObjClass;
struct ObjClosure;
struct ObjFunction;
struct ObjInstance;
struct ObjNative;
struct ObjString;

template<typename T>
struct Allocator;

struct Value
{
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
	[[nodiscard]] constexpr decltype(auto) as()const
	{
		return std::get<T>(value);
	}

	[[nodiscard]] bool is_obj_type(ObjType type)const;

	template<typename U>
	[[nodiscard]] auto as_obj()const
		->typename std::enable_if_t<std::is_base_of_v<Obj, U> && !std::is_same_v<Obj, U>, U*>
	{
		try
		{
			return static_cast<U*>(as<Obj*>());
		} catch (...)
		{
			throw std::invalid_argument(std::string("Value is not ") + nameof(U::obj_type).data());
		}
	}

};

constexpr bool operator==(const Value& v1, const Value& v2)noexcept
{
	return v1.value == v2.value;
}
constexpr bool operator!=(const Value& v1, const Value& v2)noexcept
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