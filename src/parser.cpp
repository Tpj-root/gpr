#include "parser.h"
// Includes the declaration of parsing-related functions and interfaces.
// This header defines how G-code text is converted into structured data
// (tokens, blocks, and full programs).

#include <algorithm>
// Provides standard algorithms such as std::find, std::transform,
// std::remove_if, etc.
// These are commonly used during parsing for filtering characters,
// trimming whitespace, or processing token lists.

#include <fstream>
// Provides file stream classes like std::ifstream and std::ofstream.
// Used to read G-code files from disk into memory before parsing.

#include <iostream>
// Provides standard input/output streams such as std::cout and std::cerr.
// Typically used for debugging, logging, or error reporting during parsing.

#include <sstream>
// Provides string stream classes such as std::stringstream and std::istringstream.
// These are heavily used in parsers to:
// - Break strings into tokens
// - Read formatted data from strings
// - Convert between strings and numeric types

#include <streambuf>
// Provides low-level stream buffer functionality.
// Often used for efficient reading of entire files into a std::string
// using iterators (istreambuf_iterator).

using namespace std;
// Brings all names from the std namespace into the global scope of this file.
// This allows writing:
//   string, vector, ifstream, cout
// instead of:
//   std::string, std::vector, std::ifstream, std::cout
//
// Note:
// This is generally acceptable in .cpp files (implementation files),
// but usually avoided in header files to prevent namespace pollution.


namespace gpr {
  
	template<typename T>
	// Generic template structure.
	// T represents the type of elements in the stream (e.g., char, string, token).
	struct parse_stream {
	
	  size_t i;
	  // Index pointing to the current position in the stream.
	  // Acts like a cursor while parsing.
	  // size_t is used because it is the standard type for indexing containers.
	
	  vector<T> s;
	  // Internal storage of the stream elements.
	  // The entire input is copied into this vector so that parsing
	  // can be done with random access and safe bounds checking.
	
	  template<typename R>
	  // Templated constructor to allow flexibility in input type.
	  // R can be any container type that supports begin() and end()
	  // (e.g., std::string, std::vector<T>, std::list<T>).
	  parse_stream(R v) : s(v.begin(), v.end()) {
	    // Copies all elements from the input container into vector s.
	    // This normalizes the input into a single internal representation.
	
	    i = 0;
	    // Initializes the stream cursor to the first element.
	  }
	
	  T next() {
	    // Returns the current element at index i.
	    // Does NOT advance the cursor.
	    // Caller is responsible for incrementing the stream.
	    return s[i];
	  }
	
	  int chars_left() const {
	    // Checks whether there are still elements left to parse.
	    // Returns true (1) if current index is within bounds,
	    // false (0) if the end of the stream is reached.
	    return i < s.size();
	  }
	
	  parse_stream<T>& operator++(int) {
	    // Post-increment operator (stream++).
	    // Advances the parsing cursor forward by one element.
	    // Returns a reference to the current parse_stream object.
	    i++;
	    return *this;
	  }
	
	  parse_stream<T>& operator--(int) {
	    // Post-decrement operator (stream--).
	    // Moves the parsing cursor backward by one element.
	    // Useful for backtracking during parsing.
	    i--;
	    return *this;
	  }
	
	  typename vector<T>::const_iterator end() {
	    // Returns a constant iterator to the end of the internal vector.
	    // Useful for comparisons and standard iteration logic.
	    return s.end();
	  }
	
	  typename vector<T>::const_iterator begin() {
	    // Returns a constant iterator to the beginning of the internal vector.
	    // Allows the parse_stream to behave like a standard container.
	    return s.begin();
	  }
	  
	  typename vector<T>::const_iterator remaining() {
	    // Returns an iterator pointing to the current parsing position.
	    // Represents the unconsumed part of the stream.
	    // Useful for passing remaining data to other parsing functions.
	    return s.begin() + i;
	  }
	
	};
	
	typedef parse_stream<char> parse_state;
	// Creates an alias named parse_state for parse_stream<char>.
	// This represents a parsing stream operating on characters.
	// Used for low-level character-by-character parsing of G-code text.
	
	bool is_num_char(const char c) {
	  // Checks whether a character can be part of a numeric literal.
	  // Valid numeric characters include:
	  //   - Digits: 0–9
	  //   - Decimal point: '.'
	  //   - Minus sign: '-' (for negative numbers)
	  //
	  // This function does NOT validate full numbers;
	  // it only checks whether a single character is allowed
	  // inside a numeric token.
	  return (isdigit(c) ||
	          (c == '.') ||
	          (c == '-'));
	}
	
	void ignore_whitespace(parse_state& s) {
	  // Advances the parsing cursor while the current character is:
	  //   - Any whitespace character (space, tab, newline, etc.)
	  //   - A carriage return ('\r')
	  //
	  // Purpose:
	  //   - G-code often contains arbitrary spacing
	  //   - Whitespace usually has no semantic meaning
	  //
	  // The loop stops when:
	  //   - No characters are left, OR
	  //   - The next character is not whitespace
	  while (s.chars_left() && (isspace(s.next()) || s.next() == '\r')) {
	    s++;  // Move to the next character
	  }
	}
	
	string string_remaining(parse_state& ps) {
	  // Returns the unparsed remainder of the character stream
	  // as a std::string.
	  //
	  // ps.remaining() → iterator to current parsing position
	  // ps.end()       → iterator to end of stream
	  //
	  // Useful for:
	  //   - Debug messages
	  //   - Error reporting
	  //   - Showing what part of the input could not be parsed
	  return string(ps.remaining(), ps.end());
	}
	
	void parse_char(char c, parse_state& s) {
	  // Attempts to parse (consume) a specific expected character.
	  //
	  // If the next character in the stream matches `c`:
	  //   - The cursor is advanced by one character
	  //   - Parsing continues normally
	  //
	  // If it does NOT match:
	  //   - An error message is printed
	  //   - The program aborts via assert(false)
	  //
	  // This function enforces strict syntax rules,
	  // ensuring that required characters (like '(', ')', or '%')
	  // appear exactly where expected.
	  if (s.next() == c) {
	    s++;
	    return;
	  }
	
	  cout << "Cannot parse char " << c
	       << " from string " << string_remaining(s) << endl;
	
	  assert(false); // Hard failure: parsing cannot continue
	}
	
	double parse_double(parse_stream<string>& s) {
	  // Parses the next token in the stream as a double-precision number.
	  //
	  // Assumptions:
	  //   - s.next() returns a valid numeric string
	  //   - Example: "12.5", "-3.0", "100"
	  //
	  // std::stod converts the string to a double.
	  // If conversion fails, it throws an exception.
	  double v = stod(s.next());
	
	  // Advance the token stream after successful parsing
	  s++;
	
	  // Return the parsed numeric value
	  return v;
	}
	
	int parse_int(parse_stream<string>& s) {
	  // Parses the next token in the stream as an integer.
	  //
	  // Assumptions:
	  //   - s.next() returns a valid integer string
	  //   - Example: "10", "-5"
	  //
	  // std::stoi converts the string to an int.
	  // If conversion fails, it throws an exception.
	  int i = stoi(s.next());
	
	  // Advance the token stream after successful parsing
	  s++;
	
	  // Return the parsed integer value
	  return i;
	}


	addr parse_address(char c, parse_stream<string>& s) {
	  // Parses a single G-code address (letter + value).
	  //
	  // Input:
	  //   c : The address letter already read from the G-code stream.
	  //       Examples: 'X', 'Y', 'G', 'M', 'F', etc.
	  //
	  //   s : A token stream containing the remaining values as strings.
	  //       The next token is expected to be the numeric value
	  //       associated with the address letter.
	  //
	  // Output:
	  //   addr : A strongly-typed address object representing
	  //          either a floating-point or integer G-code address.
	  //
	  // Behavior:
	  //   - Chooses how to parse the value based on the address letter
	  //   - Axis, feed, radius, and offset values → double
	  //   - Modal and program control values → int

	  switch(c) {

	    // ------------------------------------------------------------
	    // Double-valued G-code addresses
	    // ------------------------------------------------------------
	    // These addresses represent real-valued quantities such as:
	    //   - Positions (X, Y, Z)
	    //   - Rotational axes (A, B, C)
	    //   - Secondary axes (U, V, W)
	    //   - Arc offsets (I, J, K)
	    //   - Feed rate (F)
	    //   - Radius / retract / parameters (R, Q)
	    //   - Spindle speed (S)
	    //
	    // Both uppercase and lowercase letters are supported to make
	    // the parser case-insensitive.
	    case 'X': case 'Y': case 'Z':
	    case 'A': case 'B': case 'C':
	    case 'U': case 'V': case 'W':
	    case 'I': case 'J': case 'K':
	    case 'F': case 'R': case 'Q':
	    case 'S':
	    case 'x': case 'y': case 'z':
	    case 'a': case 'b': case 'c':
	    case 'u': case 'v': case 'w':
	    case 'i': case 'j': case 'k':
	    case 'f': case 'r': case 's':
	    case 'q':
	    case 'E':
	      // Parses the next token as a double and wraps it
	      // in a double-valued address object.
	      return make_double_address(parse_double(s));

	    // ------------------------------------------------------------
	    // Integer-valued G-code addresses
	    // ------------------------------------------------------------
	    // These addresses represent discrete or modal values such as:
	    //   - G codes (motion / mode selection)
	    //   - M codes (machine control)
	    //   - Tool numbers, line numbers, parameters
	    //
	    // Both uppercase and lowercase letters are accepted.
	    case 'G': case 'H': case 'M':
	    case 'N': case 'O': case 'T':
	    case 'P': case 'D': case 'L':
	    case 'g': case 'h': case 'm':
	    case 'n': case 'o': case 't':
	    case 'p': case 'd': case 'l':
	      // Parses the next token as an integer and wraps it
	      // in an integer-valued address object.
	      return make_int_address(parse_int(s));

	    // ------------------------------------------------------------
	    // Error handling
	    // ------------------------------------------------------------
	    default:
	      // If the address letter is not recognized,
	      // print detailed diagnostic information.
	      cout << "Invalid c = " << c << endl;
	      cout << "Invalid c as int = " << ((int) c) << endl;
	      cout << "Is EOF? " << (((int) c) == EOF) << endl;

	      // Hard failure: invalid or unsupported G-code address.
	      // Parsing cannot continue safely beyond this point.
	      assert(false);
	  }
	}


	string parse_line_comment_with_delimiter(string sc, parse_stream<string>& s) {
	  // Parses a line-style comment where the delimiter has already been detected.
	  //
	  // Input:
	  //   sc : The starting comment delimiter as a string.
	  //        (Note: the delimiter itself is not re-parsed here;
	  //         it is assumed to have been handled by the caller.)
	  //
	  //   s  : A token-based parsing stream (parse_stream<string>)
	  //        positioned at the first token *after* the comment delimiter.
	  //
	  // Behavior:
	  //   - Consumes all remaining tokens in the current line
	  //   - Concatenates them into a single string
	  //   - Stops only when no tokens are left
	  //
	  // Typical use case:
	  //   G-code line comments such as:
	  //     ; this is a comment
	  //     // another comment
	  //
	  // The parser treats everything after the delimiter as comment text.

	  string text = "";
	  // Accumulates the full comment content.

	  while (s.chars_left()) {
	    // Append the current token to the comment text
	    text += s.next();

	    // Advance to the next token
	    s++;
	  }

	  // Returns the complete comment text (excluding the delimiter itself)
	  return text;
	}

	string parse_comment_with_delimiters(char sc, char ec, parse_state& s) {
	  // Parses a block-style comment with explicit start and end delimiters.
	  //
	  // Input:
	  //   sc : Start comment character (e.g., '(' )
	  //   ec : End comment character   (e.g., ')' )
	  //
	  //   s  : A character-based parsing stream (parse_state)
	  //        positioned at the start delimiter.
	  //
	  // Behavior:
	  //   - Supports nested comments using a depth counter
	  //   - Continues parsing until the matching end delimiter is found
	  //   - Includes delimiters in the returned comment text
	  //
	  // Example (G-code):
	  //   (outer comment (nested comment) end)
	  //
	  // depth progression:
	  //   '('  → depth = 1
	  //   '('  → depth = 2
	  //   ')'  → depth = 1
	  //   ')'  → depth = 0  → stop parsing

	  int depth = 0;
	  // Tracks nesting level of comments.
	  // Parsing continues until depth returns to zero.

	  string text = "";
	  // Accumulates the full comment text, including delimiters.

	  do {
	    if (s.next() == sc) {
	      // Found a start delimiter → increase nesting depth
	      depth++;

	      // Append delimiter to comment text
	      text += s.next();
	    }
	    else if (s.next() == ec) {
	      // Found an end delimiter → decrease nesting depth
	      depth--;

	      // Append delimiter to comment text
	      text += s.next();
	    }
	    else {
	      // Regular character inside the comment
	      text += s.next();
	    }

	    // Advance to the next character in the stream
	    s++;

	  } while (s.chars_left() && depth > 0);
	  // Continue until:
	  //   - End of stream is reached, OR
	  //   - All nested comments are closed (depth == 0)

  // Returns the complete comment including start/end delimiters
  return text;
}




	string parse_comment_with_delimiters(string sc,
	                                     string ec,
	                                     parse_stream<string>& s) {
	  // Parses a token-based (string) comment with explicit start and end delimiters.
	  //
	  // Input:
	  //   sc : Start comment delimiter as a string
	  //        Example: "/*", "(", or "<"
	  //
	  //   ec : End comment delimiter as a string
	  //        Example: "*/", ")", or ">"
	  //
	  //   s  : A token-based parsing stream (parse_stream<string>)
	  //        positioned at the start delimiter token.
	  //
	  // Behavior:
	  //   - Supports nested comments using a depth counter
	  //   - Consumes tokens until the matching end delimiter is found
	  //   - Does NOT include the delimiters themselves in the returned text
	  //
	  // Example:
	  //   Tokens: ["(", "this", "(", "nested", ")", "comment", ")"]
	  //
	  //   depth tracking:
	  //     "(" → depth = 1
	  //     "(" → depth = 2
	  //     ")" → depth = 1
	  //     ")" → depth = 0 → stop

	  int depth = 0;
	  // Tracks the nesting level of comments.
	  // Parsing continues until depth returns to zero.

	  string text = "";
	  // Accumulates the inner content of the comment.

	  do {
	    if (s.next() == sc) {
	      // Encountered a start delimiter token
	      // Increase nesting depth
	      depth++;
	    }
	    else if (s.next() == ec) {
	      // Encountered an end delimiter token
	      // Decrease nesting depth
	      depth--;
	    }
	    else {
	      // Regular token inside the comment body
	      // Append it to the accumulated text
	      text += s.next();
	    }

	    // Advance to the next token in the stream
	    s++;

	  } while (s.chars_left() && depth > 0);
	  // Continue until:
	  //   - End of token stream is reached, OR
	  //   - All nested comment levels are closed

	  // Returns only the comment content (excluding delimiters)
	  return text;
	}

	chunk parse_isolated_word(parse_stream<string>& s) {
	  // Parses a single-character token that stands alone as a word.
	  //
	  // Input:
	  //   s : A token-based parsing stream positioned at an isolated token.
	  //
	  // Preconditions enforced by assertions:
	  //   - There must be at least one token left to parse
	  //   - The next token must contain exactly one character
	  //
	  // This function is used for tokens that represent meaningful
	  // standalone characters in G-code syntax.

	  assert(s.chars_left());
	  // Ensures there is at least one token available.

	  assert(s.next().size() == 1);
	  // Ensures the token is exactly one character long.

	  char c = s.next()[0];
	  // Extract the single character from the token.

	  s++;
	  // Advance the stream after consuming the token.

	  // Wrap the character in a chunk representing an isolated word.
	  return make_isolated_word(c);
	}




	chunk parse_word_address(parse_stream<string>& s) {
	  // Parses a standard G-code word address of the form:
	  //   <letter><value>
	  // Example:
	  //   X10.5, G01, M3, F200
	  //
	  // Input:
	  //   s : A token-based parsing stream positioned at the address letter.
	  //
	  // Preconditions (enforced by assertions):
	  //   - There must be at least one token left to parse
	  //   - The next token must be exactly one character long
	  //     (the address letter itself)

	  assert(s.chars_left());
	  // Ensures there is at least one token available.

	  assert(s.next().size() == 1);
	  // Ensures the token represents a single address letter.

	  char c = s.next()[0];
	  // Extract the address letter (e.g., 'X', 'G', 'M').

	  s++;
	  // Advance the stream to the value token.

	  addr a = parse_address(c, s);
	  // Parses the numeric value following the address letter.
	  // The type (int or double) depends on the address letter.

	  return chunk(c, a);
	  // Combines the address letter and parsed value
	  // into a single chunk object.
	}

	chunk parse_chunk(parse_stream<string>& s) {
	  // Parses the next logical chunk from the token stream.
	  //
	  // A chunk represents one atomic G-code element:
	  //   - Comments
	  //   - Addresses (X10, G01, etc.)
	  //   - Isolated symbols
	  //   - Special markers like '%'
	  //
	  // Input:
	  //   s : Token-based parsing stream at the start of a chunk.

	  assert(s.chars_left());
	  // Ensures there is something to parse.

	  if (s.next()[0] == '[') {
	    // ----------------------------------------------------------
	    // Square-bracket comment or expression
	    // Example token: "[THIS IS A COMMENT]"
	    //
	    // Assumption:
	    //   The entire bracketed content is already tokenized
	    //   into a single string token.
	    string cs = s.next();
	    s++;

	    // Remove the surrounding brackets and store inner content
	    return chunk('[', ']', cs.substr(1, cs.size() - 2));

	  } else if (s.next()[0] == '(') {
	    // ----------------------------------------------------------
	    // Parentheses comment
	    // Example token: "(THIS IS A COMMENT)"
	    //
	    // Assumption:
	    //   The entire comment is a single token.
	    string cs = s.next();
	    s++;

	    // Remove surrounding parentheses and store inner content
	    return chunk('(', ')', cs.substr(1, cs.size() - 2));

	  } else if (s.next() == "%") {
	    // ----------------------------------------------------------
	    // Program start / end marker
	    //
	    // '%' is a special standalone symbol in G-code.
	    s++;
	    return make_percent_chunk();

	  } else if (s.next() == ";") {
	    // ----------------------------------------------------------
	    // Line comment delimiter
	    // Everything after ';' until end of line is a comment.
	    s++;

	    string cs = parse_line_comment_with_delimiter(";", s);
	    // Parses the rest of the line as comment text.

	    return chunk(';', ';', cs);

	  } else {
	    // ----------------------------------------------------------
	    // Either an isolated word OR a word address
	    //
	    // Look ahead one token to decide.
	    string next_next = *(s.remaining() + 1);
	    // Peek at the token after the current one.

	    if (!is_num_char(next_next[0])) {
	      // If the following token does NOT start with a numeric character,
	      // then this is a standalone symbol or letter.
	      return parse_isolated_word(s);
	    }

	    // Otherwise, it is a standard word address (letter + number).
	    return parse_word_address(s);
	  }
	}



  
	bool parse_slash(parse_state& s) {
	  // Parses a slash character '/' from a character-based stream.
	  //
	  // Input:
	  //   s : parse_state (alias for parse_stream<char>)
	  //       positioned at the current character.
	  //
	  // Behavior:
	  //   - Checks if the next character is '/'
	  //   - If yes: consumes it and returns true
	  //   - If no : leaves the stream unchanged and returns false
	  //
	  // Use case:
	  //   In G-code, '/' is often used as an optional block skip indicator.

	  if (s.next() == '/') {
	    // Found slash → consume it
	    s++;
	    return true;
	  }

	  // No slash at current position
	  return false;
	}

	bool is_slash(const string& s) {
	  // Checks whether a string token represents a slash.
	  //
	  // Input:
	  //   s : token string
	  //
	  // Returns:
	  //   true  → token is exactly "/"
	  //   false → otherwise

	  if (s.size() != 1) {
	    // Token must be exactly one character long
	    return false;
	  }

	  return s[0] == '/';
	}

	bool parse_slash(parse_stream<string>& s) {
	  // Parses a slash '/' from a token-based stream.
	  //
	  // Input:
	  //   s : parse_stream<string>
	  //       positioned at the next token.
	  //
	  // Behavior:
	  //   - Uses is_slash() to check token validity
	  //   - If token is "/", consumes it and returns true
	  //   - Otherwise, leaves stream unchanged and returns false
	  //
	  // This overload allows the same logical operation
	  // to work on both character-level and token-level streams.

	  if (is_slash(s.next())) {
	    s++;
	    return true;
	  }

	  return false;
	}

	std::pair<bool, int> parse_line_number(parse_stream<string>& s) {
	  // Parses an optional G-code line number.
	  //
	  // Format:
	  //   N<number>
	  // Example:
	  //   N10, N100, N5
	  //
	  // Input:
	  //   s : token-based parsing stream
	  //       positioned at the potential 'N' token.
	  //
	  // Output:
	  //   std::pair<bool, int>
	  //     first  → true if a line number was parsed
	  //     second → parsed line number value
	  //
	  // If no line number is present:
	  //   returns (false, -1)

	  if (s.next() == "N") {
	    // Found line number prefix
	    s++;

	    // Parse the integer value following 'N'
	    int ln = parse_int(s);

	    return std::make_pair(true, ln);
	  }

	  // No line number at current position
	  return std::make_pair(false, -1);
	}






	block parse_tokens(const std::vector<string>& tokens) {
	  // Converts a list of lexical tokens into a single G-code block.
	  //
	  // Input:
	  //   tokens : All tokens extracted from one logical line of G-code.
	  //
	  // Output:
	  //   block  : A structured representation of that line, including:
	  //            - Optional slash (optional block skip)
	  //            - Optional line number (N-code)
	  //            - Parsed chunks (addresses, comments, symbols)

	  if (tokens.size() == 0) {
	    // Empty line → return an empty block.
	    // 'false' indicates no slash and no line number.
	    return block(false, {});
	  }

	  // Create a token stream to parse sequentially.
	  parse_stream<string> s(tokens);

	  vector<chunk> chunks;
	  // Will hold all parsed chunks for this block.

	  bool is_slashed = parse_slash(s);
	  // Checks for optional block skip ('/') at the start of the line.

	  std::pair<bool, int> line_no = parse_line_number(s);
	  // Attempts to parse an optional line number (Nxxxx).

	  while (s.chars_left()) {
	    // Parse tokens until the end of the line.
	    chunk ch = parse_chunk(s);
	    chunks.push_back(ch);
	  }

	  if (line_no.first) {
	    // Line number was present.
	    return block(line_no.second, is_slashed, chunks);
	  } else {
	    // No line number present.
	    return block(is_slashed, chunks);
	  }
	}

	vector<block> lex_gprog(const string& str) {
	  // Lexes and parses an entire G-code program string into blocks.
	  //
	  // Input:
	  //   str : Full G-code program text (multiple lines).
	  //
	  // Output:
	  //   vector<block> : One block per non-empty line.

	  vector<block> blocks;
	  // Stores all parsed blocks.

	  string::const_iterator line_start = str.begin();
	  string::const_iterator line_end;

	  while (line_start < str.end()) {
	    // Find the end of the current line.
	    line_end = find(line_start, str.end(), '\n');

	    // Extract the line text.
	    string line(line_start, line_end);

	    if (line.size() > 0) {
	      // Ignore empty lines.

	      // Lex the line into tokens.
	      vector<string> line_tokens = lex_block(line);

	      // Parse tokens into a structured block.
	      block b = parse_tokens(line_tokens);

	      blocks.push_back(b);
	    }

	    // Move to the start of the next line.
	    // +1 skips the newline character.
	    line_start += line.size() + 1;
	  }

	  return blocks;
	}

	gcode_program parse_gcode(const std::string& program_text) {
	  // High-level entry point for parsing G-code.
	  //
	  // Converts raw program text into a gcode_program object.
	  // Does NOT preserve original line text.

	  auto blocks = lex_gprog(program_text);
	  return gcode_program(blocks);
	}

	gcode_program parse_gcode_saving_block_text(const std::string& program_text) {
	  // Variant of parse_gcode that preserves original block text
	  // for debugging or inspection purposes.

	  auto blocks = lex_gprog(program_text);

	  for (auto& b : blocks) {
	    // Store original text inside each block for debugging.
	    b.set_debug_text();
	  }

	  return gcode_program(blocks);
	}

	std::string digit_string(parse_state& s) {
	  // Parses a sequence of numeric characters from a character stream.
	  //
	  // Input:
	  //   s : parse_state (character-based parsing stream)
	  //
	  // Output:
	  //   string : Continuous numeric substring
	  //            (digits, decimal point, minus sign)

	  string num_str = "";
	  // Accumulates numeric characters.

	  while (s.chars_left() && is_num_char(s.next())) {
	    // Continue while the next character is part of a number.
	    num_str += s.next();
	    s++;
	  }

	  // Returns the parsed numeric string.
	  return num_str;
	}


	std::string lex_token(parse_state& s) {
	  // Lexically extracts the next token from a character-based stream.
	  //
	  // Input:
	  //   s : parse_state (alias for parse_stream<char>)
	  //       positioned at the first character of the next token.
	  //
	  // Output:
	  //   string : The next token in the stream.
	  //            Can be:
	  //              - Numeric literal (digits, decimal, minus)
	  //              - Comment (parentheses or square brackets)
	  //              - Single-character symbols

	  assert(s.chars_left());
	  // Ensure there is at least one character left to parse.

	  char c = s.next();
	  // Peek at the current character in the stream.

	  string next_token = "";
	  // Placeholder to store the token if needed.

	  if (is_num_char(c)) {
	    // If the character is part of a number (digit, '.', '-'):
	    // Parse the full numeric sequence and return it.
	    return digit_string(s);
	  }

	  switch(c) {
	    case '(':
	      // Start of a parenthesis comment.
	      // Delegates to parse_comment_with_delimiters which handles
	      // nested comments and consumes all characters until the matching ')'.
	      return parse_comment_with_delimiters('(', ')', s);

	    case '[':
	      // Start of a square bracket comment or special expression.
	      return parse_comment_with_delimiters('[', ']', s);

	    case ')':
	      // Unexpected closing parenthesis at this point → syntax error
	      assert(false);

	    case ']':
	      // Unexpected closing square bracket → syntax error
	      assert(false);

	    default:
	      // Any other character is treated as a single-character token.
	      next_token = c;
	      s++; // Advance past the character
	      return next_token;
	  }
	}

	std::vector<std::string> lex_block(const std::string& block_text) {
	  // Converts a raw G-code line (string) into a sequence of tokens.
	  //
	  // Input:
	  //   block_text : A single line of G-code.
	  //
	  // Output:
	  //   vector<string> : All lexical tokens extracted from the line.
	  //                    Tokens include numbers, addresses, comments,
	  //                    and single-character symbols.

	  parse_state s(block_text);
	  // Create a character-based parsing stream from the line text.

	  vector<string> tokens;
	  // Stores all extracted tokens for this line.

	  ignore_whitespace(s);
	  // Skip leading spaces or carriage returns before parsing.

	  while (s.chars_left()) {
	    // Continue parsing until the end of the line.

	    ignore_whitespace(s);
	    // Skip any whitespace before the next token.

	    if (s.chars_left()) {
	      // Extract the next token using lex_token
	      string token = lex_token(s);
	      tokens.push_back(token);
	    }
	  }

	  return tokens;
	}

	// ------------------------------------------------------------
	// Notes on lexing design:
	//
	// - Numbers are recognized by is_num_char and parsed as contiguous sequences.
	// - Comments are fully captured including nested ones.
	// - Single-character symbols are preserved as individual tokens.
	// - Whitespace is ignored and never returned as a token.
	// - This function prepares input for parse_tokens, which
	//   constructs structured block objects from tokens.
	//
	// Lexing is the first step in the G-code parsing pipeline:
	// raw text → lex_block → tokens → parse_tokens → block → gcode_program


}
