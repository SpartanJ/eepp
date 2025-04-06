#include <eepp/system/parsermatcher.hpp>

namespace EE { namespace System {

// Enum to specify the language standard context
enum LanguageStandard {
	LANG_C,	 // Target C (roughly C99/C11 - no binary, no separators, no 'z' suffix)
	LANG_CPP // Target C++ (roughly C++14/C++23 - includes binary, separators, 'z' suffix)
};

// Helper functions (needed by the logic, inferred from original code)
namespace { // Use an anonymous namespace to keep helpers local to this translation unit

inline bool isBinaryDigit( char c ) {
	return c == '0' || c == '1';
}

inline bool isOctalDigit( char c ) {
	return c >= '0' && c <= '7';
}

// Checks if a character 'c' is a valid digit for the given base.
// Handles C++ digit separators (') within the main logic, not here.
inline bool isValidDigitChar( char c, int base ) {
	switch ( base ) {
		case 2:
			return isBinaryDigit( c );
		case 8:
			return isOctalDigit( c );
		case 10:
			return std::isdigit( c );
		case 16:
			return std::isxdigit( c );
		default:
			return false;
	}
}

/*
  consumeDigitsWithSep

  Consumes digits (or letters for hex) in the supplied input string starting at pos,
  while allowing numeric separators ('_') under these restrictions:
	- An underscore is allowed only between valid digits.
	- Not allowed at the start or end of the sequence.
	- Not allowed consecutively.

  The flag "requireDigits" indicates if at least one digit is required to be present.

  Parameters:
	input         - the C-string to parse.
	pos           - current position (will be updated).
	length        - total length of input.
	base          - numeric base (2, 8, 10, or 16).
	requireDigits - if true, at least one digit is required.
	underscoreSeparatorSupported - if true, '_' separators are enabled

  Returns:
	true if a valid sequence according to the rules was consumed, false otherwise.
	Updates pos to point after the consumed sequence ONLY on success.
	On failure, the value of pos is undefined/unreliable.
*/
inline bool consumeDigitsWithSep( const char* input, int& pos, int length, int base,
								  bool requireDigits, bool underscoreSeparatorSupported ) {
	int currentPos = pos; // Use temporary position to avoid altering pos on failure
	int startPos = currentPos;
	int digitCount = 0;
	bool lastWasDigit = false; // Track if the immediately preceding char was a digit

	while ( currentPos < length ) {
		char c = input[currentPos];
		if ( c == '_' ) {
			// Invalid if:
			// 1. It's the first character being consumed in this sequence.
			// 2. The immediately preceding character was not a digit.
			if ( !underscoreSeparatorSupported || currentPos == startPos || !lastWasDigit ) {
				return false; // Invalid underscore placement (start or consecutive)
			}
			// Valid underscore position, consume it.
			lastWasDigit = false; // Next char must be a digit
			currentPos++;
		} else if ( isValidDigitChar( c, base ) ) {
			digitCount++;
			lastWasDigit = true; // Mark that we saw a digit
			currentPos++;
		} else {
			break; // Not a digit or underscore, end of sequence for this part
		}
	}

	// After the loop, check validity:
	// 1. Cannot end with an underscore
	// This is implicitly checked by !lastWasDigit. If the loop ended normally
	// (not via break) or broke on a non-digit/non-underscore, and the last
	// character *consumed* was '_', then lastWasDigit will be false.
	if ( !lastWasDigit && currentPos > startPos ) {
		// Check if the loop didn't just break on a valid char (e.g. '.' or 'e')
		// If the previous char was indeed '_', it's an error.
		if ( input[currentPos - 1] == '_' ) {
			return false; // Invalid: ends with underscore
		}
	}

	// 2. Check if digits were required and if we got any
	if ( requireDigits && digitCount == 0 ) {
		return false; // Required digits but found none
	}

	// If we reached here, the sequence consumed (if any) is valid.
	// Update the original pos.
	pos = currentPos;
	return true;
}

} // anonymous namespace

/**
  isNumberLiteralBase checks if a substring (starting at stringStartOffset)
  in the given C-string is a valid JavaScript/TypeScript number literal (which supports the most
  common cases in many languages).

  It supports:
	- Decimal literal, including fractional parts and exponent.
	- Hexadecimal (0x or 0X), binary (0b or 0B), octal (0o or 0O) literals.
	- BigInt literal with trailing "n", but only on integer literals.
	- Numeric separators "_" between digits (but not multiple in a row, not
	  at the start or end of a digit sequence, and not immediately after a prefix).

  If a valid literal is found, the function writes the start and end indices
  (zero-based positions into stringSearch) to matchList, and returns 1.
  If no valid number literal is found at the offset, the function returns 0.
*/
inline size_t isNumberLiteralBase( const char* stringSearch, int stringStartOffset,
								   PatternMatcher::Range* matchList, size_t stringLength,
								   bool underscoreSeparatorSupported, bool supportsOctal,
								   bool supportsBinary, bool supportsBigInt ) {
	if ( stringStartOffset < 0 || (size_t)stringStartOffset >= stringLength )
		return 0;

	int pos = stringStartOffset;
	const int start = pos; // Keep original start index

	bool isSigned = false;
	bool hasDecimalPoint = false;
	bool hasExponent = false;
	bool consumedSomethingAfterSign =
		false; // Track if any part of number (digit, dot) is consumed after sign

	// 1. Check for optional leading sign (+ or -)
	if ( stringSearch[pos] == '+' || stringSearch[pos] == '-' ) {
		isSigned = true;
		pos++;
		if ( pos >= (int)stringLength )
			return 0; // Sign alone is invalid
	}

	// Store the position after the potential sign
	int afterSignPos = pos;

	// Cannot have underscore immediately after sign
	if ( pos < (int)stringLength && stringSearch[pos] == '_' ) {
		return 0;
	}

	// 2. Handle different literal types based on the character AFTER the sign (if any)
	if ( stringSearch[afterSignPos] == '0' ) {
		// Potential 0, 0x, 0b, 0o, 0.123, 0e5, 0123 (decimal)
		if ( isSigned && ( pos + 1 < (int)stringLength &&
						   ( stringSearch[pos + 1] == 'x' || stringSearch[pos + 1] == 'X' ||
							 stringSearch[pos + 1] == 'b' || stringSearch[pos + 1] == 'B' ||
							 stringSearch[pos + 1] == 'o' || stringSearch[pos + 1] == 'O' ) ) ) {
			// Signed non-decimal (e.g., +0x1) is invalid in JS/TS
			return 0;
		}

		pos++; // Consume '0'
		consumedSomethingAfterSign = true;

		if ( pos < (int)stringLength ) {
			char next = stringSearch[pos];
			int base = 0; // 0 indicates potential decimal or just '0'

			if ( next == 'x' || next == 'X' )
				base = 16;
			else if ( next == 'b' || next == 'B' ) {
				base = 2;
				if ( !supportsBinary )
					return 0;
			} else if ( next == 'o' || next == 'O' ) {
				base = 8;
				if ( !supportsOctal )
					return 0;
			}
			if ( base != 0 ) { // Hex, Bin, Octal (0x, 0b, 0o)
				pos++;		   // Consume 'x'/'b'/'o'
				// Cannot have underscore right after prefix
				if ( pos < (int)stringLength && stringSearch[pos] == '_' )
					return 0;
				if ( !consumeDigitsWithSep( stringSearch, pos, stringLength, base, true,
											underscoreSeparatorSupported ) )
					return 0; // Must have digits & valid separators
							  // No fractional or exponent allowed for these bases
			} else {		  // Decimal starting with '0' (e.g., 0, 0123, 0.5, 0e1)
				// Check for more digits (e.g. 0123) or invalid things like 0_...
				if ( !consumeDigitsWithSep( stringSearch, pos, stringLength, 10, false,
											underscoreSeparatorSupported ) ) {
					// Failed immediately after '0', likely invalid separator like "0_1"
					return 0;
				}
				// Now check for optional fractional part.
				if ( pos < (int)stringLength && stringSearch[pos] == '.' ) {
					// Check for underscore immediately before dot (invalid) e.g. "1_"."
					if ( pos > afterSignPos && stringSearch[pos - 1] == '_' )
						return 0;

					hasDecimalPoint = true;
					pos++;
					consumedSomethingAfterSign = true; // Consumed dot

					// Check for underscore immediately after dot (invalid) e.g. "1"._"
					if ( pos < (int)stringLength && stringSearch[pos] == '_' )
						return 0;

					// Digits after dot are optional if we started with '0'. e.g. "0." is valid
					if ( !consumeDigitsWithSep( stringSearch, pos, stringLength, 10, false,
												underscoreSeparatorSupported ) )
						return 0; // Check separators
				}

				// Now check for optional exponent part.
				if ( pos < (int)stringLength &&
					 ( stringSearch[pos] == 'e' || stringSearch[pos] == 'E' ) ) {
					// Check for underscore immediately before 'e' (invalid) e.g. "1_"e
					if ( pos > afterSignPos && stringSearch[pos - 1] == '_' )
						return 0;

					hasExponent = true;
					pos++;
					consumedSomethingAfterSign = true; // Consumed 'e'

					// Optional sign for exponent
					if ( pos < (int)stringLength &&
						 ( stringSearch[pos] == '+' || stringSearch[pos] == '-' ) ) {
						pos++;
					}

					// Check for underscore immediately after 'e'/'E' or exponent sign (invalid)
					// e.g. "1e_" or "1e+_"
					if ( pos < (int)stringLength && stringSearch[pos] == '_' )
						return 0;

					// Exponent *must* have digits
					if ( !consumeDigitsWithSep( stringSearch, pos, stringLength, 10, true,
												underscoreSeparatorSupported ) )
						return 0;
				}
				// If we got here, it's a valid decimal form starting with 0.
			}
		} else {
			// Input is just "0" (or "+0" / "-0")
			// pos is already correct (after '0')
		}

	} else if ( std::isdigit( stringSearch[afterSignPos] ) ) {
		// Decimal starting with 1-9 (potentially signed)
		pos = afterSignPos; // Start consuming from the first digit
		if ( !consumeDigitsWithSep( stringSearch, pos, stringLength, 10, true,
									underscoreSeparatorSupported ) )
			return 0; // Must have digits & valid separators
		consumedSomethingAfterSign = true;

		// Optional fractional part.
		if ( pos < (int)stringLength && stringSearch[pos] == '.' ) {
			// Check for underscore immediately before dot (invalid) e.g. "1_"."
			if ( pos > afterSignPos && stringSearch[pos - 1] == '_' )
				return 0;

			hasDecimalPoint = true;
			pos++;
			consumedSomethingAfterSign = true; // Consumed dot

			// Check for underscore immediately after dot (invalid) e.g. "1"._"
			if ( pos < (int)stringLength && stringSearch[pos] == '_' )
				return 0;

			// Fractional digits are optional if integer part exists (e.g., "123.")
			if ( !consumeDigitsWithSep( stringSearch, pos, stringLength, 10, false,
										underscoreSeparatorSupported ) )
				return 0; // Check separators
		}

		// Optional exponent part.
		if ( pos < (int)stringLength && ( stringSearch[pos] == 'e' || stringSearch[pos] == 'E' ) ) {
			// Check for underscore immediately before 'e' (invalid) e.g. "1_"e or "1.5_"e
			if ( pos > afterSignPos && stringSearch[pos - 1] == '_' )
				return 0;

			hasExponent = true;
			pos++;
			consumedSomethingAfterSign = true; // Consumed 'e'

			// Optional sign for exponent
			if ( pos < (int)stringLength &&
				 ( stringSearch[pos] == '+' || stringSearch[pos] == '-' ) ) {
				pos++;
			}

			// Check for underscore immediately after 'e'/'E' or exponent sign (invalid) e.g. "1e_"
			// or "1e+_"
			if ( pos < (int)stringLength && stringSearch[pos] == '_' )
				return 0;

			// Exponent *must* have digits
			if ( !consumeDigitsWithSep( stringSearch, pos, stringLength, 10, true,
										underscoreSeparatorSupported ) )
				return 0;
		}

	} else if ( stringSearch[afterSignPos] == '.' ) {
		// Decimal starting with '.' (potentially signed)
		// Cannot have underscore immediately before dot (checked earlier by afterSignPos check)
		pos = afterSignPos + 1; // Consume '.'
		hasDecimalPoint = true;
		consumedSomethingAfterSign = true; // Consumed dot

		// Check for underscore immediately after dot (invalid) e.g. "._"
		if ( pos < (int)stringLength && stringSearch[pos] == '_' )
			return 0;

		// Must have digits *after* the dot if it's the start (e.g., ".5", "+.5")
		if ( !consumeDigitsWithSep( stringSearch, pos, stringLength, 10, true,
									underscoreSeparatorSupported ) )
			return 0; // Require digits & valid separators

		// Optional exponent part.
		if ( pos < (int)stringLength && ( stringSearch[pos] == 'e' || stringSearch[pos] == 'E' ) ) {
			// Check for underscore immediately before 'e' (invalid) e.g. ".5_"e
			if ( pos > afterSignPos + 1 && stringSearch[pos - 1] == '_' )
				return 0; // pos > afterSignPos+1 ensures we had digits after '.'

			hasExponent = true;
			pos++;
			consumedSomethingAfterSign = true; // Consumed 'e'

			// Optional sign for exponent
			if ( pos < (int)stringLength &&
				 ( stringSearch[pos] == '+' || stringSearch[pos] == '-' ) ) {
				pos++;
			}

			// Check for underscore immediately after 'e'/'E' or exponent sign (invalid) e.g. ".1e_"
			// or ".1e+_"
			if ( pos < (int)stringLength && stringSearch[pos] == '_' )
				return 0;

			// Exponent *must* have digits
			if ( !consumeDigitsWithSep( stringSearch, pos, stringLength, 10, true,
										underscoreSeparatorSupported ) )
				return 0;
		}
	} else {
		// Invalid character after sign (or invalid starting character if not signed)
		return 0;
	}

	// If signed, we must have consumed something after the sign
	if ( isSigned && !consumedSomethingAfterSign )
		return 0;
	// If not signed, we must have consumed *something* (digit or dot followed by digit)
	if ( !isSigned && pos == start )
		return 0; // Handles "." case correctly

	// Optional BigInt suffix 'n'
	if ( pos < (int)stringLength && stringSearch[pos] == 'n' ) {
		// BigInt 'n' is only allowed for integer literals (no decimal point, no exponent)
		if ( hasDecimalPoint || hasExponent ) {
			return 0; // Invalid BigInt syntax
		}
		// Check if the character *before* 'n' was an underscore - invalid
		if ( pos > start && stringSearch[pos - 1] == '_' ) {
			return 0;
		}
		if ( supportsBigInt )
			pos++; // Consume 'n'
	}

	// Final check: Ensure we actually consumed something valid beyond just a sign
	// This is implicitly covered by the logic within the branches and the
	// consumedSomethingAfterSign check.

	// Success
	matchList->start = start;
	matchList->end = pos; // end is exclusive index

	return 1;
}

/**
 * @brief Checks if the substring starting at stringStartOffset is a valid C or C++ number literal,
 *        optionally including a leading '+' or '-' sign.
 *
 * @param stringSearch The C-style string to search within.
 * @param stringStartOffset The starting index within stringSearch.
 * @param matchList Pointer to a Range struct to store the start and end indices of the match (if
 * found).
 * @param stringLength The total length of stringSearch.
 * @param language The language standard to adhere to (LANG_C or LANG_CPP).
 * @return The number of matches
 */
inline size_t isNumberLiteral( const char* stringSearch, int stringStartOffset,
							   PatternMatcher::Range* matchList, size_t stringLength,
							   LanguageStandard language ) {
	// Use size_t for internal calculations to avoid overflow and easily compare with length
	const size_t len = stringLength; // Use the provided length

	// --- Language Standard Flags ---
	const bool isCPP = ( language == LANG_CPP );
	// Features specific to C++ (based on our target definition)
	const bool allowBinaryLiteral = isCPP;	 // 0b prefix (C++14)
	const bool allowDigitSeparators = isCPP; // ' separator (C++14)
	const bool allowZuffix = isCPP;			 // z/Z suffix (C++23)
	// Note: Hex floats (0x...p...) are C99 and C++17, so allowed in both modes here.

	// --- Basic Validation ---
	if ( stringStartOffset < 0 || static_cast<size_t>( stringStartOffset ) >= len ) {
		return 0;
	}

	// --- Use size_t for positions internally ---
	size_t start_pos = static_cast<size_t>( stringStartOffset );
	size_t pos = start_pos; // Current parsing position

	// --- 1. Handle Optional Leading Sign ---
	// Check for '+' or '-' at the very beginning.
	bool has_leading_sign = false; // Track if a sign was consumed
	if ( stringSearch[pos] == '+' || stringSearch[pos] == '-' ) {
		pos++;
		has_leading_sign = true;
		// If the sign is the *only* character, it's not a number literal.
		if ( pos == len ) {
			return 0;
		}
	}

	// --- State Flags ---
	bool is_float = false;
	bool is_hex = false;
	bool is_binary = false;
	bool has_prefix = false; // Had 0x, 0b
	bool has_digits = false; // Consumed any valid digits (part of the value)?
	bool has_decimal_point = false;
	int base = 10; // Default base

	// --- 2. Handle Start after Sign: Prefix, Base detection, Leading decimal point ---
	// Now check the character *after* the optional sign (or the first char if no sign)
	if ( stringSearch[pos] == '.' ) { // Case: Starts with '.' (e.g., ".5f", "+.5", "-.5")
		pos++;
		if ( pos == len || !std::isdigit( static_cast<unsigned char>( stringSearch[pos] ) ) ) {
			// A standalone '.', or '.' after sign, or '.' followed by non-digit isn't a number
			// literal.
			return 0;
		}
		is_float = true;
		has_decimal_point = true;
		base = 10;
		// Fall through to parsing digits (fractional part)
	} else if ( stringSearch[pos] ==
				'0' ) {	   // Case: Starts with '0' (e.g., "0", "+0", "-0xff", "+0b10")
		has_digits = true; // The '0' itself counts initially
		pos++;
		if ( pos < len ) {
			char next_char_lower = std::tolower( static_cast<unsigned char>( stringSearch[pos] ) );
			if ( next_char_lower == 'x' ) { // Hexadecimal "0x..." / "+0x..." / "-0x..."
				pos++;
				if ( pos == len ||
					 !std::isxdigit( static_cast<unsigned char>( stringSearch[pos] ) ) )
					return 0; // "0x" or "+0x" alone is invalid
				is_hex = true;
				base = 16;
				has_prefix = true;
				has_digits = false; // Reset: Need hex digits *after* 0x
			} else if ( allowBinaryLiteral && next_char_lower == 'b' ) { // Binary "0b..." (C++14)
				pos++;
				if ( pos == len || !isBinaryDigit( stringSearch[pos] ) )
					return 0; // "0b" or "+0b" alone is invalid
				is_binary = true;
				base = 2;
				has_prefix = true;
				has_digits = false; // Reset: Need bin digits *after* 0b
			} else if ( isOctalDigit( stringSearch[pos] ) ||
						( allowDigitSeparators && stringSearch[pos] == '\'' && pos + 1 < len &&
						  isOctalDigit( stringSearch[pos + 1] ) ) ) {
				// Octal "0..." or C++ "0'..." (separator needs check in digit loop)
				// If it's a separator, the digit loop needs to handle it correctly after '0'.
				base = 8;
				// Don't advance pos here if it's a digit. If it's a separator, let digit loop
				// handle.
			} else if ( stringSearch[pos] == '.' ) { // Decimal float "0." / "+0." / "-0."
				base = 10;							 // Becomes float, handled below
			} else if ( std::tolower( static_cast<unsigned char>( stringSearch[pos] ) ) ==
						'e' ) { // Decimal float "0e..."
				base = 10;		// Becomes float, handled below
			} else {
				// Could be just '0' or '+0' or '-0', or start of invalid octal like '08', '09'
				if ( std::isdigit( static_cast<unsigned char>( stringSearch[pos] ) ) ) {
					// Looks like octal start but might have invalid digits 8/9
					base = 8;
				} else {
					// Just '0' / '+0' / '-0' followed by non-digit, non-special char. Base 10.
					base = 10;
				}
			}
		} else {
			// Just "0" or "+0" or "-0" - valid. Base is 10 (or 8, doesn't matter for value 0).
			base = 10;
		}
	} else if ( std::isdigit( static_cast<unsigned char>(
					stringSearch[pos] ) ) ) { // Case: Starts with '1'-'9' (e.g., "123", "+123",
											  // "-1")
		base = 10;
		// Fall through to parsing digits
	} else {
		// This path is reached if:
		// 1. No leading sign, and first char is not '.', '0', or '1'-'9'.
		// 2. Had a leading sign, but the char *after* it is not '.', '0', or '1'-'9'.
		return 0; // Doesn't start correctly after the optional sign.
	}

	// --- 3. Parse Integer or Mantissa Digits ---
	while ( pos < len ) {
		if ( isValidDigitChar( stringSearch[pos], base ) ) {
			pos++;
			has_digits = true;
		} else if ( allowDigitSeparators && stringSearch[pos] == '\'' &&
					has_digits && // Separator requires preceding digit
					pos + 1 < len && isValidDigitChar( stringSearch[pos + 1], base ) ) {
			// C++14 digit separator: skip it if validly placed
			pos++; // Consume separator placeholder; next loop iteration checks the required digit
				   // after it
		} else if ( base == 8 && std::isdigit( static_cast<unsigned char>( stringSearch[pos] ) ) ) {
			// If parsing assumed octal (base 8) and encounter '8' or '9', it's invalid octal.
			// Stop parsing digits here. C considers "078" ill-formed. For highlighting, we stop at
			// '7'.
			break;
		} else {
			break; // Not a valid digit or separator for the current base
		}
	}

	// After prefix (0x, 0b), digits are mandatory. Check has_digits flag.
	if ( has_prefix && !has_digits )
		return 0;

	size_t end_integer_part = pos; // Position after integer/mantissa digits

	// --- 4. Handle Floating Point specific parts (Decimal Point, Exponent) ---
	// Binary literals cannot be floating point. Octal cannot be float in standard C/C++.
	bool possible_float_base = !is_binary && base != 8;

	// Check for Decimal Point '.' (only if not already seen at the start)
	if ( possible_float_base && !has_decimal_point && pos < len && stringSearch[pos] == '.' ) {
		// If hex, requires digits *before* '.' to be a valid hex float *prefix* (e.g., 0x1.).
		// Standard C99/C++17 hex floats can be 0x.fP0 if digits follow '.', but "0x." needs
		// exponent.
		if ( is_hex && !has_digits ) {
			// Allow "0x." only if fractional digits or 'p' exponent follows immediately.
			// Check next char(s).
			size_t next_pos = pos + 1;
			bool next_is_hex_digit =
				( next_pos < len &&
				  std::isxdigit( static_cast<unsigned char>( stringSearch[next_pos] ) ) );
			bool next_is_p =
				( next_pos < len &&
				  std::tolower( static_cast<unsigned char>( stringSearch[next_pos] ) ) == 'p' );
			if ( !next_is_hex_digit && !next_is_p ) {
				// Invalid hex float like "0x." or "+0x." followed by something else. Stop parsing.
				goto after_float_parts; // Suffix check will likely fail for "0x"
			}
			// Otherwise, proceed to parse fraction/exponent after '.'
		}

		pos++; // Consume '.'
		is_float = true;
		has_decimal_point = true;
		if ( is_hex )
			base = 16; // Hex float fractional part uses hex digits
		else
			base = 10; // Confirm base for fraction

		bool consumed_frac_digit = false;
		while ( pos < len ) { // Consume fractional digits
			if ( isValidDigitChar( stringSearch[pos], base ) ) {
				pos++;
				consumed_frac_digit = true;
			} else if ( allowDigitSeparators && stringSearch[pos] == '\'' && consumed_frac_digit &&
						pos + 1 < len && isValidDigitChar( stringSearch[pos + 1], base ) ) {
				pos++; // Consume separator placeholder
			} else {
				break;
			}
		}

		// C/C++ requires digits somewhere for floats (e.g., "1." is ok, ".5" is ok)
		// Hex floats require fractional digits OR an exponent if '.' is present (e.g. 0x1.p0 or
		// 0x1.0p0 ok, 0x1. invalid unless p follows)
		if ( !has_digits && !consumed_frac_digit ) {
			// Case like "." handled earlier. This means like "0x." without fraction.
			// Needs exponent 'p'. Check handled below. If no 'p', invalid.
			if ( is_hex ) {
				// Check if 'p' follows immediately. If not, backtrack.
				if ( pos == len ||
					 std::tolower( static_cast<unsigned char>( stringSearch[pos] ) ) != 'p' ) {
					pos = end_integer_part; // Backtrack before '.'
					is_float = false;
					has_decimal_point = false;
					// goto after_float_parts; // Let execution continue to exponent check (which
					// will fail) then suffix check
				}
				// If 'p' follows, it's okay, handled below.
			} else {
				// Decimal: "0." needs fractional digits or exponent. "N." is okay.
				// If !has_digits, means started with '.' (or sign then '.'), which requires digits
				// after. Handled earlier. This path for decimal likely means error state.
				return 0;
			}
		}
		// If we had digits before '.' (has_digits=true) OR consumed fractional digits, it's
		// potentially valid. Mark overall number as having digits if it didn't already.
		if ( consumed_frac_digit )
			has_digits = true;

		if ( is_hex && !consumed_frac_digit ) {
			// Hex like "0x1." - must be followed by 'p' exponent. Check in exponent section.
			if ( pos == len ||
				 std::tolower( static_cast<unsigned char>( stringSearch[pos] ) ) != 'p' ) {
				// Invalid: backtrack to before the '.'
				pos = end_integer_part;
				is_float = false; // Revert status
				has_decimal_point = false;
				// Continue to suffix check for the integer part 0xN
				goto after_float_parts;
			}
			// If 'p' follows, it's okay, handled below.
		}
	}

	// Check for Exponent 'e/E' (decimal) or 'p/P' (hex)
	if ( possible_float_base && pos < len ) { // Allow exponent check even if base 8 was temporarily
											  // assigned but '.' made it float
		// Re-evaluate base if '.' occurred
		if ( has_decimal_point && !is_hex )
			base = 10;

		char exp_char_lower = std::tolower( static_cast<unsigned char>( stringSearch[pos] ) );
		bool is_exponent_char = false;
		bool is_hex_exponent = false;

		// Hex exponent 'p' (C99 / C++17)
		if ( is_hex && exp_char_lower == 'p' ) {
			// Allowed after hex digits or hex fraction. Requires digits somewhere before it
			// (checked by has_digits).
			if ( !has_digits )
				goto after_float_parts; // e.g. 0xp1 or +0xp1 invalid
			is_exponent_char = true;
			is_hex_exponent = true;
			// Decimal exponent 'e'
		} else if ( !is_hex && base == 10 && exp_char_lower == 'e' ) {
			// Allowed after dec digits or dec fraction. Requires digits before it.
			if ( !has_digits )
				goto after_float_parts; // e.g. .e1 or +.e1 or e1 invalid
			is_exponent_char = true;
		}

		if ( is_exponent_char ) {
			pos++;			 // Consume exponent char 'e'/'E'/'p'/'P'
			is_float = true; // Using an exponent makes it a float
							 // Exponent *value* is always decimal, even for hex floats
			// int exponent_base = 10; // Implicit

			// Optional sign '+' or '-' for the exponent value
			if ( pos < len && ( stringSearch[pos] == '+' || stringSearch[pos] == '-' ) ) {
				pos++;
			}

			// Required decimal digits for exponent value
			size_t exp_digits_start = pos;
			bool consumed_exp_digit = false;
			while ( pos < len ) {
				if ( std::isdigit( static_cast<unsigned char>( stringSearch[pos] ) ) ) {
					pos++;
					consumed_exp_digit = true;
				} else if ( allowDigitSeparators && stringSearch[pos] == '\'' &&
							consumed_exp_digit && pos + 1 < len &&
							std::isdigit( static_cast<unsigned char>( stringSearch[pos + 1] ) ) ) {
					pos++; // Consume separator placeholder
				} else {
					break;
				}
			}

			if ( !consumed_exp_digit ) {
				// Invalid: Exponent char/sign not followed by digits. Backtrack.
				// Backtrack past optional sign first
				if ( pos > exp_digits_start &&
					 ( stringSearch[pos - 1] == '+' || stringSearch[pos - 1] == '-' ) ) {
					pos--;
				}
				// Backtrack past exponent character
				if ( pos > start_pos + ( has_leading_sign
											 ? 1
											 : 0 ) ) { // Ensure we don't backtrack past start/sign
					char prev_char =
						std::tolower( static_cast<unsigned char>( stringSearch[pos - 1] ) );
					char expected_exp_char = is_hex_exponent ? 'p' : 'e';
					if ( prev_char == expected_exp_char ) {
						pos--; // Backtrack exp char
					}
				}

				// The number ends before the invalid exponent attempt.
				is_float = has_decimal_point; // Revert float status only if exponent was the sole
											  // float indicator
			} else {
				// Valid exponent consumed
				has_digits = true; // Mark overall number as having digits
			}
		}
	}

after_float_parts:; // Label to jump to after float parsing attempts

	// --- 5. Handle Suffixes ---
	if ( is_float ) {
		// Floating-point suffixes: f, F, l, L (common to C99+ and C++)
		// C++11 also adds f16, f32, f64, f128, etc. Not handling those here.
		if ( pos < len ) {
			char s1 = stringSearch[pos];
			if ( s1 == 'f' || s1 == 'F' || s1 == 'l' || s1 == 'L' ) {
				pos++; // Consume one suffix character
			}
		}
	} else {
		// Integer suffixes: U, L, LL, Z (Z is C++23 only) combinations
		// C/C++ allow U/L/LL in any order, but Z only mixes with U (C++23).
		// Max one of U/Z. Max two L's.

		size_t current_pos = pos;
		// Use a flexible parsing approach - scan potential suffix chars
		std::vector<char> suffix_chars;
		while ( current_pos < len ) {
			char c = stringSearch[current_pos];
			char c_lower = std::tolower( static_cast<unsigned char>( c ) );
			if ( c_lower == 'u' || c_lower == 'l' || ( allowZuffix && c_lower == 'z' ) ) {
				suffix_chars.push_back( c_lower );
				current_pos++;
			} else {
				break;
			}
		}

		// Validate the collected suffix chars
		size_t valid_suffix_len = 0;
		if ( !suffix_chars.empty() ) {
			size_t check_len = suffix_chars.size();

			// Try to parse known valid combinations first (simplifies logic)
			if ( check_len == 1 ) {
				if ( suffix_chars[0] == 'u' || suffix_chars[0] == 'l' || suffix_chars[0] == 'z' )
					valid_suffix_len = 1;
			} else if ( check_len == 2 ) {
				char c1 = suffix_chars[0], c2 = suffix_chars[1];
				if ( ( c1 == 'u' && c2 == 'l' ) || ( c1 == 'l' && c2 == 'u' ) )
					valid_suffix_len = 2; // UL, LU
				else if ( c1 == 'l' && c2 == 'l' )
					valid_suffix_len = 2; // LL
				else if ( ( c1 == 'u' && c2 == 'z' ) || ( c1 == 'z' && c2 == 'u' ) )
					valid_suffix_len = 2; // UZ, ZU (C++23)
			} else if ( check_len == 3 ) {
				char c1 = suffix_chars[0], c2 = suffix_chars[1], c3 = suffix_chars[2];
				// ULL, LUL, LLU
				if ( c1 == 'l' && c2 == 'l' ) { // Starts LL
					if ( c3 == 'u' )
						valid_suffix_len = 3;		   // LLU
				} else if ( c2 == 'l' && c3 == 'l' ) { // Ends LL
					if ( c1 == 'u' )
						valid_suffix_len = 3;		   // ULL
				} else if ( c1 == 'l' && c3 == 'l' ) { // L_L
					if ( c2 == 'u' )
						valid_suffix_len = 3; // LUL
				}
			}

			// If a valid combination was found, advance pos
			if ( valid_suffix_len > 0 ) {
				pos += valid_suffix_len; // Advance main position pointer
			}
			// else: pos remains at suffix_start, no valid suffix consumed.
		}
		// pos is now either after a valid suffix or back at suffix_start
	}

	size_t final_end_pos = pos; // Position after number value and any valid suffix

	// --- Final Validation and Return ---
	// A valid number literal must contain *at least one digit* somewhere,
	// OR be a valid floating point literal structure like "1." or ".1" (which implies digits).
	// The `has_digits` flag covers most cases. We need it to be true.
	if ( !has_digits ) {
		// If no digits were ever consumed, it's invalid (e.g. "0x", "+0b", ".", "+.", "-")
		return 0;
	}

	// The end position must be strictly after the start position.
	// This also implicitly handles the case where only a sign was present ("+" or "-"),
	// because has_digits would be false in that case, failing the check above.
	if ( final_end_pos > start_pos ) {
		// Success! Update the output struct and return true.
		if ( matchList ) {
			matchList->start = stringStartOffset;				// Use original int offset
			matchList->end = static_cast<int>( final_end_pos ); // Cast end position
		}
		return 1;
	} else {
		// Should only happen if start_pos was the only character and failed validation early
		// Or if for some reason parsing didn't advance pos (e.g., logic error)
		return 0;
	}
}

SINGLETON_DECLARE_IMPLEMENTATION( ParserMatcherManager );

void ParserMatcherManager::registerBaseParsers() {
	if ( !mFns.empty() )
		return;

	registerParser( "cpp_number_parser",
					[]( const char* stringSearch, int stringStartOffset,
						PatternMatcher::Range* matchList, size_t stringLength ) {
						return isNumberLiteral( stringSearch, stringStartOffset, matchList,
												stringLength, LanguageStandard::LANG_CPP );
					} );

	registerParser( "c_number_parser", []( const char* stringSearch, int stringStartOffset,
										   PatternMatcher::Range* matchList, size_t stringLength ) {
		return isNumberLiteral( stringSearch, stringStartOffset, matchList, stringLength,
								LanguageStandard::LANG_C );
	} );

	registerParser( "common_number_parser",
					[]( const char* stringSearch, int stringStartOffset,
						PatternMatcher::Range* matchList, size_t stringLength ) {
						return isNumberLiteralBase( stringSearch, stringStartOffset, matchList,
													stringLength, false, false, false, false );
					} );

	registerParser( "common_number_parser_o",
					[]( const char* stringSearch, int stringStartOffset,
						PatternMatcher::Range* matchList, size_t stringLength ) {
						return isNumberLiteralBase( stringSearch, stringStartOffset, matchList,
													stringLength, false, true, false, false );
					} );

	registerParser( "common_number_parser_ob",
					[]( const char* stringSearch, int stringStartOffset,
						PatternMatcher::Range* matchList, size_t stringLength ) {
						return isNumberLiteralBase( stringSearch, stringStartOffset, matchList,
													stringLength, false, true, true, false );
					} );

	registerParser( "js_number_parser",
					[]( const char* stringSearch, int stringStartOffset,
						PatternMatcher::Range* matchList, size_t stringLength ) {
						return isNumberLiteralBase( stringSearch, stringStartOffset, matchList,
													stringLength, true, true, true, true );
					} );

	mRegisteredBaseParsers = true;
}

bool ParserMatcherManager::registeredBaseParsers() const {
	return mRegisteredBaseParsers;
}

void ParserMatcherManager::registerParser( std::string_view parserName, ParserMatcherFn fn ) {
	mFns.insert_or_assign( std::hash<std::string_view>()( parserName ), std::move( fn ) );
}

bool ParserMatcherManager::hasParser( std::string_view parserName ) const {
	return mFns.find( std::hash<std::string_view>()( parserName ) ) != mFns.end();
}

size_t ParserMatcherManager::matches( std::string_view parserName, const char* stringSearch,
									  int stringStartOffset, PatternMatcher::Range* matchList,
									  size_t stringLength ) {
	auto parserIt = mFns.find( std::hash<std::string_view>()( parserName ) );
	if ( parserIt != mFns.end() )
		return parserIt->second( stringSearch, stringStartOffset, matchList, stringLength );
	return 0;
}

ParserMatcher::ParserMatcher( const std::string_view& parserName ) :
	PatternMatcher( PatternType::Parser ), mParserName( parserName ), mMatchNum( 0 ) {}

ParserMatcher::~ParserMatcher() {}

bool ParserMatcher::isValid() const {
	return true;
}

bool ParserMatcher::matches( const char* stringSearch, int stringStartOffset,
							 PatternMatcher::Range* matchList, size_t stringLength ) const {
	mMatchNum = ParserMatcherManager::instance()->matches(
		mParserName, stringSearch, stringStartOffset, matchList, stringLength );
	return mMatchNum > 0;
}

bool ParserMatcher::matches( const std::string& str, PatternMatcher::Range* matchList,
							 int stringStartOffset ) const {
	return matches( str.c_str(), stringStartOffset, matchList, str.length() );
}

const size_t& ParserMatcher::getNumMatches() const {
	return mMatchNum;
}

}} // namespace EE::System
