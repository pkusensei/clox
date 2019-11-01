#include "object.h"

#include "vm.h"

namespace Clox {

void ObjDeleter::operator()(Obj* ptr) const
{
	//if (ptr->is_type(ObjType::String))
	return static_cast<ObjString*>(ptr)->~ObjString();
}

Obj* create_obj(Obj* obj, VM& vm)
{
	std::unique_ptr<Obj, ObjDeleter> p(obj, ObjDeleter{});
	p->next = std::move(vm.objects);
	vm.objects = std::move(p);
	return obj;
}

std::string operator+(const ObjString& lhs, const ObjString& rhs)
{
	return lhs.content + rhs.content;
}

} // Clox