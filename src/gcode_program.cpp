#include "gcode_program.h"

using namespace std;

namespace gpr {

// Stream output operator for `chunk` objects
// Allows printing a chunk directly to an output stream.
// Delegates to the chunk's own print method.
ostream& operator<<(ostream& stream, const chunk& ic) {
    ic.print(stream);
    return stream;
}

// Stream output operator for `block` objects
// Prints the block's contents, including all its chunks and optional line number.
ostream& operator<<(ostream& stream, const block& block) {
    block.print(stream);
    return stream;
}

// Stream output operator for `gcode_program` objects
// Prints all blocks of the program in sequence, each on its own line.
ostream& operator<<(ostream& stream, const gcode_program& program) {
    for (auto b : program) {      // Iterates over all blocks
        stream << b << endl;      // Prints each block and adds a newline
    }
    return stream;
}

// Helper function to create an integer-type addr object
// Used for word-address chunks with integer values (e.g., G1, N10)
addr make_int_address(const int i) {
    addr_value v;
    v.int_val = i;                       // Set integer value
    return addr{ADDRESS_TYPE_INTEGER, v}; // Construct addr object with integer type
}

// Helper function to create a double-type addr object
// Used for word-address chunks with floating-point values (e.g., X1.0, F23.5)
addr make_double_address(const double i) {
    addr_value v;
    v.dbl_val = i;                        // Set double value
    return addr{ADDRESS_TYPE_DOUBLE, v};  // Construct addr object with double type
}

// Factory function to create a word-address chunk with integer value
// Parameters:
//   c : G-code letter (e.g., 'G', 'N')
//   i : integer value associated with the letter
// Returns a chunk of type CHUNK_TYPE_WORD_ADDRESS
chunk make_word_int(const char c, const int i) {
    addr int_address = make_int_address(i); // Create integer addr
    return chunk(c, int_address);          // Construct chunk
}

// Factory function to create a word-address chunk with double value
// Parameters:
//   c : G-code letter (e.g., 'X', 'Y', 'F')
//   i : double value associated with the letter
// Returns a chunk of type CHUNK_TYPE_WORD_ADDRESS
chunk make_word_double(const char c, const double i) {
    addr double_addr = make_double_address(i); // Create double addr
    return chunk(c, double_addr);             // Construct chunk
}

// Equality operator for chunks
// Returns true if both chunks are of the same type and have equal content
bool operator==(const chunk& l, const chunk& r) {
    return l.equals(r);  // Delegates to chunk::equals
}

// Inequality operator for chunks
// Returns true if chunks differ in type or content
bool operator!=(const chunk& l, const chunk& r) {
    return !(l == r);
}

// Factory function to create a comment chunk
// Parameters:
//   start_delim : left delimiter (e.g., '(')
//   end_delim   : right delimiter (e.g., ')')
//   comment_text: text inside the comment
chunk make_comment(const char start_delim,
                   const char end_delim,
                   const std::string& comment_text) {
    return chunk(start_delim, end_delim, comment_text);
}

// Factory function to create a percent chunk (%)
// Useful for program start/end markers in G-code
chunk make_percent_chunk() {
    return chunk(); // Default constructor creates CHUNK_TYPE_PERCENT
}

// Factory function to create an isolated single-character word chunk
// Parameters:
//   c : the character to store
chunk make_isolated_word(const char c) {
    return chunk(c); // Uses constructor for CHUNK_TYPE_WORD
}

} // namespace gpr

/* Summary of this file:

- Provides operator overloads for printing `chunk`, `block`, and `gcode_program`.
- Provides helper functions to construct chunks:
    - make_word_int / make_word_double → word-address chunks
    - make_comment → comment chunk
    - make_isolated_word → single-character word chunk
    - make_percent_chunk → percent chunk
- Provides equality (==) and inequality (!=) operators for chunks.
- Serves as a high-level interface for constructing and outputting parsed G-code program data.
*/
