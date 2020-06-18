#include <eepp/scene/node.hpp>
#include <eepp/scene/nodemessage.hpp>

namespace EE { namespace Scene {

NodeMessage::NodeMessage( Node* node, const Uint32& msg, const Uint32& flags ) :
	mNode( node ), mMsg( msg ), mFlags( flags ) {}

NodeMessage::~NodeMessage() {}

Node* NodeMessage::getSender() const {
	return mNode;
}

const Uint32& NodeMessage::getMsg() const {
	return mMsg;
}

const Uint32& NodeMessage::getFlags() const {
	return mFlags;
}

NodeDropMessage::NodeDropMessage( Node* node, const Uint32& msg, Node* droppedNode ) :
	NodeMessage( node, msg ), mDroppedNode( droppedNode ) {}

}} // namespace EE::Scene
