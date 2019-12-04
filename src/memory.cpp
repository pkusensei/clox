#include "memory.h"

#include "object.h"
#include "obj_string.h"
#include "vm.h"

namespace Clox {

constexpr auto GC_HEAP_GROW_FACTOR = 2;

void GC::collect()
{
#ifdef DEBUG_LOG_GC
	std::cout << "-- gc begin\n";
	auto before = bytes_allocated;
#endif // DEBUG_LOG_GC

	mark_roots();
	trace_references();
	remove_white_string();
	sweep();

	next_gc = bytes_allocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
	std::cout << "-- gc end\n";
	std::cout << "   collected " << before - bytes_allocated;
	std::cout << " bytes (from " << before;
	std::cout << " to " << bytes_allocated << ") next at ";
	std::cout << next_gc << '\n';
#endif // DEBUG_LOG_GC
}

void GC::mark_roots()
{
	for (auto& slot : vm.stack)
		mark_value(slot);

	for (size_t i = 0; i < vm.frame_count; i++)
		mark_object(std::remove_const_t<ObjClosure*>(vm.frames.at(i).closure));

	for (auto upvalue = vm.open_upvalues; upvalue != nullptr; upvalue = upvalue->next)
		mark_object(upvalue);

	mark_table(vm.globals);
	mark_compiler_roots();
}

void GC::mark_array(const ValueArray<>& array)
{
	for (auto& value : array.values)
		mark_value(value);
}

void GC::mark_compiler_roots()
{
	auto compiler = vm.cu.current.get();
	while (compiler != nullptr)
	{
		mark_object(compiler->function);
		compiler = compiler->enclosing.get();
	}
}

void GC::mark_object(Obj* ptr)
{
	if (ptr == nullptr) return;
	if (ptr->is_marked) return;

#ifdef DEBUG_LOG_GC
	std::cout << (void*)ptr << " mark ";
	std::cout << *ptr << '\n';
#endif // DEBUG_LOG_GC

	ptr->is_marked = true;
	gray_stack.push(ptr);
}

void GC::mark_value(const Value& value)
{
	if (value.is_obj())
		mark_object(value.as<Obj*>());
}

void GC::mark_table(const table& table)
{
	for (auto& [key, value] : table)
	{
		mark_object(key);
		mark_value(value);
	}
}

void GC::trace_references()
{
	while (!gray_stack.empty())
	{
		auto obj = gray_stack.top();
		blacken_object(obj);
		gray_stack.pop();
	}
}

void GC::blacken_object(Obj* ptr)
{
#ifdef DEBUG_LOG_GC
	std::cout << (void*)ptr << " blacken ";
	std::cout << *ptr << '\n';
#endif // DEBUG_LOG_GC

	switch (ptr->type)
	{
		case ObjType::Closure:
		{
			auto closure = static_cast<ObjClosure*>(ptr);
			mark_object(closure->function);
			for (auto v : closure->upvalues)
				mark_object(v);
			break;
		}
		case ObjType::Function:
		{
			auto function = static_cast<ObjFunction*>(ptr);
			mark_object(function->name);
			mark_array(function->chunk.constants);
			break;
		}
		case ObjType::Upvalue:
			mark_value(static_cast<ObjUpvalue*>(ptr)->closed);
			break;
		case ObjType::Native:
		case ObjType::String:
		default:
			break;
	}
}

void GC::remove_white_string()
{
	std::for_each(strings.begin(), strings.end(),
		[this](auto str)
		{
			if (str != nullptr && !str->is_marked)
				strings.erase(str);
		});
}

void GC::sweep()
{
	Obj* previous = nullptr;
	Obj* object = objects.get();
	while (object != nullptr)
	{
		if (object->is_marked)
		{
			object->is_marked = false;
			previous = object;
			object = object->next.get();
		} else
		{
			if (previous == nullptr)
			{
				objects = std::move(object->next);
				object = objects.get();
			} else
			{
				previous->next = std::move(object->next);
				object = previous->next.get();
			}
		}
	}
}

} //Clox