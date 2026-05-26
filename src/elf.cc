#include "elf.h"

#include <array>
#include <cstring>
#include <vector>

namespace elf {

namespace {

constexpr char kElfMagic[] = "\177ELF";
constexpr uint8_t kElfClass32 = 1;
constexpr uint8_t kElfLittleEndian = 1;
constexpr uint16_t kMachineRiscV = 243;
constexpr uint32_t kSectionTypeSymtab = 2;

struct ElfHeader {
  std::array<uint8_t, 16> identification;
  uint16_t type;
  uint16_t machine;
  uint32_t version;
  uint32_t entry;
  uint32_t program_header_offset;
  uint32_t section_header_offset;
  uint32_t flags;
  uint16_t header_size;
  uint16_t program_header_entry_size;
  uint16_t program_header_count;
  uint16_t section_header_entry_size;
  uint16_t section_header_count;
  uint16_t section_name_string_table_index;
};

struct SectionHeader {
  uint32_t name_offset;
  uint32_t type;
  uint32_t flags;
  uint32_t address;
  uint32_t offset;
  uint32_t size;
  uint32_t link;
  uint32_t info;
  uint32_t address_alignment;
  uint32_t entry_size;
};

}  // namespace

std::expected<Header, std::string> Parse(std::span<const char> data) {
  std::optional<Section> text_section;
  std::optional<Section> symtab_section;

  ElfHeader elf_header{};
  if (data.size() < sizeof(elf_header)) {
    return std::unexpected("Bad ELF32 header.");
  }
  std::memcpy(&elf_header, data.data(), sizeof(elf_header));

  const auto& ident = elf_header.identification;
  if (std::memcmp(ident.data(), kElfMagic, sizeof(kElfMagic) - 1) != 0) {
    return std::unexpected("Bad ELF file.");
  }
  if (ident[4] != kElfClass32) {
    return std::unexpected("Bad ELF class.");
  }
  if (ident[5] != kElfLittleEndian) {
    return std::unexpected("Bad ELF endianness.");
  }
  if (elf_header.machine != kMachineRiscV) {
    return std::unexpected("Bad ELF machine.");
  }
  if (elf_header.section_header_offset == 0 ||
      elf_header.section_header_count == 0) {
    return std::unexpected("Bad ELF section header table.");
  }
  if (elf_header.section_name_string_table_index >=
      elf_header.section_header_count) {
    return std::unexpected("Bad section name string table index.");
  }

  std::vector<SectionHeader> section_headers(elf_header.section_header_count);
  for (uint32_t i = 0; i < section_headers.size(); ++i) {
    const uint64_t offset =
        static_cast<uint64_t>(elf_header.section_header_offset) +
        static_cast<uint64_t>(i) * elf_header.section_header_entry_size;
    if (offset + sizeof(SectionHeader) > data.size()) {
      return std::unexpected("Cannot read section header table.");
    }
    std::memcpy(&section_headers[i], data.data() + offset,
                sizeof(SectionHeader));
  }

  const auto& names_header =
      section_headers[elf_header.section_name_string_table_index];
  std::vector<char> section_names(names_header.size);
  if (!section_names.empty()) {
    if (static_cast<uint64_t>(names_header.offset) + names_header.size >
        data.size()) {
      return std::unexpected("Bad section name string table.");
    }
    std::memcpy(section_names.data(), data.data() + names_header.offset,
                section_names.size());
  }

  for (uint32_t i = 0; i < section_headers.size(); ++i) {
    const auto& sh = section_headers[i];
    const char* name = "";
    if (sh.name_offset < section_names.size()) {
      name = section_names.data() + sh.name_offset;
    }

    const Section section{.offset = sh.offset,
                          .size = sh.size,
                          .virtual_address = sh.address,
                          .entry_size = sh.entry_size,
                          .link = sh.link,
                          .info = sh.info,
                          .type = sh.type,
                          .flags = sh.flags,
                          .index = i};

    if (std::strcmp(name, ".text") == 0) {
      text_section = section;
    } else if (std::strcmp(name, ".symtab") == 0 ||
               std::strcmp(name, ".symtable") == 0 ||
               sh.type == kSectionTypeSymtab) {
      symtab_section = section;
    }
  }

  if (!text_section) {
    return std::unexpected("Bad ELF .text section.");
  }
  if (!symtab_section) {
    return std::unexpected("Bad ELF .symtab section.");
  }
  if (symtab_section->link >= section_headers.size()) {
    return std::unexpected("Bad string table link.");
  }

  const auto& ssh = section_headers[symtab_section->link];
  std::optional<Section> symtab_string_section =
      Section{.offset = ssh.offset,
              .size = ssh.size,
              .virtual_address = ssh.address,
              .entry_size = ssh.entry_size,
              .link = ssh.link,
              .info = ssh.info,
              .type = ssh.type,
              .flags = ssh.flags,
              .index = symtab_section->link};

  return Header{
      .text = *text_section,
      .symtab = *symtab_section,
      .symtab_string = *symtab_string_section,
  };
}

}  // namespace elf
