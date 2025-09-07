#pragma once

#include <eepp/ui/uihelper.hpp>

using namespace EE;
using namespace EE::UI;

namespace ecode {

enum class CustomWidgets : Uint32 {
	UI_TYPE_WELCOME_TAB = UI_TYPE_USER + 1,
	UI_TYPE_TREEVIEWCELLFS = UI_TYPE_WELCOME_TAB + 1,
};

}
