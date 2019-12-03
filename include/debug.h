#pragma once

#include <string_view>

namespace Clox {

struct Chunk;

void disassemble_chunk(const Chunk& chunk, std::string_view name);
[[nodiscard]] size_t disassemble_instruction(const Chunk& chunk, size_t offset);

} // Clox