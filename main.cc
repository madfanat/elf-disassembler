#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "src/elf.h"
#include "src/output.h"
#include "src/symtable.h"
#include "src/text.h"

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "Bad arguments number.\n";
    return 1;
  }

  std::ifstream in(argv[1], std::ios::binary | std::ios::ate);
  if (!in) {
    std::cerr << "Bad input file.\n";
    return 1;
  }

  const auto size = in.tellg();
  std::vector<char> data(static_cast<size_t>(size));
  in.seekg(0);
  in.read(data.data(), size);

  auto headers = elf::Parse(data);
  if (!headers) {
    std::cerr << headers.error() << '\n';
    return 1;
  }

  auto symbols =
      symtable::Parse(data, *headers->symtab, *headers->symtab_string);
  if (!symbols) {
    std::cerr << symbols.error() << '\n';
    return 1;
  }

  auto parsed_text = text::Parse(data, *headers->text, symbols->symbols);
  if (!parsed_text) {
    std::cerr << parsed_text.error() << '\n';
    return 1;
  }

  std::ofstream out(argv[2]);
  if (!out) {
    std::cerr << "Bad output file.\n";
    return 1;
  }

  if (auto result = output::Write(out, parsed_text->instructions,
                                  parsed_text->labels, symbols->symbols);
      !result) {
    std::cerr << result.error() << '\n';
    return 1;
  }

  return 0;
}
