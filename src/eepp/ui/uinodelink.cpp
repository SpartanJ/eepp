#include <eepp/ui/uinodelink.hpp>

namespace EE::UI {

UINodeLink* UINodeLink::New() {
	return eeNew( UINodeLink, () );
}

UINodeLink* UINodeLink::NewLink( UIWidget* link ) {
	return eeNew( UINodeLink, ( link ) );
}

UINodeLink::UINodeLink() : UIWidget( "nodelink" ) {}

UINodeLink::UINodeLink( UIWidget* link ) : UIWidget( "nodelink" ), mNodeLink( link ) {}

Uint32 UINodeLink::getType() const {
	return UI_TYPE_NODELINK;
}

bool UINodeLink::isType( const Uint32& type ) const {
	return UINodeLink::getType() == type ? true : UIWidget::isType( type );
}

UINodeLink* UINodeLink::setNodeLink( UIWidget* link ) {
	mNodeLink = link;
	return this;
}

UIWidget* UINodeLink::getNodeLink() const {
	return mNodeLink;
}

} // namespace EE::UI
