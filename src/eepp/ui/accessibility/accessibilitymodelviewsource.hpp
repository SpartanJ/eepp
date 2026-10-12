#ifndef EE_UI_ACCESSIBILITY_ACCESSIBILITYMODELVIEWSOURCE_HPP
#define EE_UI_ACCESSIBILITY_ACCESSIBILITYMODELVIEWSOURCE_HPP

#include <eepp/ui/accessibility/accessibilitysource.hpp>
#include <memory>

namespace EE { namespace UI {

class AccessibilityManager;
class UIWidget;

std::unique_ptr<AccessibilitySource>
createAccessibilityModelViewSource( AccessibilityManager& manager, AccessibilitySourceId sourceId,
									UIWidget* widget );

}} // namespace EE::UI

#endif
