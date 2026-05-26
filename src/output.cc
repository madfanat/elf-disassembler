#include "output.h"

#include <cstdint>
#include <format>
#include <string_view>

namespace output {

namespace {

constexpr std::string_view kSymbolTypes[] = {
    "NOTYPE", "OBJECT", "FUNC", "SECTION", "FILE", "COMMON", "TLS"};
constexpr std::string_view kSymbolBinds[] = {"LOCAL", "GLOBAL", "WEAK"};
constexpr std::string_view kSymbolVisibility[] = {"DEFAULT", "INTERNAL",
                                                  "HIDDEN", "PROTECTED"};

template <size_t N>
std::string_view GetName(const std::string_view (&table)[N], uint8_t i) {
  return i < N ? table[i] : "UNKNOWN";
}

std::string GetName(uint16_t i) {
  if (i == 0) return "UNDEF";
  if (i == 0xfff1) return "ABS";
  if (i == 0xfff2) return "COMMON";
  return std::to_string(i);
}

}  // namespace

std::expected<void, std::string> Write(
    std::ostream& out, const std::vector<text::Instruction>& instructions,
    const std::vector<text::Label>& labels,
    const std::vector<symtable::Symbol>& symbols) {
  out << ".text\n";
  size_t i = 0;
  for (const auto& instruction : instructions) {
    while (i < labels.size() && labels[i].address < instruction.address) ++i;
    while (i < labels.size() && labels[i].address == instruction.address) {
      out << std::format("{:08x}   <{}>:\n", labels[i].address, labels[i].name);
      ++i;
    }
    if (instruction.arguments.empty()) {
      out << std::format("   {:05x}:\t{:08x}\t{:>7}\n", instruction.address,
                         instruction.code, instruction.mnemonic);
    } else {
      out << std::format("   {:05x}:\t{:08x}\t{:>7}\t{}\n", instruction.address,
                         instruction.code, instruction.mnemonic,
                         instruction.arguments);
    }
  }

  out << "\n.symtab\n";
  out << "Symbol Value          \tSize Type \tBind \tVis   \tIndex Name\n";
  for (const auto& symbol : symbols) {
    out << std::format("[{:4}] 0x{:<15X} {:5} {:<8} {:<8} {:<8} {:>6} {}\n",
                       symbol.index, symbol.value, symbol.size,
                       GetName(kSymbolTypes, symbol.type),
                       GetName(kSymbolBinds, symbol.bind),
                       GetName(kSymbolVisibility, symbol.visibility),
                       GetName(symbol.section_index), symbol.name);
  }

  return {};
}

}  // namespace output
