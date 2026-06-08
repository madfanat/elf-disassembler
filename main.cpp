#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <vector>

#include "src/elf.h"
#include "src/output.h"
#include "src/symtable.h"
#include "src/text.h"

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "wrong number of arguments\n";
    return 1;
  }

  std::ifstream input(argv[1], std::ios::binary);
  if (!input) {
    std::cerr << "failed to open input file\n";
    return 1;
  }
  const std::vector<uint8_t> data((std::istreambuf_iterator<char>(input)),
                                  std::istreambuf_iterator<char>());
  const std::span<const uint8_t> bytes(data);

  const auto file = elf::Parse(bytes);
  if (!file) {
    std::cerr << file.error() << '\n';
    return 1;
  }

  const auto symbols = symtable::Parse(bytes, file->symtab, file->strtab);
  if (!symbols) {
    std::cerr << symbols.error() << '\n';
    return 1;
  }

  const auto instructions = text::Parse(bytes, file->text, *symbols);
  if (!instructions) {
    std::cerr << instructions.error() << '\n';
    return 1;
  }

  std::ofstream output(argv[2]);
  if (!output) {
    std::cerr << "failed to open output file\n";
    return 1;
  }

  const auto result = output::Write(output, *instructions, *symbols);
  if (!result) {
    std::cerr << result.error() << '\n';
    return 1;
  }

  return 0;
}
