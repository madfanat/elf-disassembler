#include "symtable.h"

#include <cstring>

namespace symtable {

namespace {

struct Elf32Symbol {
  uint32_t name_offset;
  uint32_t value;
  uint32_t size;
  uint8_t info;
  uint8_t other;
  uint16_t section_index;
};

constexpr uint32_t kSymbolEntrySize = sizeof(Elf32Symbol);

}  // namespace

std::expected<Table, std::string> Parse(
    std::span<const char> data, const elf::Section& symtab,
    const elf::Section& string_table) {
  std::vector<Symbol> symbols;

  const uint32_t entry_size =
      symtab.entry_size == 0 ? kSymbolEntrySize : symtab.entry_size;
  if (entry_size < kSymbolEntrySize) {
    return std::unexpected("Bad symbol table entry size.");
  }
  if (static_cast<uint64_t>(symtab.offset) + symtab.size > data.size()) {
    return std::unexpected("Bad symbol table.");
  }
  if (static_cast<uint64_t>(string_table.offset) + string_table.size >
      data.size()) {
    return std::unexpected("Bad symbol string table.");
  }

  const char* strings = data.data() + string_table.offset;
  const uint32_t symbol_count = symtab.size / entry_size;
  symbols.reserve(symbol_count);

  for (uint32_t i = 0; i < symbol_count; ++i) {
    Elf32Symbol elf_sym{};
    const uint64_t offset = static_cast<uint64_t>(symtab.offset) +
                            static_cast<uint64_t>(i) * entry_size;
    std::memcpy(&elf_sym, data.data() + offset, sizeof(elf_sym));

    std::string name;
    if (elf_sym.name_offset < string_table.size) {
      name = strings + elf_sym.name_offset;
    }

    symbols.push_back({
        .index = i,
        .value = elf_sym.value,
        .size = elf_sym.size,
        .type = static_cast<uint8_t>(elf_sym.info & 0x0f),
        .bind = static_cast<uint8_t>(elf_sym.info >> 4),
        .visibility = static_cast<uint8_t>(elf_sym.other & 0x03),
        .section_index = elf_sym.section_index,
        .name = std::move(name),
    });
  }

  return Table{.symbols = std::move(symbols)};
}

}  // namespace symtable
