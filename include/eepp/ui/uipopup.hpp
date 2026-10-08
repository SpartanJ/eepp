#ifndef EE_UI_UIPOPUP_HPP
#define EE_UI_UIPOPUP_HPP

#include <eepp/config.hpp>
#include <eepp/math/vector2.hpp>

namespace EE { namespace Scene {
class Node;
}} // namespace EE::Scene

namespace EE { namespace UI {

using namespace EE::Math;
using namespace EE::Scene;

class UIWidget;

/** Shared popup placement and visibility helpers for anchored UI controls.
 *
 * The helper deliberately owns no widget lifetime or selection semantics. Callers decide when a
 * popup should close and where focus returns; this only centralizes root/window parenting,
 * below/above placement, scene-bound clamping and the standard eepp fade effects.
 */
class EE_API UIPopUp {
  public:
	/** Returns a world-pixel position without reparenting or moving the popup. With an anchor,
	 * prefer below then above; trySides also permits right/left placement for trigger menus.
	 * Without an anchor, try the four quadrants around position for cursor menus. */
	static Vector2f findBestPosition( UIWidget* popup, const Vector2f& position,
									  Node* anchor = nullptr, bool trySides = false );

	static void align( UIWidget* anchor, UIWidget* popup, bool popUpToRoot = false,
					   bool centered = false );

	static bool hasFocus( UIWidget* anchor, UIWidget* popup );

	static void show( UIWidget* popup );

	static void hide( UIWidget* popup );
};

}} // namespace EE::UI

#endif
