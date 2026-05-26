#pragma once

#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

#include "elf.h"

namespace symtable {

struct Symbol {
  uint32_t index = 0;
  uint32_t value = 0;
  uint32_t size = 0;
  uint8_t type = 0;
  uint8_t bind = 0;
  uint8_t visibility = 0;
  uint16_t section_index = 0;
  std::string name;
};

struct Table {
  std::vector<Symbol> symbols;
};

std::expected<Table, std::string> Parse(
    std::span<const char> data, const elf::Section& symtab,
    const elf::Section& string_table);

}  // namespace symtable
