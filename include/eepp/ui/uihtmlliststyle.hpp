#ifndef EE_UI_UIHTMLLISTSTYLE_HPP
#define EE_UI_UIHTMLLISTSTYLE_HPP

#include <eepp/core/string.hpp>
#include <eepp/graphics/fontstyleconfig.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/ui/csslayouttypes.hpp>

namespace EE { namespace Graphics {
class Text;
}} // namespace EE::Graphics

namespace EE { namespace UI {

using namespace EE::Graphics;

class EE_API UIHTMLListStyle {
  public:
	static bool isPrimitiveMarker( CSSListStyleType type );

	static bool isTextMarker( CSSListStyleType type );

	static String getTextMarkerString( CSSListStyleType type, int index );

	static void syncTextMarkerColor( Text& marker, const Color& color );

	/**
	 * Markers are placed outside the content box on its inline-start side: left of the content
	 * in LTR, right of it in RTL. @p boxWidth is the pixel width of the list item box and is only
	 * needed when @p rtl is true.
	 */
	static Vector2f getTextMarkerPosition( const Vector2f& screenPos, const Rectf& paddingPx,
										   Float textWidth, Float fontSize, bool rtl = false,
										   Float boxWidth = 0.f );

	/** Draws a primitive marker. In RTL the marker geometry is mirrored, so disclosure-closed
	 * triangles point toward the inline-end (left) side. */
	static void drawPrimitiveMarker( CSSListStyleType type, const Vector2f& screenPos,
									 const Rectf& paddingPx, const FontStyleConfig& style,
									 bool rtl = false, Float boxWidth = 0.f );
};

}} // namespace EE::UI

#endif
