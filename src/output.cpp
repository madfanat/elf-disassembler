#include "output.h"

#include <array>
#include <cstdint>
#include <format>
#include <string>

namespace output {
namespace {

const char* Type(uint8_t type) {
  constexpr std::array<const char*, 7> kNames = {
      "NOTYPE", "OBJECT", "FUNC", "SECTION", "FILE", "COMMON", "TLS"};
  return type < kNames.size() ? kNames[type] : "UNKNOWN";
}

const char* Bind(uint8_t bind) {
  constexpr std::array<const char*, 3> kNames = {"LOCAL", "GLOBAL", "WEAK"};
  return bind < kNames.size() ? kNames[bind] : "UNKNOWN";
}

const char* Visibility(uint8_t visibility) {
  constexpr std::array<const char*, 4> kNames = {"DEFAULT", "INTERNAL",
                                                 "HIDDEN", "PROTECTED"};
  return visibility < kNames.size() ? kNames[visibility] : "UNKNOWN";
}

std::string Index(uint16_t index) {
  switch (index) {
    case 0x0000:
      return "UNDEF";
    case 0xFFF1:
      return "ABS";
    case 0xFFF2:
      return "COMMON";
    default:
      return std::to_string(index);
  }
}

}  // namespace

std::expected<void, std::string> Write(
    std::ostream& out, const std::vector<text::Instruction>& instructions,
    const std::vector<symtable::Symbol>& symbols) {
  out << ".text\n";
  for (const text::Instruction& instruction : instructions) {
    if (!instruction.label.empty()) {
      out << std::format("{:08x}   <{}>:\n", instruction.address,
                         instruction.label);
    }
    if (instruction.operands.empty()) {
      out << std::format("   {:05x}:\t{:08x}\t{:>7}\n", instruction.address,
                         instruction.code, instruction.mnemonic);
    } else {
      out << std::format("   {:05x}:\t{:08x}\t{:>7}\t{}\n", instruction.address,
                         instruction.code, instruction.mnemonic,
                         instruction.operands);
    }
  }

  out << "\n.symtab\n";
  out << "Symbol Value          \tSize Type \tBind \tVis   \tIndex Name\n";
  for (const symtable::Symbol& symbol : symbols) {
    out << std::format(
        "[{:4}] 0x{:<15X} {:5} {:<8} {:<8} {:<8} {:>6} {}\n", symbol.index,
        symbol.value, symbol.size, Type(symbol.type), Bind(symbol.bind),
        Visibility(symbol.visibility), Index(symbol.section), symbol.name);
  }

  if (!out) {
    return std::unexpected("failed to write output");
  }

  return {};
}

}  // namespace output
