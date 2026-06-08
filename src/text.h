#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "elf.h"
#include "symtable.h"

namespace text {

struct Instruction {
  uint32_t address = 0;
  uint32_t code = 0;
  std::string mnemonic;
  std::string operands;
  std::string label;
  std::optional<uint32_t> target;
};

std::expected<std::vector<Instruction>, std::string> Parse(
    std::span<const uint8_t> data, const elf::Section& text,
    const std::vector<symtable::Symbol>& symbols);

}  // namespace text
