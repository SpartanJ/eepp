#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIDropDownList::UIDropDownList( UIDropDownList::CreateParams& Params ) :
	UITextInput( Params ),
	mListBox( Params.ListBox ),
	mMinNumVisibleItems( Params.MinNumVisibleItems ),
	mPopUpToMainControl( Params.PopUpToMainControl )
{
	allowEditing( false );

	applyDefaultTheme();

	if ( NULL == mListBox ) {
		Uint32 flags = UI_CLIP_ENABLE | UI_AUTO_PADDING;

		if ( Params.Flags & UI_TOUCH_DRAG_ENABLED )
			flags |= UI_TOUCH_DRAG_ENABLED;

		if ( Params.Flags & UI_TEXT_SELECTION_ENABLED )
			flags |= UI_TEXT_SELECTION_ENABLED;

		UITheme * Theme = UIThemeManager::instance()->defaultTheme();

		if ( NULL != Theme ) {
			mListBox = Theme->createListBox( NULL, Sizei( mSize.width(), mMinNumVisibleItems * mSize.height() ),Vector2i(), flags );
		} else {
			UIListBox::CreateParams LBParams;
			LBParams.Size 				= Sizei( mSize.width(), mMinNumVisibleItems * mSize.height() );
			LBParams.Flags 				= flags;
			LBParams.FontSelectedColor	= ColorA( 255, 255, 255, 255 );
			mListBox = eeNew( UIListBox, ( LBParams ) );
		}
	}

	mListBox->enabled( false );
	mListBox->visible( false );

	mListBox->addEventListener( UIEvent::EventOnComplexControlFocusLoss, cb::Make1( this, &UIDropDownList::onListBoxFocusLoss ) );
	mListBox->addEventListener( UIEvent::EventOnItemSelected	, cb::Make1( this, &UIDropDownList::onItemSelected ) );
	mListBox->addEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &UIDropDownList::onItemClicked ) );
	mListBox->addEventListener( UIEvent::EventOnItemKeyDown, cb::Make1( this, &UIDropDownList::onItemKeyDown ) );
	mListBox->addEventListener( UIEvent::EventKeyDown		, cb::Make1( this, &UIDropDownList::onItemKeyDown ) );
	mListBox->addEventListener( UIEvent::EventOnControlClear, cb::Make1( this, &UIDropDownList::onControlClear ) );
}

UIDropDownList::~UIDropDownList() {
	destroyListBox();
}

Uint32 UIDropDownList::getType() const {
	return UI_TYPE_DROPDOWNLIST;
}

bool UIDropDownList::isType( const Uint32& type ) const {
	return UIDropDownList::getType() == type ? true : UITextInput::isType( type );
}

void UIDropDownList::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "dropdownlist" );

	autoSizeControl();

	autoPadding();

	onSizeChange();
}

void UIDropDownList::onSizeChange() {
	UIComplexControl::onSizeChange();
}

void UIDropDownList::autoSizeControl() {
	if ( mFlags & UI_AUTO_SIZE ) {
		size( mSize.x, getSkinSize().height() );
	}
}

void UIDropDownList::autoSize() {
}

UIListBox * UIDropDownList::getListBox() const {
	return mListBox;
}

Uint32 UIDropDownList::onMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( Flags & EE_BUTTON_LMASK )
		showListBox();

	return 1;
}

void UIDropDownList::showListBox() {
	if ( !mListBox->visible() ) {
		if ( !mPopUpToMainControl )
			mListBox->parent( parent() );
		else
			mListBox->parent( UIManager::instance()->mainControl() );

		mListBox->toFront();

		Vector2i Pos( mPos.x, mPos.y + mSize.height() );

		if ( mPopUpToMainControl ) {
			parent()->controlToWorld( Pos );
		}

		mListBox->position( Pos );

		if ( mListBox->count() ) {
			Recti tPadding = mListBox->paddingContainer();

			Float sliderValue = mListBox->verticalScrollBar()->value();

			if ( mMinNumVisibleItems < mListBox->count() )
				mListBox->size( mSize.width(), (Int32)( mMinNumVisibleItems * mListBox->rowHeight() ) + tPadding.Top + tPadding.Bottom );
			else {
				mListBox->size( mSize.width(), (Int32)( mListBox->count() * mListBox->rowHeight() ) + tPadding.Top + tPadding.Bottom );
			}

			mListBox->updateQuad();

			Rectf aabb( mListBox->getPolygon().toAABB() );
			Recti aabbi( aabb.Left, aabb.Top, aabb.Right, aabb.Bottom );

			if ( !UIManager::instance()->mainControl()->getScreenRect().contains( aabbi ) ) {
				Pos = Vector2i( mPos.x, mPos.y );

				if ( mPopUpToMainControl ) {
					parent()->controlToWorld( Pos );
				}

				Pos.y -= mListBox->size().height();

				mListBox->position( Pos );
			}

			mListBox->verticalScrollBar()->value( sliderValue );

			show();

			mListBox->setFocus();
		}
	} else {
		hide();
	}
}

void UIDropDownList::onControlClear( const UIEvent * Event ) {
	text( "" );
}

void UIDropDownList::onItemKeyDown( const UIEvent * Event ) {
	const UIEventKey * KEvent = reinterpret_cast<const UIEventKey*> ( Event );

	if ( KEvent->getKeyCode() == KEY_RETURN )
		onItemClicked( Event );
}

void UIDropDownList::onListBoxFocusLoss( const UIEvent * Event ) {
	if ( UIManager::instance()->focusControl() != this && !isChild( UIManager::instance()->focusControl() ) ) {
		hide();
	}
}

void UIDropDownList::onItemClicked( const UIEvent * Event ) {
	hide();
	setFocus();
}

void UIDropDownList::onItemSelected( const UIEvent * Event ) {
	text( mListBox->getItemSelectedText() );

	UIMessage Msg( this, UIMessage::MsgSelected, mListBox->getItemSelectedIndex() );
	messagePost( &Msg );

	sendCommonEvent( UIEvent::EventOnItemSelected );
}

void UIDropDownList::show() {
	mListBox->enabled( true );
	mListBox->visible( true );

	if ( UIThemeManager::instance()->defaultEffectsEnabled() ) {
		mListBox->startAlphaAnim( 255.f == mListBox->alpha() ? 0.f : mListBox->alpha(), 255.f, UIThemeManager::instance()->controlsFadeInTime() );
	}
}

void UIDropDownList::hide() {
	if ( UIThemeManager::instance()->defaultEffectsEnabled() ) {
		mListBox->disableFadeOut( UIThemeManager::instance()->controlsFadeOutTime() );
	} else {
		mListBox->enabled( false );
		mListBox->visible( false );
	}
}

void UIDropDownList::update() {
	if ( mEnabled && mVisible ) {
		if ( isMouseOver() ) {
			Uint32 Flags 			= UIManager::instance()->getInput()->clickTrigger();

			if ( Flags & EE_BUTTONS_WUWD ) {
				if ( Flags & EE_BUTTON_WUMASK ) {
					mListBox->selectPrev();
				} else if ( Flags & EE_BUTTON_WDMASK ) {
					mListBox->selectNext();
				}
			}
		}
	}

	UITextInput::update();
}

Uint32 UIDropDownList::onKeyDown( const UIEventKey &Event ) {
	mListBox->onKeyDown( Event );

	return UITextInput::onKeyDown( Event );
}

void UIDropDownList::destroyListBox() {
	if ( !UIManager::instance()->isShootingDown() ) {
		mListBox->close();
	}
}

}}
