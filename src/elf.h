#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>

namespace elf {

struct Section {
  uint32_t offset = 0;
  uint32_t size = 0;
  uint32_t virtual_address = 0;
  uint32_t entry_size = 0;
  uint32_t link = 0;
  uint32_t info = 0;
  uint32_t type = 0;
  uint32_t flags = 0;
  uint32_t index = 0;
};

struct Header {
  std::optional<Section> text;
  std::optional<Section> symtab;
  std::optional<Section> symtab_string;
};

std::expected<Header, std::string> Parse(std::span<const char> data);

}  // namespace elf
