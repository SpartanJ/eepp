#include <eepp/ui/doc/markdownhelper.hpp>
#include <eepp/ui/uimarkdownview.hpp>
#include <eepp/ui/uiscenenode.hpp>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

using namespace EE::UI::Doc;

namespace EE { namespace UI {

UIMarkdownView* UIMarkdownView::New() {
	return eeNew( UIMarkdownView, () );
}

UIMarkdownView::UIMarkdownView() : UILinearLayout( "markdownview", UIOrientation::Vertical ) {
	mWidthPolicy = SizePolicy::MatchParent;
	mHeightPolicy = SizePolicy::WrapContent;
}

Uint32 UIMarkdownView::getType() const {
	return UI_TYPE_MARKDOWNVIEW;
}

bool UIMarkdownView::isType( const Uint32& type ) const {
	return UIMarkdownView::getType() == type || UILinearLayout::isType( type );
}

void UIMarkdownView::loadFromString( std::string_view markdown ) {
	closeAllChildren();
	auto xhtml = Markdown::toXHTML( markdown );
	printf( "%s", xhtml.c_str() );
	getUISceneNode()->loadLayoutFromString( xhtml, this );
}

void UIMarkdownView::loadFromXmlNode( const pugi::xml_node& node ) {
	UILinearLayout::loadFromXmlNode( node );
	if ( !node.text().empty() )
		loadFromString( node.text().as_string() );
}

}} // namespace EE::UI
