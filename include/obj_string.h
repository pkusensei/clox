#pragma once

#include "obj.h"
#include "memory.h"

namespace Clox {

struct ObjString final :public ObjT<ObjString>
{
	std::string content;

	ObjString()noexcept :ObjT(ObjType::String) {}
	std::string_view text()const { return content; }
};

std::ostream& operator<<(std::ostream& out, const ObjString& s);
[[nodiscard]] std::string operator+(const ObjString& lhs, const ObjString& rhs);

template<typename T>
[[nodiscard]] ObjString* create_obj_string(T&& str, GC& gc)
{
	auto interned = gc.find_string(str);
	if (interned != nullptr)
		return interned;

	auto p = new ObjString();
	p->content = std::forward<T>(str);
	gc.strings.emplace(p);
	register_obj(p, gc);
	return p;
}

} // Clox