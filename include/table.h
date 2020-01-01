#pragma once

#include <map>
#include "value.h"

namespace Clox {

struct ObjString;

template<typename T>
struct Allocator;

using table = std::map<ObjString*, Value, std::less<ObjString*>, Allocator<std::pair<ObjString* const, Value>>>;

}// Clox