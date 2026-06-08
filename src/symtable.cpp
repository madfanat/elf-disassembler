#include "symtable.h"

#include <cstddef>
#include <cstring>

#include "elf.h"

namespace symtable {

std::expected<std::vector<Symbol>, std::string> Parse(
    std::span<const uint8_t> data, const elf::Section& symtab,
    const elf::Section& strtab) {
  if (symtab.entry_size == 0) {
    return std::unexpected(".symtab has a zero entry size");
  }
  if (static_cast<size_t>(symtab.offset) + symtab.size > data.size()) {
    return std::unexpected(".symtab is out of range");
  }

  std::vector<Symbol> symbols;
  const size_t count = symtab.size / symtab.entry_size;
  symbols.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    elf::Sym raw;
    std::memcpy(&raw, data.data() + symtab.offset + i * symtab.entry_size,
                sizeof raw);

    Symbol symbol;
    symbol.index = static_cast<uint32_t>(i);
    symbol.value = raw.value;
    symbol.size = raw.size;
    symbol.type = raw.info & 0x0F;
    symbol.bind = raw.info >> 4;
    symbol.visibility = raw.other & 0x03;
    symbol.section = raw.shndx;
    symbol.name = elf::Read(data, strtab.offset + raw.name);
    symbols.push_back(std::move(symbol));
  }

  return symbols;
}

}  // namespace symtable
