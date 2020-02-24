#pragma once

#include "obj.h"
#include "vm.h"

namespace Clox {

using clox_string = std::basic_string<char, std::char_traits<char>, Allocator<char>>;

struct ObjString final :public Obj
{
	clox_string content;

	ObjString()noexcept :Obj(ObjType::String) {}
	std::string_view text()const { return content; }
};

std::ostream& operator<<(std::ostream& out, const ObjString& s);
[[nodiscard]] clox_string operator+(const ObjString& lhs, const ObjString& rhs);

template<typename T>
[[nodiscard]] ObjString* create_obj_string(T&& str, VM& vm)
{
	auto interned = vm.gc.find_string(str);
	if (interned != nullptr)
		return interned;

	auto p = alloc_unique_obj<ObjString>();
	auto res = static_cast<ObjString*>(p.get());
	vm.push(res);
	res->content = std::forward<T>(str);
	vm.gc.strings.emplace(res);
	register_obj(std::move(p), vm.gc);
	vm.pop();
	return res;
}

} // Clox