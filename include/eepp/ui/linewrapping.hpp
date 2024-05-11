#ifndef EE_UI_LINEWRAPPING_HPP
#define EE_UI_LINEWRAPPING_HPP

#include <eepp/graphics/fontstyleconfig.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/doc/textposition.hpp>
#include <optional>

using namespace EE::Graphics;
using namespace EE::UI::Doc;

namespace EE { namespace UI {

enum class LineWrapMode { NoWrap, Letter, Word };

struct LineWrapInfo {
	std::vector<Int64> wraps;
	Float paddingStart{ 0.f };
};

class EE_API LineWrapping {
  public:
	struct Config {
		LineWrapMode mode{ LineWrapMode::NoWrap };
		bool keepIndentation{ true };
		Uint32 tabWidth{ 4 };
		std::optional<Uint32> maxCharactersWidth;
		bool operator==( const Config& other ) {
			return mode == other.mode && keepIndentation == other.keepIndentation &&
				   tabWidth == other.tabWidth && maxCharactersWidth == other.maxCharactersWidth;
		}
		bool operator!=( const Config& other ) { return !( *this == other ); }
	};

	static LineWrapInfo computeLineBreaks( const String& string, const FontStyleConfig& fontStyle,
										   Float maxWidth, LineWrapMode mode, bool keepIndentation,
										   Uint32 tabWidth = 4 );

	static LineWrapInfo computeLineBreaks( const TextDocument& doc, size_t line,
										   const FontStyleConfig& fontStyle, Float maxWidth,
										   LineWrapMode mode, bool keepIndentation,
										   Uint32 tabWidth = 4 );

	LineWrapping( std::shared_ptr<TextDocument> doc, FontStyleConfig fontStyle, Config config );

	size_t getTotalLines() const;

	const Config& config() const { return mConfig; }

	void reconstructBreaks();

	void updateBreaks( Int64 fromLine, Int64 toLine, Int64 numLines );

	void setConfig( Config config );

	void setMaxWidth( Float maxWidth );

	void setFontStyle( FontStyleConfig fontStyle );

  protected:
	std::shared_ptr<TextDocument> mDoc;
	FontStyleConfig mFontStyle;
	Config mConfig;
	Float mMaxWidth{ 0 };
	std::vector<TextPosition> mWrappedLines;
	std::vector<Float> mWrappedLinesOffset;
	std::vector<Int64> mWrappedLineToIndex;

	Int64 toWrappedIndex( Int64 docIdx, bool retLast );
};

}} // namespace EE::UI

#endif