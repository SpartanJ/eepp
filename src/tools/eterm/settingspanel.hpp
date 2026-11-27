#pragma once

#include <eepp/core/string.hpp>

namespace EE::UI {
class UIWindow;
class UISceneNode;
} // namespace EE::UI

namespace EE::UI::Tools {
class UISettingsPanel;
}

using namespace EE;
using namespace EE::UI;

namespace eterm {

class App;
class QuakeWindow;

struct SettingsPanel {
	static UIWindow* create( App& );

	static void refreshQuakeShortcutLabels( Tools::UISettingsPanel& panel,
											const QuakeWindow& dropDown );
};

} // namespace eterm
