#ifndef EE_UI_DOCUMENTVIEW_HPP
#define EE_UI_DOCUMENTVIEW_HPP

#include <eepp/graphics/fontstyleconfig.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/doc/textposition.hpp>
#include <optional>

using namespace EE::Graphics;
using namespace EE::UI::Doc;

namespace EE { namespace UI { namespace Doc {

enum class LineWrapMode { NoWrap, Letter, Word };

enum class LineWrapType { Viewport, LineBreakingColumn };

enum class VisibleIndex : Int64 { first = 0, invalid = std::numeric_limits<Int64>::max() };

inline VisibleIndex visibleIndexOffset( VisibleIndex idx, Int64 offset ) {
	return static_cast<VisibleIndex>( static_cast<Int64>( idx ) + offset );
}

class EE_API DocumentView {
  public:
	static LineWrapMode toLineWrapMode( std::string mode );

	static std::string fromLineWrapMode( LineWrapMode mode );

	static LineWrapType toLineWrapType( std::string type );

	static std::string fromLineWrapType( LineWrapType type );

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

	struct LineWrapInfo {
		std::vector<Int64> wraps;
		Float paddingStart{ 0 };
	};

	struct VisibleLineInfo {
		VisibleIndex visibleIndex{ VisibleIndex::invalid };
		Float paddingStart{ 0 };
		std::vector<TextPosition> visualLines;
	};

	struct VisibleLineRange {
		VisibleIndex visibleIndex{ VisibleIndex::invalid };
		TextRange range;
	};

	static LineWrapInfo computeLineBreaks( const String::View& string,
										   const FontStyleConfig& fontStyle, Float maxWidth,
										   LineWrapMode mode, bool keepIndentation,
										   Uint32 tabWidth = 4 );

	static LineWrapInfo computeLineBreaks( const String& string, const FontStyleConfig& fontStyle,
										   Float maxWidth, LineWrapMode mode, bool keepIndentation,
										   Uint32 tabWidth = 4 );

	static LineWrapInfo computeLineBreaks( const TextDocument& doc, size_t line,
										   const FontStyleConfig& fontStyle, Float maxWidth,
										   LineWrapMode mode, bool keepIndentation,
										   Uint32 tabWidth = 4 );

	static Float computeOffsets( const String& string, const FontStyleConfig& fontStyle,
								 Uint32 tabWidth );

	DocumentView( std::shared_ptr<TextDocument> doc, FontStyleConfig fontStyle, Config config );

	bool isWrapEnabled() const;

	size_t getVisibleLinesCount() const;

	const Config& config() const { return mConfig; }

	void invalidateCache();

	void updateCache( Int64 fromLine, Int64 toLine, Int64 numLines );

	Config getConfig() const { return mConfig; }

	void setConfig( Config config );

	void setMaxWidth( Float maxWidth, bool forceReconstructBreaks = false );

	void setFontStyle( FontStyleConfig fontStyle );

	void setLineWrapMode( LineWrapMode mode );

	TextPosition getVisibleIndexPosition( VisibleIndex visibleIndex ) const;

	Float getLinePadding( Int64 docIdx ) const;

	VisibleIndex toVisibleIndex( Int64 docIdx, bool retLast = false ) const;

	bool isWrappedLine( Int64 docIdx ) const;

	VisibleLineInfo getVisibleLineInfo( Int64 docIdx ) const;

	VisibleLineRange getVisibleLineRange( const TextPosition& pos,
										  bool allowVisualLineEnd = false ) const;

	TextRange getVisibleIndexRange( VisibleIndex visibleIndex ) const;

	std::shared_ptr<TextDocument> getDocument() const;

	void setDocument( const std::shared_ptr<TextDocument>& doc );

	bool isPendingReconstruction() const;

	void setPendingReconstruction( bool pendingReconstruction );

	void clear();

	Float getLineYOffset( VisibleIndex visibleIndex, Float lineHeight ) const;

	Float getLineYOffset( Int64 docIdx, Float lineHeight ) const;

	bool isLineVisible( Int64 docIdx ) const;

  protected:
	std::shared_ptr<TextDocument> mDoc;
	FontStyleConfig mFontStyle;
	Config mConfig;
	Float mMaxWidth{ 0 };
	std::vector<TextPosition> mWrappedLines;
	std::vector<Float> mWrappedLinesOffset;
	std::vector<Int64> mWrappedLineToIndex;
	bool mPendingReconstruction{ false };
	bool mUnderConstruction{ false };
};

}}} // namespace EE::UI::Doc

#endif
