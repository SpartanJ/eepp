#ifndef EE_UI_TOOLS_UIFINDBARSTYLE_HPP
#define EE_UI_TOOLS_UIFINDBARSTYLE_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace UI {

class UISceneNode;

namespace Tools {

class EE_API UIFindBarStyle {
  public:
	static const char* getStyleSheet();

	/** Installs the shared find-bar stylesheet into a scene once. */
	static void ensure( UISceneNode* scene );
};

} // namespace Tools
}} // namespace EE::UI

#endif
