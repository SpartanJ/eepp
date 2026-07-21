#include <eepp/scene/nodemessage.hpp>
#include <eepp/ui/layoutinvalidation.hpp>

namespace EE { namespace UI {

LayoutInvalidationFlags layoutInvalidationFromMessage( const Scene::NodeMessage* msg ) {
	Uint32 flags = msg->getFlags();
	if ( flags == Scene::NodeMessage::NoMessage || flags == 0 )
		return toLayoutInvalidationFlags( LayoutInvalidationReason::SelfGeometry ) |
			   toLayoutInvalidationFlags( LayoutInvalidationReason::NormalFlowChild ) |
			   toLayoutInvalidationFlags( LayoutInvalidationReason::IntrinsicSize ) |
			   toLayoutInvalidationFlags( LayoutInvalidationReason::FormattingContext );
	return static_cast<LayoutInvalidationFlags>( flags );
}

}} // namespace EE::UI
