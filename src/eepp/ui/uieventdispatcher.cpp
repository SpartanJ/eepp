#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/window/inputevent.hpp>

namespace EE { namespace UI {

UIEventDispatcher * UIEventDispatcher::New( SceneNode * sceneNode ) {
	return eeNew( UIEventDispatcher, ( sceneNode ) );
}

UIEventDispatcher::UIEventDispatcher( SceneNode * sceneNode ) :
	EventDispatcher( sceneNode )
{}

void UIEventDispatcher::inputCallback(InputEvent * Event) {
	EventDispatcher::inputCallback( Event );

	switch( Event->Type ) {
		case InputEvent::KeyDown:
			checkTabPress( Event->key.keysym.sym );
			break;
	}
}

void UIEventDispatcher::checkTabPress( const Uint32& KeyCode ) {
	eeASSERT( NULL != mFocusControl );

	if ( KeyCode == KEY_TAB && mFocusControl->isUINode() ) {
		Node * Ctrl = static_cast<UINode*>( mFocusControl )->getNextWidget();

		if ( NULL != Ctrl )
			Ctrl->setFocus();
	}
}

}} 
