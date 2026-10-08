#ifndef ECODE_UIRIGHTPANEL_HPP
#define ECODE_UIRIGHTPANEL_HPP

#include <eepp/core/containers.hpp>
#include <eepp/ui/uirelativelayout.hpp>
#include <eepp/ui/uisplitter.hpp>

using namespace EE;
using namespace EE::UI;

namespace ecode {

class AppConfig;

class UIRightPanel {
  public:
	UIRightPanel( UISplitter* splitter, UILayout* container, AppConfig* config );

	UILayout* registerPanel( const std::string& id );

	void unregisterPanel( const std::string& id );

	void setPanelVisible( const std::string& id, bool visible );

	bool isPanelVisible( const std::string& id ) const;

	void saveState();

	UISplitter* getSplitter() const;

	UILayout* getContainer() const;

  protected:
	struct Panel {
		UILayout* layout{ nullptr };
		bool visible{ false };
	};

	UISplitter* mSplitter{ nullptr };
	UILayout* mContainer{ nullptr };
	AppConfig* mConfig{ nullptr };
	UnorderedMap<std::string, Panel> mPanels;
	bool mVisible{ false };

	void updateVisibility();
};

} // namespace ecode

#endif // ECODE_UIRIGHTPANEL_HPP
