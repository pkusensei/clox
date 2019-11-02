#include "object.h"

#include "vm.h"

namespace Clox {

void ObjDeleter::operator()(Obj* ptr) const
{
	switch (ptr->type)
	{
		case ObjType::Function:
			static_cast<ObjFunction*>(ptr)->~ObjFunction();
			break;
		case ObjType::String:
			static_cast<ObjString*>(ptr)->~ObjString();
			break;
		default:
			break;
	}
}

void create_obj(Obj* obj, VM& vm)
{
	std::unique_ptr<Obj, ObjDeleter> p(obj, ObjDeleter{});
	p->next = std::move(vm.objects);
	vm.objects = std::move(p);
}

std::ostream& operator<<(std::ostream& out, const ObjFunction& f)
{
	if (f.name == nullptr)
		out << "<script>";
	else
		out << "<fn " << *f.name << " >";
	return out;
}

ObjFunction* create_obj_function(VM& vm)
{
	auto p = new ObjFunction();
	create_obj(p, vm);
	return p;
}

std::ostream& operator<<(std::ostream& out, const ObjString& s)
{
	out << s.content;
	return out;
}

std::string operator+(const ObjString& lhs, const ObjString& rhs)
{
	return lhs.content + rhs.content;
}

} // Clox