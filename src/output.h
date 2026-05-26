#pragma once

#include <expected>
#include <ostream>
#include <string>
#include <vector>

#include "symtable.h"
#include "text.h"

namespace output {

std::expected<void, std::string> Write(
    std::ostream& out, const std::vector<text::Instruction>& instructions,
    const std::vector<text::Label>& labels,
    const std::vector<symtable::Symbol>& symbols);

}  // namespace output
