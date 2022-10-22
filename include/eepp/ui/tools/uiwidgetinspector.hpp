#ifndef EE_UI_TOOLS_UIWIDGETINSPECTOR_HPP
#define EE_UI_TOOLS_UIWIDGETINSPECTOR_HPP

#include <eepp/config.hpp>
#include <functional>

namespace EE { namespace UI {
class UISceneNode;
class UITreeView;
class UITableView;
class UIWindow;
}} // namespace EE::UI

namespace EE { namespace UI { namespace Tools {

class EE_API UIWidgetInspector {
  public:
	static UIWindow* create( UISceneNode* sceneNode, const Float& menuIconSize = 16.f,
							 std::function<void()> highlightToggle = std::function<void()>(),
							 std::function<void()> drawBoxesToggle = std::function<void()>(),
							 std::function<void()> drawDebugDataToggle = std::function<void()>() );

  protected:
	static void checkWidgetPick( UISceneNode* sceneNode, UITreeView* widgetTree,
								 bool wasHighlightOver, UITableView* tableView );
};

}}} // namespace EE::UI::Tools

#endif // EE_UI_TOOLS_
