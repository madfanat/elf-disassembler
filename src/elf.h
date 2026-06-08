#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>

namespace elf {

struct Ehdr {
  uint8_t ident[16];
  uint16_t type;
  uint16_t machine;
  uint32_t version;
  uint32_t entry;
  uint32_t phoff;
  uint32_t shoff;
  uint32_t flags;
  uint16_t ehsize;
  uint16_t phentsize;
  uint16_t phnum;
  uint16_t shentsize;
  uint16_t shnum;
  uint16_t shstrndx;
};
static_assert(sizeof(Ehdr) == 52);

struct Shdr {
  uint32_t name;
  uint32_t type;
  uint32_t flags;
  uint32_t addr;
  uint32_t offset;
  uint32_t size;
  uint32_t link;
  uint32_t info;
  uint32_t addralign;
  uint32_t entsize;
};
static_assert(sizeof(Shdr) == 40);

struct Sym {
  uint32_t name;
  uint32_t value;
  uint32_t size;
  uint8_t info;
  uint8_t other;
  uint16_t shndx;
};
static_assert(sizeof(Sym) == 16);

inline std::string Read(std::span<const uint8_t> data, size_t offset) {
  if (offset >= data.size()) {
    return {};
  }

  size_t end = offset;
  while (end < data.size() && data[end] != 0) {
    ++end;
  }

  return std::string(reinterpret_cast<const char*>(data.data() + offset),
                     end - offset);
}

struct Section {
  uint32_t address = 0;
  uint32_t offset = 0;
  uint32_t size = 0;
  uint32_t entry_size = 0;
};

struct File {
  Section text;
  Section symtab;
  Section strtab;
};

std::expected<File, std::string> Parse(std::span<const uint8_t> data);

}  // namespace elf
