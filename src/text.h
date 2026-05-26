#pragma once

#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

#include "elf.h"
#include "symtable.h"

namespace text {

struct Label {
  uint32_t address = 0;
  std::string name;
};

struct Instruction {
  uint32_t address = 0;
  uint32_t code = 0;
  std::string mnemonic;
  std::string arguments;
};

struct Text {
  std::vector<Instruction> instructions;
  std::vector<Label> labels;
};

std::expected<Text, std::string> Parse(
    std::span<const char> data, const elf::Section& text_section,
    const std::vector<symtable::Symbol>& symbols);

}  // namespace text
