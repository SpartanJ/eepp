#ifndef EE_UI_PLATFORMMENUBAR_HPP
#define EE_UI_PLATFORMMENUBAR_HPP

#include <eepp/config.hpp>
#include <memory>

namespace EE { namespace UI {

class UIMenuBar;

class PlatformMenuBar {
  public:
	virtual ~PlatformMenuBar() = default;

	virtual void install( UIMenuBar* menuBar ) = 0;

	virtual void uninstall() = 0;

	virtual void syncTopLevel() = 0;

	static bool isSupported();

	static std::unique_ptr<PlatformMenuBar> create();
};

}} // namespace EE::UI

#endif
