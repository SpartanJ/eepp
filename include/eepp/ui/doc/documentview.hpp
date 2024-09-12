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

	static LineWrapInfo
	computeLineBreaks( const String::View& string, const FontStyleConfig& fontStyle, Float maxWidth,
					   LineWrapMode mode, bool keepIndentation, Uint32 tabWidth = 4,
					   Float whiteSpaceWidth = 0.f /* 0 = should calculate it */ );

	static LineWrapInfo computeLineBreaks( const String& string, const FontStyleConfig& fontStyle,
										   Float maxWidth, LineWrapMode mode, bool keepIndentation,
										   Uint32 tabWidth = 4, Float whiteSpaceWidth = 0.f );

	static LineWrapInfo computeLineBreaks( const TextDocument& doc, size_t line,
										   const FontStyleConfig& fontStyle, Float maxWidth,
										   LineWrapMode mode, bool keepIndentation,
										   Uint32 tabWidth = 4, Float whiteSpaceWidth = 0.f );

	static Float computeOffsets( const String::View& string, const FontStyleConfig& fontStyle,
								 Uint32 tabWidth, Float maxWidth = 0.f );

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

	void clearCache();

	double getLineYOffset( VisibleIndex visibleIndex, Float lineHeight ) const;

	double getLineYOffset( Int64 docIdx, Float lineHeight ) const;

	bool isLineVisible( Int64 docIdx ) const;

	bool isFolded( Int64 docIdx, bool andNotFirstLine = false ) const;

	std::optional<TextRange> isInFoldedRange( TextRange range, bool andNotFirstLine ) const;

	void foldRegion( Int64 foldDocIdx );

	void unfoldRegion( Int64 foldDocIdx );

	bool isOneToOne() const;

	std::vector<TextRange> intersectsFoldedRegions( const TextRange& range ) const;

	Float getWhiteSpaceWidth() const;

	/* Unfolds any folded region that contains a current non-visible cursor */
	void ensureCursorVisibility();

	const std::vector<TextRange> getFoldedRegions() const { return mFoldedRegions; }

	void onFoldRegionsUpdated();

  protected:
	std::shared_ptr<TextDocument> mDoc;
	FontStyleConfig mFontStyle;
	Config mConfig;
	Float mMaxWidth{ 0 };
	Float mWhiteSpaceWidth{ 0 };
	std::vector<TextPosition> mVisibleLines;
	std::vector<Float> mVisibleLinesOffset;
	std::vector<Int64> mDocLineToVisibleIndex;
	std::vector<TextRange> mFoldedRegions;
	bool mPendingReconstruction{ false };
	bool mUnderConstruction{ false };
	bool mUpdatingFoldRegions{ false };

	void changeVisibility( Int64 fromDocIdx, Int64 toDocIdx, bool visible,
						   bool recomputeOffset = true, bool recomputeLineToVisibleIndex = true );

	void removeFoldedRegion( const TextRange& region );

	void shiftFoldingRegions( Int64 fromLine, Int64 numLines );

	void verifyStructuralConsistency();

	void recomputeDocLineToVisibleIndex( Int64 fromVisibleIndex, bool ensureDocSize = true );

	void unfoldRegion( Int64 foldDocIdx, bool verifyConsistency, bool recomputeOffset = true,
					   bool recomputeLineToVisibleIndex = true );

	void moveCursorToVisibleArea();
};

}}} // namespace EE::UI::Doc

#endif
