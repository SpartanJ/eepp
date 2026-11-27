#ifndef EE_SYSTEM_LOCALE_HPP
#define EE_SYSTEM_LOCALE_HPP

#include <array>
#include <concepts>
#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <optional>
#include <string_view>
#include <type_traits>

namespace EE::System {

/** A normalized language and optional script, region and variant.
 *
 * Language is 2 or 3 ASCII letters, script is 4 letters, and region is 2 letters or 3 digits.
 * A single standard variant is accepted and lowercased. Separators can be hyphens or underscores.
 * POSIX codesets are ignored. @latin and @cyrillic supply a script, and a standard variant may
 * also appear as a POSIX modifier. Extensions, multiple variants and malformed IDs are rejected.
 * This checks the shape of a tag without requiring a registry of assigned codes.
 */
class EE_API LocaleId {
  public:
	LocaleId() = default;

	explicit LocaleId( std::string_view tag );

	/** The returned view is valid for the life of this value. */
	std::string_view getTag() const { return { mTag.data(), mSize }; }

	bool isValid() const { return mSize != 0; }

	/** Removes the variant, then the region, then the script. A bare language has no parent. */
	LocaleId parent() const;

	/** The next ID to search for locale data or translations. When the ID has a region but no
	 * script, and CLDR's likely script for that language and region differs from the bare
	 * language's, the script is inserted: zh-TW becomes zh-Hant-TW and sr-ME becomes sr-Latn-ME.
	 * Otherwise this is parent(). Repeated calls end with an invalid ID after the bare language.
	 */
	LocaleId fallback() const;

	bool operator==( const LocaleId& other ) const { return getTag() == other.getTag(); }

	/** System message language. On POSIX systems, a nonempty GNU LANGUAGE list takes priority
	 * over LC_ALL, LC_MESSAGES and LANG unless the messages locale is C or POSIX.
	 * Falls back to en. */
	static LocaleId systemUiLanguage();

	/** System regional setting for numbers. Falls back to en-US. */
	static LocaleId systemNumberLocale();

  private:
	// Longest normalized tag: 3-letter language, -Script, -000 region and -8letters variant.
	std::array<char, 21> mTag{};
	Uint8 mSize{ 0 };
};

/** Integer types that represent numbers. bool and character types are excluded, and wider
 * extension types are rejected because WideInteger cannot hold their values. */
template <typename T>
concept NumericInteger =
	std::integral<T> && sizeof( T ) <= 8 && !std::same_as<std::remove_cv_t<T>, bool> &&
	!std::same_as<std::remove_cv_t<T>, char> && !std::same_as<std::remove_cv_t<T>, wchar_t> &&
	!std::same_as<std::remove_cv_t<T>, char8_t> && !std::same_as<std::remove_cv_t<T>, char16_t> &&
	!std::same_as<std::remove_cv_t<T>, char32_t>;

/** The 64-bit type that holds every value of a NumericInteger type. String::toString() has
 * no overload for every platform's long and long long spellings, so convert through this. */
template <NumericInteger T>
using WideInteger = std::conditional_t<std::is_signed_v<T>, Int64, Uint64>;

enum class NumberRounding : Uint8 { HalfEven, HalfAwayFromZero, TowardZero };

enum class NumberingSystem : Uint8 { LocaleDefault, Latin };

struct NumberFormatOptions {
	/** The largest accepted fraction-digit limit. Negative or larger limits format as an
	 * empty String. */
	static constexpr int MaxFractionDigits = 100;

	/** When omitted, precision follows the CLDR pattern (typically 3 for numbers and 0 for
	 * percents), increased if necessary to satisfy minFractionDigits. Limits are int, so an
	 * out-of-range value stays out of range instead of wrapping into the accepted range. */
	std::optional<int> maxFractionDigits;
	int minFractionDigits{ 0 };
	NumberRounding rounding{ NumberRounding::HalfEven };
	bool grouping{ true };
	bool showPlusSign{ false };
};

/** Localized decimal and percent formatting, based on immutable CLDR profiles.
 *
 * Profiles resolve through LocaleId::fallback(), then to US English, so a region alias such as
 * zh-TW uses its likely script. Latin selects that locale's CLDR Latin profile.
 * Locale IDs are also accepted by DateTimeLocale::fromLocaleName() via getId().getTag().
 */
class EE_API Locale {
  public:
	explicit Locale( LocaleId id = LocaleId::systemNumberLocale(),
					 NumberingSystem digits = NumberingSystem::LocaleDefault );

	explicit Locale( std::string_view tag,
					 NumberingSystem digits = NumberingSystem::LocaleDefault ) :
		Locale( LocaleId( tag ), digits ) {}

	const LocaleId& getId() const { return mId; }

	/** Formats the shortest round-trippable decimal representation of a double. Negative zero
	 * keeps its sign. Non-finite values use the locale's NaN and infinity symbols. */
	String formatNumber( double value, NumberFormatOptions options = {} ) const;

	/** Accepts an exact ASCII decimal: optional sign, integer digits, optional dot and fraction
	 * digits. Leading zeros are removed and fractional zeros are kept to minFractionDigits.
	 * Invalid input or conflicting or excessive fraction limits return an empty String. */
	String formatNumber( std::string_view value, NumberFormatOptions options = {} ) const;

	template <NumericInteger Integer>
	String formatNumber( Integer value, NumberFormatOptions options = {} ) const {
		return formatDecimal( String::toString( WideInteger<Integer>( value ) ), options, false );
	}

	/** bool and character values are not numbers. This prevents conversion to double. */
	template <std::integral Integer>
		requires( !NumericInteger<Integer> )
	String formatNumber( Integer, NumberFormatOptions = {} ) const = delete;

	/** A percent value is a ratio. It is multiplied by 100 before decimal rounding. */
	String formatPercent( double value, NumberFormatOptions options = {} ) const;

	String formatPercent( std::string_view value, NumberFormatOptions options = {} ) const;

	template <NumericInteger Integer>
	String formatPercent( Integer value, NumberFormatOptions options = {} ) const {
		return formatDecimal( String::toString( WideInteger<Integer>( value ) ), options, true );
	}

	template <std::integral Integer>
		requires( !NumericInteger<Integer> )
	String formatPercent( Integer, NumberFormatOptions = {} ) const = delete;

  private:
	String formatFloating( double value, NumberFormatOptions options, bool percent ) const;

	String formatDecimal( std::string_view value, NumberFormatOptions options, bool percent,
						  int exponent = 0 ) const;

	LocaleId mId;
	Uint8 mProfile;
};

} // namespace EE::System

#endif
