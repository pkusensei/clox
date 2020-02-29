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
	if (before > bytes_allocated)
	{
		std::cout << "   collected " << before - bytes_allocated;
		std::cout << " bytes (from " << before;
		std::cout << " to " << bytes_allocated << ") next at ";
		std::cout << next_gc << '\n';
	}
#endif // DEBUG_LOG_GC
}

void GC::mark_roots()
{
	for (auto slot = vm.stack.data(); slot < vm.stacktop; ++slot)
		mark_value(*slot);

	for (size_t i = 0; i < vm.frame_count; i++)
		mark_object(std::remove_const_t<ObjClosure*>(vm.frames.at(i).closure));

	for (auto upvalue = vm.open_upvalues; upvalue != nullptr; upvalue = upvalue->next)
		mark_object(upvalue);

	mark_table(vm.globals);
	mark_compiler_roots();
	mark_object(vm.init_string);
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

void GC::mark_object(Obj* const ptr)
{
	if (ptr == nullptr) return;
	if (ptr->is_marked) return;

#ifdef DEBUG_LOG_GC
	std::cout << (void*)ptr << " mark ";
	std::cout << *ptr << '\n';
#endif // DEBUG_LOG_GC

	ptr->is_marked = true;
	gray_stack.push_back(ptr);
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
		auto obj = gray_stack.front();
		blacken_object(obj);
		gray_stack.pop_front();
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
		case ObjType::BoundMethod:
		{
			auto bound = static_cast<ObjBoundMethod*>(ptr);
			mark_value(bound->receiver);
			mark_object(bound->method);
			break;
		}
		case ObjType::Class:
		{
			auto klass = static_cast<ObjClass*>(ptr);
			mark_object(klass->name);
			mark_table(klass->methods);
			break;
		}
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
		case ObjType::Instance:
		{
			auto instance = static_cast<ObjInstance*>(ptr);
			mark_object(instance->klass);
			mark_table(instance->fields);
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

void GC::remove_white_string()noexcept
{
	for (auto it = strings.begin(); it != strings.end();)
	{
		if (*it != nullptr && !(*it)->is_marked)
			it = strings.erase(it);
		else ++it;
	}
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
			decltype(object->next) temp = std::move(object->next);
			if (previous == nullptr)
			{
				objects = std::move(temp);
				object = objects.get();
			} else
			{
				previous->next = std::move(temp);
				object = previous->next.get();
			}
		}
	}
}

} //Clox