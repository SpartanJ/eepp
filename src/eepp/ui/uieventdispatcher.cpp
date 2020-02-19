#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/inputevent.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace UI {

UIEventDispatcher* UIEventDispatcher::New( SceneNode* sceneNode ) {
	return eeNew( UIEventDispatcher, ( sceneNode ) );
}

UIEventDispatcher::UIEventDispatcher( SceneNode* sceneNode ) :
	EventDispatcher( sceneNode ), mJustGainedFocus( false ) {}

void UIEventDispatcher::inputCallback( InputEvent* Event ) {
	EventDispatcher::inputCallback( Event );

	switch ( Event->Type ) {
		case InputEvent::Window:
			if ( Event->window.type == InputEvent::WindowKeyboardFocusGain ) {
				mJustGainedFocus = true;
			}
			break;
		case InputEvent::KeyDown:
			checkTabPress( Event->key.keysym.sym );
			break;
		case InputEvent::EventsSent:
			mJustGainedFocus = false;
			break;
	}
}

void UIEventDispatcher::checkTabPress( const Uint32& KeyCode ) {
	eeASSERT( NULL != mFocusControl );

	Window::Window* win = mFocusControl->getSceneNode()->getWindow();
	if ( KeyCode == KEY_TAB && mFocusControl->isUINode() && NULL != win && win->isActive() &&
		 !mJustGainedFocus ) {
		Node* Ctrl = static_cast<UINode*>( mFocusControl )->getNextWidget();

		if ( NULL != Ctrl )
			Ctrl->setFocus();
	}
}

}} // namespace EE::UI
