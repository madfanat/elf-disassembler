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
  uint16_t section = 0;
  std::string name;
};

inline constexpr uint8_t kTypeSection = 3;
inline constexpr uint8_t kTypeFile = 4;
inline constexpr uint8_t kBindLocal = 0;

std::expected<std::vector<Symbol>, std::string> Parse(
    std::span<const uint8_t> data, const elf::Section& symtab,
    const elf::Section& strtab);

}  // namespace symtable
