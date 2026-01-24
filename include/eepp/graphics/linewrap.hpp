#pragma once

#include <eepp/config.hpp>
#include <eepp/graphics/fontstyleconfig.hpp>
#include <vector>

namespace EE::Graphics {

enum class LineWrapMode { NoWrap, Letter, Word };

enum class LineWrapType { Viewport, LineBreakingColumn };

struct LineWrapInfo {
	std::vector<Int64> wraps;
	Float paddingStart{ 0 };
};

class EE_API LineWrap {
  public:
	static LineWrapMode toLineWrapMode( std::string mode );

	static std::string fromLineWrapMode( LineWrapMode mode );

	static LineWrapType toLineWrapType( std::string type );

	static std::string fromLineWrapType( LineWrapType type );

	static bool isWrapChar( String::StringBaseType ch );

	static LineWrapInfo computeLineBreaks(
		const String::View& string, Font* font, Uint32 characterSize, Float maxWidth,
		LineWrapMode mode, Uint32 fontStyle, Float outlineThickness, bool keepIndentation,
		Uint32 tabWidth = 4, Float whiteSpaceWidth = 0.f /* 0 = should calculate it */,
		Uint32 textHints = TextHints::None, bool tabStops = false, Float initialXOffset = 0.f );

	static LineWrapInfo computeLineBreaks(
		const String& string, Font* font, Uint32 characterSize, Float maxWidth, LineWrapMode mode,
		Uint32 fontStyle, Float outlineThickness, bool keepIndentation, Uint32 tabWidth = 4,
		Float whiteSpaceWidth = 0.f /* 0 = should calculate it */,
		Uint32 textHints = TextHints::None, bool tabStops = false, Float initialXOffset = 0.f );

	static LineWrapInfo
	computeLineBreaks( const String::View& string, const FontStyleConfig& fontStyle, Float maxWidth,
					   LineWrapMode mode, bool keepIndentation, Uint32 tabWidth = 4,
					   Float whiteSpaceWidth = 0.f /* 0 = should calculate it */,
					   Uint32 textHints = TextHints::None, bool tabStops = false,
					   Float initialXOffset = 0.f );

	static LineWrapInfo computeLineBreaks( const String& string, const FontStyleConfig& fontStyle,
										   Float maxWidth, LineWrapMode mode, bool keepIndentation,
										   Uint32 tabWidth = 4, Float whiteSpaceWidth = 0.f,
										   Uint32 textHints = TextHints::None,
										   bool tabStops = false, Float initialXOffset = 0.f );

	static Float computeOffsets( const String::View& string, Font* font, Uint32 characterSize,
								 Uint32 fontStyle, Float outlineThickness, Uint32 tabWidth = 0.f,
								 Float maxWidth = 0.f, bool tabStops = false );

	static Float computeOffsets( const String::View& string, const FontStyleConfig& fontStyle,
								 Uint32 tabWidth, Float maxWidth = 0.f, bool tabStops = false );
};

} // namespace EE::Graphics
