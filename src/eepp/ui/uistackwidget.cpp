#include <eepp/ui/uistackwidget.hpp>

namespace EE { namespace UI {

UIStackWidget* UIStackWidget::New() {
	return NewWithTag( "stackwidget" );
}

UIStackWidget* UIStackWidget::NewWithTag( const std::string& tag ) {
	return eeNew( UIStackWidget, ( tag ) );
}

UIStackWidget::UIStackWidget( const std::string& tag ) : UIWidget( tag ) {}

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

}} // namespace EE::UI
