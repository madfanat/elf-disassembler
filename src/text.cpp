#include "text.h"

#include <array>
#include <cstddef>
#include <cstring>
#include <format>
#include <map>
#include <optional>
#include <string>

namespace text {
namespace {

constexpr uint32_t kInstructionSize = 4;

constexpr std::array<const char*, 32> kRegisters = {
    "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
    "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
    "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

std::string Reg(uint32_t index) { return kRegisters[index & 0x1F]; }

constexpr std::array<const char*, 8> kBranch = {"beq", "bne", nullptr, nullptr,
                                                "blt", "bge", "bltu",  "bgeu"};
constexpr std::array<const char*, 8> kLoad = {"lb",  "lh",  "lw",    nullptr,
                                              "lbu", "lhu", nullptr, nullptr};
constexpr std::array<const char*, 8> kStore = {
    "sb", "sh", "sw", nullptr, nullptr, nullptr, nullptr, nullptr};
constexpr std::array<const char*, 8> kOpImm = {"addi", nullptr, "slti", "sltiu",
                                               "xori", nullptr, "ori",  "andi"};
constexpr std::array<const char*, 8> kOp = {"add", "sll", "slt", "sltu",
                                            "xor", "srl", "or",  "and"};
constexpr std::array<const char*, 8> kOpM = {"mul", "mulh", "mulhsu", "mulhu",
                                             "div", "divu", "rem",    "remu"};

int32_t Extend(uint32_t value, int bits) {
  const int shift = 32 - bits;
  return static_cast<int32_t>(value << shift) >> shift;
}

int32_t ImmI(uint32_t c) { return Extend(c >> 20, 12); }

int32_t ImmS(uint32_t c) {
  return Extend(((c >> 25) << 5) | ((c >> 7) & 0x1F), 12);
}

int32_t ImmB(uint32_t c) {
  const uint32_t imm = (((c >> 31) & 1) << 12) | (((c >> 7) & 1) << 11) |
                       (((c >> 25) & 0x3F) << 5) | (((c >> 8) & 0xF) << 1);
  return Extend(imm, 13);
}

int32_t ImmU(uint32_t c) { return static_cast<int32_t>(c >> 12); }

int32_t ImmJ(uint32_t c) {
  const uint32_t imm = (((c >> 31) & 1) << 20) | (((c >> 12) & 0xFF) << 12) |
                       (((c >> 20) & 1) << 11) | (((c >> 21) & 0x3FF) << 1);
  return Extend(imm, 21);
}

std::string RegRegImm(uint32_t rd, uint32_t rs1, int32_t imm) {
  return Reg(rd) + ", " + Reg(rs1) + ", " + std::to_string(imm);
}

std::string RegImmBase(uint32_t reg, int32_t imm, uint32_t base) {
  return Reg(reg) + ", " + std::to_string(imm) + "(" + Reg(base) + ")";
}

Instruction Decode(uint32_t address, uint32_t code) {
  Instruction out;
  out.address = address;
  out.code = code;
  out.mnemonic = "unknown_instruction";

  const uint32_t opcode = code & 0x7F;
  const uint32_t rd = (code >> 7) & 0x1F;
  const uint32_t rs1 = (code >> 15) & 0x1F;
  const uint32_t rs2 = (code >> 20) & 0x1F;
  const uint32_t funct3 = (code >> 12) & 0x7;
  const uint32_t funct7 = (code >> 25) & 0x7F;
  const uint32_t shamt = (code >> 20) & 0x1F;

  switch (opcode) {
    case 0x37:
      out.mnemonic = "lui";
      out.operands = Reg(rd) + ", " + std::to_string(ImmU(code));
      break;
    case 0x17:
      out.mnemonic = "auipc";
      out.operands = Reg(rd) + ", " + std::to_string(ImmU(code));
      break;
    case 0x6F:
      out.mnemonic = "jal";
      out.operands = Reg(rd) + ", ";
      out.target = address + ImmJ(code);
      break;
    case 0x67:
      if (funct3 == 0) {
        out.mnemonic = "jalr";
        out.operands = RegImmBase(rd, ImmI(code), rs1);
      }
      break;
    case 0x63:
      if (const char* m = kBranch[funct3]) {
        out.mnemonic = m;
        out.operands = Reg(rs1) + ", " + Reg(rs2) + ", ";
        out.target = address + ImmB(code);
      }
      break;
    case 0x03:
      if (const char* m = kLoad[funct3]) {
        out.mnemonic = m;
        out.operands = RegImmBase(rd, ImmI(code), rs1);
      }
      break;
    case 0x23:
      if (const char* m = kStore[funct3]) {
        out.mnemonic = m;
        out.operands = RegImmBase(rs2, ImmS(code), rs1);
      }
      break;
    case 0x13:
      if (funct3 == 1) {
        out.mnemonic = "slli";
        out.operands = RegRegImm(rd, rs1, shamt);
      } else if (funct3 == 5) {
        out.mnemonic = funct7 == 0x20 ? "srai" : "srli";
        out.operands = RegRegImm(rd, rs1, shamt);
      } else {
        out.mnemonic = kOpImm[funct3];
        out.operands = RegRegImm(rd, rs1, ImmI(code));
      }
      break;
    case 0x33: {
      const char* m = nullptr;
      if (funct7 == 0x00) {
        m = kOp[funct3];
      } else if (funct7 == 0x01) {
        m = kOpM[funct3];
      } else if (funct7 == 0x20) {
        m = funct3 == 0 ? "sub" : funct3 == 5 ? "sra" : nullptr;
      }
      if (m != nullptr) {
        out.mnemonic = m;
        out.operands = Reg(rd) + ", " + Reg(rs1) + ", " + Reg(rs2);
      }
      break;
    }
    case 0x0F:
      if (funct3 == 0)
        out.mnemonic = "fence";
      else if (funct3 == 1)
        out.mnemonic = "fence.i";
      break;
    case 0x73:
      if (code == 0x00000073)
        out.mnemonic = "ecall";
      else if (code == 0x00100073)
        out.mnemonic = "ebreak";
      break;
  }

  return out;
}

}  // namespace

std::expected<std::vector<Instruction>, std::string> Parse(
    std::span<const uint8_t> data, const elf::Section& text,
    const std::vector<symtable::Symbol>& symbols) {
  if (static_cast<size_t>(text.offset) + text.size > data.size()) {
    return std::unexpected(".text is out of range");
  }

  std::vector<Instruction> instructions;
  const size_t count = text.size / kInstructionSize;
  instructions.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    const uint32_t address =
        text.address + static_cast<uint32_t>(i * kInstructionSize);
    uint32_t code;
    std::memcpy(&code, data.data() + text.offset + i * kInstructionSize,
                sizeof code);
    instructions.push_back(Decode(address, code));
  }

  std::map<uint32_t, std::string> labels;

  const uint32_t end = text.address + text.size;
  auto is_label_symbol = [&](const symtable::Symbol& symbol) {
    return !symbol.name.empty() && symbol.name.front() != '$' &&
           symbol.type != symtable::kTypeSection &&
           symbol.type != symtable::kTypeFile && symbol.value >= text.address &&
           symbol.value < end;
  };
  for (const symtable::Symbol& symbol : symbols) {
    if (!is_label_symbol(symbol)) {
      continue;
    }
    if (symbol.bind == symtable::kBindLocal) {
      labels.try_emplace(symbol.value, symbol.name);
    } else {
      labels[symbol.value] = symbol.name;
    }
  }

  for (const Instruction& instruction : instructions) {
    if (instruction.target) {
      labels.try_emplace(*instruction.target, "");
    }
  }
  int next = 0;
  for (auto& [address, name] : labels) {
    if (name.empty()) {
      name = "L" + std::to_string(next++);
    }
  }

  for (Instruction& instruction : instructions) {
    if (auto it = labels.find(instruction.address); it != labels.end()) {
      instruction.label = it->second;
    }
    if (instruction.target) {
      instruction.operands += labels[*instruction.target];
      instruction.operands += std::format(" ; 0x{:x}", *instruction.target);
    }
  }

  return instructions;
}

}  // namespace text
