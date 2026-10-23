#ifndef EE_UI_UITABLECELL_HPP
#define EE_UI_UITABLECELL_HPP

#include <eepp/ui/layoutinvalidation.hpp>
#include <eepp/ui/models/modelindex.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/window.hpp>

using namespace EE::UI::Models;

namespace EE { namespace UI {

class EE_API UITableCell : public UIPushButton {
  public:
	static UITableCell* New() { return eeNew( UITableCell, () ); }

	static UITableCell* New( const std::string& tag ) { return eeNew( UITableCell, ( tag ) ); }

	static UITableCell*
	NewWithOpt( const std::string& tag,
				const std::function<UITextView*( UIPushButton* )>& newTextViewCb ) {
		return eeNew( UITableCell, ( tag, newTextViewCb ) );
	}

	Uint32 getType() const { return UI_TYPE_TABLECELL; }

	bool isType( const Uint32& type ) const {
		return UITableCell::getType() == type ? true : UIPushButton::isType( type );
	}

	ModelIndex getCurIndex() const { return mCurIndex; }

	void setCurIndex( const ModelIndex& curIndex ) {
		if ( curIndex != mCurIndex ) {
			mCurIndex = curIndex;
			onModelIndexChange();
		}
	}

	void setTheme( UITheme* Theme ) {
		UIPushButton::setTheme( Theme );
		setThemeSkin( Theme, "tablecell" );
		onThemeLoaded();
	}

	virtual void updateCell( Model* ){};

  protected:
	ModelIndex mCurIndex;

	UITableCell() : UITableCell( "table::cell" ) {}

	UITableCell( const std::string& tag,
				 const std::function<UITextView*( UIPushButton* )>& newTextViewCb = nullptr ) :
		UIPushButton( tag, newTextViewCb ) {
		// Recycled cells are navigated through their model view; embedded editors opt in
		// separately.
		unsetTabFocusable();
		applyDefaultTheme();
	}

	virtual void onModelIndexChange() {}

	Uint32 onMessage( const NodeMessage* msg ) {
		// The view assigns the cell's geometry. Only its measured text contributes a new
		// intrinsic height; unrelated embedded widgets must not invalidate the row metrics.
		if ( msg->getMsg() == NodeMessage::LayoutAttributeChange && msg->getSender() == mTextBox &&
			 ( layoutInvalidationFromMessage( msg ) &
			   toLayoutInvalidationFlags( LayoutInvalidationReason::IntrinsicSize ) ) ) {
			// Re-evaluate font-relative icon bounds, including unchanged model drawables.
			updateIconBounds();
			notifyRowHeightChange();
		}
		return UIPushButton::onMessage( msg );
	}

	void onSizeChange() {
		// Percentage bounds follow the assigned row height after its deferred measurement.
		updateIconBounds();
		UIPushButton::onSizeChange();
	}

	void updateIconBounds() {
		if ( mIcon )
			mIcon->setPixelsSize( mIcon->getPixelsSize() );
	}

	void onPaddingChange() {
		UIPushButton::onPaddingChange();
		notifyRowHeightChange();
	}

	void notifyRowHeightChange() {
		if ( getParent() ) {
			NodeMessage msg( this, NodeMessage::TableCellMetricsChange );
			getParent()->messagePost( &msg );
		}
	}

	/** Enter belongs to the model view, which opens the row or edits the cell. Clicking the cell
	 * as well would open the row twice with single-click navigation. */
	Uint32 onKeyDown( const KeyEvent& event ) {
		if ( event.getKeyCode() == KEY_RETURN || event.getKeyCode() == KEY_KP_ENTER )
			return UIWidget::onKeyDown( event );
		return UIPushButton::onKeyDown( event );
	}

	Uint32 onFocus( NodeFocusReason reason ) {
		getUISceneNode()->getWindow()->startTextInput();
		return UIPushButton::onFocus( reason );
	}

	Uint32 onFocusLoss() {
		getUISceneNode()->getWindow()->stopTextInput();
		return UIPushButton::onFocusLoss();
	}
};

}} // namespace EE::UI

#endif // EE_UI_UITABLECELL_HPP
