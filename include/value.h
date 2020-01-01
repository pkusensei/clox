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
	[[nodiscard]] bool is_class()const;
	[[nodiscard]] bool is_closure()const;
	[[nodiscard]] bool is_function()const;
	[[nodiscard]] bool is_instance()const;
	[[nodiscard]] bool is_native()const;
	[[nodiscard]] bool is_string()const;
	[[nodiscard]] ObjClass* as_class()const;
	[[nodiscard]] ObjClosure* as_closure()const;
	[[nodiscard]] ObjFunction* as_function()const;
	[[nodiscard]] ObjInstance* as_instance()const;
	[[nodiscard]] ObjNative* as_native()const;
	[[nodiscard]] ObjString* as_string()const;
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