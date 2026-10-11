#ifndef EE_UI_UIDIRECTION_HPP
#define EE_UI_UIDIRECTION_HPP

#include <eepp/config.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/shapedglyph.hpp>

#include <string>
#include <string_view>

namespace EE { namespace UI {

using namespace EE::Graphics;

/**
 * @brief Logical UI reading/layout direction of a widget subtree.
 *
 * The UI direction is independent from the direction of the text a widget renders: a
 * right-to-left UI can contain left-to-right paragraphs and vice versa. It decides how logical
 * concepts (`start`, `end`, `inline-start`, `inline-end`, `layout-to-start-of`, ...) resolve to
 * physical edges. Physical concepts (`left`, `right`, `margin-left`, `UI_HALIGN_LEFT`, ...)
 * never change their meaning.
 */
enum class UIDirection : Uint8 {
	LeftToRight,
	RightToLeft,
};

/** @brief Direction value specified on a widget. Inherit uses the parent's direction. */
enum class UIDirectionValue : Uint8 {
	Inherit,
	LeftToRight,
	RightToLeft,
};

/**
 * @brief Horizontal alignment that can express both physical and logical intents.
 *
 * Left, Right and Center are physical and keep their meaning in every direction. Start and End
 * resolve against the effective UIDirection of the widget that owns the alignment.
 */
enum class UIHorizontalAlignment : Uint8 {
	Left,
	Center,
	Right,
	Start,
	End,
};

/** @brief Logical edge in the inline (horizontal) axis. */
enum class UIInlineEdge : Uint8 {
	Start,
	End,
};

/** @brief Physical box edge. */
enum class UIPhysicalEdge : Uint8 {
	Left,
	Right,
	Top,
	Bottom,
};

/** @return True when the inline-start edge is the physical left edge. */
inline constexpr bool startIsLeft( UIDirection direction ) {
	return direction == UIDirection::LeftToRight;
}

/** @return The physical edge that corresponds to a logical inline edge. */
inline constexpr UIPhysicalEdge resolveInlineEdge( UIInlineEdge edge, UIDirection direction ) {
	return ( edge == UIInlineEdge::Start ) == startIsLeft( direction ) ? UIPhysicalEdge::Left
																	   : UIPhysicalEdge::Right;
}

/** @return True for Start and End. */
inline constexpr bool isLogicalAlignment( UIHorizontalAlignment alignment ) {
	return alignment == UIHorizontalAlignment::Start || alignment == UIHorizontalAlignment::End;
}

/**
 * @return The physical horizontal alignment bits (TEXT_ALIGN_LEFT,
 * TEXT_ALIGN_RIGHT or TEXT_ALIGN_CENTER) of an alignment under the given
 * direction.
 */
inline constexpr Uint32 resolveHorizontalAlignment( UIHorizontalAlignment alignment,
													UIDirection direction ) {
	switch ( alignment ) {
		case UIHorizontalAlignment::Center:
			return TEXT_ALIGN_CENTER;
		case UIHorizontalAlignment::Right:
			return TEXT_ALIGN_RIGHT;
		case UIHorizontalAlignment::Start:
			return startIsLeft( direction ) ? TEXT_ALIGN_LEFT : TEXT_ALIGN_RIGHT;
		case UIHorizontalAlignment::End:
			return startIsLeft( direction ) ? TEXT_ALIGN_RIGHT : TEXT_ALIGN_LEFT;
		case UIHorizontalAlignment::Left:
		default:
			return TEXT_ALIGN_LEFT;
	}
}

/** @return The physical alignment value that matches the physical alignment bits. */
inline constexpr UIHorizontalAlignment horizontalAlignmentFromFlags( Uint32 flags ) {
	switch ( flags & TEXT_HALIGN_MASK ) {
		case TEXT_ALIGN_RIGHT:
			return UIHorizontalAlignment::Right;
		case TEXT_ALIGN_CENTER:
			return UIHorizontalAlignment::Center;
		default:
			return UIHorizontalAlignment::Left;
	}
}

/** @return The physical alignment bits mirrored horizontally (left <-> right). */
inline constexpr Uint32 mirrorHorizontalAlignFlags( Uint32 halign ) {
	switch ( halign & TEXT_HALIGN_MASK ) {
		case TEXT_ALIGN_LEFT:
			return TEXT_ALIGN_RIGHT;
		case TEXT_ALIGN_RIGHT:
			return TEXT_ALIGN_LEFT;
		default:
			return halign & TEXT_HALIGN_MASK;
	}
}

/**
 * @return The text base direction used for a UI direction. This is the strong paragraph
 * direction; widgets that have not been given any explicit direction keep
 * TextDirection::Unspecified (first-strong detection) for backwards compatibility.
 */
inline constexpr TextDirection toTextDirection( UIDirection direction ) {
	return direction == UIDirection::RightToLeft ? TextDirection::RightToLeft
												 : TextDirection::LeftToRight;
}

struct EE_API UIDirectionHelper {
	/** Parses "ltr", "rtl", "inherit" and "auto" (treated as inherit). Defaults to Inherit. */
	static UIDirectionValue fromString( std::string_view str );

	static std::string toString( UIDirectionValue direction );

	static std::string toString( UIDirection direction );

	/**
	 * @return The conventional UI direction of a BCP 47 / POSIX locale name, such as "ar",
	 * "he_IL", "fa-IR" or "ur.UTF-8". Unknown locales resolve to left-to-right. This helper does
	 * not change any widget: applications decide when locale changes should update direction.
	 */
	static UIDirection fromLocale( std::string_view locale );
};

struct EE_API UIHorizontalAlignmentHelper {
	/**
	 * Parses "left", "right", "center", "center_horizontal", "start" and "end".
	 * @return False if the value is not a horizontal alignment keyword.
	 */
	static bool fromString( std::string_view str, UIHorizontalAlignment& alignment );

	static std::string toString( UIHorizontalAlignment alignment );
};

/** @return The conventional UI direction of a locale name. @see UIDirectionHelper::fromLocale */
inline UIDirection directionForLocale( std::string_view locale ) {
	return UIDirectionHelper::fromLocale( locale );
}

}} // namespace EE::UI

#endif
