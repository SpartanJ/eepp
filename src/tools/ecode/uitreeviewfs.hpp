#pragma once

#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/uitreeview.hpp>

using namespace EE::UI;

namespace ecode {

class UITreeViewFS : public UITreeView {
  public:
	static UITreeViewFS* New() { return eeNew( UITreeViewFS, () ); }

	UITreeViewFS();

	UIWidget* createCell( UIWidget* rowWidget, const ModelIndex& index ) override;

	Uint32 onMessage( const NodeMessage* msg ) override;

	KeyBindings& getKeyBindings() { return mKeyBindings; }

	void setSourceDrag( const std::string& src ) { mSrcDrag = src; }

	void setSourceCopy( const std::string& src ) { mSrcCopy = src; }

	void execute( const std::string& cmd );

  protected:
	KeyBindings mKeyBindings;
	UnorderedMap<std::string, std::function<void()>> mCommands;
	std::string mSrcDrag;
	std::string mSrcCopy;
	bool mWasCut{ false };

	std::string getSelectionPath() const;

	void moveFile( const std::string& src, const std::string& dst );

	void copyFile( const std::string& src, const std::string& dst );
};

} // namespace ecode
