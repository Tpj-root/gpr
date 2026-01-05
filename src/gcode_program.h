#pragma once
// Header guard to ensure this header file is included only once
// in a single compilation unit. Prevents duplicate definitions
// when multiple files include this header.

#include <cassert>
// Provides the assert() macro for runtime checks.
// Used throughout the parser to enforce preconditions, such as:
//   - Tokens or characters must exist before parsing
//   - Single-character tokens must have exactly one character
//   - Valid syntax conditions (e.g., balanced parentheses)

#include <iostream>
// Provides input/output stream functionality.
// Used for:
//   - Printing debugging messages to std::cout or std::cerr
//   - Displaying errors when assertions fail or invalid tokens occur

#include <sstream>
// Provides string streams (std::stringstream, std::istringstream).
// Useful for:
//   - Converting numeric strings to numbers (stod, stoi)
//   - Building or parsing strings dynamically
//   - Tokenization support when reading numbers or multi-character sequences

#include <string>
// Provides std::string class for managing text.
// Used for:
//   - Holding G-code program text
//   - Tokens, comments, addresses, and numeric strings

#include <vector>
// Provides std::vector container for dynamic arrays.
// Used extensively for:
//   - Storing sequences of characters (parse_stream<char>)
//   - Storing sequences of tokens (parse_stream<string>)
//   - Storing chunks within a block, and blocks within a program

namespace gpr {
// All parsing functionality is encapsulated inside the gpr namespace.
// This prevents naming collisions with other libraries or global functions.
// Contains types and functions for:
//   - Lexical analysis (lex_block, lex_token)
//   - Parsing individual tokens into structured chunks
//   - Converting sequences of chunks into blocks
//   - Creating gcode_program objects from blocks



	enum address_type {
	    ADDRESS_TYPE_INTEGER = 0,
	    ADDRESS_TYPE_DOUBLE
	};
	// Defines the type of a G-code address value.
	// Used to distinguish between integer-valued and double-valued addresses.
	// Examples:
	//   - Integer: G-codes, M-codes, line numbers
	//   - Double: X/Y/Z positions, feed rates, offsets

	union addr_value {
	    double dbl_val;
	    int int_val;
	};
	// Union to store the actual value of an address.
	// Only one of the members is valid at a time, determined by ad_type.
	// Saves memory compared to storing both types separately.

	class addr {
	private:
	    address_type ad_type;
	    // Type of the stored value (int or double)
	    addr_value val;
	    // Actual value stored in the union

	public:

	    addr(const address_type& p_ad_type,
	         const addr_value& p_val) {
	        // Constructor initializes both type and value
	        ad_type = p_ad_type;
	        val = p_val;
	    }

	    address_type tp() const { return ad_type; }
	    // Returns the type of the address
	    // Useful to decide which accessor to call (int_value or double_value)

	    double double_value() const {
	        // Returns the value as double
	        // Preconditions enforced by assertion:
	        assert(tp() == ADDRESS_TYPE_DOUBLE);
	        return val.dbl_val;
	    }

	    int int_value() const {
	        // Returns the value as int
	        // Preconditions enforced by assertion:
	        assert(tp() == ADDRESS_TYPE_INTEGER);
	        return val.int_val;
	    }

	    bool equals(const addr& other) const {
	        // Compares two addresses for equality
	        // Returns true if both type and value match
	        if (other.tp() != tp()) {
	            return false; // Different types cannot be equal
	        }

	        if (other.tp() == ADDRESS_TYPE_DOUBLE) {
	            return other.double_value() == double_value();
	        }

	        // Type must be integer here
	        return other.int_value() == int_value();
	    }

	    void print(std::ostream& out) const {
	        // Prints the address value to an output stream
	        // Chooses formatting based on type
	        if (tp() == ADDRESS_TYPE_DOUBLE) {
	            out << double_value();
	            return;
	        }

	        out << int_value();
	    }

	};
	// Summary of usage:
	//
	// - The `addr` class represents a single G-code address value, which can be either an integer or double.
	// - Union + type enum pattern ensures type safety and memory efficiency.
	// - Provides accessors with assertions to enforce correct type usage.
	// - Includes equality checking and printing utilities.
	// - Used in chunks to store numeric values associated with address letters (e.g., X10.5, G01, F200).






	enum chunk_type {
	    CHUNK_TYPE_COMMENT,
	    CHUNK_TYPE_WORD_ADDRESS,
	    CHUNK_TYPE_PERCENT,
	    CHUNK_TYPE_WORD
	};
	// Defines the type of a "chunk" in a G-code line.
	// A chunk is a single logical element of a block.
	// Possible types:
	// 1. CHUNK_TYPE_COMMENT       → A comment, usually enclosed in () or [].
	// 2. CHUNK_TYPE_WORD_ADDRESS  → A standard G-code word with a letter and numeric value, e.g., X10.5, G01.
	// 3. CHUNK_TYPE_PERCENT       → The '%' symbol, often used as program start/end marker.
	// 4. CHUNK_TYPE_WORD          → An isolated single-character word that does not have a numeric value, e.g., single letters or symbols.

	struct comment_data {
	    char left_delim;
	    char right_delim;
	    std::string comment_text;

	    comment_data() : left_delim('('), right_delim(')'), comment_text("") {}
	    // Default constructor:
	    //   - left_delim = '('
	    //   - right_delim = ')'
	    //   - comment_text = empty string
	    // Provides sensible defaults for parenthesis-style G-code comments.

	    comment_data(const char p_left_delim,
	                 const char p_right_delim,
	                 const std::string& p_comment_text) :
	      left_delim(p_left_delim),
	      right_delim(p_right_delim),
	      comment_text(p_comment_text) {}
	    // Parameterized constructor:
	    //   - Allows specifying custom delimiters and comment text
	    //   - Useful for comments using [], (), or other user-defined delimiters.
	};

	struct word_address_data {
	    char wd;   // The address letter (e.g., 'X', 'G', 'F')
	    addr adr;  // The numeric value associated with the address

	    word_address_data() :
	      wd('\0'),
	      adr(ADDRESS_TYPE_INTEGER, {-1}) {}
	    // Default constructor:
	    //   - wd = null character
	    //   - adr = integer -1
	    // Provides a safe uninitialized state.

	    word_address_data(const char p_wd,
	                      const addr p_adr) :
	      wd(p_wd), adr(p_adr) {}
	    // Parameterized constructor:
	    //   - wd = actual address letter
	    //   - adr = associated value (int or double)
	    // Used to store parsed word addresses as chunks in a block.
	};

	// Summary of usage:
	//
	// - `chunk_type` categorizes chunks for later interpretation.
	// - `comment_data` stores information about a comment's delimiters and text.
	// - `word_address_data` stores a letter + numeric value pair from G-code.
	// - Together with addr, these structures allow G-code lines to be parsed into
	//   a structured representation for further processing or execution.




	// The `chunk` class represents a single logical element within a G-code block (line).
	// A chunk can be one of four types, as defined in chunk_type:
	// 1. CHUNK_TYPE_COMMENT       → a comment enclosed in () or [] or other delimiters
	// 2. CHUNK_TYPE_WORD_ADDRESS  → a G-code word with a letter and numeric value (e.g., X10.0, G01)
	// 3. CHUNK_TYPE_PERCENT       → the special '%' symbol
	// 4. CHUNK_TYPE_WORD          → an isolated single-character word (e.g., standalone letters or symbols)
	//
	// Each block in a G-code program consists of one or more chunks. Examples:
	//
	// G-code example:
	//    (*** Toolpath 1 ***)
	//    G0 X0.0 Y0.0 Z0.0
	//    G1 X1.0 F23.0
	//    G1 Z-1.0 F10.0
	//
	// Block breakdown:
	// - First block: 1 chunk → CHUNK_TYPE_COMMENT ("(*** Toolpath 1 ***)")
	// - Second block: 4 chunks → CHUNK_TYPE_WORD_ADDRESS (G0, X0.0, Y0.0, Z0.0)
	//   In G0, letter = 'G', address = 0
	//   In X0.0, letter = 'X', address = 0.0, etc.

	class chunk {
	private:
	    chunk_type chunk_tp;       // Stores the type of this chunk

	    // Comment fields
	    comment_data cd;           // Stores left/right delimiters and comment text

	    // Word-address fields
	    word_address_data wad;     // Stores a letter and its associated addr value

	    // Isolated word fields
	    char single_word;          // Single character words (CHUNK_TYPE_WORD)

	public:
	    // Default constructor creates a percent chunk
	    chunk() : chunk_tp(CHUNK_TYPE_PERCENT) {}
	    virtual ~chunk() {}

	    // Constructor for isolated single-character word chunk
	    chunk(const char c) : chunk_tp(CHUNK_TYPE_WORD), single_word(c) {}

	    // Constructor for comment chunk
	    chunk(const char p_left_delim,
	          const char p_right_delim,
	          const std::string& p_comment_text) :
	        chunk_tp(CHUNK_TYPE_COMMENT),
	        cd(p_left_delim, p_right_delim, p_comment_text) {}

	    // Constructor for word-address chunk
	    chunk(const char p_wd,
	          const addr p_adr) :
	        chunk_tp(CHUNK_TYPE_WORD_ADDRESS),
	        cd('(', ')', ""),    // default comment fields unused
	        wad(p_wd, p_adr) {}

	    // Accessor for chunk type
	    chunk_type tp() const { return chunk_tp; }

	    // Accessors for comment fields (only valid if CHUNK_TYPE_COMMENT)
	    char get_left_delim() const { assert(tp() == CHUNK_TYPE_COMMENT); return cd.left_delim; }
	    char get_right_delim() const { assert(tp() == CHUNK_TYPE_COMMENT); return cd.right_delim; }
	    std::string get_comment_text() const { assert(tp() == CHUNK_TYPE_COMMENT); return cd.comment_text; }

	    // Accessors for word-address fields (only valid if CHUNK_TYPE_WORD_ADDRESS)
	    char get_word() const { assert(tp() == CHUNK_TYPE_WORD_ADDRESS); return wad.wd; }
	    addr get_address() const { assert(tp() == CHUNK_TYPE_WORD_ADDRESS); return wad.adr; }

	    // Accessor for isolated single-word character (only valid if CHUNK_TYPE_WORD)
	    char get_single_word() const { assert(tp() == CHUNK_TYPE_WORD); return single_word; }

	    // Comparison helpers
	    bool equals_word_address(const chunk& other_addr) const {
	        assert(other_addr.tp() == CHUNK_TYPE_WORD_ADDRESS);
	        return (get_word() == other_addr.get_word()) &&
	               (get_address().equals(other_addr.get_address()));
	    }

	    bool equals_comment(const chunk& other_comment) const {
	        assert(other_comment.tp() == CHUNK_TYPE_COMMENT);
	        return (get_comment_text() == other_comment.get_comment_text()) &&
	               (get_left_delim() == other_comment.get_left_delim()) &&
	               (get_right_delim() == other_comment.get_right_delim());
	    }

	    // General equality comparison
	    virtual bool equals(const chunk& other) const {
	        if (other.tp() != tp()) return false;

	        if (tp() == CHUNK_TYPE_WORD_ADDRESS) return equals_word_address(other);
	        else if (tp() == CHUNK_TYPE_COMMENT) return equals_comment(other);
	        else if (tp() == CHUNK_TYPE_PERCENT) return true; // any two '%' chunks are equal
	        else if (tp() == CHUNK_TYPE_WORD) return get_single_word() == other.get_single_word();
	        else assert(false);
	    }

	    // Printing helpers
	    void print_comment(std::ostream& stream) const {
	        stream << get_left_delim() << get_comment_text() << get_right_delim();
	    }

	    void print_word_address(std::ostream& stream) const {
	        stream << wad.wd;
	        wad.adr.print(stream); // prints numeric value (int or double)
	    }

	    void print_word(std::ostream& stream) const {
	        stream << get_single_word();
	    }

	    // General print function
	    void print(std::ostream& stream) const {
	        if (tp() == CHUNK_TYPE_COMMENT) print_comment(stream);
	        else if (tp() == CHUNK_TYPE_WORD_ADDRESS) print_word_address(stream);
	        else if (tp() == CHUNK_TYPE_PERCENT) stream << "%";
	        else if (tp() == CHUNK_TYPE_WORD) print_word(stream);
	        else assert(false);
	    }
	};

	// Summary:
	//
	// - A `chunk` encapsulates all types of data that can appear in a G-code block.
	// - Provides type-safe access to its internal data using assertions.
	// - Supports equality comparisons and printing for debugging or program generation.
	// - Forms the building block for parsing lines into structured G-code representations.





	// Factory functions and operators for creating and manipulating `chunk` objects.
	// These functions provide convenient, type-safe ways to instantiate chunks
	// without needing to call constructors directly, and they also enable comparisons
	// and stream output.

	chunk make_comment(const char start_delim,
	                   const char end_delim,
	                   const std::string& comment_text);
	// Creates a chunk of type CHUNK_TYPE_COMMENT.
	// Parameters:
	//   start_delim   : The opening delimiter of the comment, e.g., '(' or '['
	//   end_delim     : The closing delimiter of the comment, e.g., ')' or ']'
	//   comment_text  : The text contained within the comment
	// Returns:
	//   A `chunk` object initialized as a comment with the given delimiters and text.
	// Usage:
	//   chunk c = make_comment('(', ')', "Toolpath 1");

	chunk make_isolated_word(const char c);
	// Creates a chunk of type CHUNK_TYPE_WORD.
	// Parameters:
	//   c : Single character representing the isolated word
	// Returns:
	//   A `chunk` object initialized as a standalone word.
	// Usage:
	//   chunk w = make_isolated_word('A');

	chunk make_word_int(const char c, const int i);
	// Creates a chunk of type CHUNK_TYPE_WORD_ADDRESS with an integer value.
	// Parameters:
	//   c : The G-code letter (e.g., 'G', 'N', 'M')
	//   i : The integer value associated with the letter
	// Returns:
	//   A `chunk` representing a word-address pair with integer value.
	// Usage:
	//   chunk g = make_word_int('G', 1);

	chunk make_word_double(const char c, const double i);
	// Creates a chunk of type CHUNK_TYPE_WORD_ADDRESS with a double value.
	// Parameters:
	//   c : The G-code letter (e.g., 'X', 'Y', 'F')
	//   i : The floating-point value associated with the letter
	// Returns:
	//   A `chunk` representing a word-address pair with double value.
	// Usage:
	//   chunk x = make_word_double('X', 10.5);

	chunk make_percent_chunk();
	// Creates a chunk of type CHUNK_TYPE_PERCENT.
	// Represents the '%' symbol in G-code programs (often used as program start/end marker).
	// Usage:
	//   chunk p = make_percent_chunk();

	bool operator==(const chunk& l, const chunk& r);
	// Compares two chunks for equality.
	// Returns true if both chunks are of the same type and contain the same data.
	// Delegates to `chunk::equals` internally.
	// Usage:
	//   if (chunk1 == chunk2) { ... }

	bool operator!=(const chunk& l, const chunk& r);
	// Compares two chunks for inequality.
	// Returns true if the chunks are different in type or data.
	// Usage:
	//   if (chunk1 != chunk2) { ... }

	std::ostream& operator<<(std::ostream& stream, const chunk& ic);
	// Overloads the insertion operator to allow printing chunks to output streams.
	// Prints the chunk based on its type:
	//   - CHUNK_TYPE_COMMENT       → prints comment with delimiters
	//   - CHUNK_TYPE_WORD_ADDRESS  → prints letter + numeric value
	//   - CHUNK_TYPE_PERCENT       → prints '%'
	//   - CHUNK_TYPE_WORD          → prints single-character word
	// Usage:
	//   std::cout << chunk_obj << std::endl;






	// The `block` class represents a single line of G-code in a structured form. 
	// In G-code terminology, a "block" is synonymous with a "line" of code. 
	// Example G-code program:
	//      (*** Toolpath 1 ***)
	//      G0 X0.0 Y0.0 Z0.0
	//      G1 X1.0 F23.0
	//      G1 Z-1.0 F10.0
	// This program consists of 4 blocks, each corresponding to one line.

	// Each block stores:
	// - Optional line number (e.g., N1, N2)
	// - Optional slash status (slashed_out) indicating that the block is logically deleted
	// - A sequence of chunks (G-code words, addresses, comments, or symbols)
	// - Optional debug text for inspection in a debugger

	class block {
	protected:
	    bool has_line_no;                 // True if the block has a line number
	    int line_no;                      // Line number (valid only if has_line_no is true)
	    bool slashed_out;                 // True if block is marked as deleted (starts with '/')
	    std::vector<chunk> chunks;        // All parsed chunks contained in this block
	    std::string debug_text;           // Optional string representation for debugging

	public:

	    // Constructor for block with line number
	    block(const int p_line_no,
	          const bool p_slashed_out,
	          const std::vector<chunk> p_chunks) :
	        has_line_no(true),
	        line_no(p_line_no),
	        slashed_out(p_slashed_out),
	        chunks(p_chunks) {}

	    // Constructor for block without line number
	    block(const bool p_slashed_out,
	          const std::vector<chunk> p_chunks) :
	        has_line_no(false),
	        line_no(-1),             // -1 indicates no line number
	        slashed_out(p_slashed_out),
	        chunks(p_chunks) {}

	    // Copy constructor
	    block(const block& other) :
	        has_line_no(other.has_line_no),
	        line_no(other.line_no),
	        slashed_out(other.slashed_out) {
	        // Deep copy chunks
	        for (size_t i = 0; i < other.chunks.size(); i++) {
	            chunks.push_back(other.chunks[i]);
	        }
	    }

	    // Copy assignment operator
	    block& operator=(const block& other) {
	        has_line_no = other.has_line_no;
	        line_no = other.line_no;
	        slashed_out = other.slashed_out;
	        chunks.clear(); // Clear current chunks before copying
	        for (size_t i = 0; i < other.chunks.size(); i++) {
	            chunks.push_back(other.chunks[i]);
	        }
	        return *this;
	    }

	    // Returns a string representation of the block (line) for debugging or printing
	    std::string to_string() const {
	        std::ostringstream ss;
	        this->print(ss);
	        return ss.str();
	    }

	    // Set debug text explicitly
	    void set_debug_text(const std::string& text) {
	        debug_text = text;
	    }

	    // Default debug text: set to string representation of the block
	    void set_debug_text() {
	        set_debug_text(this->to_string());
	    }

	    // Print the block to an output stream
	    void print(std::ostream& stream) const {
	        if (has_line_number()) {
	            // Include line number if present
	            stream << "N" << line_number() << " ";
	        }
	        // Print all chunks separated by spaces
	        for (auto i : *this) { 
	            stream << i << " "; 
	        }
	    }

	    // Number of chunks in the block
	    int size() const { return chunks.size(); }

	    // Access a specific chunk by index (with bounds check)
	    const chunk& get_chunk(const int i) {
	        assert(i < size());
	        return chunks[i];
	    }

	    // Check if block is logically deleted (starts with '/')
	    bool is_deleted() const { return slashed_out; }

	    // Check if block has a line number
	    bool has_line_number() const { return has_line_no; }

	    // Get the line number (asserts if none)
	    int line_number() const {
	        assert(has_line_number());
	        return line_no;
	    }

	    // Iterators to allow range-based loops over chunks
	    std::vector<chunk>::const_iterator begin() const { return std::begin(chunks); }
	    std::vector<chunk>::const_iterator end() const { return std::end(chunks); }
	    std::vector<chunk>::iterator begin() { return std::begin(chunks); }
	    std::vector<chunk>::iterator end() { return std::end(chunks); }
	};

	// Summary of functionality:
	//
	// - Represents a parsed line of G-code with all its logical elements (chunks).
	// - Supports line numbers and slashed-out blocks (logical deletion).
	// - Provides deep copy constructors and assignment operators to manage chunk copies safely.
	// - Can generate string representations for debugging and printing.
	// - Implements iterators for convenient traversal of chunks.
	// - Core building block for parsing entire G-code programs into gcode_program objects.


	// The `gcode_program` class represents an entire G-code program, 
	// composed of multiple `block` objects. Each block corresponds to a line of G-code.

	class gcode_program {
	protected:
	    std::vector<block> blocks; // Stores all blocks (lines) in program order

	public:
	    // Constructor: initializes the program with a vector of blocks
	    gcode_program(const std::vector<block>& p_blocks) :
	        blocks(p_blocks) {}

	    // Returns the number of blocks in the program
	    int num_blocks() const { return blocks.size(); }

	    // Access a specific block by index
	    block get_block(const size_t i) {
	        assert(i < blocks.size()); // Ensure the index is valid
	        return blocks[i];
	        // Note: returns a copy of the block, not a reference
	        // If modification is needed, one should consider returning reference
	    }

	    // Constant iterators for read-only traversal of blocks
	    std::vector<block>::const_iterator begin() const { return std::begin(blocks); }
	    std::vector<block>::const_iterator end() const { return std::end(blocks); }

	    // Mutable iterators for modifying blocks during traversal
	    std::vector<block>::iterator begin() { return std::begin(blocks); }
	    std::vector<block>::iterator end() { return std::end(blocks); }
	};

	// Stream output operator overloads
	// Enables printing blocks and programs directly to output streams

	std::ostream& operator<<(std::ostream& stream, const block& block);
	// Prints a single block to the stream, typically using block.print()

	std::ostream& operator<<(std::ostream& stream, const gcode_program& program);
	// Prints the entire program block by block, separating them appropriately

	// Helper functions to create address objects conveniently

	addr make_int_address(const int i);
	// Creates an `addr` object with integer type and value i
	// Useful for G-codes, M-codes, line numbers, and other integer addresses

	addr make_double_address(const double i);
	// Creates an `addr` object with double type and value i
	// Useful for positional coordinates, feed rates, or any floating-point parameters

	// Summary:
	//
	// - gcode_program aggregates multiple blocks into a structured representation of a program.
	// - Provides safe access to individual blocks and iteration over all blocks.
	// - Stream operators allow easy printing for debugging or exporting.
	// - Helper functions provide type-safe creation of address values (int or double) for chunks.
	// - Forms the top-level container for fully parsed G-code programs.

}
