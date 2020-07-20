#ifndef EE_UI_UITABLEROW_HPP
#define EE_UI_UITABLEROW_HPP

#include <eepp/ui/models/modelindex.hpp>
#include <eepp/ui/uiwidget.hpp>

using namespace EE::UI::Models;

namespace EE { namespace UI {

class EE_API UITableRow : public UIWidget {
  public:
	static UITableRow* New( const std::string& tag ) { return eeNew( UITableRow, ( tag ) ); }

	ModelIndex getCurIndex() const { return mCurIndex; }

	void setCurIndex( const ModelIndex& curIndex ) { mCurIndex = curIndex; }

  protected:
	UITableRow( const std::string& tag ) : UIWidget( tag ) {}

	virtual Uint32 onMessage( const NodeMessage* msg ) {
		if ( msg->getMsg() == NodeMessage::MouseDown && ( msg->getFlags() & EE_BUTTON_LMASK ) &&
			 ( !getEventDispatcher()->getMouseDownNode() ||
			   getEventDispatcher()->getMouseDownNode() == this ||
			   isParentOf( getEventDispatcher()->getMouseDownNode() ) ) &&
			 getEventDispatcher()->getNodeDragging() == nullptr ) {
			sendMouseEvent( Event::MouseDown, getEventDispatcher()->getMousePos(),
							msg->getFlags() );
		}
		return 0;
	}

	ModelIndex mCurIndex;
};

}} // namespace EE::UI

#endif // EE_UI_UITABLEROW_HPP
