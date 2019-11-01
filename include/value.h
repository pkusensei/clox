#pragma once

#include <iostream>
#include <variant>
#include <vector>

#include "object.h"

namespace Clox {

struct Value
{
	using var_t = std::variant<bool, std::monostate, double, Obj*>;

	var_t value;

	constexpr Value(bool value) : value(value) {}
	constexpr Value() : value(std::monostate()) {}
	constexpr Value(double value) : value(value) {}
	constexpr Value(Obj* obj) : value(obj) {}

	constexpr bool is_bool()const noexcept { return std::holds_alternative<bool>(value); }
	constexpr bool is_nil()const noexcept { return std::holds_alternative<std::monostate>(value); }
	constexpr bool is_number()const noexcept { return std::holds_alternative<double>(value); }
	constexpr bool is_obj()const noexcept { return std::holds_alternative<Obj*>(value); }

	template<typename T>
	constexpr decltype(auto) as()const
	{
		return std::get<T>(value);
	}

	constexpr bool is_obj_type(ObjType type)const
	{
		if (is_obj())
		{
			auto obj = as<Obj*>();
			return obj->is_type(type);
		}
		return false;
	}
	constexpr bool is_string()const { return is_obj_type(ObjType::String); }

	constexpr ObjString* as_string()const
	{
		if (!is_obj_type(ObjType::String))
			throw std::invalid_argument("Value is not a string.");
		return static_cast<ObjString*>(as<Obj*>());
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

struct ValueArray
{
	std::vector<Value> values;

	constexpr size_t count()const noexcept { return values.size(); }

	template<typename T>
	typename std::enable_if_t<std::is_convertible_v<T, Value>, void>
		write(T&& value)
	{
		values.emplace_back(std::forward<T>(value));
	}
};

} //Clox