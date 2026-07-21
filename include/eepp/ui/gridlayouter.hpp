#ifndef EE_UI_GRIDLAYOUTER_HPP
#define EE_UI_GRIDLAYOUTER_HPP

#include <eepp/core/containers.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/csslayouttypes.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/uilayouter.hpp>
#include <limits>

namespace EE { namespace UI {

class UIWidget;
class UIHTMLWidget;

// ── Structured grid track data model (used by GridLayouter and tests) ──

enum class GridTrackBreadthType {
	Length,
	Percentage,
	Flex,
	Auto,
	MinContent,
	MaxContent,
	FitContent
};

struct EE_API GridTrackBreadth {
	GridTrackBreadthType type{ GridTrackBreadthType::Auto };
	Float value{ 0.f };
	std::string raw{ "auto" };
	bool valid{ true };
};

struct EE_API GridTrackSize {
	GridTrackBreadth min;
	GridTrackBreadth max;
};

struct EE_API GridExplicitTrack {
	SmallVector<std::string, 2> beforeLineNames;
	GridTrackSize size;
};

struct EE_API GridTrackList {
	std::vector<GridExplicitTrack> tracks;
	bool none{ false };
	bool valid{ true };
	bool hasAutoRepeat{ false };
	bool autoRepeatIsFit{ false };
	size_t autoRepeatPosition{ 0 };
	std::vector<GridExplicitTrack> autoRepeatTemplate;
};

struct EE_API GridTrackParser {
	static GridTrackList parseTrackList( const std::string& value );
	static bool isValidTrackList( const std::string& value );
	static void expandAutoRepeat( GridTrackList& list, Float availableSize, Float gap );
};

struct EE_API GridTrack {
	GridTrackSize definition;
	Float baseSize{ 0.f };
	Float growthLimit{ std::numeric_limits<Float>::infinity() };
	bool isFlex{ false };
};

struct EE_API GridItem {
	UIWidget* widget;
	int order{ 0 };
	std::string gridRowStart{ "auto" };
	std::string gridRowEnd{ "auto" };
	std::string gridColumnStart{ "auto" };
	std::string gridColumnEnd{ "auto" };
	int resolvedRowStart{ 0 }; // 0 = auto
	int resolvedRowEnd{ 0 };
	int resolvedColumnStart{ 0 };
	int resolvedColumnEnd{ 0 };
	int columnSpan{ 1 };
	int rowSpan{ 1 };
	CSSJustifySelf justifySelf{ CSSJustifySelf::Stretch };
	CSSAlignSelf alignSelf{ CSSAlignSelf::Stretch };
	bool isTextNode{ false };
	bool isOutOfFlow{ false };
};

// ── Parser helper exposed for tests ──

struct EE_API GridAreaTemplate {
	struct Area {
		int startRow{ 1 };
		int startColumn{ 1 };
		int endRow{ 2 };
		int endColumn{ 2 };
	};
	UnorderedMap<std::string, Area> areas;
	bool valid{ true };
	bool none{ false };
};

struct EE_API GridAreasParser {
	static GridAreaTemplate parseAreas( const std::string& value );
	static bool isValidAreas( const std::string& value );
	static std::vector<std::string> getLineNamesForRowLine( const GridAreaTemplate& tmpl,
															int line );
	static std::vector<std::string> getLineNamesForColumnLine( const GridAreaTemplate& tmpl,
															   int line );
};

struct EE_API GridLineResult {
	bool isAuto{ true };
	bool isSpan{ false };
	int lineNumber{ 0 };
	int spanCount{ 1 };
	std::string nameId;
};

struct EE_API GridLineParser {
	static GridLineResult parse( const std::string& value );
};

// ── Layouter ──

class EE_API GridLayouter : public UILayouter {
  public:
	GridLayouter( UIWidget* container ) : UILayouter( container ) {}

	void updateLayout() override;
	void computeIntrinsicWidths() override;
	Float getMinIntrinsicWidth() override;
	Float getMaxIntrinsicWidth() override;

	Float getBaseline() const { return mContainerBaseline; }

	const SmallVector<GridItem, 16>& getItems() const { return mItems; }
	const std::vector<GridTrack>& getColumns() const { return mColumns; }

  protected:
	void readContainerStyle();
	void collectGridItems();
	void resolveDefinitePlacements();
	void autoPlaceItems();
	void buildImplicitTracks();
	void collapseEmptyAutoFitTracks();
	void preSizeItemsForRowSizing();
	void sizeTracksForAxis( bool isColumns );
	void applyLayout();

	SmallVector<GridItem, 16> mItems;
	std::vector<GridTrack> mColumns;
	std::vector<GridTrack> mRows;
	GridAreaTemplate mTemplateAreas;
	Float mColumnGap{ 0.f };
	Float mRowGap{ 0.f };
	Float mContainerBaseline{ 0.f };
	int mMaxColumn{ 0 };
	int mMaxRow{ 0 };
	CSSGridAutoFlow mAutoFlow{ CSSGridAutoFlow::Row };
	bool mAutoFlowDense{ false };
	bool mColAutoRepeatIsFit{ false };
	bool mRowAutoRepeatIsFit{ false };
	GridTrackSize mAutoColumnSize;
	GridTrackSize mAutoRowSize;
	size_t mExplicitColCount{ 0 };
	size_t mExplicitRowCount{ 0 };
	UnorderedMap<std::string, std::vector<int>> mColLineNames;
	UnorderedMap<std::string, std::vector<int>> mRowLineNames;
};

}} // namespace EE::UI

#endif
