#include <eepp/scene/actions/actions.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uidropdownmodellist.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uithememanager.hpp>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIDropDownModelList* UIDropDownModelList::NewWithTag( const std::string& tag ) {
	return eeNew( UIDropDownModelList, ( tag ) );
}

UIDropDownModelList* UIDropDownModelList::New() {
	return eeNew( UIDropDownModelList, () );
}

UIDropDownModelList::UIDropDownModelList( const std::string& tag ) :
	UIDropDown( tag ), mListView( NULL ) {
	mListView = static_cast<UIAbstractTableView*>( createDefaultListView() );
	mListView->setEnabled( false );
	mListView->setVisible( false );
	mListView->setParent( this );
	mListView->setSingleClickNavigation( true );

	mListView->on( Event::OnWidgetFocusLoss, [this]( auto event ) { onPopUpFocusLoss(); } );
	mListView->on( Event::OnModelEvent, [this]( auto event ) { onItemSelected( event ); } );
	mListView->on( Event::KeyDown, [this]( auto event ) { onItemKeyDown( event ); } );
	mListView->on( Event::OnClear, [this]( auto event ) { onWidgetClear( event ); } );
	mListViewCloseCb =
		mListView->on( Event::OnClose, [this]( const Event* ) { mListView = nullptr; } );

	mListView->setOnSelectionChange( [this]() {
		updateSelectionIndex();
		if ( !mListView->getSelection().isEmpty() ) {
			sendCommonEvent( Event::OnSelectionChanged );
		}
	} );

	applyDefaultTheme();
}

UIDropDownModelList::~UIDropDownModelList() {
	if ( mListView != nullptr && mListViewCloseCb )
		mListView->removeEventListener( mListViewCloseCb );
	destroyListView();
}

UIWidget* UIDropDownModelList::createDefaultListView() {
	return UIListView::NewWithTag( mTag + "::listview" );
}

Uint32 UIDropDownModelList::getType() const {
	return UI_TYPE_DROPDOWNMODELLIST;
}

bool UIDropDownModelList::isType( const Uint32& type ) const {
	return UIDropDownModelList::getType() == type ? true : UIDropDown::isType( type );
}

UIWidget* UIDropDownModelList::getPopUpWidget() const {
	return mListView;
}

UIAbstractTableView* UIDropDownModelList::getListView() const {
	return mListView;
}

void UIDropDownModelList::setListView( UIAbstractTableView* listView ) {
	if ( listView == mListView )
		return;

	if ( mListView != nullptr ) {
		if ( mListViewCloseCb )
			mListView->removeEventListener( mListViewCloseCb );
		mListView->close();
	}

	mListView = listView;
	mListView->setEnabled( false );
	mListView->setVisible( false );
	mListView->setParent( this );

	mListView->on( Event::OnWidgetFocusLoss, [this]( auto event ) { onPopUpFocusLoss(); } );
	mListView->on( Event::OnModelEvent, [this]( auto event ) { onItemSelected( event ); } );
	mListView->on( Event::KeyDown, [this]( auto event ) { onItemKeyDown( event ); } );
	mListView->on( Event::OnClear, [this]( auto event ) { onWidgetClear( event ); } );
	mListViewCloseCb =
		mListView->on( Event::OnClose, [this]( const Event* ) { mListView = nullptr; } );

	mListView->setOnSelectionChange( [this]() {
		updateSelectionIndex();
		if ( !mListView->getSelection().isEmpty() ) {
			sendCommonEvent( Event::OnSelectionChanged );
		}
	} );

	if ( mModel ) {
		mListView->setModel( mModel );
	}
	updateSelectionIndex();
}

std::shared_ptr<Model> UIDropDownModelList::getModel() const {
	return mModel;
}

void UIDropDownModelList::setModel( std::shared_ptr<Model> model ) {
	if ( mModel == model )
		return;
	updateSelectedIcon( {} );
	mModel = std::move( model );
	if ( mListView ) {
		mListView->setModel( mModel );
	}
}

UIImage* UIDropDownModelList::getIcon() const {
	return mIcon;
}

Uint32 UIDropDownModelList::onMouseUp( const Vector2i& Pos, const Uint32& Flags ) {
	if ( mEnabled && mVisible && isMouseOver() && NULL != mListView && mModel ) {
		if ( Flags & EE_BUTTONS_WUWD ) {
			if ( Flags & EE_BUTTON_WUMASK ) {
				mListView->moveSelection( -1 );
				updateSelectionIndex();
			} else if ( Flags & EE_BUTTON_WDMASK ) {
				if ( !mListView->getSelection().isEmpty() ) {
					mListView->moveSelection( 1 );
					updateSelectionIndex();
				} else if ( mModel->hasChildren() ) {
					mListView->getSelection().set( mModel->index( 0, 0 ) );
					updateSelectionIndex();
				}
			}
		}
	}

	return UIDropDown::onMouseUp( Pos, Flags );
}

Uint32 UIDropDownModelList::onKeyDown( const KeyEvent& Event ) {
	if ( Event.getKeyCode() == KEY_F4 ||
		 ( Event.getKeyCode() == KEY_DOWN && ( Event.getSanitizedMod() & KEYMOD_ALT ) ) )
		return UIDropDown::onKeyDown( Event );
	if ( NULL != mListView )
		mListView->onKeyDown( Event );

	return UIDropDown::onKeyDown( Event );
}

UIDropDownModelList* UIDropDownModelList::showList() {
	if ( NULL == mListView || NULL == mModel )
		return this;

	if ( !mListView->isVisible() || !mListView->isEnabled() ) {
		if ( !mModel->hasChildren() )
			return this;

		const Uint32 itemCount = static_cast<Uint32>( mModel->rowCount() );
		mListView->setVerticalScrollMode( itemCount > mStyleConfig.MaxNumVisibleItems
											  ? ScrollBarMode::AlwaysOn
											  : ScrollBarMode::AlwaysOff );

		Float sliderValue = 0;
		if ( mListView->getVerticalScrollBar() )
			sliderValue = mListView->getVerticalScrollBar()->getValue();

		Float contentsWidth = 0;
		if ( mStyleConfig.menuWidthRule != MenuWidthMode::DropDown ) {
			contentsWidth = eeceil( PixelDensity::pxToDp(
				mListView->getMaxColumnContentWidth( 0, true ) +
				PixelDensity::dpToPx( mListView->getPadding().getWidth() ) +
				( mListView->getVerticalScrollBar()
					  ? mListView->getVerticalScrollBar()->getPixelsSize().getWidth()
					  : 0.f ) ) );
		}

		Float width = getPopUpWidth( contentsWidth );

		const Rectf padding = mListView->getPixelsPadding();
		const Float height = std::ceil(
			eemin( itemCount, mStyleConfig.MaxNumVisibleItems ) * mListView->getRowHeight() +
			padding.Top + padding.Bottom + mListView->getHeaderHeight() );
		// Resolve columns and scrollbars at the new width before reserving scrollbar height.
		// The previous viewport can need a horizontal scrollbar even when this popup does not.
		mListView->setSize( width, PixelDensity::pxToDp( height ) );
		if ( auto* scrollBar = mListView->getHorizontalScrollBar();
			 scrollBar && scrollBar->isVisible() ) {
			mListView->setSize(
				width, PixelDensity::pxToDp( height + scrollBar->getPixelsSize().getHeight() ) );
		}

		if ( mListView->getVerticalScrollBar() )
			mListView->getVerticalScrollBar()->setValue( sliderValue );

		alignPopUp( mListView );
	} else {
		hide();
	}
	return this;
}

UIDropDownModelList*
UIDropDownModelList::setMaxNumVisibleItems( const Uint32& maxNumVisibleItems ) {
	if ( maxNumVisibleItems != mStyleConfig.MaxNumVisibleItems ) {
		mStyleConfig.MaxNumVisibleItems = maxNumVisibleItems;

		if ( NULL != mListView && mModel )
			mListView->setSize( getSize().getWidth(), std::min( mStyleConfig.MaxNumVisibleItems,
																(Uint32)mModel->rowCount() ) *
														  mListView->getRowHeight() );
	}
	return this;
}

void UIDropDownModelList::onItemClicked( const Event* ) {
	updateSelectionIndex();
	hide();
	setFocus();
}

void UIDropDownModelList::updateSelectionIndex() {
	if ( !mListView || !mModel || mListView->getSelection().isEmpty() ) {
		updateSelectedIcon( {} );
		return;
	}
	ModelIndex idx = mListView->getSelection().first();
	if ( idx.isValid() ) {
		updateSelectedIcon( idx );
		Variant var = mModel->data( idx, ModelRole::Display );
		if ( var.isValid() && var.isString() )
			setText( var.toString() );
		sendCommonEvent( Event::OnItemSelected );
		sendCommonEvent( Event::OnValueChange );
	}
}

void UIDropDownModelList::updateSelectedIcon( const ModelIndex& index ) {
	DrawablePtr drawable;
	if ( mModel && index.isValid() ) {
		Variant icon = mModel->data( index, ModelRole::Icon );
		if ( icon.is( Variant::Type::Icon ) && icon.asIcon() )
			drawable = icon.asIcon()->createDrawable( mListView->getIconSize() );
		else if ( icon.is( Variant::Type::Drawable ) )
			drawable = icon.asDrawable();
	}
	if ( drawable && !mIcon ) {
		mIcon = UIImage::NewWithTag( mTag + "::icon" );
		mIcon->setScaleType( UIScaleType::FitInside );
		mIcon->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		mIcon->setEnabled( false );
		mIcon->setParent( this );
		mIcon->setAlpha( mAlpha );
		mIcon->on( Event::OnSizeChange, [this]( const Event* ) {
			onAutoSize();
			alignFix();
		} );
	}
	if ( mIcon ) {
		if ( drawable )
			mIcon->setPixelsSize( drawable->getPixelsSize() );
		mIcon->setDrawable( std::move( drawable ) );
		mIcon->setVisible( mIcon->getDrawable() != nullptr );
		onAutoSize();
		alignFix();
	}
}

void UIDropDownModelList::alignFix() {
	if ( !mIcon || !mIcon->isVisible() ) {
		if ( mIcon )
			updateTextOverflow();
		UIDropDown::alignFix();
		return;
	}
	const bool rtl = isRTL();
	const Float space = mIcon->getPixelsSize().getWidth() + PixelDensity::dpToPx( 4 );
	mIcon->setPixelsPosition( rtl ? mSize.x - mPaddingPx.Right - mIcon->getPixelsSize().getWidth()
								  : mPaddingPx.Left,
							  ( mSize.y - mIcon->getPixelsSize().getHeight() ) * 0.5f );
	// Measure overflow and alignment in the space after the icon. Keep authored padding intact;
	// the icon itself remains inside the content box used to clip this widget and its children.
	const Rectf padding = mPaddingPx;
	if ( rtl )
		mPaddingPx.Right += space;
	else
		mPaddingPx.Left += space;
	updateTextOverflow();
	UIDropDown::alignFix();
	// The input aligns using the full label width. With ellipsis, align the visible label instead.
	if ( hasTextOverflow() ) {
		const Float width = mSize.x - mPaddingPx.Left - mPaddingPx.Right;
		const Float textWidth = mTextCache.getTextWidth();
		if ( Font::getHorizontalAlign( getFlags() ) == UI_HALIGN_RIGHT )
			mRealAlignOffset.x = width - textWidth;
		else if ( Font::getHorizontalAlign( getFlags() ) == UI_HALIGN_CENTER )
			mRealAlignOffset.x = std::trunc( ( width - textWidth ) * 0.5f );
	}
	mPaddingPx = padding;
	if ( !rtl )
		mRealAlignOffset.x += space;
}

void UIDropDownModelList::onAutoSize() {
	if ( mFlags & UI_AUTO_SIZING )
		return;
	UIDropDown::onAutoSize();
	mFlags |= UI_AUTO_SIZING;
	if ( mIcon && mIcon->isVisible() && mHeightPolicy == SizePolicy::WrapContent ) {
		setInternalPixelsHeight( eemax( mSize.y, mIcon->getPixelsSize().getHeight() +
													 mPaddingPx.Top + mPaddingPx.Bottom ) );
	}
	mFlags &= ~UI_AUTO_SIZING;
}

void UIDropDownModelList::onAlphaChange() {
	UIDropDown::onAlphaChange();
	if ( mIcon )
		mIcon->setAlpha( mAlpha );
}

void UIDropDownModelList::onWidgetClear( const Event* event ) {
	updateSelectedIcon( {} );
	UIDropDown::onWidgetClear( event );
}

void UIDropDownModelList::onItemSelected( const Event* event ) {
	if ( event->getType() == Event::OnModelEvent ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		if ( modelEvent->getModelEventType() == ModelEventType::Open ) {
			updateSelectionIndex();
			hide();
			setFocus();
		}
	}
}

void UIDropDownModelList::destroyListView() {
	if ( !SceneManager::instance()->isShuttingDown() && NULL != mListView &&
		 mListView->getParent() != this ) {
		mListView->setParent( this );
	}
}

std::string UIDropDownModelList::getPropertyString( const PropertyDefinition* propertyDef,
													const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	std::string res = UIDropDown::getPropertyString( propertyDef, propertyIndex );
	if ( res.empty() && NULL != mListView ) {
		res = mListView->getPropertyString( propertyDef, propertyIndex );
	}
	return res;
}

std::vector<PropertyId> UIDropDownModelList::getPropertiesImplemented() const {
	auto props = UIDropDown::getPropertiesImplemented();
	if ( mListView ) {
		auto listProps = mListView->getPropertiesImplemented();
		props.insert( props.end(), listProps.begin(), listProps.end() );
	}
	return props;
}

void UIDropDownModelList::onClassChange() {
	if ( mListView )
		mListView->setClasses( getClasses() );
}

}} // namespace EE::UI
