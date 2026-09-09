#pragma once

#include <eepp/core/string.hpp>

namespace EE::UI {
class UIWindow;
class UISceneNode;
} // namespace EE::UI

using namespace EE;
using namespace EE::UI;

namespace eterm {

class App;

struct SettingsPanel {
	static UIWindow* create( App& );
};

} // namespace eterm
