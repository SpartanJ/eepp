#include <eepp/core/containers.hpp>
#include <eepp/math/rect.hpp>
#include <functional>

using namespace EE::Math;

namespace EE::UI {

enum class PlacementDirection { Right, Left, Bottom, Top, None };

enum class PlacementLayout {
	Horizontal, // Strongly favors Right/Left (Ideal for tooltips/documentation)
	Vertical	// Strongly favors Bottom/Top (Ideal for dropdowns/menus)
};

struct PopupPlacementConfig {
	Rectf areaRect;	  // The full visible screen/window area bounds
	Rectf targetRect; // The main box we are attaching the popup to
	Rectf alignRect;  // Box to horizontally align with
	Rectf avoidRect;  // Box to strictly avoid overlapping (e.g., cursor line)

	PlacementLayout layoutBias = PlacementLayout::Horizontal;
	bool supportHorizontal = true; // Set to false for Dropdowns
	bool supportVertical = true;   // Set to false if you strictly want side-panels

	Float userMaxWidth;
	Float margin = 4.f;

	// Thresholds
	Float minHorizontalSpace = 200.f; // Min width needed to trigger Horizontal bonus
	Float minVerticalSpace = 100.f;	  // Min height needed to trigger Vertical bonus
	Float minScoreHeight;			  // Minimum height considered "good"
	Float maxScoreHeight;			  // Cap for height in the score calculation
};

struct PopupPlacementResult {
	Rectf rect;
	PlacementDirection direction = PlacementDirection::None;
};

class EE_API UIPlacementUtils {
  public:
	static PopupPlacementResult
	findBestPopupPlacement( const PopupPlacementConfig& config,
							const std::function<Sizef( Float availableWidth )>& measureContentCb );
};

} // namespace EE::UI
