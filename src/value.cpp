#include "value.h"

#include "object.h"
#include "obj_string.h"

namespace Clox {

std::ostream& operator<<(std::ostream& out, const Value& value)
{
	std::visit([&out](auto&& arg)
		{
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v <T, bool>)
				std::cout << std::boolalpha << arg << std::noboolalpha;
			else if constexpr (std::is_same_v<T, std::monostate>)
				std::cout << "nil";
			else if constexpr (std::is_same_v<T, double>)
				std::cout << arg;
			else if constexpr (std::is_same_v<T, Obj*>)
				out << *arg;
		}, value.value);
	return out;
}

bool Value::is_obj_type(ObjType type) const
{
	if (is_obj())
	{
		auto obj = as<Obj*>();
		return obj->is_type(type);
	}
	return false;
}

bool Value::is_class() const
{
	return is_obj_type(ObjType::Class);
}

bool Value::is_closure() const
{
	return is_obj_type(ObjType::Closure);
}

bool Value::is_function() const
{
	return is_obj_type(ObjType::Function);
}

bool Value::is_instance() const
{
	return is_obj_type(ObjType::Instance);
}

bool Value::is_native() const
{
	return is_obj_type(ObjType::Native);
}

bool Value::is_string() const
{
	return is_obj_type(ObjType::String);
}

ObjClass* Value::as_class() const
{
	if (!is_class())
		throw std::invalid_argument("Value is not a class.");
	return static_cast<ObjClass*>(as<Obj*>());;
}

ObjClosure* Value::as_closure() const
{
	if (!is_closure())
		throw std::invalid_argument("Value is not a closure.");
	return static_cast<ObjClosure*>(as<Obj*>());
}

ObjFunction* Value::as_function() const
{
	if (!is_function())
		throw std::invalid_argument("Value is not a function.");
	return static_cast<ObjFunction*>(as<Obj*>());
}

ObjInstance* Value::as_instance() const
{
	if (!is_instance())
		throw std::invalid_argument("Value is not an instance.");
	return static_cast<ObjInstance*>(as<Obj*>());;
}

ObjNative* Value::as_native() const
{
	if (!is_native())
		throw std::invalid_argument("Value is not a native fn.");
	return static_cast<ObjNative*>(as<Obj*>());
}

ObjString* Value::as_string() const
{
	if (!is_string())
		throw std::invalid_argument("Value is not a string.");
	return static_cast<ObjString*>(as<Obj*>());
}

} //Clox