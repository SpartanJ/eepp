#include "utest.h"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/scene/nodemessage.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uihtmlform.hpp>
#include <eepp/ui/uihtmlinput.hpp>
#include <eepp/ui/uihtmltextarea.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Window;
using namespace EE::Scene;
using namespace EE::UI;

static UISceneNode* initFormTest( const std::string& title ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, title, WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );
	return sceneNode;
}

UTEST( UIHTMLForm, submitGET ) {
	auto* sceneNode = initFormTest( "Form Submit GET Test" );

	std::string interceptedUri;
	std::string interceptedBody;
	std::string interceptedMethod;
	sceneNode->setNavigationInterceptorCb( [&]( const NavigationRequest& req ) -> bool {
		interceptedUri = req.uri.toString();
		interceptedBody = req.body;
		interceptedMethod = req.method;
		return true;
	} );

	auto* form = UIHTMLForm::New();
	form->setParent( sceneNode->getRoot() );
	form->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
	form->setAction( "https://example.com/search" );
	form->setMethod( "GET" );

	auto* input = UIHTMLInput::New();
	input->setParent( form );
	input->setInputType( "text" );
	input->setStyleSheetInlineProperty( "name", "q" );
	static_cast<UITextInput*>( input->getChildWidget() )->setText( "hello world" );

	auto* submit = UIHTMLInput::New();
	submit->setParent( form );
	submit->setInputType( "submit" );

	SceneManager::instance()->update();
	form->submit();

	EXPECT_TRUE( interceptedUri.find( "example.com/search" ) != std::string::npos );
	EXPECT_TRUE( interceptedUri.find( "q=" ) != std::string::npos );
	EXPECT_TRUE( interceptedUri.find( URI::encode( "hello world" ) ) != std::string::npos );
	EXPECT_TRUE( interceptedMethod == "GET" );
	EXPECT_TRUE( interceptedBody.empty() );

	Engine::destroySingleton();
}

UTEST( UIHTMLForm, submitPOST ) {
	auto* sceneNode = initFormTest( "Form Submit POST Test" );

	std::string interceptedUri;
	std::string interceptedBody;
	std::string interceptedMethod;
	sceneNode->setNavigationInterceptorCb( [&]( const NavigationRequest& req ) -> bool {
		interceptedUri = req.uri.toString();
		interceptedBody = req.body;
		interceptedMethod = req.method;
		return true;
	} );

	auto* form = UIHTMLForm::New();
	form->setParent( sceneNode->getRoot() );
	form->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
	form->setAction( "https://example.com/login" );
	form->setMethod( "POST" );

	auto* userInput = UIHTMLInput::New();
	userInput->setParent( form );
	userInput->setInputType( "text" );
	userInput->setStyleSheetInlineProperty( "name", "username" );
	static_cast<UITextInput*>( userInput->getChildWidget() )->setText( "admin" );

	auto* passInput = UIHTMLInput::New();
	passInput->setParent( form );
	passInput->setInputType( "password" );
	passInput->setStyleSheetInlineProperty( "name", "password" );
	static_cast<UITextInput*>( passInput->getChildWidget() )->setText( "secret" );

	auto* submit = UIHTMLInput::New();
	submit->setParent( form );
	submit->setInputType( "submit" );

	SceneManager::instance()->update();
	form->submit();

	EXPECT_TRUE( interceptedUri.find( "example.com/login" ) != std::string::npos );
	EXPECT_TRUE( interceptedMethod == "POST" );
	EXPECT_TRUE( interceptedBody.find( "username=" ) != std::string::npos );
	EXPECT_TRUE( interceptedBody.find( URI::encode( "admin" ) ) != std::string::npos );
	EXPECT_TRUE( interceptedBody.find( "password=" ) != std::string::npos );
	EXPECT_TRUE( interceptedBody.find( URI::encode( "secret" ) ) != std::string::npos );

	Engine::destroySingleton();
}

UTEST( UIHTMLForm, getValueCheckbox ) {
	auto* sceneNode = initFormTest( "Form Checkbox Value" );

	auto* checkbox = UIHTMLInput::New();
	checkbox->setParent( sceneNode->getRoot() );
	checkbox->setInputType( "checkbox" );
	checkbox->setStyleSheetInlineProperty( "name", "agree" );

	EXPECT_TRUE( checkbox->getFormValue().empty() );
	static_cast<UICheckBox*>( checkbox->getChildWidget() )->setChecked( true );
	EXPECT_TRUE( checkbox->getFormValue() == "on" );

	Engine::destroySingleton();
}

UTEST( UIHTMLForm, getValueTextArea ) {
	auto* sceneNode = initFormTest( "Form TextArea Value" );

	auto* textarea = UIHTMLTextArea::New();
	textarea->setParent( sceneNode->getRoot() );
	textarea->setStyleSheetInlineProperty( "name", "bio" );
	textarea->setText( "Hello World" );

	EXPECT_TRUE( textarea->getText() == "Hello World" );

	Engine::destroySingleton();
}

UTEST( UIHTMLForm, submitOnButtonMessage ) {
	auto* sceneNode = initFormTest( "Form Button Message" );

	bool intercepted = false;
	sceneNode->setNavigationInterceptorCb( [&]( const NavigationRequest& ) -> bool {
		intercepted = true;
		return true;
	} );

	auto* form = UIHTMLForm::New();
	form->setParent( sceneNode->getRoot() );
	form->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
	form->setAction( "https://example.com/submit" );

	auto* submit = UIHTMLInput::New();
	submit->setParent( form );
	submit->setInputType( "submit" );

	SceneManager::instance()->update();

	NodeMessage msg( submit, NodeMessage::MouseClick, 0 );
	msg.getSender()->messagePost( &msg );

	EXPECT_TRUE( intercepted );

	Engine::destroySingleton();
}

UTEST( UIHTMLForm, submitMultipartEnctype ) {
	auto* sceneNode = initFormTest( "Form Multipart Enctype" );

	std::string interceptedBody;
	std::string interceptedContentType;
	sceneNode->setNavigationInterceptorCb( [&]( const NavigationRequest& req ) -> bool {
		interceptedBody = req.body;
		auto it = req.extraHeaders.find( "Content-Type" );
		if ( it != req.extraHeaders.end() )
			interceptedContentType = it->second;
		return true;
	} );

	auto* form = UIHTMLForm::New();
	form->setParent( sceneNode->getRoot() );
	form->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
	form->setAction( "https://example.com/upload" );
	form->setMethod( "POST" );
	form->setEnctype( "multipart/form-data" );

	auto* nameInput = UIHTMLInput::New();
	nameInput->setParent( form );
	nameInput->setInputType( "text" );
	nameInput->setStyleSheetInlineProperty( "name", "username" );
	static_cast<UITextInput*>( nameInput->getChildWidget() )->setText( "admin" );

	SceneManager::instance()->update();
	form->submit();

	EXPECT_TRUE( interceptedBody.find( "name=\"username\"" ) != std::string::npos );
	EXPECT_TRUE( interceptedBody.find( "admin" ) != std::string::npos );
	EXPECT_TRUE( interceptedContentType.find( "multipart/form-data" ) != std::string::npos );
	EXPECT_TRUE( interceptedContentType.find( "boundary=" ) != std::string::npos );

	Engine::destroySingleton();
}

UTEST( UIHTMLForm, submitTextPlainEnctype ) {
	auto* sceneNode = initFormTest( "Form Text Plain Enctype" );

	std::string interceptedBody;
	std::string interceptedContentType;
	sceneNode->setNavigationInterceptorCb( [&]( const NavigationRequest& req ) -> bool {
		interceptedBody = req.body;
		auto it = req.extraHeaders.find( "Content-Type" );
		if ( it != req.extraHeaders.end() )
			interceptedContentType = it->second;
		return true;
	} );

	auto* form = UIHTMLForm::New();
	form->setParent( sceneNode->getRoot() );
	form->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
	form->setAction( "https://example.com/data" );
	form->setMethod( "POST" );
	form->setEnctype( "text/plain" );

	auto* nameInput = UIHTMLInput::New();
	nameInput->setParent( form );
	nameInput->setInputType( "text" );
	nameInput->setStyleSheetInlineProperty( "name", "msg" );
	static_cast<UITextInput*>( nameInput->getChildWidget() )->setText( "hello" );

	SceneManager::instance()->update();
	form->submit();

	EXPECT_TRUE( interceptedBody.find( "msg=hello" ) != std::string::npos );
	EXPECT_TRUE( interceptedContentType == "text/plain" );

	Engine::destroySingleton();
}
