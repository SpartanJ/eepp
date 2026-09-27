#include <eepp/ui/doc/markdownhelper.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
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
	mTextSelectionController.setHost( this );
	mTextSelectionController.setSelectionRoot( this );
	subscribeScheduledUpdate();
	mWidthPolicy = SizePolicy::MatchParent;
	mHeightPolicy = SizePolicy::WrapContent;
	getUISceneNode()->loadHTMLBaseCSS();
}

UIMarkdownView::~UIMarkdownView() {
	mTextSelectionController.onDocumentWillChange();
}

Uint32 UIMarkdownView::getType() const {
	return UI_TYPE_MARKDOWNVIEW;
}

bool UIMarkdownView::isType( const Uint32& type ) const {
	return UIMarkdownView::getType() == type || UILinearLayout::isType( type );
}

UITextSelectionController* UIMarkdownView::getTextSelectionController() {
	return &mTextSelectionController;
}

const UITextSelectionController* UIMarkdownView::getTextSelectionController() const {
	return &mTextSelectionController;
}

Uint32 UIMarkdownView::onKeyDown( const KeyEvent& event ) {
	if ( mTextSelectionController.onKeyDown( event ) )
		return 1;
	return UILinearLayout::onKeyDown( event );
}

Uint32 UIMarkdownView::onMessage( const NodeMessage* message ) {
	if ( message->getMsg() == NodeMessage::MouseUp &&
		 mTextSelectionController.onMouseUpMessage( message ) )
		return 1;
	return UILinearLayout::onMessage( message );
}

void UIMarkdownView::scheduledUpdate( const Time& time ) {
	UILinearLayout::scheduledUpdate( time );
	// Continue a captured drag when the pointer leaves a text owner or the view scrolls.
	mTextSelectionController.updateSelectionDrag();
}

void UIMarkdownView::loadFromString( std::string_view markdown ) {
	mTextSelectionController.onDocumentWillChange();
	closeAllChildren();
	auto xhtml = Tools::HTMLFormatter::HTMLBodyToXML( Markdown::toXHTML( markdown ) );
	getUISceneNode()->loadLayoutFromString( xhtml, this );
	mTextSelectionController.onDocumentChanged();
}

void UIMarkdownView::loadFromXmlNode( const pugi::xml_node& node ) {
	UILinearLayout::loadFromXmlNode( node );
	if ( !node.text().empty() )
		loadFromString( node.text().as_string() );
}

}} // namespace EE::UI
