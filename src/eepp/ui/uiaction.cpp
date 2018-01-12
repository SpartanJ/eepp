#include <eepp/ui/uiaction.hpp>
#include <eepp/ui/uinode.hpp>

namespace EE { namespace UI {

UIAction::UIAction() :
	mNode( NULL ),
	mFlags( 0 ),
	mTag( 0 ),
	mNumCallBacks( 0 )
{
}

UIAction::~UIAction()
{}

Uint32 UIAction::getFlags() const {
	return mFlags;
}

void UIAction::setFlags( const Uint32 & flags ) {
	mFlags = flags;
}

Uint32 UIAction::getTag() const {
	return mTag;
}

void UIAction::setTag( const Uint32 & tag ) {
	mTag = tag;
}

void UIAction::setTarget( UINode * target ) {
	mNode = target;
}

UINode * UIAction::getTarget() const {
	return mNode;
}

UIAction * UIAction::clone() const {
	return NULL;
}

UIAction * UIAction::reverse() const {
	return NULL;
}

Uint32 UIAction::addEventListener( const ActionType& actionType, const ActionCallback& callback ) {
	mNumCallBacks++;

	mCallbacks[ actionType ][ mNumCallBacks ] = callback;

	return mNumCallBacks;
}

void UIAction::removeEventListener( const Uint32& callbackId ) {
	for ( auto it = mCallbacks.begin(); it != mCallbacks.end(); ++it ) {
		std::map<Uint32, ActionCallback> event = it->second;

		if ( event.erase( callbackId ) )
			break;
	}
}

void UIAction::sendEvent( const ActionType& actionType ) {
	if ( 0 != mCallbacks.count( actionType ) ) {
		auto event = mCallbacks[ actionType ];

		if ( !event.empty() ) {
			for ( auto it = event.begin(); it != event.end(); ++it )
				it->second( this, actionType );
		}
	}
}

UINode * UIAction::getNode() const {
	return mNode;
}

void UIAction::onStart() {}

void UIAction::onStop() {}

void UIAction::onUpdate( const Time& time ) {}

}}
