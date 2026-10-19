#ifndef EEFM_DIALOGS_HPP
#define EEFM_DIALOGS_HPP

#include <eepp/ui/uiscenenode.hpp>
#include <functional>
#include <optional>
#include <string>

namespace eefm {

using namespace EE;
using namespace EE::UI;

/** Modal message boxes. UI thread only. */
class Dialogs {
  public:
	/** Modal text prompt. done( nullopt ) when cancelled. */
	static void promptText( UISceneNode* ui, const String& title, const String& message,
							const std::string& initial, bool password,
							std::function<void( std::optional<std::string> )> done );

	/** OK / Cancel confirmation. */
	static void confirm( UISceneNode* ui, const String& title, const String& message,
						 std::function<void( bool )> done );

	static void showError( UISceneNode* ui, const String& message );

	static void showMessage( const String& title, const String& message );
};

} // namespace eefm

#endif
