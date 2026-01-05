#pragma once
// Ensures this header file is included only once per compilation unit.
// Prevents multiple-definition and re-declaration errors without using
// traditional include guards (#ifndef / #define).

#include <string>
// Provides std::string, used to store G-code text and individual tokens.

#include "gcode_program.h"
// Declares the gcode_program class.
// This class represents a full parsed G-code program, typically composed
// of multiple blocks (lines), where each block contains tokens/commands.

namespace gpr {
// All declarations are placed inside the gpr namespace to:
// - Avoid name collisions with other libraries
// - Group all G-code parser–related functionality logically

  // --------------------------------------------------------------------
  // lex_block
  // --------------------------------------------------------------------
  // Purpose:
  //   Performs lexical analysis (tokenization) on a single G-code block.
  //
  // Input:
  //   block_text:
  //     A single logical G-code block (usually one line of G-code).
  //     Example:
  //       "G01 X10.0 Y5.0 F200"
  //
  // Output:
  //   std::vector<std::string>:
  //     A list of tokens extracted from the block.
  //     Example output:
  //       ["G01", "X10.0", "Y5.0", "F200"]
  //
  // Responsibility:
  //   - Splits a block into meaningful lexical units
  //   - Does NOT interpret or validate semantics
  //   - Used internally by the parser when constructing blocks
  //
  std::vector<std::string> lex_block(const std::string& block_text);

  // --------------------------------------------------------------------
  // parse_gcode
  // --------------------------------------------------------------------
  // Purpose:
  //   Parses a full G-code program provided as raw text.
  //
  // Input:
  //   program_text:
  //     Entire contents of a G-code file as a single string.
  //     Typically contains multiple lines / blocks.
  //
  // Processing (high-level):
  //   - Splits the input text into logical blocks (lines)
  //   - Lexes each block using lex_block(...)
  //   - Builds a gcode_program object containing all parsed blocks
  //
  // Output:
  //   gcode_program:
  //     Structured representation of the G-code program.
  //     Each block contains parsed tokens only (no original text).
  //
  // Notes:
  //   - Original block text is NOT preserved
  //   - Useful when only parsed commands matter
  //
  gcode_program parse_gcode(const std::string& program_text);

  // --------------------------------------------------------------------
  // parse_gcode_saving_block_text
  // --------------------------------------------------------------------
  // Purpose:
  //   Same as parse_gcode(), but additionally preserves the original
  //   text of each G-code block.
  //
  // Input:
  //   program_text:
  //     Entire G-code file contents as a single string.
  //
  // Output:
  //   gcode_program:
  //     Contains:
  //       - Parsed tokens for each block
  //       - The original unmodified block text
  //
  // Use cases:
  //   - Debugging and testing
  //   - Pretty-printing or re-exporting G-code
  //   - Error reporting with original source lines
  //
  // Difference from parse_gcode:
  //   - Slightly higher memory usage
  //   - More information retained per block
  //
  gcode_program parse_gcode_saving_block_text(const std::string& program_text);

} // namespace gpr
