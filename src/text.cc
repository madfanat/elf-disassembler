#include "text.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <format>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace text {

namespace {

constexpr std::array<std::string_view, 32> kRegisters = {
    "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
    "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
    "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

uint32_t Read(const char* data) {
  uint32_t value = 0;
  std::memcpy(&value, data, sizeof(value));
  return value;
}

int32_t SignExtend(uint32_t value, uint32_t bits) {
  const uint32_t sign_bit = 1u << (bits - 1);
  return static_cast<int32_t>((value ^ sign_bit) - sign_bit);
}

template <size_t N>
std::optional<std::string_view> GetMnemonic(
    const std::array<std::string_view, N>& table, uint32_t index) {
  if (index < N && !table[index].empty()) return table[index];
  return std::nullopt;
}

std::string FormatFence(uint32_t value) {
  constexpr std::string_view bits = "iorw";
  std::string result;
  for (int i = 3; i >= 0; --i)
    if (value & (1u << i)) result += bits[3 - i];
  return result.empty() ? "unknown" : result;
}

Instruction Decode(uint32_t code, uint32_t address,
                   std::unordered_map<uint32_t, std::string>& labels,
                   uint32_t& generated) {
  const uint32_t opcode = code & 0x7f;
  const uint32_t rd = (code >> 7) & 0x1f;
  const uint32_t funct3 = (code >> 12) & 0x07;
  const uint32_t rs1 = (code >> 15) & 0x1f;
  const uint32_t rs2 = (code >> 20) & 0x1f;
  const uint32_t funct7 = (code >> 25) & 0x7f;

  auto unknown = [&] {
    return Instruction{address, code, "unknown_instruction", {}};
  };
  auto r_args = [&] {
    return std::format("{}, {}, {}", kRegisters[rd], kRegisters[rs1],
                       kRegisters[rs2]);
  };
  auto i_imm = [&](int32_t imm) {
    return std::format("{}, {}, {}", kRegisters[rd], kRegisters[rs1], imm);
  };
  auto load = [&](int32_t imm) {
    return std::format("{}, {}({})", kRegisters[rd], imm, kRegisters[rs1]);
  };
  auto store = [&](int32_t imm) {
    return std::format("{}, {}({})", kRegisters[rs2], imm, kRegisters[rs1]);
  };
  auto target = [&](uint32_t address) -> std::string {
    auto [it, inserted] =
        labels.try_emplace(address, "L" + std::to_string(generated));
    if (inserted) ++generated;
    return std::format("{} ; 0x{:x}", it->second, address);
  };

  if (opcode == 0x37)
    return {address, code, "lui",
            std::format("{}, {}", kRegisters[rd], SignExtend(code >> 12, 20))};

  if (opcode == 0x17)
    return {address, code, "auipc",
            std::format("{}, {}", kRegisters[rd], SignExtend(code >> 12, 20))};

  if (opcode == 0x6f) {
    const uint32_t raw = ((code >> 31) << 20) | (((code >> 21) & 0x3ff) << 1) |
                         (((code >> 20) & 0x01) << 11) |
                         (((code >> 12) & 0xff) << 12);
    return {address, code, "jal",
            std::format("{}, {}", kRegisters[rd],
                        target(address + SignExtend(raw, 21)))};
  }

  if (opcode == 0x67 && funct3 == 0)
    return {address, code, "jalr", load(SignExtend(code >> 20, 12))};

  if (opcode == 0x63) {
    constexpr std::array<std::string_view, 8> names = {
        "beq", "bne", "", "", "blt", "bge", "bltu", "bgeu"};
    auto mnemonic = GetMnemonic(names, funct3);
    if (!mnemonic) return unknown();
    const uint32_t raw = ((code >> 31) << 12) | (((code >> 7) & 0x01) << 11) |
                         (((code >> 25) & 0x3f) << 5) |
                         (((code >> 8) & 0x0f) << 1);
    return {address, code, std::string(*mnemonic),
            std::format("{}, {}, {}", kRegisters[rs1], kRegisters[rs2],
                        target(address + SignExtend(raw, 13)))};
  }

  if (opcode == 0x03) {
    constexpr std::array<std::string_view, 8> names = {"lb",  "lh",  "lw", "",
                                                       "lbu", "lhu", "",   ""};
    auto mnemonic = GetMnemonic(names, funct3);
    if (!mnemonic) return unknown();
    return {address, code, std::string(*mnemonic),
            load(SignExtend(code >> 20, 12))};
  }

  if (opcode == 0x23) {
    constexpr std::array<std::string_view, 8> names = {"sb", "sh", "sw", "",
                                                       "",   "",   "",   ""};
    auto mnemonic = GetMnemonic(names, funct3);
    if (!mnemonic) return unknown();
    const uint32_t raw = ((code >> 25) << 5) | ((code >> 7) & 0x1f);
    return {address, code, std::string(*mnemonic), store(SignExtend(raw, 12))};
  }

  if (opcode == 0x13) {
    if (funct3 == 1 && funct7 == 0) return {address, code, "slli", i_imm(rs2)};
    if (funct3 == 1) return unknown();
    if (funct3 == 5 && (funct7 == 0 || funct7 == 0x20))
      return {address, code, std::string(funct7 == 0 ? "srli" : "srai"),
              i_imm(rs2)};
    constexpr std::array<std::string_view, 8> names = {
        "addi", "", "slti", "sltiu", "xori", "", "ori", "andi"};
    auto mnemonic = GetMnemonic(names, funct3);
    if (!mnemonic) return unknown();
    return {address, code, std::string(*mnemonic),
            i_imm(SignExtend(code >> 20, 12))};
  }

  if (opcode == 0x33) {
    if (funct7 == 1) {
      constexpr std::array<std::string_view, 8> names = {
          "mul", "mulh", "mulhsu", "mulhu", "div", "divu", "rem", "remu"};
      return {address, code, std::string(names[funct3]), r_args()};
    }
    if (funct7 == 0 || funct7 == 0x20) {
      std::string_view mnemonic;
      if (funct3 == 0)
        mnemonic = funct7 == 0 ? "add" : "sub";
      else if (funct3 == 5 && funct7 == 0x20)
        mnemonic = "sra";
      else if (funct7 == 0) {
        constexpr std::array<std::string_view, 8> names = {
            "", "sll", "slt", "sltu", "xor", "srl", "or", "and"};
        if (auto n = GetMnemonic(names, funct3)) mnemonic = *n;
      }
      if (!mnemonic.empty())
        return {address, code, std::string(mnemonic), r_args()};
    }
    return unknown();
  }

  if (opcode == 0x0f) {
    if (funct3 == 0)
      return {address, code, "fence",
              std::format("{}, {}", FormatFence((code >> 24) & 0x0f),
                          FormatFence((code >> 20) & 0x0f))};
    if (funct3 == 1) return {address, code, "fence.i", {}};
    return unknown();
  }

  if (opcode == 0x73) {
    if (code == 0x00000073) return {address, code, "ecall", {}};
    if (code == 0x00100073) return {address, code, "ebreak", {}};
    constexpr std::array<std::string_view, 8> names = {
        "", "csrrw", "csrrs", "csrrc", "", "csrrwi", "csrrsi", "csrrci"};
    auto mnemonic = GetMnemonic(names, funct3);
    if (!mnemonic) return unknown();
    const uint32_t csr = code >> 20;
    std::string args =
        funct3 < 5 ? std::format("{}, 0x{:x}, {}", kRegisters[rd], csr,
                                 kRegisters[rs1])
                   : std::format("{}, 0x{:x}, {}", kRegisters[rd], csr, rs1);
    return {address, code, std::string(*mnemonic), std::move(args)};
  }

  return unknown();
}

}  // namespace

std::expected<Text, std::string> Parse(
    std::span<const char> data, const elf::Section& text_section,
    const std::vector<symtable::Symbol>& symbols) {
  std::vector<Instruction> instructions;
  std::vector<Label> labels;

  if (static_cast<uint64_t>(text_section.offset) + text_section.size >
      data.size()) {
    return std::unexpected("Bad text section.");
  }
  if (text_section.size % 4 != 0) {
    return std::unexpected("Bad text section size.");
  }

  std::unordered_map<uint32_t, std::string> label_map;
  for (const auto& symbol : symbols) {
    if (!symbol.name.empty() && symbol.name[0] != '$' &&
        symbol.value >= text_section.virtual_address &&
        symbol.value < text_section.virtual_address + text_section.size) {
      label_map.try_emplace(symbol.value, symbol.name);
    }
  }

  uint32_t generated = 0;
  const uint32_t count = text_section.size / 4;
  instructions.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    const uint32_t code = Read(data.data() + text_section.offset + i * 4);
    const uint32_t address = text_section.virtual_address + i * 4;
    instructions.push_back(Decode(code, address, label_map, generated));
  }

  labels.reserve(label_map.size());
  for (auto& [address, name] : label_map) {
    labels.push_back({address, std::move(name)});
  }
  std::ranges::sort(labels, [](const Label& a, const Label& b) {
    if (a.address != b.address) return a.address < b.address;
    return a.name < b.name;
  });

  return Text{
      .instructions = std::move(instructions),
      .labels = std::move(labels),
  };
}

}  // namespace text
