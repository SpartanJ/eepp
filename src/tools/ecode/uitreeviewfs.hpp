#pragma once

#include <eepp/ui/uitreeview.hpp>

using namespace EE::UI;

namespace ecode {

class UITreeViewFS : public UITreeView {
  public:
	static UITreeViewFS* New() { return eeNew( UITreeViewFS, () ); }

	UITreeViewFS();

	UIWidget* createCell( UIWidget* rowWidget, const ModelIndex& index ) override;

	Uint32 onMessage( const NodeMessage* msg ) override;

	void setSourceDrag( const std::string& src ) { mSrcDrag = src; }

  protected:
	std::string mSrcDrag;

	void moveFile( const std::string& src, const std::string& dst );
};

} // namespace ecode
