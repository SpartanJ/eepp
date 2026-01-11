#include <eepp/ui/uistackwidget.hpp>

namespace EE { namespace UI {

UIStackWidget* UIStackWidget::New() {
	return NewWithTag( "stackwidget" );
}

UIStackWidget* UIStackWidget::NewWithTag( const std::string& tag ) {
	return eeNew( UIStackWidget, ( tag ) );
}

UIStackWidget::UIStackWidget( const std::string& tag ) : UIWidget( tag ) {}

Uint32 UIStackWidget::getType() const {
	return UI_TYPE_STACK_WIDGET;
}

bool UIStackWidget::isType( const Uint32& type ) const {
	return getType() == type || UIWidget::isType( type );
}

void UIStackWidget::setActiveWidget( UIWidget* widget ) {
	if ( widget == mActiveWidget )
		return;

	if ( isChild( widget ) ) {
		bool activeWidgetHadFocus = mActiveWidget && mActiveWidget->hasFocusWithin();

		if ( mActiveWidget ) {
			mActiveWidget->setVisible( false );
			mActiveWidget->setEnabled( false );
		}

		mActiveWidget = widget;

		if ( mActiveWidget ) {
			mActiveWidget->setPixelsSize( mSize );

			if ( activeWidgetHadFocus )
				mActiveWidget->setFocus();

			mActiveWidget->setVisible( true );
			mActiveWidget->setEnabled( true );
		}

		sendCommonEvent( Event::OnActiveWidgetChange );
	}
}

UIWidget* UIStackWidget::getActiveWidget() const {
	return mActiveWidget;
}

void UIStackWidget::onSizeChange() {
	UIWidget::onSizeChange();
	if ( mActiveWidget )
		mActiveWidget->setPixelsSize( mSize );
}

void UIStackWidget::onChildCountChange( Node* child, const bool& removed ) {
	UIWidget::onChildCountChange( child, removed );

	if ( child && child->isWidget() ) {
		UIWidget* widget = child->asType<UIWidget>();

		if ( !removed ) {
			if ( nullptr == mActiveWidget ) {
				setActiveWidget( widget );
			} else {
				widget->setVisible( false );
				widget->setEnabled( false );
			}
		} else {
			if ( widget == mActiveWidget ) {
				Node* newActiveWidget = getFirstWidget();
				if ( newActiveWidget ) {
					setActiveWidget( newActiveWidget->asType<UIWidget>() );
				} else {
					mActiveWidget = nullptr;
					sendCommonEvent( Event::OnActiveWidgetChange );
				}
			}
		}
	}
}

void UIStackWidget::invalidate( Node* invalidator ) {
	// Only invalidate if the invalidator is actually visible
	if ( NULL != invalidator ) {
		if ( invalidator == mActiveWidget ) {
			if ( mActiveWidget->isVisible() )
				mNodeDrawInvalidator->invalidate( mActiveWidget );
		} else if ( invalidator->getParent() == mActiveWidget ) {
			if ( invalidator->isVisible() )
				mNodeDrawInvalidator->invalidate( mActiveWidget );
		} else {
			Node* container = invalidator->getParent();
			while ( container->getParent() != NULL && container->getParent() != mActiveWidget ) {
				container = container->getParent();
			}
			if ( container->getParent() == mActiveWidget && container->isVisible() ) {
				mNodeDrawInvalidator->invalidate( mActiveWidget );
			}
		}
	} else if ( NULL != mNodeDrawInvalidator ) {
		mNodeDrawInvalidator->invalidate( this );
	} else if ( NULL != mSceneNode ) {
		mSceneNode->invalidate( this );
	}
}

}} // namespace EE::UI
