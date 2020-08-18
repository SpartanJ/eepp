#ifndef EE_UI_UITABLEROW_HPP
#define EE_UI_UITABLEROW_HPP

#include <eepp/ui/models/modelindex.hpp>
#include <eepp/ui/uiwidget.hpp>

using namespace EE::UI::Models;

namespace EE { namespace UI {

class EE_API UITableRow : public UIWidget {
  public:
	static UITableRow* New( const std::string& tag ) { return eeNew( UITableRow, ( tag ) ); }

	static UITableRow* New() { return eeNew( UITableRow, () ); }

	ModelIndex getCurIndex() const { return mCurIndex; }

	void setCurIndex( const ModelIndex& curIndex ) { mCurIndex = curIndex; }

	void setTheme( UITheme* Theme ) {
		UIWidget::setTheme( Theme );
		setThemeSkin( Theme, "tablerow" );
		onThemeLoaded();
	}

  protected:
	UITableRow( const std::string& tag ) : UIWidget( tag ) { applyDefaultTheme(); }

	UITableRow() : UIWidget( "table::row" ) {}

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
