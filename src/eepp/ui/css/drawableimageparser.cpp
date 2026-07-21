#include <eepp/graphics/circledrawable.hpp>
#include <eepp/graphics/convexshapedrawable.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/rectangledrawable.hpp>
#include <eepp/graphics/triangledrawable.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/ui/css/drawableimageparser.hpp>
#include <eepp/ui/lineargradientdrawable.hpp>
#include <eepp/ui/radialgradientdrawable.hpp>
#include <eepp/ui/uiiconthememanager.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uiscenenode.hpp>

using namespace EE::Graphics;
using namespace EE::Scene;

namespace EE { namespace UI { namespace CSS {

struct ColorStopResult {
	bool valid{ false };
	bool isHint{ false };
	Color color;
	Float value{ -1.f };
	StyleSheetLength::Unit unit{ StyleSheetLength::Percentage };
	Float secondValue{ -1.f };
	StyleSheetLength::Unit secondUnit{ StyleSheetLength::Percentage };
};

static StyleSheetLength parseStyleSheetLength( const std::string& token ) {
	if ( token.empty() )
		return StyleSheetLength( -1.f, StyleSheetLength::Percentage );
	if ( token.back() == '%' ) {
		std::string num = token.substr( 0, token.size() - 1 );
		Float val;
		if ( String::fromString( val, num ) )
			return StyleSheetLength( eemax( 0.f, eemin( 100.f, val ) ),
									 StyleSheetLength::Percentage );
		return StyleSheetLength( -1.f, StyleSheetLength::Percentage );
	}
	// Handle bare numbers as percentages (CSS default for gradient stops)
	{
		Float val;
		if ( String::fromString( val, token ) )
			return StyleSheetLength( eemax( 0.f, eemin( 100.f, val ) ),
									 StyleSheetLength::Percentage );
	}
	StyleSheetLength len = StyleSheetLength::fromString( token );
	if ( len.getValue() >= 0.f )
		return len;
	return StyleSheetLength( -1.f, StyleSheetLength::Percentage );
}

static ColorStopResult parseColorStop( const std::string& stopStr ) {
	ColorStopResult result;

	std::string str = String::trim( stopStr );
	if ( str.empty() )
		return result;

	// Try to parse the whole string as a bare percentage hint first
	if ( str.back() == '%' && str.find( ' ' ) == std::string::npos &&
		 !Color::isColorString( str ) ) {
		std::string num = str.substr( 0, str.size() - 1 );
		Float val;
		if ( String::fromString( val, num ) ) {
			result.value = eemax( 0.f, eemin( 100.f, val ) );
			result.unit = StyleSheetLength::Percentage;
			result.isHint = true;
			result.valid = true;
			return result;
		}
	}

	// Find the last whitespace that separates color from position(s)
	size_t lastSpace = std::string::npos;
	for ( size_t i = str.size(); i > 0; i-- ) {
		if ( str[i - 1] == ' ' ) {
			lastSpace = i - 1;
			break;
		}
	}

	std::string colorPart;
	std::string posPart;

	if ( lastSpace != std::string::npos ) {
		colorPart = str.substr( 0, lastSpace );
		posPart = str.substr( lastSpace + 1 );
	} else {
		if ( Color::isColorString( str ) ) {
			result.color = Color::fromString( str );
			result.valid = true;
			return result;
		}
		return result;
	}

	String::trimInPlace( posPart );
	String::trimInPlace( colorPart );

	// Try to split colorPart further for two-length stops (e.g. "red 0 10px")
	if ( !Color::isColorString( colorPart ) ) {
		size_t innerSpace = std::string::npos;
		for ( size_t i = colorPart.size(); i > 0; i-- ) {
			if ( colorPart[i - 1] == ' ' ) {
				innerSpace = i - 1;
				break;
			}
		}
		if ( innerSpace != std::string::npos ) {
			std::string altColor = colorPart.substr( 0, innerSpace );
			String::trimInPlace( altColor );
			if ( Color::isColorString( altColor ) ) {
				std::string firstPos = colorPart.substr( innerSpace + 1 );
				String::trimInPlace( firstPos );
				StyleSheetLength p1 = parseStyleSheetLength( firstPos );
				StyleSheetLength p2 = parseStyleSheetLength( posPart );
				result.color = Color::fromString( altColor );
				result.value = p1.getValue();
				result.unit = p1.getUnit();
				result.secondValue = p2.getValue();
				result.secondUnit = p2.getUnit();
				result.valid = true;
				return result;
			}
		}
	}

	// Try to parse the position part (single position)
	StyleSheetLength posLen = parseStyleSheetLength( posPart );

	if ( Color::isColorString( colorPart ) ) {
		result.color = Color::fromString( colorPart );
		result.value = posLen.getValue();
		result.unit = posLen.getUnit();
		result.valid = true;
	} else if ( posLen.getValue() < 0.f && Color::isColorString( str ) ) {
		result.color = Color::fromString( str );
		result.valid = true;
	}

	return result;
}
DrawableImageParser::DrawableImageParser() {
	registerBaseParsers();
}

bool DrawableImageParser::exists( const std::string& name ) const {
	return mFuncs.find( name ) != mFuncs.end();
}

Drawable* DrawableImageParser::createDrawable( const std::string& value, const Sizef& size,
											   bool& ownIt, UINode* node ) {
	FunctionString functionType = FunctionString::parse( value );
	Drawable* res = NULL;
	ownIt = false;

	if ( "none" == value )
		return NULL;

	if ( !functionType.isEmpty() ) {
		if ( exists( functionType.getName() ) )
			return mFuncs[functionType.getName()]( functionType, size, ownIt, node );
	} else if ( NULL != ( res = DrawableSearcher::searchByName(
							  value, false, node->getUISceneNode()->getReferer() ) ) ) {
		if ( res->getDrawableType() == Drawable::SPRITE )
			ownIt = true;
		return res;
	}

	return res;
}

void DrawableImageParser::addParser( const std::string& name,
									 const DrawableImageParserFunc& func ) {
	if ( exists( name ) ) {
		Log::warning(
			"DrawableImageParser::addParser: image parser function \"%s\" already exists.",
			name.c_str() );
		return;
	}

	mFuncs[name] = func;
}

void DrawableImageParser::registerBaseParsers() {
	// Shared parsing logic for linear-gradient and repeating-linear-gradient
	auto parseGradient = []( const FunctionString& functionType, bool& ownIt, UINode* node,
							 bool repeating ) -> Drawable* {
		const auto& params( functionType.getParameters() );
		if ( params.size() < 2 )
			return NULL;

		size_t paramIdx = 0;
		Float angle = 180.f; /* default: to bottom */

		// Parse direction / angle from first parameter
		std::string firstParam = params[0];
		String::toLowerInPlace( firstParam );
		String::trimInPlace( firstParam );

		if ( String::startsWith( firstParam, "to " ) ) {
			paramIdx = 1;
			if ( firstParam == "to top" ) {
				angle = 0.f;
			} else if ( firstParam == "to right" ) {
				angle = 90.f;
			} else if ( firstParam == "to bottom" ) {
				angle = 180.f;
			} else if ( firstParam == "to left" ) {
				angle = 270.f;
			} else if ( firstParam == "to top right" || firstParam == "to right top" ) {
				angle = 45.f;
			} else if ( firstParam == "to bottom right" || firstParam == "to right bottom" ) {
				angle = 135.f;
			} else if ( firstParam == "to bottom left" || firstParam == "to left bottom" ) {
				angle = 225.f;
			} else if ( firstParam == "to top left" || firstParam == "to left top" ) {
				angle = 315.f;
			}
		} else if ( !Color::isColorString( firstParam ) && !functionType.parameterWasString( 0 ) ) {
			std::string angleStr = firstParam;
			String::toLowerInPlace( angleStr );

			auto parseAngle = []( const std::string& s, Float& out ) -> bool {
				Float val;
				if ( String::endsWith( s, "deg" ) ) {
					std::string num = s.substr( 0, s.size() - 3 );
					if ( String::fromString( val, num ) ) {
						out = val;
						return true;
					}
				} else if ( String::endsWith( s, "rad" ) ) {
					std::string num = s.substr( 0, s.size() - 3 );
					if ( String::fromString( val, num ) ) {
						out = val * 180.f / EE_PI;
						return true;
					}
				} else if ( String::endsWith( s, "grad" ) ) {
					std::string num = s.substr( 0, s.size() - 4 );
					if ( String::fromString( val, num ) ) {
						out = val * 0.9f;
						return true;
					}
				} else if ( String::endsWith( s, "turn" ) ) {
					std::string num = s.substr( 0, s.size() - 4 );
					if ( String::fromString( val, num ) ) {
						out = val * 360.f;
						return true;
					}
				} else if ( String::fromString( val, s ) ) {
					out = val;
					return true;
				}
				return false;
			};

			if ( parseAngle( angleStr, angle ) ) {
				paramIdx = 1;
			}
		}

		// Parse color stops and hints
		std::vector<ColorStopResult> gradientStops;
		for ( size_t i = paramIdx; i < params.size(); i++ ) {
			std::string stopParam = params[i];
			String::trimInPlace( stopParam );

			ColorStopResult stop = parseColorStop( stopParam );
			if ( !stop.valid )
				continue;

			gradientStops.push_back( stop );
			if ( stop.secondValue >= 0.f ) {
				ColorStopResult stop2;
				stop2.valid = true;
				stop2.color = stop.color;
				stop2.value = stop.secondValue;
				stop2.unit = stop.secondUnit;
				stop2.secondValue = -1.f;
				gradientStops.push_back( stop2 );
			}
		}

		// CSS §4.3: If a color-stop has a position that is less than
		// the specified position of any color-stop before it in the
		// list, set its position to be equal to the largest specified
		// position of any color-stop before it.  This ensures hard
		// transitions instead of accidental gradients.
		// We also convert the unit to match the dominating unit of the
		// gradient, so that mixed px/% stops produce consistent results.
		{
			Float maxPos = -1.f;
			StyleSheetLength::Unit maxUnit = StyleSheetLength::Percentage;
			bool hasMax = false;
			for ( auto& s : gradientStops ) {
				if ( s.isHint )
					continue;
				if ( s.value >= 0.f ) {
					if ( hasMax && s.value < maxPos ) {
						s.value = maxPos;
						s.unit = maxUnit;
					} else {
						maxPos = s.value;
						maxUnit = s.unit;
						hasMax = true;
					}
				}
				if ( s.secondValue >= 0.f ) {
					if ( hasMax && s.secondValue < maxPos ) {
						s.secondValue = maxPos;
						s.secondUnit = maxUnit;
					} else {
						maxPos = s.secondValue;
						maxUnit = s.secondUnit;
						hasMax = true;
					}
				}
			}
		}

		// Need at least two color stops (hints don't count)
		{
			int colorStopCount = 0;
			for ( const auto& s : gradientStops ) {
				if ( !s.isHint )
					colorStopCount++;
			}
			if ( colorStopCount < 2 )
				return NULL;
		}

		// Sort by position. Hints with the same position as a color stop are
		// irrelevant (the hint has no effect); drop them.
		std::sort( gradientStops.begin(), gradientStops.end(),
				   []( const ColorStopResult& a, const ColorStopResult& b ) {
					   return a.value < b.value;
				   } );

		// Set position 0 for the first unpositioned color stop, and position 1
		// for the last, so interior auto-distribution has proper boundaries.
		for ( size_t i = 0; i < gradientStops.size(); i++ ) {
			if ( !gradientStops[i].isHint && gradientStops[i].value < 0.f ) {
				gradientStops[i].value = 0.f;
				gradientStops[i].unit = StyleSheetLength::Percentage;
				break;
			}
		}
		for ( size_t i = gradientStops.size(); i > 0; i-- ) {
			if ( !gradientStops[i - 1].isHint && gradientStops[i - 1].value < 0.f ) {
				gradientStops[i - 1].value = 100.f;
				gradientStops[i - 1].unit = StyleSheetLength::Percentage;
				break;
			}
		}

		// Fill missing positions on color stops. Hints always have explicit
		// positions so they are left untouched.
		for ( size_t i = 0; i < gradientStops.size(); i++ ) {
			if ( gradientStops[i].isHint || gradientStops[i].value >= 0.f )
				continue;
			size_t prevKnown = i;
			size_t nextKnown = i;
			while ( prevKnown > 0 && ( gradientStops[prevKnown - 1].value < 0.f ) )
				prevKnown--;
			while ( nextKnown < gradientStops.size() &&
					( gradientStops[nextKnown].isHint || gradientStops[nextKnown].value < 0.f ) )
				nextKnown++;

			Float startPos = ( prevKnown > 0 && !gradientStops[prevKnown - 1].isHint )
								 ? gradientStops[prevKnown - 1].value
								 : 0.f;
			Float endPos =
				( nextKnown < gradientStops.size() ) ? gradientStops[nextKnown].value : 100.f;
			size_t range = nextKnown - prevKnown + 1;

			for ( size_t j = prevKnown; j < nextKnown; j++ ) {
				if ( gradientStops[j].isHint )
					continue;
				gradientStops[j].value =
					startPos +
					( endPos - startPos ) * ( (Float)( j - prevKnown + 1 ) / (Float)range );
			}

			i = nextKnown;
		}

		// Expand hinted segments into finely sampled color stops
		std::vector<LinearGradientDrawable::ColorStop> stops;
		const int HINT_SAMPLES = 16;

		for ( size_t i = 0; i < gradientStops.size(); i++ ) {
			if ( gradientStops[i].isHint )
				continue;

			// Add the current color stop
			stops.push_back( LinearGradientDrawable::ColorStop(
				gradientStops[i].value, gradientStops[i].color, gradientStops[i].unit ) );

			// Find the next color stop and any hints between them
			size_t nextColor = i + 1;
			while ( nextColor < gradientStops.size() && gradientStops[nextColor].isHint )
				nextColor++;
			if ( nextColor >= gradientStops.size() )
				break;

			// Collect hints between current and next color stop
			std::vector<Float> hints;
			for ( size_t h = i + 1; h < nextColor; h++ ) {
				if ( gradientStops[h].isHint )
					hints.push_back( gradientStops[h].value );
			}

			if ( hints.empty() )
				continue;

			Float p0 = gradientStops[i].value;
			Float p1 = gradientStops[nextColor].value;
			Color c0 = gradientStops[i].color;
			Color c1 = gradientStops[nextColor].color;

			if ( std::abs( p1 - p0 ) < 0.0001f )
				continue;

			// Apply hints sequentially. Each hint adjusts the transition
			// between the previously computed mid-points.
			// For a single hint at position h between p0 and p1:
			//   normalized hint hn = (h - p0) / (p1 - p0)
			//   power = log(0.5) / log(hn)
			//   color(p) = lerp(c0, c1, ((p-p0)/(p1-p0))^power)
			// We sample HINT_SAMPLES points and add them as color stops.
			Float hintNorm = ( hints[0] - p0 ) / ( p1 - p0 );
			hintNorm = eemax( 0.001f, eemin( 0.999f, hintNorm ) );
			Float power = std::log( 0.5f ) / std::log( hintNorm );

			for ( int s = 1; s < HINT_SAMPLES; s++ ) {
				Float x = (Float)s / (Float)HINT_SAMPLES;
				Float t = std::pow( x, power );
				Float pos = p0 + x * ( p1 - p0 );
				Uint8 r = (Uint8)( (Float)c0.r + t * (Float)( c1.r - c0.r ) );
				Uint8 g = (Uint8)( (Float)c0.g + t * (Float)( c1.g - c0.g ) );
				Uint8 b = (Uint8)( (Float)c0.b + t * (Float)( c1.b - c0.b ) );
				Uint8 a = (Uint8)( (Float)c0.a + t * (Float)( c1.a - c0.a ) );
				stops.push_back( LinearGradientDrawable::ColorStop( pos, Color( r, g, b, a ) ) );
			}
		}

		if ( stops.size() < 2 )
			return NULL;

		LinearGradientDrawable* drawable =
			repeating ? LinearGradientDrawable::NewRepeating() : LinearGradientDrawable::New();
		drawable->setColorStops( std::move( stops ) );
		drawable->setAngle( angle );
		ownIt = true;
		return drawable;
	};

	mFuncs["linear-gradient"] = [parseGradient]( const FunctionString& functionType,
												 const Sizef& /*size*/, bool& ownIt,
												 UINode* node ) -> Drawable* {
		return parseGradient( functionType, ownIt, node, false );
	};

	mFuncs["repeating-linear-gradient"] = [parseGradient]( const FunctionString& functionType,
														   const Sizef& /*size*/, bool& ownIt,
														   UINode* node ) -> Drawable* {
		return parseGradient( functionType, ownIt, node, true );
	};

	// Shared parsing logic for radial-gradient and repeating-radial-gradient
	auto parseRadialGradient = []( const FunctionString& functionType, bool& ownIt, UINode* node,
								   bool repeating ) -> Drawable* {
		const auto& params( functionType.getParameters() );
		if ( params.size() < 2 )
			return NULL;

		size_t paramIdx = 0;
		RadialGradientDrawable::ShapeType shape = RadialGradientDrawable::CIRCLE;
		RadialGradientDrawable::Extent extent = RadialGradientDrawable::FARTHEST_CORNER;
		Vector2f center( 0.5f, 0.5f );

		if ( paramIdx < params.size() ) {
			std::string kw = String::toLower( String::trim( params[paramIdx] ) );
			if ( kw == "circle" ) {
				shape = RadialGradientDrawable::CIRCLE;
				paramIdx++;
			} else if ( kw == "ellipse" ) {
				shape = RadialGradientDrawable::ELLIPSE;
				paramIdx++;
			}
		}

		std::vector<ColorStopResult> gradientStops;
		for ( size_t i = paramIdx; i < params.size(); i++ ) {
			std::string stopParam = params[i];
			String::trimInPlace( stopParam );

			ColorStopResult stop = parseColorStop( stopParam );
			if ( !stop.valid )
				continue;

			gradientStops.push_back( stop );
			if ( stop.secondValue >= 0.f ) {
				ColorStopResult stop2;
				stop2.valid = true;
				stop2.color = stop.color;
				stop2.value = stop.secondValue;
				stop2.unit = stop.secondUnit;
				stop2.secondValue = -1.f;
				gradientStops.push_back( stop2 );
			}
		}

		// CSS §4.3: If a color-stop has a position that is less than
		// the specified position of any color-stop before it in the
		// list, set its position to be equal to the largest specified
		// position of any color-stop before it.
		{
			Float maxPos = -1.f;
			StyleSheetLength::Unit maxUnit = StyleSheetLength::Percentage;
			bool hasMax = false;
			for ( auto& s : gradientStops ) {
				if ( s.isHint )
					continue;
				if ( s.value >= 0.f ) {
					if ( hasMax && s.value < maxPos ) {
						s.value = maxPos;
						s.unit = maxUnit;
					} else {
						maxPos = s.value;
						maxUnit = s.unit;
						hasMax = true;
					}
				}
				if ( s.secondValue >= 0.f ) {
					if ( hasMax && s.secondValue < maxPos ) {
						s.secondValue = maxPos;
						s.secondUnit = maxUnit;
					} else {
						maxPos = s.secondValue;
						maxUnit = s.secondUnit;
						hasMax = true;
					}
				}
			}
		}

		{
			int colorStopCount = 0;
			for ( const auto& s : gradientStops ) {
				if ( !s.isHint )
					colorStopCount++;
			}
			if ( colorStopCount < 2 )
				return NULL;
		}

		std::sort( gradientStops.begin(), gradientStops.end(),
				   []( const ColorStopResult& a, const ColorStopResult& b ) {
					   return a.value < b.value;
				   } );

		// Set boundary defaults for unpositioned stops
		for ( size_t i = 0; i < gradientStops.size(); i++ ) {
			if ( !gradientStops[i].isHint && gradientStops[i].value < 0.f ) {
				gradientStops[i].value = 0.f;
				break;
			}
		}
		for ( size_t i = gradientStops.size(); i > 0; i-- ) {
			if ( !gradientStops[i - 1].isHint && gradientStops[i - 1].value < 0.f ) {
				gradientStops[i - 1].value = 1.f;
				break;
			}
		}

		// Fill missing positions on color stops. Hints always have explicit
		// positions so they are left untouched.
		for ( size_t i = 0; i < gradientStops.size(); i++ ) {
			if ( gradientStops[i].isHint || gradientStops[i].value >= 0.f )
				continue;
			size_t prevKnown = i;
			size_t nextKnown = i;
			while ( prevKnown > 0 && ( gradientStops[prevKnown - 1].value < 0.f ) )
				prevKnown--;
			while ( nextKnown < gradientStops.size() &&
					( gradientStops[nextKnown].isHint || gradientStops[nextKnown].value < 0.f ) )
				nextKnown++;

			Float startPos = ( prevKnown > 0 && !gradientStops[prevKnown - 1].isHint )
								 ? gradientStops[prevKnown - 1].value
								 : 0.f;
			Float endPos =
				( nextKnown < gradientStops.size() ) ? gradientStops[nextKnown].value : 100.f;
			size_t range = nextKnown - prevKnown + 1;

			for ( size_t j = prevKnown; j < nextKnown; j++ ) {
				if ( gradientStops[j].isHint )
					continue;
				gradientStops[j].value =
					startPos +
					( endPos - startPos ) * ( (Float)( j - prevKnown + 1 ) / (Float)range );
			}

			i = nextKnown;
		}

		// Expand hinted segments into finely sampled color stops
		std::vector<RadialGradientDrawable::ColorStop> stops;
		const int HINT_SAMPLES = 16;

		for ( size_t i = 0; i < gradientStops.size(); i++ ) {
			if ( gradientStops[i].isHint )
				continue;

			// Add the current color stop
			stops.push_back( RadialGradientDrawable::ColorStop(
				gradientStops[i].value, gradientStops[i].color, gradientStops[i].unit ) );

			// Find the next color stop and any hints between them
			size_t nextColor = i + 1;
			while ( nextColor < gradientStops.size() && gradientStops[nextColor].isHint )
				nextColor++;
			if ( nextColor >= gradientStops.size() )
				break;

			// Collect hints between current and next color stop
			std::vector<Float> hints;
			for ( size_t h = i + 1; h < nextColor; h++ ) {
				if ( gradientStops[h].isHint )
					hints.push_back( gradientStops[h].value );
			}

			if ( hints.empty() )
				continue;

			Float p0 = gradientStops[i].value;
			Float p1 = gradientStops[nextColor].value;
			Color c0 = gradientStops[i].color;
			Color c1 = gradientStops[nextColor].color;

			if ( std::abs( p1 - p0 ) < 0.0001f )
				continue;

			// Apply hints sequentially. Each hint adjusts the transition
			// between the previously computed mid-points.
			// For a single hint at position h between p0 and p1:
			//   normalized hint hn = (h - p0) / (p1 - p0)
			//   power = log(0.5) / log(hn)
			//   color(p) = lerp(c0, c1, ((p-p0)/(p1-p0))^power)
			// We sample HINT_SAMPLES points and add them as color stops.
			Float hintNorm = ( hints[0] - p0 ) / ( p1 - p0 );
			hintNorm = eemax( 0.001f, eemin( 0.999f, hintNorm ) );
			Float power = std::log( 0.5f ) / std::log( hintNorm );

			for ( int s = 1; s < HINT_SAMPLES; s++ ) {
				Float x = (Float)s / (Float)HINT_SAMPLES;
				Float t = std::pow( x, power );
				Float pos = p0 + x * ( p1 - p0 );
				Uint8 r = (Uint8)( (Float)c0.r + t * (Float)( c1.r - c0.r ) );
				Uint8 g = (Uint8)( (Float)c0.g + t * (Float)( c1.g - c0.g ) );
				Uint8 b = (Uint8)( (Float)c0.b + t * (Float)( c1.b - c0.b ) );
				Uint8 a = (Uint8)( (Float)c0.a + t * (Float)( c1.a - c0.a ) );
				stops.push_back( RadialGradientDrawable::ColorStop( pos, Color( r, g, b, a ) ) );
			}
		}

		if ( stops.size() < 2 )
			return NULL;

		RadialGradientDrawable* drawable =
			repeating ? RadialGradientDrawable::NewRepeating() : RadialGradientDrawable::New();
		drawable->setColorStops( std::move( stops ) );
		drawable->setShape( shape );
		drawable->setExtent( extent );
		drawable->setCenter( center );
		ownIt = true;
		return drawable;
	};

	mFuncs["radial-gradient"] = [parseRadialGradient]( const FunctionString& functionType,
													   const Sizef& /*size*/, bool& ownIt,
													   UINode* node ) -> Drawable* {
		return parseRadialGradient( functionType, ownIt, node, false );
	};

	mFuncs["repeating-radial-gradient"] = [parseRadialGradient]( const FunctionString& functionType,
																 const Sizef& /*size*/, bool& ownIt,
																 UINode* node ) -> Drawable* {
		return parseRadialGradient( functionType, ownIt, node, true );
	};

	mFuncs["circle"] = []( const FunctionString& functionType, const Sizef& size, bool& ownIt,
						   UINode* node ) -> Drawable* {
		if ( functionType.getParameters().size() < 1 ) {
			return NULL;
		}

		CircleDrawable* drawable = CircleDrawable::New();

		const auto& params( functionType.getParameters() );

		StyleSheetLength length( params[0] );
		drawable->setRadius( node->convertLength( length, size.getWidth() / 2.f ) );

		if ( params.size() >= 2 ) {
			drawable->setColor( Color::fromString( params[1] ) );
		}

		if ( params.size() >= 3 ) {
			std::string fillMode( String::toLower( params[2] ) );
			if ( fillMode == "line" || fillMode == "solid" || fillMode == "fill" )
				drawable->setFillMode( fillMode == "line" ? DRAW_LINE : DRAW_FILL );

			if ( params.size() >= 4 && params[3] == "smooth" )
				drawable->setSmooth( true );
		}

		drawable->setOffset( drawable->getSize() / 2.f );
		ownIt = true;
		return drawable;
	};

	mFuncs["rectangle"] = []( const FunctionString& functionType, const Sizef& size, bool& ownIt,
							  UINode* node ) -> Drawable* {
		if ( functionType.getParameters().size() < 1 ) {
			return NULL;
		}

		RectangleDrawable* drawable = RectangleDrawable::New();
		RectColors rectColors;
		std::vector<Color> colors;

		const auto& params( functionType.getParameters() );

		for ( size_t i = 0; i < params.size(); i++ ) {
			std::string param( String::toLower( params[i] ) );

			if ( param == "solid" || param == "fill" ) {
				drawable->setFillMode( DRAW_FILL );
			} else if ( String::startsWith( param, "line" ) ) {
				drawable->setFillMode( DRAW_LINE );

				std::vector<std::string> parts( String::split( param, ' ' ) );

				if ( parts.size() >= 2 ) {
					StyleSheetLength length( parts[1] );
					drawable->setLineWidth( node->convertLength( length, size.getWidth() ) );
				}
			} else if ( param.find( "º" ) != std::string::npos ) {
				String::replaceAll( param, "º", "" );
				Float floatVal;
				if ( String::fromString( floatVal, param ) ) {
					drawable->setRotation( floatVal );
				}
			} else if ( Color::isColorString( param ) ) {
				colors.push_back( Color::fromString( param ) );
			} else {
				int intVal = 0;

				if ( PixelDensity::toPxFromStringI( param ) ) {
					drawable->setCorners( intVal );
				}
			}
		}

		if ( colors.size() > 0 ) {
			while ( colors.size() < 4 ) {
				colors.push_back( colors[colors.size() - 1] );
			};

			rectColors.TopLeft = colors[0];
			rectColors.BottomLeft = colors[1];
			rectColors.BottomRight = colors[2];
			rectColors.TopRight = colors[3];
			drawable->setRectColors( rectColors );
			ownIt = true;
			return drawable;
		} else {
			eeSAFE_DELETE( drawable );
		}

		return drawable;
	};

	mFuncs["triangle"] = []( const FunctionString& functionType, const Sizef& size, bool& ownIt,
							 UINode* node ) -> Drawable* {
		if ( functionType.getParameters().size() < 2 ) {
			return NULL;
		}

		TriangleDrawable* drawable = TriangleDrawable::New();
		std::vector<Color> colors;
		std::vector<Vector2f> vertices;

		const auto& params( functionType.getParameters() );
		Float lineWidth = PixelDensity::dpToPx( 1.f );

		for ( size_t i = 0; i < params.size(); i++ ) {
			std::string param( String::toLower( params[i] ) );

			if ( param == "solid" || param == "fill" ) {
				drawable->setFillMode( DRAW_FILL );
			} else if ( String::startsWith( param, "line" ) ) {
				drawable->setFillMode( DRAW_LINE );
			} else if ( Color::isColorString( param ) ) {
				colors.push_back( Color::fromString( param ) );
			} else if ( !functionType.parameterWasString( i ) &&
						StyleSheetLength::isLength( param ) ) {
				lineWidth = node->convertLength( StyleSheetLength( param ), size.getWidth() );
			} else {
				std::vector<std::string> vertex( String::split( param, ',' ) );

				if ( vertex.size() == 3 ) {
					for ( size_t v = 0; v < vertex.size(); v++ ) {
						String::trimInPlace( vertex[v] );
						std::vector<std::string> coords( String::split( vertex[v], ' ' ) );

						if ( coords.size() == 2 ) {
							StyleSheetLength posX( coords[0] );
							StyleSheetLength posY( coords[1] );
							vertices.push_back(
								Vector2f( node->convertLength( posX, size.getWidth() ),
										  node->convertLength( posY, size.getHeight() ) ) );
						}
					}
				}
			}
		}

		if ( vertices.size() == 3 && !colors.empty() ) {
			drawable->setLineWidth( lineWidth );

			Triangle2f triangle;

			for ( size_t i = 0; i < 3; i++ ) {
				triangle.V[i] = vertices[i];
			}

			if ( colors.size() == 3 ) {
				drawable->setTriangleColors( colors[0], colors[1], colors[2] );
			} else {
				drawable->setColor( colors[0] );
			}

			drawable->setTriangle( triangle );
			ownIt = true;
			return drawable;
		} else {
			eeSAFE_DELETE( drawable );
		}

		return drawable;
	};

	mFuncs["poly"] = []( const FunctionString& functionType, const Sizef& size, bool& ownIt,
						 UINode* node ) -> Drawable* {
		if ( functionType.getParameters().size() < 2 ) {
			return NULL;
		}

		ConvexShapeDrawable* drawable = ConvexShapeDrawable::New();
		std::vector<Color> colors;
		std::vector<Vector2f> vertices;

		const auto& params( functionType.getParameters() );
		Float lineWidth = PixelDensity::dpToPx( 1.f );

		for ( size_t i = 0; i < params.size(); i++ ) {
			std::string param( String::toLower( params[i] ) );

			if ( param == "solid" || param == "fill" ) {
				drawable->setFillMode( DRAW_FILL );
			} else if ( String::startsWith( param, "line" ) ) {
				drawable->setFillMode( DRAW_LINE );
			} else if ( Color::isColorString( param ) ) {
				colors.push_back( Color::fromString( param ) );
			} else if ( !functionType.parameterWasString( i ) &&
						StyleSheetLength::isLength( param ) ) {
				lineWidth = node->convertLength( StyleSheetLength( param ), size.getWidth() );
			} else {
				std::vector<std::string> vertex( String::split( param, ',' ) );

				for ( size_t v = 0; v < vertex.size(); v++ ) {
					vertex[v] = String::trim( vertex[v] );
					std::vector<std::string> coords( String::split( vertex[v], ' ' ) );

					if ( coords.size() == 2 ) {
						StyleSheetLength posX( coords[0] );
						StyleSheetLength posY( coords[1] );
						vertices.push_back(
							Vector2f( node->convertLength( posX, size.getWidth() ),
									  node->convertLength( posY, size.getHeight() ) ) );
					}
				}
			}
		}

		if ( vertices.size() >= 2 && !colors.empty() ) {
			drawable->setLineWidth( lineWidth );

			for ( size_t i = 0; i < vertices.size(); i++ ) {
				drawable->addPoint( vertices[i], colors[i % colors.size()] );
			}

			ownIt = true;
			return drawable;
		} else {
			eeSAFE_DELETE( drawable );
		}

		return drawable;
	};

	mFuncs["url"] = []( const FunctionString& functionType, const Sizef& /*size*/, bool& /*ownIt*/,
						UINode* node ) -> Drawable* {
		if ( functionType.getParameters().size() < 1 )
			return NULL;
		const auto& param = functionType.getParameters().at( 0 );
		if ( functionType.getName() == "url" && !param.empty() && param[0] != '@' &&
			 !String::startsWith( param, "data:image/" ) ) {
			return DrawableSearcher::searchByName(
				node->getUISceneNode()->solveRelativePath( param ).toString(), false,
				node->getUISceneNode()->getReferer() );
		} else if ( functionType.getParameters().size() > 1 &&
					String::startsWith( param, "data:image/" ) ) {
			auto cparam = functionType.getParameters().at( 0 );
			for ( size_t i = 1; i < functionType.getParameters().size(); i++ ) {
				cparam += ',';
				cparam += functionType.getParameters().at( i );
			}
			return DrawableSearcher::searchByName( cparam, false,
												   node->getUISceneNode()->getReferer() );
		}
		return DrawableSearcher::searchByName( param, false, node->getUISceneNode()->getReferer() );
	};

	mFuncs["icon"] = []( const FunctionString& functionType, const Sizef& size, bool&,
						 UINode* node ) -> Drawable* {
		auto* uiScene = SceneManager::instance()->getUISceneNode();
		const auto& params = functionType.getParameters();
		if ( params.size() < 2 )
			return nullptr;
		StyleSheetLength length( params[1] );
		return uiScene->findIconDrawable( params[0],
										  node->convertLength( length, size.getWidth() ) );
	};

	mFuncs["glyph"] = []( const FunctionString& functionType, const Sizef& size, bool&,
						  UINode* node ) -> Drawable* {
		const auto& params = functionType.getParameters();
		if ( params.size() < 3 )
			return nullptr;
		Font* font = FontManager::instance()->getByName( params[0] );
		if ( font == nullptr )
			return nullptr;
		Uint32 codePoint = 0;
		std::string buffer( params[1] );
		Uint32 value;
		if ( functionType.parameterWasString( 2 ) ) {
			String unicodeChar = String::fromUtf8( params[2] );
			if ( !unicodeChar.empty() )
				codePoint = unicodeChar[0];
		} else if ( String::startsWith( buffer, "0x" ) ) {
			if ( String::fromString( value, buffer, 16 ) )
				codePoint = value;
		} else if ( String::fromString( value, buffer ) ) {
			codePoint = value;
		}
		return font->getGlyphDrawable( codePoint,
									   node->convertLength( params[1], size.getWidth() ) );
	};
}

}}} // namespace EE::UI::CSS
