#include "utest.hpp"

#include <array>
#include <chrono>
#include <eepp/network/tcpsocket.hpp>
#include <eepp/scene/keyevent.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/tools/uiinspector.hpp>
#include <eepp/ui/tools/uiinspectorserver.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextnode.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uiwebview.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/engine.hpp>
#include <nlohmann/json.hpp>
#include <thread>

using namespace EE;
using namespace EE::Network;
using namespace EE::Scene;
using namespace EE::UI;
using namespace EE::Window;
using json = nlohmann::json;

namespace {

class InspectorClient {
  public:
	InspectorClient( Uint16 port ) {
		connected = socket.connect( IpAddress( "127.0.0.1" ), port, Seconds( 2 ) ) == Socket::Done;
		if ( connected )
			socket.setBlocking( false );
	}

	void send( const json& request ) { sendBytes( request.dump() + "\n" ); }

	void sendBytes( const std::string& bytes ) {
		socket.setBlocking( true );
		socket.send( bytes.data(), bytes.size() );
		socket.setBlocking( false );
	}

	json receive() {
		using namespace std::chrono;
		auto deadline = steady_clock::now() + seconds( 3 );
		while ( steady_clock::now() < deadline ) {
			auto end = input.find( '\n' );
			if ( end != std::string::npos ) {
				std::string line = input.substr( 0, end );
				input.erase( 0, end + 1 );
				return json::parse( line );
			}
			Engine::instance()->updateInput();
			SceneManager::instance()->update();
			char bytes[4096];
			size_t length = 0;
			if ( socket.receive( bytes, sizeof( bytes ), length ) == Socket::Done )
				input.append( bytes, length );
			std::this_thread::sleep_for( milliseconds( 1 ) );
		}
		return nullptr;
	}

	bool connected{ false };
	TcpSocket socket;
	std::string input;
};

bool connectInspector( InspectorClient& client, const char* token ) {
	client.send( { { "id", 1 },
				   { "method", "session.connect" },
				   { "params", { { "protocolVersion", 1 }, { "token", token } } } } );
	json reply = client.receive();
	return reply.is_object() && reply.contains( "result" );
}

} // namespace

UTEST( UIInspector, HandlesInvalidateAndScenesStaySeparate ) {
	UIApplication app( WindowSettings( 320, 240, "Inspector Domain", WindowStyle::Default,
									   WindowBackend::Default, 32, {}, 1, false, true ),
					   UIApplication::Settings(
						   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f, true ),
					   ContextSettings( false, 0, 0, GLv_default, true, false ) );
	auto* scene = app.getUI();
	EXPECT_EQ( UIInspectorServer::instance(), nullptr );
	auto* webview = UIWebView::New();
	webview->setId( "preview" );
	webview->setParent( scene->getRoot() );
	auto* document = webview->getDocumentSceneNode();
	auto* inside = UIWidget::New();
	inside->setId( "inside" );
	inside->setParent( document->getRoot() );
	UIInspector inspector;
	inspector.setDefaultScene( scene );
	std::string handle = inspector.widgetHandle( inside );
	std::string handleAgain = inspector.widgetHandle( inside );
	EXPECT_STREQ( handle.c_str(), handleAgain.c_str() );
	EXPECT_EQ( inspector.query( scene, "#inside" ).size(), 0u );
	EXPECT_EQ( inspector.query( document, "#inside" ).size(), 1u );
	SmallVector<UIWidget*, 64> registered;
	SmallVector<std::string, 64> registeredHandles;
	for ( size_t i = 0; i < 64; ++i ) {
		auto* widget = UIWidget::New();
		widget->setParent( scene->getRoot() );
		registered.push_back( widget );
		registeredHandles.push_back( inspector.widgetHandle( widget ) );
	}
	EXPECT_EQ( inspector.widget( registeredHandles.back() ), registered.back() );
	registered.front()->close();
	SceneManager::instance()->update();
	EXPECT_EQ( inspector.widget( registeredHandles.front() ), nullptr );
	EXPECT_EQ( inspector.widget( registeredHandles.back() ), registered.back() );
	inside->close();
	SceneManager::instance()->update();
	EXPECT_EQ( inspector.widget( handle ), nullptr );
	ASSERT_TRUE( UIInspectorServer::start( scene ) );
	EXPECT_TRUE( UIInspectorServer::instance()->getToken().size() >= 32 );
	UIInspectorServer::stop();
	Engine::destroySingleton();
}

UTEST( UIInspector, TcpBatchWatchAndMultipleClients ) {
	UIApplication app( WindowSettings( 320, 240, "Inspector TCP", WindowStyle::Default,
									   WindowBackend::Default, 32, {}, 1, false, true ),
					   UIApplication::Settings(
						   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f, true ),
					   ContextSettings( false, 0, 0, GLv_default, true, false ) );
	auto* scene = app.getUI();
	auto* widget = UIWidget::New();
	widget->setId( "observed" );
	widget->setPixelsSize( 100, 40 );
	widget->setParent( scene->getRoot() );
	auto* password = UITextInput::NewPassword();
	password->setId( "password" );
	password->setPixelsPosition( 0, 60 );
	password->setText( String::fromUtf8( std::string_view( "secret-value" ) ) );
	password->setParent( scene->getRoot() );
	auto* longText = UITextView::New();
	longText->setId( "long-text" );
	longText->setPixelsPosition( 0, 110 );
	longText->setText( String::fromUtf8( std::string( 5000, 'x' ) ) );
	longText->setParent( scene->getRoot() );
	ASSERT_TRUE( UIInspectorServer::start( scene, { "127.0.0.1", 0, false, "test-token" } ) );
	auto* server = UIInspectorServer::instance();
	ASSERT_TRUE( server->getPort() != 0 );
	InspectorClient first( server->getPort() );
	InspectorClient second( server->getPort() );
	ASSERT_TRUE( first.connected );
	ASSERT_TRUE( second.connected );
	ASSERT_TRUE( connectInspector( first, "test-token" ) );
	ASSERT_TRUE( connectInspector( second, "test-token" ) );
	first.send(
		{ { "id", 2 },
		  { "method", "ui.query" },
		  { "params", { { "selector", "#observed" }, { "properties", { "geometry.size" } } } } } );
	json query = first.receive();
	ASSERT_TRUE( query.contains( "result" ) );
	ASSERT_EQ( query["result"]["total"].get<int>(), 1 );
	std::string handle = query["result"]["nodes"][0]["handle"];
	second.send(
		{ { "id", 6 }, { "method", "ui.query" }, { "params", { { "selector", "#password" } } } } );
	json secretQuery = second.receive();
	ASSERT_TRUE( secretQuery.contains( "result" ) );
	EXPECT_TRUE( secretQuery["result"]["nodes"][0]["text"] == "[redacted]" );
	second.send( { { "id", 7 },
				   { "method", "ui.inspect" },
				   { "params", { { "selector", "#password" }, { "properties", { "*" } } } } } );
	json secretInspect = second.receive();
	ASSERT_TRUE( secretInspect.contains( "result" ) );
	EXPECT_TRUE( secretInspect["result"]["properties"]["content.text"] == "[redacted]" );
	second.send( { { "id", 8 },
				   { "method", "ui.inspect" },
				   { "params",
					 { { "selector", "#long-text" },
					   { "properties", { "content.text" } },
					   { "maxStringLength", 10 } } } } );
	json truncated = second.receive();
	ASSERT_TRUE( truncated.contains( "result" ) );
	EXPECT_EQ( truncated["result"]["properties"]["content.text"].get<std::string>().size(), 10u );
	EXPECT_TRUE( truncated["result"].contains( "truncatedProperties" ) );
	first.send( { { "id", 3 },
				  { "method", "watch.subscribe" },
				  { "params", { { "handle", handle }, { "properties", { "geometry.size" } } } } } );
	json watch = first.receive();
	ASSERT_TRUE( watch.contains( "result" ) );
	widget->setPixelsSize( 120, 40 );
	json changed = first.receive();
	std::string method = changed["method"].get<std::string>();
	ASSERT_STREQ( method.c_str(), "watch.changed" );
	ASSERT_EQ( changed["params"]["changes"]["geometry.size"]["new"][0].get<int>(), 120 );
	int clicks = 0;
	widget->onClick( [&]( const MouseEvent* ) {
		++clicks;
		widget->setPixelsSize( 140, 40 );
	} );
	second.send( { { "id", 5 },
				   { "method", "input.click" },
				   { "params", { { "selector", "#observed" } } } } );
	json click = second.receive();
	ASSERT_TRUE( click.contains( "result" ) );
	json clickChange = first.receive();
	std::string clickMethod = clickChange["method"].get<std::string>();
	ASSERT_STREQ( clickMethod.c_str(), "watch.changed" );
	EXPECT_EQ( clickChange["params"]["changes"]["geometry.size"]["new"][0].get<int>(), 140 );
	EXPECT_EQ( clicks, 1 );
	second.send(
		{ { "id", 4 },
		  { "method", "session.batch" },
		  { "params",
			{ { "commands",
				json::array( { { { "name", "found" },
								 { "method", "ui.query" },
								 { "return", false },
								 { "params", { { "selector", "#observed" } } } },
							   { { "name", "state" },
								 { "method", "ui.inspect" },
								 { "params",
								   { { "handle", { { "$ref", "found#/nodes/0/handle" } } },
									 { "properties", { "geometry.size" } } } } } } ) } } } } );
	json batch = second.receive();
	ASSERT_TRUE( batch.contains( "result" ) );
	EXPECT_TRUE( batch["result"]["results"].contains( "state" ) );
	EXPECT_FALSE( batch["result"]["results"].contains( "found" ) );
	second.send(
		{ { "id", 10 },
		  { "method", "session.batch" },
		  { "params",
			{ { "commands",
				json::array(
					{ { { "method", "ui.inspect" },
						{ "params",
						  { { "handle", { { "$ref", "unknown#/handle" } } } } } } } ) } } } } );
	json badReference = second.receive();
	EXPECT_TRUE( badReference.contains( "error" ) &&
				 badReference["error"]["code"] == "batch-command-failed" );
	EXPECT_TRUE( badReference["error"]["data"]["commandError"]["code"] == "batch-reference-error" );
	second.send(
		{ { "id", 11 },
		  { "method", "session.batch" },
		  { "params",
			{ { "commands",
				json::array( { { { "method", "input.click" },
								 { "delay", "bad" },
								 { "params", { { "selector", "#observed" } } } } } ) } } } } );
	json badDelay = second.receive();
	EXPECT_TRUE( badDelay.contains( "error" ) );
	EXPECT_EQ( clicks, 1 );
	widget->close();
	Engine::instance()->updateInput();
	SceneManager::instance()->update();
	json removed = first.receive();
	json ended = first.receive();
	EXPECT_TRUE( removed["method"] == "watch.targetRemoved" );
	EXPECT_TRUE( ended["method"] == "watch.ended" );
	second.send(
		{ { "id", 9 }, { "method", "ui.inspect" }, { "params", { { "handle", handle } } } } );
	json invalid = second.receive();
	EXPECT_TRUE( invalid.contains( "error" ) && invalid["error"]["code"] == "invalid-widget" );
	UIInspectorServer::stop();
	Engine::destroySingleton();
}

UTEST( UIInspector, InputActionErrorsAndKeyEvents ) {
	UIApplication app( WindowSettings( 320, 240, "Inspector Input", WindowStyle::Default,
									   WindowBackend::Default, 32, {}, 1, false, true ),
					   UIApplication::Settings(
						   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f, true ),
					   ContextSettings( false, 0, 0, GLv_default, true, false ) );
	auto* scene = app.getUI();
	auto* first = UIWidget::New();
	first->setId( "first" );
	first->setClass( "ambiguous" );
	first->setPixelsPosition( 10, 10 );
	first->setPixelsSize( 30, 30 );
	first->setParent( scene->getRoot() );
	auto* second = UIWidget::New();
	second->setClass( "ambiguous" );
	second->setPixelsPosition( 50, 10 );
	second->setPixelsSize( 30, 30 );
	second->setParent( scene->getRoot() );
	auto* hidden = UIWidget::New();
	hidden->setId( "hidden" );
	hidden->setPixelsPosition( 10, 50 );
	hidden->setPixelsSize( 30, 30 );
	hidden->setParent( scene->getRoot() );
	hidden->setVisible( false );
	auto* outside = UIWidget::New();
	outside->setId( "outside" );
	outside->setPixelsPosition( 400, 400 );
	outside->setPixelsSize( 30, 30 );
	outside->setParent( scene->getRoot() );
	auto* covered = UIWidget::New();
	covered->setId( "covered" );
	covered->setPixelsPosition( 100, 100 );
	covered->setPixelsSize( 40, 40 );
	covered->setParent( scene->getRoot() );
	auto* blocker = UIWidget::New();
	blocker->setPixelsPosition( 100, 100 );
	blocker->setPixelsSize( 40, 40 );
	blocker->setParent( scene->getRoot() );
	int clicks = 0;
	first->onClick( [&]( const MouseEvent* ) { ++clicks; } );
	covered->onClick( [&]( const MouseEvent* ) { ++clicks; } );
	scene->flushDirtyStyleAndLayout();
	ASSERT_TRUE( UIInspectorServer::start( scene, { "127.0.0.1", 0, false, "input-token" } ) );
	InspectorClient client( UIInspectorServer::instance()->getPort() );
	ASSERT_TRUE( client.connected );
	ASSERT_TRUE( connectInspector( client, "input-token" ) );
	const std::array<std::pair<const char*, const char*>, 4> rejectedTargets{
		{ { ".ambiguous", "target-ambiguous" },
		  { "#hidden", "target-not-interactable" },
		  { "#outside", "target-not-interactable" },
		  { "#covered", "target-not-interactable" } } };
	int id = 2;
	for ( const auto& [selector, expectedCode] : rejectedTargets ) {
		client.send( { { "id", id++ },
					   { "method", "input.click" },
					   { "params", { { "selector", selector } } } } );
		json response = client.receive();
		ASSERT_TRUE( response.contains( "error" ) );
		EXPECT_TRUE( response["error"]["code"] == expectedCode );
	}
	EXPECT_EQ( clicks, 0 );
	client.send( { { "id", id++ },
				   { "method", "input.click" },
				   { "params", { { "selector", "#first" } } } } );
	ASSERT_TRUE( client.receive().contains( "result" ) );
	EXPECT_EQ( clicks, 1 );
	int rightClicks = 0;
	first->on( Event::MouseUp, [&]( const Event* event ) {
		if ( event->asMouseEvent()->getFlags() & EE_BUTTON_RMASK )
			++rightClicks;
	} );
	client.send( { { "id", id++ },
				   { "method", "input.click" },
				   { "params", { { "selector", "#first" }, { "button", "right" } } } } );
	ASSERT_TRUE( client.receive().contains( "result" ) );
	EXPECT_EQ( rightClicks, 1 );
	EXPECT_EQ( clicks, 1 );
	int middleClicks = 0;
	first->on( Event::MouseClick, [&]( const Event* event ) {
		if ( event->asMouseEvent()->getFlags() & EE_BUTTON_MMASK )
			++middleClicks;
	} );
	scene->getEventDispatcher()->setFocusNode( second );
	client.send( { { "id", id++ },
				   { "method", "input.click" },
				   { "params", { { "selector", "#first" }, { "button", "middle" } } } } );
	ASSERT_TRUE( client.receive().contains( "result" ) );
	EXPECT_EQ( middleClicks, 1 );
	EXPECT_EQ( clicks, 1 );
	EXPECT_TRUE( scene->getEventDispatcher()->getFocusNode() == second );
	client.send( { { "id", id++ },
				   { "method", "input.click" },
				   { "params", { { "selector", "#first" }, { "button", "other" } } } } );
	json invalidButton = client.receive();
	ASSERT_TRUE( invalidButton.contains( "error" ) );
	EXPECT_TRUE( invalidButton["error"]["code"] == "invalid-params" );

	scene->getEventDispatcher()->setFocusNode( first );
	int keyDown = 0;
	int keyUp = 0;
	Uint32 lastModifiers = 0;
	first->addEventListener( Event::KeyDown, [&]( const Event* event ) {
		++keyDown;
		lastModifiers = event->asKeyEvent()->getMod();
	} );
	first->addEventListener( Event::KeyUp, [&]( const Event* event ) {
		++keyUp;
		lastModifiers = event->asKeyEvent()->getMod();
	} );
	client.send(
		{ { "id", id++ },
		  { "method", "input.key" },
		  { "params",
			{ { "key", "F" }, { "action", "down" }, { "modifiers", { "Ctrl", "Shift" } } } } } );
	ASSERT_TRUE( client.receive().contains( "result" ) );
	EXPECT_EQ( keyDown, 1 );
	EXPECT_EQ( keyUp, 0 );
	EXPECT_EQ( lastModifiers & ( KEYMOD_LCTRL | KEYMOD_LSHIFT ),
			   static_cast<Uint32>( KEYMOD_LCTRL | KEYMOD_LSHIFT ) );
	client.send( { { "id", id++ },
				   { "method", "input.key" },
				   { "params", { { "key", "F" }, { "action", "up" } } } } );
	ASSERT_TRUE( client.receive().contains( "result" ) );
	EXPECT_EQ( keyDown, 1 );
	EXPECT_EQ( keyUp, 1 );
	client.send( { { "id", id++ },
				   { "method", "input.key" },
				   { "params", { { "key", "F" }, { "action", "press" } } } } );
	ASSERT_TRUE( client.receive().contains( "result" ) );
	EXPECT_EQ( keyDown, 2 );
	EXPECT_EQ( keyUp, 2 );
	client.send(
		{ { "id", id++ }, { "method", "input.key" }, { "params", { { "key", "not-a-key" } } } } );
	json invalidKey = client.receive();
	ASSERT_TRUE( invalidKey.contains( "error" ) );
	EXPECT_TRUE( invalidKey["error"]["code"] == "invalid-params" );
	EXPECT_EQ( keyDown, 2 );
	EXPECT_EQ( keyUp, 2 );
	UIInspectorServer::stop();
	Engine::destroySingleton();
}

UTEST( UIInspector, HTMLTextSummariesAndProperties ) {
	UIApplication app( WindowSettings( 320, 240, "Inspector HTML Text", WindowStyle::Default,
									   WindowBackend::Default, 32, {}, 1, false, true ),
					   UIApplication::Settings(
						   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f, true ),
					   ContextSettings( false, 0, 0, GLv_default, true, false ) );
	auto* scene = app.getUI();
	auto* webview = UIWebView::New();
	webview->setId( "preview" );
	webview->setPixelsSize( 220, 160 );
	webview->setParent( scene->getRoot() );
	auto* document = webview->getDocumentSceneNode();
	auto* paragraph = UIRichText::NewParagraph();
	paragraph->setId( "article" );
	paragraph->setParent( document->getRoot() );
	auto* source = UITextNode::New();
	source->setId( "source" );
	source->setText( "Hello " );
	source->setParent( paragraph );
	auto* span = UITextSpan::NewStrong();
	span->setId( "emphasis" );
	span->setText( "world" );
	span->setParent( paragraph );
	auto* punctuation = UITextNode::New();
	punctuation->setText( "!" );
	punctuation->setParent( span );
	paragraph->setPixelsSize( 200, 30 );
	ASSERT_TRUE( UIInspectorServer::start( scene, { "127.0.0.1", 0, false, "html-token" } ) );
	InspectorClient client( UIInspectorServer::instance()->getPort() );
	ASSERT_TRUE( client.connected );
	ASSERT_TRUE( connectInspector( client, "html-token" ) );
	client.send(
		{ { "id", 2 }, { "method", "ui.query" }, { "params", { { "selector", "#preview" } } } } );
	json preview = client.receive();
	ASSERT_TRUE( preview.contains( "result" ) );
	std::string documentScene = preview["result"]["nodes"][0]["documentScene"];
	client.send( { { "id", 3 },
				   { "method", "ui.query" },
				   { "params", { { "scene", documentScene }, { "selector", "#article" } } } } );
	json article = client.receive();
	ASSERT_TRUE( article.contains( "result" ) );
	ASSERT_EQ( article["result"]["nodes"].size(), 1u );
	EXPECT_TRUE( article["result"]["nodes"][0]["text"] == "Hello world!" );
	std::string articleHandle = article["result"]["nodes"][0]["handle"];
	client.send( { { "id", 4 },
				   { "method", "ui.query" },
				   { "params", { { "scene", documentScene }, { "selector", "#emphasis" } } } } );
	json emphasis = client.receive();
	ASSERT_TRUE( emphasis.contains( "result" ) );
	EXPECT_TRUE( emphasis["result"]["nodes"][0]["text"] == "world!" );
	client.send( { { "id", 5 },
				   { "method", "ui.query" },
				   { "params", { { "scene", documentScene }, { "selector", "#source" } } } } );
	json sourceResult = client.receive();
	ASSERT_TRUE( sourceResult.contains( "result" ) );
	EXPECT_TRUE( sourceResult["result"]["nodes"][0]["text"] == "Hello " );
	client.send(
		{ { "id", 6 },
		  { "method", "ui.inspect" },
		  { "params", { { "handle", articleHandle }, { "properties", { "content.text" } } } } } );
	json inspection = client.receive();
	ASSERT_TRUE( inspection.contains( "result" ) );
	EXPECT_TRUE( inspection["result"]["properties"]["content.text"] == "Hello world!" );
	client.send( { { "id", 11 },
				   { "method", "ui.query" },
				   { "params",
					 { { "scene", documentScene },
					   { "selector", "#article" },
					   { "properties", { "content.text" } } } } } );
	json inlineProperties = client.receive();
	ASSERT_TRUE( inlineProperties.contains( "result" ) );
	EXPECT_TRUE( inlineProperties["result"]["nodes"][0]["properties"]["content.text"] ==
				 "Hello world!" );
	client.send(
		{ { "id", 12 },
		  { "method", "ui.tree" },
		  { "params", { { "root", articleHandle }, { "depth", 0 }, { "includeText", true } } } } );
	json tree = client.receive();
	ASSERT_TRUE( tree.contains( "result" ) );
	EXPECT_TRUE( tree["result"]["nodes"][0]["text"] == "Hello world!" );
	client.send(
		{ { "id", 7 },
		  { "method", "watch.subscribe" },
		  { "params", { { "handle", articleHandle }, { "properties", { "content.text" } } } } } );
	json watch = client.receive();
	ASSERT_TRUE( watch.contains( "result" ) );
	EXPECT_TRUE( watch["result"]["targets"][0]["values"]["content.text"] == "Hello world!" );
	span->setText( "eepp" );
	json changed = client.receive();
	ASSERT_TRUE( changed.contains( "method" ) );
	EXPECT_TRUE( changed["method"] == "watch.changed" );
	EXPECT_TRUE( changed["params"]["changes"]["content.text"]["old"] == "Hello world!" );
	EXPECT_TRUE( changed["params"]["changes"]["content.text"]["new"] == "Hello eepp!" );
	client.send( { { "id", 8 },
				   { "method", "watch.unsubscribe" },
				   { "params", { { "subscription", watch["result"]["subscription"] } } } } );
	ASSERT_TRUE( client.receive().contains( "result" ) );
	auto* password = UITextInput::NewPassword();
	password->setText( String::fromUtf8( std::string_view( "do-not-expose" ) ) );
	password->setParent( paragraph );
	client.send(
		{ { "id", 13 },
		  { "method", "ui.inspect" },
		  { "params", { { "handle", articleHandle }, { "properties", { "content.text" } } } } } );
	json parentText = client.receive();
	ASSERT_TRUE( parentText.contains( "result" ) );
	EXPECT_TRUE( parentText["result"]["properties"]["content.text"] == "Hello eepp!" );
	auto* longSource = UITextNode::New();
	longSource->setId( "long-source" );
	longSource->setText( String::fromUtf8( std::string( 300, 'x' ) ) );
	longSource->setParent( paragraph );
	client.send( { { "id", 9 },
				   { "method", "ui.query" },
				   { "params", { { "scene", documentScene }, { "selector", "#long-source" } } } } );
	json longSummary = client.receive();
	ASSERT_TRUE( longSummary.contains( "result" ) );
	EXPECT_EQ( longSummary["result"]["nodes"][0]["text"].get<std::string>().size(), 256u );
	EXPECT_TRUE( longSummary["result"]["nodes"][0]["textTruncated"] == true );
	client.send( { { "id", 10 },
				   { "method", "ui.inspect" },
				   { "params",
					 { { "scene", documentScene },
					   { "selector", "#article" },
					   { "properties", { "content.text" } },
					   { "maxStringLength", 10 } } } } );
	json shortInspection = client.receive();
	ASSERT_TRUE( shortInspection.contains( "result" ) );
	EXPECT_TRUE( shortInspection["result"]["properties"]["content.text"] == "Hello eepp" );
	EXPECT_TRUE( shortInspection["result"]["truncatedProperties"][0] == "content.text" );
	UIInspectorServer::stop();
	Engine::destroySingleton();
}

UTEST( UIInspector, WebViewSecondaryWindowAndDeferredBatch ) {
	UIApplication app( WindowSettings( 320, 240, "Inspector Primary", WindowStyle::Default,
									   WindowBackend::Default, 32, {}, 1, false, true ),
					   UIApplication::Settings(
						   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f, true ),
					   ContextSettings( false, 0, 0, GLv_default, true, false ) );
	auto* mainScene = app.getUI();
	auto* webview = UIWebView::New();
	webview->setId( "preview" );
	webview->setPixelsSize( 200, 100 );
	webview->setParent( mainScene->getRoot() );
	auto* document = webview->getDocumentSceneNode();
	auto* htmlWidget = UIWidget::New();
	htmlWidget->setId( "inside" );
	htmlWidget->setPixelsSize( 60, 30 );
	htmlWidget->setParent( document->getRoot() );
	int innerClicks = 0;
	htmlWidget->onClick( [&]( const MouseEvent* ) { ++innerClicks; } );
	auto* textInput = UITextInput::New();
	textInput->setId( "edit" );
	textInput->setPixelsPosition( 0, 40 );
	textInput->setPixelsSize( 100, 30 );
	textInput->setParent( document->getRoot() );
	auto* secondary =
		app.createWindow( WindowSettings( 280, 180, "Inspector Secondary", WindowStyle::Default,
										  WindowBackend::Default, 32, {}, 1, false, true ),
						  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( secondary != nullptr );
	auto* button = UIWidget::New();
	button->setId( "secondary-button" );
	button->setPixelsPosition( 30, 30 );
	button->setPixelsSize( 100, 50 );
	button->setParent( secondary->getRoot() );
	int clicks = 0;
	button->onClick( [&]( const MouseEvent* ) { ++clicks; } );
	mainScene->flushDirtyStyleAndLayout();
	secondary->flushDirtyStyleAndLayout();
	ASSERT_TRUE(
		UIInspectorServer::start( mainScene, { "127.0.0.1", 0, false, "contexts-token" } ) );
	InspectorClient client( UIInspectorServer::instance()->getPort() );
	ASSERT_TRUE( client.connected );
	ASSERT_TRUE( connectInspector( client, "contexts-token" ) );
	client.send( { { "id", 2 }, { "method", "ui.contexts" } } );
	json contexts = client.receive();
	ASSERT_TRUE( contexts.contains( "result" ) );
	EXPECT_EQ( contexts["result"]["windows"].size(), 2u );
	std::string secondaryScene;
	std::string secondaryWindow;
	for ( const auto& item : contexts["result"]["scenes"] ) {
		if ( item["kind"] == "top-level" &&
			 item["window"] != contexts["result"]["defaultWindow"] ) {
			secondaryScene = item["handle"].get<std::string>();
			secondaryWindow = item["window"].get<std::string>();
		}
	}
	ASSERT_TRUE( !secondaryScene.empty() );
	client.send(
		{ { "id", 3 }, { "method", "ui.query" }, { "params", { { "selector", "#preview" } } } } );
	json preview = client.receive();
	ASSERT_TRUE( preview.contains( "result" ) );
	std::string documentScene = preview["result"]["nodes"][0]["documentScene"];
	std::string previewHandle = preview["result"]["nodes"][0]["handle"];
	bool foundDocumentScene = false;
	for ( const auto& item : contexts["result"]["scenes"] ) {
		if ( item["handle"] == documentScene ) {
			foundDocumentScene = true;
			EXPECT_TRUE( item["kind"] == "webview" );
			EXPECT_TRUE( item["parentScene"] == contexts["result"]["defaultScene"] );
			EXPECT_TRUE( item["owner"] == previewHandle );
			EXPECT_TRUE( item["window"] == contexts["result"]["defaultWindow"] );
		}
	}
	EXPECT_TRUE( foundDocumentScene );
	client.send( { { "id", 24 },
				   { "method", "ui.screenshot" },
				   { "params", { { "scene", documentScene } } } } );
	json documentImage = client.receive();
	ASSERT_TRUE( documentImage.contains( "result" ) );
	EXPECT_TRUE( documentImage["result"]["scene"] == documentScene );
	EXPECT_TRUE( documentImage["result"]["sizePx"] == json::array( { 200, 100 } ) );
	FileSystem::fileRemove( documentImage["result"]["path"].get<std::string>() );
	client.send( { { "id", 25 },
				   { "method", "ui.screenshot" },
				   { "params", { { "window", secondaryWindow } } } } );
	json secondaryImage = client.receive();
	ASSERT_TRUE( secondaryImage.contains( "result" ) );
	EXPECT_TRUE( secondaryImage["result"]["sizePx"] == json::array( { 280, 180 } ) );
	FileSystem::fileRemove( secondaryImage["result"]["path"].get<std::string>() );
	int secondaryKeyDown = 0;
	button->addEventListener( Event::KeyDown, [&]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_F )
			++secondaryKeyDown;
	} );
	secondary->getEventDispatcher()->setFocusNode( button );
	client.send( { { "id", 11 },
				   { "method", "input.key" },
				   { "params", { { "scene", secondaryScene }, { "key", "F" } } } } );
	ASSERT_TRUE( client.receive().contains( "result" ) );
	EXPECT_EQ( secondaryKeyDown, 1 );
	client.send(
		{ { "id", 4 }, { "method", "ui.query" }, { "params", { { "selector", "#inside" } } } } );
	json outer = client.receive();
	ASSERT_TRUE( outer.contains( "result" ) );
	EXPECT_EQ( outer["result"]["total"].get<int>(), 0 );
	client.send( { { "id", 5 },
				   { "method", "ui.query" },
				   { "params", { { "scene", documentScene }, { "selector", "#inside" } } } } );
	json inner = client.receive();
	ASSERT_TRUE( inner.contains( "result" ) );
	EXPECT_EQ( inner["result"]["total"].get<int>(), 1 );
	client.send( { { "id", 7 },
				   { "method", "input.click" },
				   { "params", { { "scene", documentScene }, { "selector", "#inside" } } } } );
	json innerClick = client.receive();
	ASSERT_TRUE( innerClick.contains( "result" ) );
	EXPECT_EQ( innerClicks, 1 );
	client.send( { { "id", 8 },
				   { "method", "input.click" },
				   { "params", { { "scene", documentScene }, { "selector", "#edit" } } } } );
	json editClick = client.receive();
	ASSERT_TRUE( editClick.contains( "result" ) );
	client.send( { { "id", 9 },
				   { "method", "input.text" },
				   { "params", { { "scene", documentScene }, { "text", "h\u00e9llo" } } } } );
	json typed = client.receive();
	ASSERT_TRUE( typed.contains( "result" ) );
	std::string entered = textInput->getText().toUtf8();
	EXPECT_STREQ( entered.c_str(), "h\u00e9llo" );
	client.send(
		{ { "id", 10 },
		  { "method", "input.key" },
		  { "params",
			{ { "scene", documentScene }, { "key", "Backspace" }, { "action", "press" } } } } );
	json pressed = client.receive();
	ASSERT_TRUE( pressed.contains( "result" ) );
	std::string afterBackspace = textInput->getText().toUtf8();
	EXPECT_STREQ( afterBackspace.c_str(), "h\u00e9ll" );
	client.send(
		{ { "id", 6 },
		  { "method", "session.batch" },
		  { "params",
			{ { "commands",
				json::array(
					{ { { "method", "input.click" },
						{ "return", false },
						{ "delay", "20ms" },
						{ "params",
						  { { "scene", secondaryScene }, { "selector", "#secondary-button" } } } },
					  { { "method", "session.nextFrame" }, { "return", false } },
					  { { "name", "state" },
						{ "method", "ui.inspect" },
						{ "params",
						  { { "scene", secondaryScene },
							{ "selector", "#secondary-button" },
							{ "properties", { "state.visible" } } } } } } ) } } } } );
	json batch = client.receive();
	ASSERT_TRUE( batch.contains( "result" ) );
	EXPECT_EQ( clicks, 1 );
	EXPECT_TRUE( batch["result"]["results"].contains( "state" ) );
	UIInspectorServer::stop();
	Engine::destroySingleton();
}

UTEST( UIInspector, KeyBindingsKeepAliasesAndSceneScopeInReadOnlyMode ) {
	UIApplication app( WindowSettings( 320, 240, "Inspector Keybindings", WindowStyle::Default,
									   WindowBackend::Default, 32, {}, 1, false, true ),
					   UIApplication::Settings(
						   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f, true ),
					   ContextSettings( false, 0, 0, GLv_default, true, false ) );
	auto* scene = app.getUI();
	int executions = 0;
	scene->getKeyBindings().reset();
	scene->getKeyBindings().addKeybind( { KEY_R, KEYMOD_CTRL }, "reload" );
	scene->getKeyBindings().addKeybind( { KEY_F5, 0 }, "reload" );
	scene->setKeyBindingCommand( "reload", [&executions] { ++executions; } );
	auto* input = UITextInput::New();
	input->setId( "address" );
	input->setParent( scene->getRoot() );
	input->getKeyBindings().reset();
	input->getKeyBindings().addKeybind( { KEY_A, KEYMOD_CTRL }, "select-all" );
	auto* webview = UIWebView::New();
	webview->setId( "preview" );
	webview->setParent( scene->getRoot() );
	ASSERT_TRUE( UIInspectorServer::start( scene, { "127.0.0.1", 0, true, "bindings-token" } ) );
	InspectorClient client( UIInspectorServer::instance()->getPort() );
	ASSERT_TRUE( client.connected );
	client.send( { { "id", 1 },
				   { "method", "session.connect" },
				   { "params", { { "protocolVersion", 1 }, { "token", "bindings-token" } } } } );
	json connected = client.receive();
	ASSERT_TRUE( connected.contains( "result" ) );
	const auto& capabilities = connected["result"]["capabilities"];
	EXPECT_TRUE( std::find( capabilities.begin(), capabilities.end(), "ui.keybindings" ) !=
				 capabilities.end() );
	client.send( { { "id", 2 }, { "method", "ui.keybindings" } } );
	json bindings = client.receive();
	ASSERT_TRUE( bindings.contains( "result" ) );
	const auto& result = bindings["result"];
	EXPECT_TRUE( result["supported"] == true );
	EXPECT_TRUE( result["handle"].is_null() );
	ASSERT_EQ( result["total"].get<int>(), 2 );
	ASSERT_EQ( result["bindings"].size(), 2u );
	EXPECT_TRUE( result["bindings"][0]["command"] == "reload" );
	EXPECT_TRUE( result["bindings"][1]["command"] == "reload" );
	EXPECT_EQ( result["bindings"][0]["keycode"].get<int>(), KEY_F5 );
	EXPECT_EQ( result["bindings"][1]["keycode"].get<int>(), KEY_R );
	EXPECT_EQ( result["bindings"][1]["mod"].get<Uint32>(), KEYMOD_CTRL );
	EXPECT_STDSTREQ( result["bindings"][1]["shortcut"].get<std::string>(),
					 scene->getKeyBindings().getShortcutString( { KEY_R, KEYMOD_CTRL } ) );
	client.send(
		{ { "id", 3 }, { "method", "ui.keybindings" }, { "params", { { "limit", 1 } } } } );
	json page = client.receive();
	ASSERT_TRUE( page.contains( "result" ) );
	EXPECT_EQ( page["result"]["returned"].get<int>(), 1 );
	EXPECT_TRUE( page["result"]["truncated"] == true );
	client.send( { { "id", 4 },
				   { "method", "ui.keybindings" },
				   { "params", { { "offset", 1 }, { "limit", 1 } } } } );
	json nextPage = client.receive();
	ASSERT_TRUE( nextPage.contains( "result" ) );
	EXPECT_TRUE( nextPage["result"]["bindings"][0] == result["bindings"][1] );
	EXPECT_TRUE( nextPage["result"]["truncated"] == false );
	client.send( { { "id", 5 },
				   { "method", "ui.keybindings" },
				   { "params", { { "selector", "#address" } } } } );
	json widgetBindings = client.receive();
	ASSERT_TRUE( widgetBindings.contains( "result" ) );
	EXPECT_TRUE( widgetBindings["result"]["supported"] == true );
	ASSERT_EQ( widgetBindings["result"]["total"].get<int>(), 1 );
	EXPECT_TRUE( widgetBindings["result"]["bindings"][0]["command"] == "select-all" );
	client.send( { { "id", 6 },
				   { "method", "ui.keybindings" },
				   { "params", { { "handle", widgetBindings["result"]["handle"] } } } } );
	json byHandle = client.receive();
	ASSERT_TRUE( byHandle.contains( "result" ) );
	EXPECT_TRUE( byHandle["result"] == widgetBindings["result"] );
	client.send( { { "id", 7 },
				   { "method", "ui.keybindings" },
				   { "params", { { "selector", "#preview" } } } } );
	json unsupported = client.receive();
	ASSERT_TRUE( unsupported.contains( "result" ) );
	EXPECT_TRUE( unsupported["result"]["supported"] == false );
	EXPECT_TRUE( unsupported["result"]["bindings"].empty() );
	client.send(
		{ { "id", 8 }, { "method", "ui.query" }, { "params", { { "selector", "#preview" } } } } );
	json preview = client.receive();
	ASSERT_TRUE( preview.contains( "result" ) );
	client.send(
		{ { "id", 9 },
		  { "method", "ui.keybindings" },
		  { "params", { { "scene", preview["result"]["nodes"][0]["documentScene"] } } } } );
	json nested = client.receive();
	ASSERT_TRUE( nested.contains( "result" ) );
	EXPECT_TRUE( nested["result"]["supported"] == true );
	EXPECT_TRUE( nested["result"]["bindings"].empty() );
	client.send(
		{ { "id", 10 }, { "method", "ui.keybindings" }, { "params", { { "limit", 1001 } } } } );
	json invalidLimit = client.receive();
	ASSERT_TRUE( invalidLimit.contains( "error" ) );
	EXPECT_TRUE( invalidLimit["error"]["code"] == "invalid-params" );
	client.send(
		{ { "id", 11 },
		  { "method", "ui.keybindings" },
		  { "params",
			{ { "handle", widgetBindings["result"]["handle"] }, { "selector", "#address" } } } } );
	json invalidTarget = client.receive();
	ASSERT_TRUE( invalidTarget.contains( "error" ) );
	EXPECT_TRUE( invalidTarget["error"]["code"] == "invalid-params" );
	EXPECT_EQ( executions, 0 );
	UIInspectorServer::stop();
	Engine::destroySingleton();
}

UTEST( UIInspector, AuthenticationFramingAndReadOnly ) {
	UIApplication app( WindowSettings( 320, 240, "Inspector Security", WindowStyle::Default,
									   WindowBackend::Default, 32, {}, 1, false, true ),
					   UIApplication::Settings(
						   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f, true ),
					   ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( UIInspectorServer::start( app.getUI(), { "127.0.0.1", 0, true, "read-token" } ) );
	Uint16 port = UIInspectorServer::instance()->getPort();
	InspectorClient wrong( port );
	ASSERT_TRUE( wrong.connected );
	wrong.send( { { "id", 1 },
				  { "method", "session.connect" },
				  { "params", { { "protocolVersion", 1 }, { "token", "wrong" } } } } );
	json rejected = wrong.receive();
	ASSERT_TRUE( rejected.contains( "error" ) );
	std::string code = rejected["error"]["code"];
	EXPECT_STREQ( code.c_str(), "authentication-failed" );
	InspectorClient client( port );
	ASSERT_TRUE( client.connected );
	ASSERT_TRUE( connectInspector( client, "read-token" ) );
	client.sendBytes( "{\"id\":2,\"method\":\"ui.con" );
	client.sendBytes( "texts\"}\r\n{\"id\":3,\"method\":\"ui.focus\"}\n" );
	json contexts = client.receive();
	json focus = client.receive();
	ASSERT_TRUE( contexts.contains( "result" ) );
	ASSERT_TRUE( focus.contains( "result" ) );
	EXPECT_EQ( contexts["id"].get<int>(), 2 );
	EXPECT_EQ( focus["id"].get<int>(), 3 );
	const std::array<json, 3> actions{
		{ { { "method", "input.click" }, { "params", { { "selector", "*" } } } },
		  { { "method", "input.key" }, { "params", { { "key", "Enter" } } } },
		  { { "method", "input.text" }, { "params", { { "text", "blocked" } } } } } };
	int actionId = 4;
	for ( auto action : actions ) {
		action["id"] = actionId++;
		client.send( action );
		json denied = client.receive();
		ASSERT_TRUE( denied.contains( "error" ) );
		EXPECT_TRUE( denied["error"]["code"] == "permission-denied" );
	}
	client.sendBytes( "not-json\n" );
	json parse = client.receive();
	ASSERT_TRUE( parse.contains( "error" ) );
	std::string parseCode = parse["error"]["code"];
	EXPECT_STREQ( parseCode.c_str(), "parse-error" );
	UIInspectorServer::stop();
	Engine::destroySingleton();
}

UTEST( UIInspector, ScreenshotCapturesRenderedPixelsAndCrop ) {
	UIApplication app( WindowSettings( 320, 240, "Inspector Screenshot", WindowStyle::Default,
									   WindowBackend::Default, 32, {}, 1, false, true ),
					   UIApplication::Settings(
						   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f, true ),
					   ContextSettings( false, 0, 0, GLv_default, true, false ) );
	auto* scene = app.getUI();
	auto* panel = UIWidget::New();
	panel->setPixelsSize( 320, 240 );
	panel->setBackgroundColor( Color::Red );
	panel->setParent( scene->getRoot() );
	ASSERT_TRUE( UIInspectorServer::start( scene, { "127.0.0.1", 0, true, "image-token" } ) );
	InspectorClient client( UIInspectorServer::instance()->getPort() );
	ASSERT_TRUE( client.connected );
	ASSERT_TRUE( connectInspector( client, "image-token" ) );
	client.send( { { "id", 2 }, { "method", "ui.screenshot" } } );
	json full = client.receive();
	ASSERT_TRUE( full.contains( "result" ) );
	std::string fullPath = full["result"]["path"];
	EXPECT_TRUE( FileSystem::fileExists( fullPath ) );
	EXPECT_TRUE( full["result"]["sizePx"] == json::array( { 320, 240 } ) );
	Graphics::Image fullImage( fullPath );
	EXPECT_EQ( fullImage.getWidth(), 320u );
	EXPECT_EQ( fullImage.getHeight(), 240u );
	EXPECT_GT( fullImage.getPixel( 10, 10 ).r, 200 );
	EXPECT_LT( fullImage.getPixel( 10, 10 ).b, 50 );
	FileSystem::fileRemove( fullPath );

	panel->setBackgroundColor( Color::Blue );
	client.send( { { "id", 3 },
				   { "method", "ui.screenshot" },
				   { "params", { { "rect", { 8, 12, 40, 30 } }, { "format", "bmp" } } } } );
	json cropped = client.receive();
	ASSERT_TRUE( cropped.contains( "result" ) );
	std::string croppedPath = cropped["result"]["path"];
	Graphics::Image croppedImage( croppedPath );
	EXPECT_EQ( croppedImage.getWidth(), 40u );
	EXPECT_EQ( croppedImage.getHeight(), 30u );
	EXPECT_GT( croppedImage.getPixel( 2, 2 ).b, 200 );
	EXPECT_LT( croppedImage.getPixel( 2, 2 ).r, 50 );
	FileSystem::fileRemove( croppedPath );

	client.send( { { "id", 4 },
				   { "method", "ui.screenshot" },
				   { "params", { { "rect", { 300, 0, 40, 30 } } } } } );
	json invalidRect = client.receive();
	ASSERT_TRUE( invalidRect.contains( "error" ) );
	EXPECT_TRUE( invalidRect["error"]["code"] == "invalid-params" );
	client.send(
		{ { "id", 5 }, { "method", "ui.screenshot" }, { "params", { { "format", "gif" } } } } );
	json invalidFormat = client.receive();
	ASSERT_TRUE( invalidFormat.contains( "error" ) );
	EXPECT_TRUE( invalidFormat["error"]["code"] == "invalid-params" );
	UIInspectorServer::stop();
	Engine::destroySingleton();
}
