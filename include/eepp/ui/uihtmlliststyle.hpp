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

class EE_API UIHTMLListStyle {
  public:
	static bool isPrimitiveMarker( CSSListStyleType type );

	static bool isTextMarker( CSSListStyleType type );

	static String getTextMarkerString( CSSListStyleType type, int index );

	static void syncTextMarkerColor( Graphics::Text& marker, const Color& color );

	static Vector2f getTextMarkerPosition( const Vector2f& screenPos, const Rectf& paddingPx,
										   Float textWidth, Float fontSize );

	static void drawPrimitiveMarker( CSSListStyleType type, const Vector2f& screenPos,
									 const Rectf& paddingPx,
									 const Graphics::FontStyleConfig& style );
};

}} // namespace EE::UI

#endif
