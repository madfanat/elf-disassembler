#include "elf.h"

#include <cstddef>
#include <cstring>
#include <optional>

namespace elf {
namespace {

Section ToSection(const Shdr& header) {
  return Section{header.addr, header.offset, header.size, header.entsize};
}

}  // namespace

std::expected<File, std::string> Parse(std::span<const uint8_t> data) {
  if (data.size() < sizeof(Ehdr)) {
    return std::unexpected("not an ELF file");
  }
  if (std::memcmp(data.data(),
                  "\x7f"
                  "ELF",
                  4) != 0) {
    return std::unexpected("not an ELF file");
  }
  if (data[4] != 1) {
    return std::unexpected("not a 32-bit ELF file");
  }
  if (data[5] != 1) {
    return std::unexpected("not a little-endian ELF file");
  }

  Ehdr header;
  std::memcpy(&header, data.data(), sizeof header);

  auto read_section_header = [&](uint32_t index) -> std::optional<Shdr> {
    const size_t base =
        header.shoff + static_cast<size_t>(index) * header.shentsize;
    if (base + sizeof(Shdr) > data.size()) {
      return std::nullopt;
    }

    Shdr section;
    std::memcpy(&section, data.data() + base, sizeof section);

    return section;
  };

  const auto names = read_section_header(header.shstrndx);
  if (!names) {
    return std::unexpected("section-name string table is out of range");
  }

  File file;
  bool has_text = false;
  bool has_symtab = false;
  uint32_t strtab_index = 0;
  for (uint16_t i = 0; i < header.shnum; ++i) {
    const auto section = read_section_header(i);
    if (!section) {
      return std::unexpected("section header is out of range");
    }
    const std::string name = Read(data, names->offset + section->name);
    if (name == ".text") {
      file.text = ToSection(*section);
      has_text = true;
    } else if (name == ".symtab") {
      file.symtab = ToSection(*section);
      strtab_index = section->link;
      has_symtab = true;
    }
  }

  if (!has_text) {
    return std::unexpected(".text not found");
  }
  if (!has_symtab) {
    return std::unexpected(".symtab not found");
  }

  const auto strtab = read_section_header(strtab_index);
  if (!strtab) {
    return std::unexpected("string table is out of range");
  }
  file.strtab = ToSection(*strtab);

  return file;
}

}  // namespace elf
