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
		Float paddingStart{ 0.f };
	};

	struct VisualLine {
		Int64 visualIndex{ 0 };
		Float paddingStart{ 0 };
		std::vector<TextPosition> visualLines;
	};

	struct VisualLineInfo {
		Int64 visualIndex;
		TextRange range;
	};

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

	size_t getTotalLines() const;

	const Config& config() const { return mConfig; }

	void invalidateCache();

	void updateCache( Int64 fromLine, Int64 toLine, Int64 numLines );

	Config getConfig() const { return mConfig; }

	void setConfig( Config config );

	void setMaxWidth( Float maxWidth, bool forceReconstructBreaks = false );

	void setFontStyle( FontStyleConfig fontStyle );

	void setLineWrapMode( LineWrapMode mode );

	TextPosition getDocumentLine( Int64 visibleIndex ) const;

	Float getLineOffset( Int64 docIdx ) const;

	Int64 toWrappedIndex( Int64 docIdx, bool retLast = false ) const;

	bool isWrappedLine( Int64 docIdx ) const;

	VisualLine getVisualLine( Int64 docIdx ) const;

	VisualLineInfo getVisualLineInfo( const TextPosition& pos,
									  bool allowVisualLineEnd = false ) const;

	TextRange getVisualLineRange( Int64 visualLine ) const;

	std::shared_ptr<TextDocument> getDocument() const;

	void setDocument( const std::shared_ptr<TextDocument>& doc );

	bool isPendingReconstruction() const;

	void setPendingReconstruction( bool pendingReconstruction );

	void clear();

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
