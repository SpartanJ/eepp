#include <eepp/graphics/font.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/ui/uihtmlliststyle.hpp>

namespace EE { namespace UI {

bool UIHTMLListStyle::isPrimitiveMarker( CSSListStyleType type ) {
	switch ( type ) {
		case CSSListStyleType::Disc:
		case CSSListStyleType::Circle:
		case CSSListStyleType::Square:
		case CSSListStyleType::DisclosureClosed:
		case CSSListStyleType::DisclosureOpen:
			return true;
		default:
			return false;
	}
}

bool UIHTMLListStyle::isTextMarker( CSSListStyleType type ) {
	return type != CSSListStyleType::None && !isPrimitiveMarker( type );
}

String UIHTMLListStyle::getTextMarkerString( CSSListStyleType type, int index ) {
	switch ( type ) {
		case CSSListStyleType::Decimal:
			return String( String::toString( index + 1 ) + ". " );
		case CSSListStyleType::LowerAlpha: {
			char c = 'a' + ( index % 26 );
			return String( 1, (String::StringBaseType)c ) + ". ";
		}
		case CSSListStyleType::UpperAlpha: {
			char c = 'A' + ( index % 26 );
			return String( 1, (String::StringBaseType)c ) + ". ";
		}
		case CSSListStyleType::LowerRoman: {
			static const char* numerals[] = { "i",	 "ii",	 "iii", "iv", "v",	"vi",
											  "vii", "viii", "ix",	"x",  "xi", "xii" };
			return index < 12 ? String( numerals[index] ) + ". "
							  : String( String::toString( index + 1 ) + ". " );
		}
		case CSSListStyleType::UpperRoman: {
			static const char* numerals[] = { "I",	 "II",	 "III", "IV", "V",	"VI",
											  "VII", "VIII", "IX",	"X",  "XI", "XII" };
			return index < 12 ? String( numerals[index] ) + ". "
							  : String( String::toString( index + 1 ) + ". " );
		}
		default:
			return {};
	}
}

void UIHTMLListStyle::drawPrimitiveMarker( CSSListStyleType type, const Vector2f& screenPos,
										   const Rectf& paddingPx,
										   const Graphics::FontStyleConfig& style ) {
	Float fontSize = style.CharacterSize;
	Float offset = 0.25f * fontSize;
	Float lineHeight = style.Font ? style.Font->getLineSpacing( (unsigned int)fontSize ) : fontSize;
	Float lineTop = screenPos.y + paddingPx.Top;
	Graphics::Primitives p;
	p.setColor( style.FontColor );

	switch ( type ) {
		case CSSListStyleType::Disc: {
			Float radius = fontSize * 0.22f;
			Float markerX = std::floor( screenPos.x + paddingPx.Left - radius * 2.f - offset );
			Float markerY = std::floor( lineTop + ( lineHeight - radius * 2.f ) * 0.5f + radius );
			p.setFillMode( Graphics::PrimitiveFillMode::DRAW_FILL );
			p.drawCircle( { markerX, markerY }, radius );
			break;
		}
		case CSSListStyleType::Circle: {
			Float radius = fontSize * 0.2f;
			Float lineWidth = fontSize * 0.04f;
			Float markerX = std::floor( screenPos.x + paddingPx.Left - radius * 2.f - offset );
			Float markerY = std::floor( lineTop + ( lineHeight - radius * 2.f ) * 0.5f + radius );
			p.setFillMode( Graphics::PrimitiveFillMode::DRAW_LINE );
			p.setLineWidth( lineWidth );
			p.drawCircle( { markerX, markerY }, radius );
			break;
		}
		case CSSListStyleType::Square: {
			Float size = fontSize * 0.38f;
			Float markerX = std::floor( screenPos.x + paddingPx.Left - size - fontSize * 0.5 );
			Float markerY = std::floor( lineTop + ( lineHeight - size ) * 0.5f );
			p.setFillMode( Graphics::PrimitiveFillMode::DRAW_FILL );
			p.drawRectangle( Rectf( markerX, markerY, markerX + size, markerY + size ) );
			break;
		}
		case CSSListStyleType::DisclosureClosed: {
			Float size = fontSize * 0.48f;
			Float cx = std::floor( screenPos.x + paddingPx.Left - fontSize * 0.8f );
			Float cy = std::floor( lineTop + lineHeight * 0.5f );
			p.setFillMode( Graphics::PrimitiveFillMode::DRAW_FILL );
			p.drawTriangle( Triangle2f( { cx - size * 0.35f, cy - size * 0.5f },
										{ cx - size * 0.35f, cy + size * 0.5f },
										{ cx + size * 0.35f, cy } ) );
			break;
		}
		case CSSListStyleType::DisclosureOpen: {
			Float size = fontSize * 0.52f;
			Float cx = std::floor( screenPos.x + paddingPx.Left - fontSize * 0.8f );
			Float cy = std::floor( lineTop + lineHeight * 0.5f );
			p.setFillMode( Graphics::PrimitiveFillMode::DRAW_FILL );
			p.drawTriangle( Triangle2f( { cx - size * 0.5f, cy - size * 0.25f },
										{ cx + size * 0.5f, cy - size * 0.25f },
										{ cx, cy + size * 0.45f } ) );
			break;
		}
		default:
			break;
	}
}

}} // namespace EE::UI
