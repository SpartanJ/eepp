#pragma once

namespace EE::UI {
class UIWindow;
}

namespace eproc {

class App;

struct SettingsPanel {
	static EE::UI::UIWindow* create( App& app );
};

} // namespace eproc
