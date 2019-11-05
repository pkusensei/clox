#pragma once

#include "object.h"
#include "vm.h"

namespace Clox {

template<typename T>
ObjString* create_obj_string(T&& str, VM& vm)
{
	auto interned = vm.find_string(str);
	if (interned != nullptr)
		return interned;

	auto p = new ObjString();
	p->content = std::forward<T>(str);
	vm.strings.emplace(p);
	register_obj(p, vm);
	return p;
}

} // Clox