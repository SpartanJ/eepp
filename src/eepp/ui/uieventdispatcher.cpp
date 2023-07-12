#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/inputevent.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace UI {

UIEventDispatcher* UIEventDispatcher::New( SceneNode* sceneNode ) {
	return eeNew( UIEventDispatcher, ( sceneNode ) );
}

UIEventDispatcher::UIEventDispatcher( SceneNode* sceneNode ) :
	EventDispatcher( sceneNode ), mJustGainedFocus( false ) {}

const bool& UIEventDispatcher::justGainedFocus() const {
	return mJustGainedFocus;
}

void UIEventDispatcher::inputCallback( InputEvent* event ) {
	EventDispatcher::inputCallback( event );

	switch ( event->Type ) {
		case InputEvent::Window:
			if ( event->window.type == InputEvent::WindowKeyboardFocusGain ) {
				mJustGainedFocus = true;
			}
			break;
		case InputEvent::KeyDown:
			checkTabPress( event->key.keysym.sym, event->key.keysym.mod );
			break;
		case InputEvent::EventsSent:
			mJustGainedFocus = false;
			break;
	}
}

void UIEventDispatcher::checkTabPress( const Uint32& KeyCode, const Uint32& mod ) {
	eeASSERT( NULL != mFocusNode );
	if ( KeyCode == KEY_TAB ) {
		Window::Window* win = mFocusNode->getSceneNode()->getWindow();
		if ( mFocusNode->isWidget() && NULL != win && !mJustGainedFocus ) {
			if ( mod & KEYMOD_SHIFT ) {
				mFocusNode->asType<UIWidget>()->onFocusPrevWidget();
			} else {
				mFocusNode->asType<UIWidget>()->onFocusNextWidget();
			}
		}
	}
}

}} // namespace EE::UI
