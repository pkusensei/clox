#include "object.h"

#include "vm.h"

namespace Clox {

void ObjDeleter::operator()(Obj* ptr) const
{
	switch (ptr->type)
	{
		case ObjType::Closure:
			static_cast<ObjClosure*>(ptr)->~ObjClosure();
			break;
		case ObjType::Function:
			static_cast<ObjFunction*>(ptr)->~ObjFunction();
			break;
		case ObjType::Native:
			static_cast<ObjNative*>(ptr)->~ObjNative();
			break;
		case ObjType::String:
			static_cast<ObjString*>(ptr)->~ObjString();
			break;
		case ObjType::Upvalue:
			static_cast<ObjUpvalue*>(ptr)->~ObjUpvalue();
			break;

		default:
			break;
	}
}

std::ostream& operator<<(std::ostream& out, const Obj& obj)
{
	switch (obj.type)
	{
		case ObjType::Closure:
			out << static_cast<const ObjClosure&>(obj);
			break;
		case ObjType::Function:
			out << static_cast<const ObjFunction&>(obj);
			break;
		case ObjType::Native:
			out << static_cast<const ObjNative&>(obj);
			break;
		case ObjType::String:
			out << static_cast<const ObjString&>(obj);
			break;
		case ObjType::Upvalue:
			out << static_cast<const ObjUpvalue&>(obj);
			break;
		default:
			break;
	}
	return out;
}

void register_obj(Obj* obj, VM& vm)
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
		out << "<fn " << *f.name << ">";
	return out;
}

ObjFunction* create_obj_function(VM& vm)
{
	auto p = new ObjFunction();
	register_obj(p, vm);
	return p;
}

std::ostream& operator<<(std::ostream& out, const ObjClosure& s)
{
	out << *s.function;
	return out;
}

ObjClosure* create_obj_closure(ObjFunction* func, VM& vm)
{
	auto p = new ObjClosure(func);
	register_obj(p, vm);
	return p;
}

std::ostream& operator<<(std::ostream& out, [[maybe_unused]] const ObjNative& s)
{
	out << "<native fn>";
	return out;
}

ObjNative* create_obj_native(NativeFn func, VM& vm)
{
	auto p = new ObjNative(func);
	register_obj(p, vm);
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

std::ostream& operator<<(std::ostream& out, [[maybe_unused]] const ObjUpvalue& s)
{
	out << "upvalue";
	return out;
}

ObjUpvalue* create_obj_upvalue(Value* slot, VM& vm)
{
	auto p = new ObjUpvalue(slot);
	register_obj(p, vm);
	return p;
}

} // Clox