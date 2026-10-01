#include <eepp/ui/tools/uiinspectorserver.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifdef KEY_EXECUTE
#undef KEY_EXECUTE
#endif

#include <bcrypt.h>
#endif

#include <eepp/network/ipaddress.hpp>
#include <eepp/network/socketselector.hpp>
#include <eepp/network/tcplistener.hpp>
#include <eepp/network/tcpsocket.hpp>
#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/tools/uiinspector.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiconsole.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextnode.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uiwebview.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/widgetcommandexecuter.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <mutex>
#include <random>
#include <set>
#include <string_view>
#include <thread>

using json = nlohmann::json;
using namespace EE::Network;
using namespace EE::Scene;

namespace EE { namespace UI {

namespace {

struct InspectorError {
	std::string code;
	std::string message;
	json data;
};

void fail( const char* code, const std::string& message, json data = nullptr ) {
	throw InspectorError{ code, message, std::move( data ) };
}

json errorObject( const InspectorError& error ) {
	json value = { { "code", error.code }, { "message", error.message } };
	if ( !error.data.is_null() )
		value["data"] = error.data;
	return value;
}

std::string tokenFromOS() {
	std::array<Uint32, 8> values;
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( !BCRYPT_SUCCESS( BCryptGenRandom( nullptr, reinterpret_cast<PUCHAR>( values.data() ),
										   static_cast<ULONG>( sizeof( values ) ),
										   BCRYPT_USE_SYSTEM_PREFERRED_RNG ) ) )
		return {};
#else
	try {
		std::random_device random;
		if ( random.entropy() <= 0 )
			return {};
		for ( auto& value : values )
			value = random();
	} catch ( const std::exception& ) {
		return {};
	}
#endif
	static const char* hex = "0123456789abcdef";
	std::string token( 64, '0' );
	for ( size_t i = 0; i < values.size(); ++i ) {
		Uint32 value = values[i];
		for ( size_t j = 0; j < 8; ++j ) {
			token[i * 8 + j] = hex[value & 15];
			value >>= 4;
		}
	}
	return token;
}

std::string requireString( const json& params, const char* name ) {
	if ( !params.contains( name ) || !params[name].is_string() )
		fail( "invalid-params", std::string( name ) + " must be a string" );
	return params[name].get<std::string>();
}

unsigned boundedUnsigned( const json& params, const char* name, unsigned fallback,
						  unsigned maximum ) {
	if ( !params.contains( name ) )
		return fallback;
	if ( !params[name].is_number_unsigned() &&
		 !( params[name].is_number_integer() && params[name].get<Int64>() >= 0 ) )
		fail( "invalid-params", std::string( name ) + " must be non-negative" );
	auto number = params[name].get<Uint64>();
	if ( number > maximum )
		fail( "invalid-params", std::string( name ) + " exceeds limit" );
	return static_cast<unsigned>( number );
}

bool optionalBool( const json& params, const char* name, bool fallback ) {
	if ( !params.contains( name ) )
		return fallback;
	if ( !params[name].is_boolean() )
		fail( "invalid-params", std::string( name ) + " must be boolean" );
	return params[name].get<bool>();
}

json vector( const Vector2f& value ) {
	return json::array( { value.x, value.y } );
}

bool envEnabled( const char* value ) {
	return value && *value && std::string( value ) != "0";
}

bool validDelay( const std::string& value ) {
	size_t suffix = value.size() >= 2 && value.compare( value.size() - 2, 2, "ms" ) == 0 ? 2
					: value.size() >= 1 && value.back() == 's'							 ? 1
																						 : 0;
	if ( suffix == 0 || value.size() <= suffix || value[value.size() - suffix - 1] == '.' )
		return false;
	bool dot = false;
	for ( size_t i = 0; i < value.size() - suffix; ++i ) {
		if ( value[i] == '.' && !dot && i > 0 )
			dot = true;
		else if ( value[i] < '0' || value[i] > '9' )
			return false;
	}
	return true;
}

bool validSelector( const std::string& selector ) {
	auto first = selector.find_first_not_of( " \t\r\n" );
	if ( first == std::string::npos )
		return false;
	auto last = selector.find_last_not_of( " \t\r\n" );
	constexpr std::string_view combinators{ ">+~|" };
	if ( combinators.find( selector[first] ) != std::string_view::npos ||
		 combinators.find( selector[last] ) != std::string_view::npos )
		return false;
	int brackets = 0;
	int parens = 0;
	char quote = 0;
	for ( char character : selector ) {
		if ( quote ) {
			if ( character == quote )
				quote = 0;
			continue;
		}
		if ( character == '\'' || character == '"' )
			quote = character;
		else if ( character == '[' )
			++brackets;
		else if ( character == ']' ) {
			if ( --brackets < 0 )
				return false;
		} else if ( character == '(' )
			++parens;
		else if ( character == ')' ) {
			if ( --parens < 0 )
				return false;
		}
	}
	return !quote && brackets == 0 && parens == 0;
}

} // namespace

struct UIInspectorServer::Impl {
	struct Incoming {
		Uint64 client;
		std::string line;
		bool disconnected{ false };
	};
	struct Outgoing {
		Uint64 client;
		std::string line;
	};
	struct Client {
		Uint64 id;
		std::unique_ptr<TcpSocket> socket;
		std::string input;
		std::string output;
		bool authenticated{ false };
		bool closing{ false };
	};
	struct Subscription {
		Uint64 client;
		Uint64 sequence{ 0 };
		std::map<std::string, std::map<std::string, json>> values;
	};
	struct Pending {
		Uint64 client;
		json id;
		json commands;
		json results = json::object();
		std::map<std::string, json> named;
		size_t index{ 0 };
		unsigned frames{ 0 };
		bool standalone{ false };
		bool ready{ true };
		bool cancelled{ false };
	};

	UIInspector inspector;
	UIInspectorServerSettings settings;
	TcpListener listener;
	Uint16 port{ 0 };
	std::atomic<bool> running{ false };
	std::atomic<unsigned> authenticatedClients{ 0 };
	std::thread thread;
	std::mutex mutex;
	std::deque<Incoming> incoming;
	std::deque<Outgoing> outgoing;
	Uint64 nextClient{ 1 };
	Uint64 nextSubscription{ 1 };
	std::map<std::string, Subscription> subscriptions;
	std::vector<std::shared_ptr<Pending>> pending;
	Uint64 topologyRevision{ 0 };
	unsigned topologyPollFrames{ 0 };
	std::map<std::string, json> knownWindows;
	std::map<std::string, json> knownScenes;

	void enqueue( Uint64 client, const json& message ) {
		std::string line = message.dump() + '\n';
		std::lock_guard<std::mutex> lock( mutex );
		if ( outgoing.size() < 1024 )
			outgoing.push_back( { client, std::move( line ) } );
	}

	void broadcast( const json& message ) { enqueue( 0, message ); }

	void disconnect( Uint64 client ) {
		for ( auto it = subscriptions.begin(); it != subscriptions.end(); ) {
			if ( it->second.client == client )
				it = subscriptions.erase( it );
			else
				++it;
		}
		for ( auto& batch : pending ) {
			if ( batch->client == client )
				batch->cancelled = true;
		}
	}

	void networkLoop() {
		SocketSelector selector;
		selector.add( listener );
		std::map<Uint64, Client> clients;
		while ( running ) {
			{
				std::lock_guard<std::mutex> lock( mutex );
				while ( !outgoing.empty() ) {
					auto item = std::move( outgoing.front() );
					outgoing.pop_front();
					for ( auto& pair : clients ) {
						if ( ( item.client == 0 && pair.second.authenticated ) ||
							 item.client == pair.first ) {
							if ( pair.second.output.size() + item.line.size() > 4 * 1024 * 1024 )
								pair.second.closing = true;
							else
								pair.second.output += item.line;
						}
					}
				}
			}
			selector.wait( Milliseconds( 10 ) );
			if ( selector.isReady( listener ) && clients.size() < 16 ) {
				auto socket = std::make_unique<TcpSocket>();
				if ( listener.accept( *socket ) == Socket::Done ) {
					socket->setBlocking( false );
					Uint64 id = nextClient++;
					selector.add( *socket );
					clients.emplace( id, Client{ id, std::move( socket ) } );
				}
			}
			for ( auto it = clients.begin(); it != clients.end(); ) {
				Client& client = it->second;
				if ( selector.isReady( *client.socket ) ) {
					char buffer[8192];
					size_t length = 0;
					auto status = client.socket->receive( buffer, sizeof( buffer ), length );
					if ( status == Socket::Done ) {
						client.input.append( buffer, length );
						if ( client.input.size() > 1024 * 1024 &&
							 client.input.find( '\n' ) == std::string::npos ) {
							client.output +=
								json( { { "id", nullptr },
										{ "error", errorObject( { "request-too-large",
																  "Request line exceeds 1 MiB",
																  nullptr } ) } } )
									.dump() +
								'\n';
							client.closing = true;
						}
						while ( !client.closing ) {
							auto end = client.input.find( '\n' );
							if ( end == std::string::npos )
								break;
							if ( end > 1024 * 1024 ) {
								client.output +=
									json( { { "id", nullptr },
											{ "error", errorObject( { "request-too-large",
																	  "Request line exceeds 1 MiB",
																	  nullptr } ) } } )
										.dump() +
									'\n';
								client.closing = true;
								break;
							}
							std::string line = client.input.substr( 0, end );
							client.input.erase( 0, end + 1 );
							if ( !line.empty() && line.back() == '\r' )
								line.pop_back();
							json request = json::parse( line, nullptr, false );
							if ( !client.authenticated ) {
								json id = request.is_object() && request.contains( "id" )
											  ? request["id"]
											  : json( nullptr );
								bool isConnect = request.is_object() &&
												 request.contains( "method" ) &&
												 request["method"].is_string() &&
												 request["method"] == "session.connect";
								bool hasParams = isConnect && request.contains( "params" ) &&
												 request["params"].is_object();
								bool tokenMatches = hasParams &&
													request["params"].contains( "token" ) &&
													request["params"]["token"].is_string() &&
													request["params"]["token"] == settings.token;
								if ( !tokenMatches ) {
									const char* code = request.is_discarded() ? "parse-error"
													   : isConnect ? "authentication-failed"
																   : "unauthenticated";
									const char* message =
										request.is_discarded() ? "Malformed JSON"
										: isConnect ? "Invalid inspector token"
													: "First request must be session.connect";
									client.output +=
										json( { { "id", id },
												{ "error",
												  errorObject( { code, message, nullptr } ) } } )
											.dump() +
										'\n';
									client.closing = true;
									break;
								}
								if ( !request["params"].contains( "protocolVersion" ) ||
									 !request["params"]["protocolVersion"].is_number_integer() ||
									 request["params"]["protocolVersion"] != 1 ) {
									client.output +=
										json( { { "id", id },
												{ "error",
												  errorObject( { "unsupported-protocol",
																 "Protocol version 1 required",
																 nullptr } ) } } )
											.dump() +
										'\n';
									client.closing = true;
									break;
								}
								client.authenticated = true;
								authenticatedClients.fetch_add( 1 );
							}
							std::lock_guard<std::mutex> lock( mutex );
							if ( incoming.size() < 1024 )
								incoming.push_back( { client.id, std::move( line ) } );
							else
								client.closing = true;
						}
					} else if ( status != Socket::NotReady ) {
						client.closing = true;
					}
				}
				if ( !client.output.empty() ) {
					size_t sent = 0;
					auto status =
						client.socket->send( client.output.data(), client.output.size(), sent );
					client.output.erase( 0, sent );
					if ( status == Socket::Error || status == Socket::Disconnected ) {
						client.closing = true;
						client.output.clear();
					}
				}
				if ( client.closing && client.output.empty() ) {
					if ( client.authenticated )
						authenticatedClients.fetch_sub( 1 );
					selector.remove( *client.socket );
					std::lock_guard<std::mutex> lock( mutex );
					incoming.push_back( { client.id, {}, true } );
					it = clients.erase( it );
				} else {
					++it;
				}
			}
		}
	}

	UISceneNode* resolveScene( const json& params ) {
		if ( !params.contains( "scene" ) ) {
			auto* scene = inspector.defaultScene();
			if ( !scene )
				fail( "default-scene-unavailable", "Default scene was destroyed" );
			return scene;
		}
		auto* scene = inspector.scene( requireString( params, "scene" ) );
		if ( !scene )
			fail( "invalid-scene", "Scene handle is no longer valid" );
		return scene;
	}

	UIWidget* resolveWidget( const json& params ) {
		if ( params.contains( "handle" ) == params.contains( "selector" ) )
			fail( "invalid-params", "Specify exactly one of handle or selector" );
		if ( params.contains( "handle" ) ) {
			if ( params.contains( "scene" ) )
				fail( "invalid-params", "Handle already determines scene" );
			auto* widget = inspector.widget( requireString( params, "handle" ) );
			if ( !widget )
				fail( "invalid-widget", "Widget handle is no longer valid" );
			return widget;
		}
		auto* scene = resolveScene( params );
		std::string selector = requireString( params, "selector" );
		if ( !validSelector( selector ) )
			fail( "selector-invalid", "Malformed selector" );
		auto matches = inspector.query( scene, selector );
		if ( matches.empty() )
			fail( "target-not-found", "Selector matched no widgets" );
		if ( matches.size() != 1 )
			fail( "target-ambiguous", "Selector matched multiple widgets",
				  { { "count", matches.size() } } );
		return matches.front();
	}

	json boundedText( const String& content, size_t maxLength, bool* truncated ) {
		if ( content.size() > maxLength ) {
			if ( truncated )
				*truncated = true;
			return content.substr( 0, maxLength ).toUtf8();
		}
		return content.toUtf8();
	}

	json textValue( UIWidget* widget, size_t maxLength, bool* truncated = nullptr ) {
		if ( inspector.isSecret( widget ) )
			return "[redacted]";
		if ( widget->isType( UI_TYPE_TEXTVIEW ) )
			return boundedText( widget->asType<UITextView>()->getText(), maxLength, truncated );
		if ( widget->isType( UI_TYPE_TEXTNODE ) )
			return boundedText( widget->asType<UITextNode>()->getText(), maxLength, truncated );
		if ( !widget->isType( UI_TYPE_RICHTEXT ) )
			return nullptr;

		String content;
		auto append = [&]( const String& text ) {
			if ( content.size() <= maxLength ) {
				const size_t remaining = maxLength + 1 - content.size();
				content.append( text, 0, std::min( text.size(), remaining ) );
			}
		};
		if ( widget->isType( UI_TYPE_TEXTSPAN ) )
			append( widget->asType<UITextSpan>()->getText() );

		// Read HTML source text without forcing a RichText layout. Stop once the result is full or
		// the walk becomes too large for a compact inspection response.
		constexpr size_t maxVisitedNodes = 2048;
		size_t visited = 0;
		Scene::Node* node = widget->getFirstChild();
		while ( node && visited++ < maxVisitedNodes && content.size() <= maxLength ) {
			bool descend = true;
			if ( node->isWidget() ) {
				auto* child = node->asType<UIWidget>();
				if ( child->getUISceneNode() != widget->getUISceneNode() ||
					 inspector.isSecret( child ) || child->isType( UI_TYPE_WEBVIEW ) ||
					 child->isType( UI_TYPE_TEXTVIEW ) ) {
					descend = false;
				} else if ( child->isType( UI_TYPE_TEXTNODE ) ) {
					append( child->asType<UITextNode>()->getText() );
					descend = false;
				} else if ( child->isType( UI_TYPE_TEXTSPAN ) ) {
					append( child->asType<UITextSpan>()->getText() );
				}
			}
			Scene::Node* next = descend ? node->getFirstChild() : nullptr;
			if ( !next ) {
				while ( node != widget && !node->getNextNode() )
					node = node->getParent();
				next = node == widget ? nullptr : node->getNextNode();
			}
			node = next;
		}
		if ( node || content.size() > maxLength ) {
			if ( truncated )
				*truncated = true;
			return content.substr( 0, maxLength ).toUtf8();
		}
		return content.toUtf8();
	}

	json summary( UIWidget* widget, bool includeText = true ) {
		auto* scene = widget->getUISceneNode();
		auto bounds = widget->getWorldBounds();
		json pseudoClasses = json::array();
		for ( const char* name : widget->getStyleSheetPseudoClassesStrings() )
			pseudoClasses.push_back( name );
		json value = { { "handle", inspector.widgetHandle( widget ) },
					   { "scene", inspector.sceneHandle( scene ) },
					   { "tag", widget->getElementTag() },
					   { "id", widget->getId() },
					   { "classes", widget->getClasses() },
					   { "pseudoClasses", pseudoClasses },
					   { "boundsPx", json::array( { bounds.Left, bounds.Top, bounds.getWidth(),
													bounds.getHeight() } ) },
					   { "visible", widget->isVisible() },
					   { "enabled", widget->isEnabled() },
					   { "focused", scene && scene->getEventDispatcher() &&
										scene->getEventDispatcher()->getFocusNode() == widget } };
		if ( widget->isType( UI_TYPE_WEBVIEW ) ) {
			if ( auto* document = widget->asType<UIWebView>()->getDocumentSceneNode() )
				value["documentScene"] = inspector.sceneHandle( document );
		}
		if ( includeText ) {
			bool truncated = false;
			json content = textValue( widget, 256, &truncated );
			if ( !content.is_null() )
				value["text"] = std::move( content );
			if ( truncated )
				value["textTruncated"] = true;
		}
		return value;
	}

	json readProperty( UIWidget* widget, const std::string& property, size_t maxStringLength = 4096,
					   bool* truncated = nullptr ) {
		if ( property == "identity.tag" )
			return widget->getElementTag();
		if ( property == "identity.id" )
			return widget->getId();
		if ( property == "identity.classes" )
			return widget->getClasses();
		if ( property == "identity.pseudoClasses" )
			return widget->getStyleSheetPseudoClassesStrings();
		if ( property == "geometry.position" )
			return vector( widget->getPixelsPosition() );
		if ( property == "geometry.size" )
			return vector( widget->getPixelsSize() );
		if ( property == "geometry.worldPosition" ) {
			auto bounds = widget->getWorldBounds();
			return json::array( { bounds.Left, bounds.Top } );
		}
		if ( property == "geometry.bounds" ) {
			auto bounds = widget->getWorldBounds();
			return json::array(
				{ bounds.Left, bounds.Top, bounds.getWidth(), bounds.getHeight() } );
		}
		if ( property == "state.visible" )
			return widget->isVisible();
		if ( property == "state.enabled" )
			return widget->isEnabled();
		if ( property == "state.focused" ) {
			auto* scene = widget->getUISceneNode();
			return scene && scene->getEventDispatcher() &&
				   scene->getEventDispatcher()->getFocusNode() == widget;
		}
		if ( property == "content.text" ) {
			return textValue( widget, maxStringLength, truncated );
		}
		if ( property.compare( 0, 4, "css." ) == 0 ) {
			std::string name = property.substr( 4 );
			if ( CSS::StyleSheetSpecification::instance()->getProperty( name ) )
				return inspector.property( widget, name );
		}
		return nullptr;
	}

	std::vector<std::string> properties( UIWidget* widget, const json& params ) {
		std::vector<std::string> request;
		if ( !params.contains( "properties" ) )
			return { "identity.*", "geometry.*", "state.*" };
		if ( !params["properties"].is_array() )
			fail( "invalid-params", "properties must be an array" );
		for ( const auto& name : params["properties"] ) {
			if ( !name.is_string() )
				fail( "invalid-params", "property names must be strings" );
			request.push_back( name.get<std::string>() );
		}
		return request;
	}

	json inspectProperties( UIWidget* widget, const std::vector<std::string>& requested,
							json& unavailable, json& truncated, size_t maxStringLength ) {
		json values = json::object();
		std::vector<std::string> names;
		for ( const auto& name : requested ) {
			if ( name == "*" || name == "identity.*" ) {
				names.insert( names.end(), { "identity.tag", "identity.id", "identity.classes",
											 "identity.pseudoClasses" } );
			}
			if ( name == "*" || name == "geometry.*" ) {
				names.insert( names.end(), { "geometry.position", "geometry.worldPosition",
											 "geometry.size", "geometry.bounds" } );
			}
			if ( name == "*" || name == "state.*" ) {
				names.insert( names.end(), { "state.visible", "state.enabled", "state.focused" } );
			}
			if ( name == "*" || name == "content.*" )
				names.push_back( "content.text" );
			if ( name == "*" || name == "css.*" ) {
				for ( auto id : widget->getPropertiesImplemented() ) {
					auto* definition = CSS::StyleSheetSpecification::instance()->getProperty( id );
					if ( definition )
						names.push_back( "css." + definition->getName() );
				}
			}
			if ( name != "*" && name.find( '*' ) == std::string::npos )
				names.push_back( name );
		}
		for ( const auto& name : names ) {
			bool wasTruncated = false;
			json value = readProperty( widget, name, maxStringLength, &wasTruncated );
			if ( value.is_null() ) {
				unavailable.push_back( name );
			} else {
				if ( value.is_string() &&
					 value.get_ref<const std::string&>().size() > maxStringLength ) {
					String content = String::fromUtf8( value.get<std::string>() );
					if ( content.size() > maxStringLength ) {
						value = content.substr( 0, maxStringLength ).toUtf8();
						wasTruncated = true;
					}
				}
				if ( wasTruncated )
					truncated.push_back( name );
				values[name] = std::move( value );
			}
		}
		return values;
	}

	json contexts( bool sendEvents = true ) {
		json windows = json::array();
		json scenes = json::array();
		std::map<std::string, json> newWindows;
		std::map<std::string, json> newScenes;
		Window::Engine::instance()->forEachWindow( [&]( Window::Window* window ) {
			auto size = window->getSize();
			auto pos = window->getPosition();
			json entry = { { "handle", inspector.windowHandle( window ) },
						   { "id", window->getWindowID() },
						   { "title", window->getTitle() },
						   { "sizePx", json::array( { size.x, size.y } ) },
						   { "positionPx", json::array( { pos.x, pos.y } ) },
						   { "focused", window->hasFocus() } };
			newWindows.emplace( entry["handle"].get<std::string>(), entry );
			windows.push_back( std::move( entry ) );
		} );
		SmallVector<std::tuple<UISceneNode*, UISceneNode*, UIWidget*>, 8> pending;
		for ( auto* scene : inspector.topLevelScenes() )
			pending.emplace_back( scene, nullptr, nullptr );
		std::set<UISceneNode*> seen;
		while ( !pending.empty() ) {
			auto [scene, parent, owner] = pending.back();
			pending.pop_back();
			if ( !scene || !seen.insert( scene ).second )
				continue;
			if ( parent && !owner ) {
				for ( auto* child = scene->getParent(); child; child = child->getParent() ) {
					if ( child->isWidget() &&
						 child->asType<UIWidget>()->isType( UI_TYPE_WEBVIEW ) &&
						 child->asType<UIWebView>()->getDocumentSceneNode() == scene ) {
						owner = child->asType<UIWidget>();
						break;
					}
				}
			}
			json entry = {
				{ "handle", inspector.sceneHandle( scene ) },
				{ "window", scene->getWindow()
								? json( inspector.windowHandle( scene->getWindow() ) )
								: json( nullptr ) },
				{ "kind", !parent									  ? "top-level"
						  : owner && owner->isType( UI_TYPE_WEBVIEW ) ? "webview"
																	  : "nested" },
				{ "parentScene",
				  parent ? json( inspector.sceneHandle( parent ) ) : json( nullptr ) },
				{ "owner", owner ? json( inspector.widgetHandle( owner ) ) : json( nullptr ) },
				{ "root", scene->getRoot() ? json( inspector.widgetHandle( scene->getRoot() ) )
										   : json( nullptr ) } };
			if ( owner && owner->isType( UI_TYPE_WEBVIEW ) )
				entry["uri"] = owner->asType<UIWebView>()->getCurrentURI().toString();
			newScenes.emplace( entry["handle"].get<std::string>(), entry );
			scenes.push_back( std::move( entry ) );
			for ( auto* nested : scene->getChildUISceneNodes() ) {
				pending.emplace_back( nested, scene, nullptr );
			}
		}
		if ( sendEvents ) {
			for ( const auto& [handle, value] : newWindows ) {
				if ( !knownWindows.count( handle ) )
					broadcast(
						{ { "method", "ui.windowCreated" },
						  { "params",
							{ { "revision", ++topologyRevision }, { "window", value } } } } );
			}
			for ( const auto& [handle, value] : newScenes ) {
				if ( !knownScenes.count( handle ) )
					broadcast( { { "method", "ui.sceneCreated" },
								 { "params",
								   { { "revision", ++topologyRevision }, { "scene", value } } } } );
			}
			for ( const auto& [handle, value] : knownScenes ) {
				if ( !newScenes.count( handle ) )
					broadcast(
						{ { "method", "ui.sceneDestroyed" },
						  { "params",
							{ { "revision", ++topologyRevision }, { "scene", handle } } } } );
			}
			for ( const auto& [handle, value] : knownWindows ) {
				if ( !newWindows.count( handle ) )
					broadcast(
						{ { "method", "ui.windowDestroyed" },
						  { "params",
							{ { "revision", ++topologyRevision }, { "window", handle } } } } );
			}
		}
		knownWindows = std::move( newWindows );
		knownScenes = std::move( newScenes );
		std::string defaultScene =
			inspector.defaultScene() ? inspector.sceneHandle( inspector.defaultScene() ) : "";
		json defaultWindow =
			inspector.defaultScene() && inspector.defaultScene()->getWindow()
				? json( inspector.windowHandle( inspector.defaultScene()->getWindow() ) )
				: json( nullptr );
		return { { "revision", topologyRevision },
				 { "defaultScene", defaultScene.empty() ? json( nullptr ) : json( defaultScene ) },
				 { "defaultWindow", defaultWindow },
				 { "windows", windows },
				 { "scenes", scenes } };
	}

	KeyBindings* widgetKeyBindings( UIWidget* widget ) {
		if ( widget->isType( UI_TYPE_TEXTINPUT ) )
			return &widget->asType<UITextInput>()->getKeyBindings();
		if ( widget->isType( UI_TYPE_CODEEDITOR ) )
			return &widget->asType<UICodeEditor>()->getKeyBindings();
		if ( widget->isType( UI_TYPE_WINDOW ) )
			return &widget->asType<UIWindow>()->getKeyBindings();
		if ( widget->isType( UI_TYPE_CONSOLE ) )
			return &widget->asType<UIConsole>()->getKeyBindings();
		if ( auto* executer = dynamic_cast<WidgetCommandExecuter*>( widget ) )
			return &executer->getKeyBindings();
		return nullptr;
	}

	json execute( Uint64 client, const std::string& method, const json& params ) {
		if ( method == "session.connect" ) {
			auto context = contexts( false );
			json capabilities = json::array( { "ui.contexts", "ui.query", "ui.tree", "ui.inspect",
											   "ui.focus", "ui.screenshot", "ui.keybindings",
											   "session.batch", "session.nextFrame",
											   "watch.properties", "events.context-lifecycle" } );
			if ( !settings.readOnly ) {
				capabilities.push_back( "input.click" );
				capabilities.push_back( "input.key" );
				capabilities.push_back( "input.text" );
			}
			return { { "protocolVersion", 1 },
					 { "application",
					   context["windows"].empty() ? "eepp" : context["windows"][0]["title"] },
					 { "pid", Sys::getProcessID() },
					 { "readOnly", settings.readOnly },
					 { "defaultWindow", context["defaultWindow"] },
					 { "defaultScene", context["defaultScene"] },
					 { "capabilities", capabilities } };
		}
		if ( method == "ui.contexts" )
			return contexts();
		if ( method == "ui.screenshot" ) {
			if ( params.contains( "window" ) && params.contains( "scene" ) )
				fail( "invalid-params", "Specify window or scene, not both" );
			UISceneNode* scene = nullptr;
			EE::Window::Window* window = nullptr;
			if ( params.contains( "window" ) ) {
				window = inspector.window( requireString( params, "window" ) );
				if ( !window )
					fail( "invalid-window", "Window handle is no longer valid" );
			} else {
				scene = resolveScene( params );
				window = scene->getWindow();
			}
			if ( !window )
				fail( "screenshot-failed", "Scene has no native window" );

			std::string format =
				params.contains( "format" ) ? requireString( params, "format" ) : "png";
			const auto saveType = Graphics::Image::extensionToSaveType( format );
			if ( saveType == Graphics::Image::SaveType::Unknown ||
				 saveType == Graphics::Image::SaveType::DDS )
				fail( "invalid-params", "Unsupported screenshot format" );
			format = Graphics::Image::saveTypeToExtension( saveType );

			const int width = static_cast<int>( window->getWidth() );
			const int height = static_cast<int>( window->getHeight() );
			if ( width <= 0 || height <= 0 )
				fail( "screenshot-failed", "Window has no drawable area" );
			int x = 0, y = 0, captureWidth = width, captureHeight = height;
			if ( params.contains( "rect" ) ) {
				const auto& rect = params["rect"];
				if ( !rect.is_array() || rect.size() != 4 )
					fail( "invalid-params", "rect must be [x,y,width,height]" );
				for ( const auto& value : rect ) {
					if ( !value.is_number_integer() )
						fail( "invalid-params", "rect values must be integers" );
				}
				const Int64 rx = rect[0].get<Int64>();
				const Int64 ry = rect[1].get<Int64>();
				const Int64 rw = rect[2].get<Int64>();
				const Int64 rh = rect[3].get<Int64>();
				if ( rx < 0 || ry < 0 || rw <= 0 || rh <= 0 || rx >= width || ry >= height ||
					 rw > width - rx || rh > height - ry )
					fail( "invalid-params", "rect must fit inside the native window" );
				x = static_cast<int>( rx );
				y = static_cast<int>( ry );
				captureWidth = static_cast<int>( rw );
				captureHeight = static_cast<int>( rh );
			} else if ( params.contains( "scene" ) ) {
				const auto& bounds = scene->getVisibleWorldBounds();
				const int left =
					std::clamp( static_cast<int>( std::floor( bounds.Left ) ), 0, width );
				const int top =
					std::clamp( static_cast<int>( std::floor( bounds.Top ) ), 0, height );
				const int right =
					std::clamp( static_cast<int>( std::ceil( bounds.Right ) ), 0, width );
				const int bottom =
					std::clamp( static_cast<int>( std::ceil( bounds.Bottom ) ), 0, height );
				x = left;
				y = top;
				captureWidth = right - left;
				captureHeight = bottom - top;
				if ( captureWidth <= 0 || captureHeight <= 0 )
					fail( "screenshot-failed", "Scene is outside the native window" );
			}

			const std::string suffix = tokenFromOS();
			if ( suffix.empty() )
				fail( "screenshot-failed", "Could not generate a temporary screenshot name" );
			const std::string path = Sys::getTempPath() + "eepp-inspector-" +
									 std::to_string( Sys::getProcessID() ) + "-" + suffix + "." +
									 format;
			bool saved = false;
			{
				auto context = Engine::instance()->makeWindowCurrent( window );
				window->clear();
				SceneManager::instance()->draw( window );
				if ( x == 0 && y == 0 && captureWidth == width && captureHeight == height ) {
					saved = window->takeScreenshot( path, saveType );
				} else {
					auto image = window->getFrontBufferImage();
					auto* cropped = image.crop( Rect( x, y, x + captureWidth, y + captureHeight ) );
					if ( cropped ) {
						saved = cropped->saveToFile( path, saveType );
						eeDelete( cropped );
					}
				}
			}
			// The application still needs to present the frame on its normal draw path.
			SceneManager::instance()->forEachSceneNode( [&]( SceneNode* node ) {
				if ( node->getWindow() == window )
					node->invalidate( nullptr );
			} );
			if ( !saved ) {
				FileSystem::fileRemove( path );
				fail( "screenshot-failed", "Could not capture or save the screenshot" );
			}
			return { { "path", path },
					 { "format", format },
					 { "window", inspector.windowHandle( window ) },
					 { "scene", scene ? json( inspector.sceneHandle( scene ) ) : json( nullptr ) },
					 { "rectPx", json::array( { x, y, captureWidth, captureHeight } ) },
					 { "sizePx", json::array( { captureWidth, captureHeight } ) } };
		}
		if ( method == "ui.query" ) {
			auto* scene = resolveScene( params );
			std::string selector = requireString( params, "selector" );
			if ( !validSelector( selector ) )
				fail( "selector-invalid", "Malformed selector" );
			auto widgets = inspector.query( scene, selector );
			unsigned offset = boundedUnsigned( params, "offset", 0, 1000000 );
			unsigned limit = boundedUnsigned( params, "limit", 50, 1000 );
			unsigned maxStringLength = boundedUnsigned( params, "maxStringLength", 4096, 65536 );
			json nodes = json::array();
			for ( size_t i = offset; i < widgets.size() && nodes.size() < limit; ++i ) {
				json node = summary( widgets[i] );
				if ( params.contains( "properties" ) ) {
					json unavailable = json::array();
					json truncated = json::array();
					node["properties"] =
						inspectProperties( widgets[i], properties( widgets[i], params ),
										   unavailable, truncated, maxStringLength );
					if ( !unavailable.empty() )
						node["unavailable"] = unavailable;
					if ( !truncated.empty() )
						node["truncatedProperties"] = truncated;
				}
				nodes.push_back( std::move( node ) );
			}
			return { { "scene", inspector.sceneHandle( scene ) },
					 { "selector", selector },
					 { "total", widgets.size() },
					 { "offset", offset },
					 { "returned", nodes.size() },
					 { "truncated", offset + nodes.size() < widgets.size() },
					 { "nodes", nodes } };
		}
		if ( method == "ui.inspect" ) {
			auto* widget = resolveWidget( params );
			json unavailable = json::array();
			json truncated = json::array();
			unsigned maxStringLength = boundedUnsigned( params, "maxStringLength", 4096, 65536 );
			json values = inspectProperties( widget, properties( widget, params ), unavailable,
											 truncated, maxStringLength );
			json result = { { "handle", inspector.widgetHandle( widget ) },
							{ "scene", inspector.sceneHandle( widget->getUISceneNode() ) },
							{ "properties", std::move( values ) },
							{ "unavailable", unavailable } };
			if ( !truncated.empty() )
				result["truncatedProperties"] = truncated;
			return result;
		}
		if ( method == "ui.keybindings" ) {
			auto* widget = params.contains( "handle" ) || params.contains( "selector" )
							   ? resolveWidget( params )
							   : nullptr;
			auto* scene = widget ? widget->getUISceneNode() : resolveScene( params );
			auto* bindings = widget ? widgetKeyBindings( widget ) : &scene->getKeyBindings();
			unsigned offset = boundedUnsigned( params, "offset", 0, 1000000 );
			unsigned limit = boundedUnsigned( params, "limit", 50, 1000 );
			json entries = json::array();
			const size_t total = bindings ? bindings->getShortcutMap().size() : 0;
			if ( bindings ) {
				const auto& shortcuts = bindings->getShortcutMap();
				const auto ordered = KeyBindings::getOrderedShortcuts( shortcuts );
				for ( size_t i = offset; i < ordered.size() && entries.size() < limit; ++i ) {
					const auto& shortcut = ordered[i];
					entries.push_back( { { "shortcut", bindings->getShortcutString( shortcut ) },
										 { "command", shortcuts.find( shortcut )->second },
										 { "keycode", shortcut.key },
										 { "mod", shortcut.mod } } );
				}
			}
			return {
				{ "scene", inspector.sceneHandle( scene ) },
				{ "handle", widget ? json( inspector.widgetHandle( widget ) ) : json( nullptr ) },
				{ "supported", bindings != nullptr },
				{ "total", total },
				{ "offset", offset },
				{ "returned", entries.size() },
				{ "truncated", offset + entries.size() < total },
				{ "bindings", std::move( entries ) } };
		}
		if ( method == "ui.focus" ) {
			auto* scene = resolveScene( params );
			auto* focus =
				scene->getEventDispatcher() ? scene->getEventDispatcher()->getFocusNode() : nullptr;
			return { { "scene", inspector.sceneHandle( scene ) },
					 { "widget", focus && focus->isWidget() &&
										 focus->asType<UIWidget>()->getUISceneNode() == scene
									 ? json( summary( focus->asType<UIWidget>(), false ) )
									 : json( nullptr ) } };
		}
		if ( method == "ui.tree" ) {
			UIWidget* root = nullptr;
			UISceneNode* scene = nullptr;
			if ( params.contains( "root" ) ) {
				if ( params.contains( "scene" ) )
					fail( "invalid-params", "root determines scene" );
				root = inspector.widget( requireString( params, "root" ) );
				if ( !root )
					fail( "invalid-widget", "Tree root no longer exists" );
				scene = root->getUISceneNode();
			} else {
				scene = resolveScene( params );
				root = scene->getRoot();
			}
			unsigned depth = boundedUnsigned( params, "depth", 3, 32 );
			unsigned maxNodes = boundedUnsigned( params, "maxNodes", 200, 1000 );
			bool includeText = optionalBool( params, "includeText", false );
			json nodes = json::array();
			SmallVector<std::tuple<UIWidget*, UIWidget*, unsigned>, 8> stack{
				{ root, nullptr, 0 } };
			while ( !stack.empty() && nodes.size() < maxNodes ) {
				auto [widget, parent, level] = stack.back();
				stack.pop_back();
				if ( !widget || widget->getUISceneNode() != scene )
					continue;
				json node = summary( widget, includeText );
				node.erase( "boundsPx" );
				node.erase( "visible" );
				node.erase( "enabled" );
				node.erase( "focused" );
				node["parent"] =
					parent ? json( inspector.widgetHandle( parent ) ) : json( nullptr );
				node["children"] = json::array();
				if ( level < depth ) {
					SmallVector<UIWidget*, 8> children;
					for ( auto* child = widget->getFirstChild(); child;
						  child = child->getNextNode() ) {
						if ( child->isWidget() &&
							 child->asType<UIWidget>()->getUISceneNode() == scene ) {
							auto* childWidget = child->asType<UIWidget>();
							children.push_back( childWidget );
							node["children"].push_back( inspector.widgetHandle( childWidget ) );
						}
					}
					for ( auto it = children.rbegin(); it != children.rend(); ++it )
						stack.emplace_back( *it, widget, level + 1 );
				}
				nodes.push_back( std::move( node ) );
			}
			return { { "scene", inspector.sceneHandle( scene ) },
					 { "root", inspector.widgetHandle( root ) },
					 { "truncated", !stack.empty() },
					 { "nodes", nodes } };
		}
		if ( method == "watch.subscribe" ) {
			if ( !params.contains( "properties" ) || !params["properties"].is_array() ||
				 params["properties"].empty() )
				fail( "invalid-params", "Explicit properties are required" );
			WidgetQueryResult targets;
			if ( params.contains( "handle" ) )
				targets.push_back( resolveWidget( params ) );
			else {
				std::string selector = requireString( params, "selector" );
				if ( !validSelector( selector ) )
					fail( "selector-invalid", "Malformed selector" );
				targets = inspector.query( resolveScene( params ), selector );
			}
			if ( targets.empty() )
				fail( "target-not-found", "No watch targets" );
			bool initial = optionalBool( params, "initial", true );
			std::string handle = "sub:" + std::to_string( nextSubscription++ );
			Subscription sub{ client };
			json resultTargets = json::array();
			for ( auto* widget : targets ) {
				json values = json::object();
				for ( const auto& property : params["properties"] ) {
					if ( !property.is_string() ||
						 property.get<std::string>().find( '*' ) != std::string::npos )
						fail( "invalid-params", "Watches require explicit property names" );
					std::string name = property.get<std::string>();
					json value = readProperty( widget, name );
					if ( value.is_null() )
						fail( "property-unknown", "Unavailable watch property: " + name );
					values[name] = value;
					sub.values[inspector.widgetHandle( widget )][name] = std::move( value );
				}
				json target = { { "handle", inspector.widgetHandle( widget ) } };
				if ( initial )
					target["values"] = std::move( values );
				resultTargets.push_back( std::move( target ) );
			}
			subscriptions.emplace( handle, std::move( sub ) );
			return { { "subscription", handle }, { "targets", resultTargets } };
		}
		if ( method == "watch.unsubscribe" ) {
			std::string handle = requireString( params, "subscription" );
			auto it = subscriptions.find( handle );
			if ( it == subscriptions.end() || it->second.client != client )
				fail( "invalid-params", "Unknown subscription" );
			subscriptions.erase( it );
			return { { "unsubscribed", true } };
		}
		if ( method.compare( 0, 6, "input." ) == 0 ) {
			if ( settings.readOnly )
				fail( "permission-denied", "Inspector is read-only" );
			UISceneNode* scene = nullptr;
			UIWidget* target = nullptr;
			if ( method == "input.click" ) {
				target = resolveWidget( params );
				scene = target->getUISceneNode();
				if ( !target->hasVisibility() || !target->isEnabled() )
					fail( "target-not-interactable", "Target is invisible or disabled" );
			} else {
				scene = resolveScene( params );
			}
			if ( !scene || !scene->getWindow() || !scene->getEventDispatcher() )
				fail( "input-unavailable", "Scene has no input context" );
			auto context = scene->makeCurrent();
			auto* window = scene->getWindow();
			auto* input = window->getInput();
			if ( !input )
				fail( "input-unavailable", "Window has no input" );
			const std::string sceneHandle = inspector.sceneHandle( scene );
			const std::string windowHandle = inspector.windowHandle( window );
			if ( method == "input.click" ) {
				const std::string targetHandle = inspector.widgetHandle( target );
				if ( params.contains( "button" ) && !params["button"].is_string() )
					fail( "invalid-params", "button must be a string" );
				const std::string button = params.value( "button", "left" );
				if ( button != "left" && button != "middle" && button != "right" )
					fail( "invalid-params", "button must be left, middle, or right" );
				const Uint8 inputButton = button == "right"	   ? EE_BUTTON_RIGHT
										  : button == "middle" ? EE_BUTTON_MIDDLE
															   : EE_BUTTON_LEFT;
				unsigned count = boundedUnsigned( params, "count", 1, 2 );
				auto bounds = target->getWorldBounds();
				Vector2i point( static_cast<int>( ( bounds.Left + bounds.Right ) * 0.5f ),
								static_cast<int>( ( bounds.Top + bounds.Bottom ) * 0.5f ) );
				auto windowSize = window->getSize();
				if ( bounds.getWidth() <= 0 || bounds.getHeight() <= 0 || point.x < 0 ||
					 point.y < 0 || point.x >= windowSize.x || point.y >= windowSize.y )
					fail( "target-not-interactable", "Target is outside the window" );
				auto* hit =
					scene->getEventDispatcher()->getSceneNode()->overFind( point.asFloat() );
				bool hitsTarget = false;
				for ( auto* node = hit; node; node = node->getParent() ) {
					if ( node == target ) {
						hitsTarget = true;
						break;
					}
				}
				if ( !hitsTarget )
					fail( "target-not-interactable", "Target is clipped or covered" );
				InputEvent motion( InputEvent::MouseMotion );
				motion.WinID = window->getWindowID();
				motion.motion = {};
				motion.motion.x = point.x;
				motion.motion.y = point.y;
				input->pushEvent( motion );
				for ( unsigned i = 0; i < count; ++i ) {
					if ( !inspector.scene( sceneHandle ) || !inspector.window( windowHandle ) ||
						 !inspector.widget( targetHandle ) )
						fail( "target-not-interactable", "Target disappeared during click" );
					InputEvent press( InputEvent::MouseButtonDown );
					press.WinID = window->getWindowID();
					press.button = {};
					press.button.button = inputButton;
					press.button.x = point.x;
					press.button.y = point.y;
					input->pushEvent( press );
					if ( inspector.scene( sceneHandle ) && inspector.window( windowHandle ) )
						scene->getEventDispatcher()->update( Time::Zero );
					else
						fail( "input-unavailable", "Input context closed during click" );
					InputEvent release( InputEvent::MouseButtonUp );
					release.WinID = window->getWindowID();
					release.button = press.button;
					input->pushEvent( release );
					if ( inspector.scene( sceneHandle ) && inspector.window( windowHandle ) )
						scene->getEventDispatcher()->update( Time::Zero );
				}
				return { { "target", targetHandle },
						 { "scene", sceneHandle },
						 { "pointPx", json::array( { point.x, point.y } ) } };
			}
			if ( method == "input.key" ) {
				std::string name = requireString( params, "key" );
				Keycode key = input->getKeyFromName( name == "Enter" ? "Return" : name );
				if ( key == 0 )
					fail( "invalid-params", "Unknown key name" );
				if ( params.contains( "action" ) && !params["action"].is_string() )
					fail( "invalid-params", "action must be a string" );
				std::string action = params.value( "action", "press" );
				if ( action != "press" && action != "down" && action != "up" )
					fail( "invalid-params", "Unknown key action" );
				Uint32 modifiers = 0;
				if ( params.contains( "modifiers" ) ) {
					if ( !params["modifiers"].is_array() )
						fail( "invalid-params", "modifiers must be array" );
					for ( const auto& mod : params["modifiers"] ) {
						if ( mod == "Ctrl" )
							modifiers |= KEYMOD_LCTRL;
						else if ( mod == "Shift" )
							modifiers |= KEYMOD_LSHIFT;
						else if ( mod == "Alt" )
							modifiers |= KEYMOD_LALT;
						else if ( mod == "Meta" )
							modifiers |= KEYMOD_LMETA;
						else
							fail( "invalid-params", "Unknown modifier" );
					}
				}
				auto sendKey = [&]( InputEvent::EventType type ) {
					InputEvent event( type );
					event.WinID = window->getWindowID();
					event.key = {};
					event.key.keysym.sym = key;
					event.key.keysym.scancode = input->getScancodeFromKey( key );
					event.key.keysym.mod = modifiers;
					input->pushEvent( event );
				};
				if ( action != "up" )
					sendKey( InputEvent::KeyDown );
				if ( action != "down" && inspector.window( windowHandle ) )
					sendKey( InputEvent::KeyUp );
				return { { "key", name }, { "action", action } };
			}
			if ( method == "input.text" ) {
				std::string content = requireString( params, "text" );
				if ( content.size() > 65536 )
					fail( "invalid-params", "Text exceeds 64 KiB" );
				for ( auto character : String::fromUtf8( content ) ) {
					if ( !inspector.window( windowHandle ) )
						fail( "input-unavailable", "Window closed during text input" );
					InputEvent event( InputEvent::TextInput );
					event.WinID = window->getWindowID();
					event.text = {};
					event.text.text = character;
					input->pushEvent( event );
				}
				return { { "characters", String::fromUtf8( content ).size() } };
			}
		}
		fail( "method-not-found", "Unknown inspector method: " + method );
		return nullptr;
	}

	bool resolveReferences( json& value, const std::map<std::string, json>& named ) {
		if ( value.is_object() && value.size() == 1 && value.contains( "$ref" ) ) {
			if ( !value["$ref"].is_string() )
				fail( "batch-reference-error", "Reference must be a string" );
			std::string reference = value["$ref"].get<std::string>();
			auto hash = reference.find( '#' );
			if ( hash == std::string::npos )
				fail( "batch-reference-error", "Reference needs # pointer" );
			auto it = named.find( reference.substr( 0, hash ) );
			if ( it == named.end() )
				fail( "batch-reference-error", "Unknown or forward command reference" );
			try {
				value = it->second.at( json::json_pointer( reference.substr( hash + 1 ) ) );
			} catch ( const json::exception& ) {
				fail( "batch-reference-error", "Invalid result pointer" );
			}
			return true;
		} else if ( value.is_array() || value.is_object() ) {
			bool referenced = false;
			for ( auto& item : value )
				referenced |= resolveReferences( item, named );
			return referenced;
		}
		return false;
	}

	void advanceBatch( const std::shared_ptr<Pending>& batch ) {
		while ( !batch->cancelled && batch->ready && batch->index < batch->commands.size() ) {
			size_t index = batch->index;
			const json& original = batch->commands[index];
			std::string name;
			try {
				if ( !original.is_object() || !original.contains( "method" ) ||
					 !original["method"].is_string() )
					fail( "invalid-params", "Batch command needs method" );
				if ( original.contains( "name" ) && !original["name"].is_string() )
					fail( "invalid-params", "Batch command name must be a string" );
				name = original.value( "name", "" );
				if ( !name.empty() && batch->named.count( name ) )
					fail( "invalid-params", "Duplicate command name" );
				std::string method = original["method"].get<std::string>();
				if ( method == "session.batch" || method == "session.connect" )
					fail( "invalid-params", "Nested batch/connect forbidden" );
				json params = original.value( "params", json::object() );
				if ( !params.is_object() )
					fail( "invalid-params", "Command params must be object" );
				bool referenced = resolveReferences( params, batch->named );
				Time duration = Time::Zero;
				if ( original.contains( "delay" ) ) {
					std::string delay = requireString( original, "delay" );
					if ( !validDelay( delay ) )
						fail( "invalid-params", "Invalid delay" );
					duration = Time::fromString( delay );
					if ( duration.asMilliseconds() < 0 || duration.asMilliseconds() > 60000 )
						fail( "invalid-params", "Delay exceeds 60 seconds" );
				}
				json result;
				if ( method == "session.nextFrame" ) {
					unsigned count = boundedUnsigned( params, "count", 1, 10 );
					if ( count == 0 )
						fail( "invalid-params", "Frame count must be positive" );
					batch->frames = count + 1;
					result = { { "frames", count } };
				} else {
					try {
						result = execute( batch->client, method, params );
					} catch ( const InspectorError& error ) {
						if ( referenced && error.code == "invalid-params" )
							fail( "batch-reference-error", "Referenced value is incompatible" );
						throw;
					}
				}
				if ( !name.empty() )
					batch->named.emplace( name, result );
				if ( optionalBool( original, "return", true ) )
					batch->results[name.empty() ? std::to_string( index ) : name] = result;
				++batch->index;
				if ( original.contains( "delay" ) ) {
					batch->ready = false;
					UISceneNode* scene = inspector.defaultScene();
					if ( !scene ) {
						auto scenes = inspector.topLevelScenes();
						if ( !scenes.empty() )
							scene = scenes.front();
					}
					if ( scene ) {
						scene->getRoot()->setTimeout( [batch]() { batch->ready = true; },
													  duration );
					} else {
						fail( "default-scene-unavailable", "Cannot schedule delay" );
					}
					break;
				}
				if ( batch->frames ) {
					batch->ready = false;
					break;
				}
			} catch ( const InspectorError& error ) {
				enqueue( batch->client,
						 { { "id", batch->id },
						   { "error",
							 errorObject( { "batch-command-failed",
											"Batch command failed",
											{ { "index", index },
											  { "name", name },
											  { "commandError", errorObject( error ) } } } ) } } );
				batch->cancelled = true;
			}
		}
		if ( !batch->cancelled && batch->index == batch->commands.size() && batch->ready &&
			 batch->frames == 0 ) {
			enqueue(
				batch->client,
				{ { "id", batch->id },
				  { "result", batch->standalone ? batch->results.value( "0", json::object() )
												: json( { { "results", batch->results } } ) } } );
			batch->cancelled = true;
		}
	}

	void handleRequest( Uint64 client, const std::string& line ) {
		json request = json::parse( line, nullptr, false );
		json id = request.is_object() && request.contains( "id" ) ? request["id"] : json( nullptr );
		try {
			if ( request.is_discarded() )
				fail( "parse-error", "Malformed JSON" );
			if ( !request.is_object() || ( !id.is_number_unsigned() &&
										   !( id.is_number_integer() && id.get<Int64>() >= 0 ) ) )
				fail( "invalid-request", "Request needs a non-negative integer id" );
			if ( !request.contains( "method" ) || !request["method"].is_string() )
				fail( "invalid-request", "Request needs method" );
			json params = request.value( "params", json::object() );
			if ( !params.is_object() )
				fail( "invalid-request", "params must be an object" );
			std::string method = request["method"].get<std::string>();
			if ( request.contains( "delay" ) && method != "session.batch" ) {
				json command = request;
				command.erase( "id" );
				auto batch = std::make_shared<Pending>();
				batch->client = client;
				batch->id = id;
				batch->commands = json::array( { command } );
				batch->standalone = true;
				pending.push_back( batch );
				advanceBatch( batch );
				return;
			}
			if ( method == "session.batch" ) {
				if ( !params.contains( "commands" ) || !params["commands"].is_array() ||
					 params["commands"].size() > 100 )
					fail( "invalid-params", "Batch requires at most 100 commands" );
				auto batch = std::make_shared<Pending>();
				batch->client = client;
				batch->id = id;
				batch->commands = params["commands"];
				pending.push_back( batch );
				advanceBatch( batch );
				return;
			}
			if ( method == "session.nextFrame" ) {
				auto batch = std::make_shared<Pending>();
				batch->client = client;
				batch->id = id;
				batch->commands = json::array( { { { "method", method }, { "params", params } } } );
				batch->standalone = true;
				pending.push_back( batch );
				advanceBatch( batch );
				return;
			}
			json result = execute( client, method, params );
			enqueue( client, { { "id", id }, { "result", result } } );
		} catch ( const InspectorError& error ) {
			enqueue( client, { { "id", id }, { "error", errorObject( error ) } } );
		} catch ( const std::exception& ) {
			enqueue( client,
					 { { "id", id },
					   { "error", errorObject( { "internal-error", "Inspector command failed",
												 nullptr } ) } } );
		}
	}

	void updateWatches() {
		for ( auto it = subscriptions.begin(); it != subscriptions.end(); ) {
			auto& sub = it->second;
			for ( auto target = sub.values.begin(); target != sub.values.end(); ) {
				auto* widget = inspector.widget( target->first );
				if ( !widget ) {
					enqueue( sub.client, { { "method", "watch.targetRemoved" },
										   { "params",
											 { { "subscription", it->first },
											   { "target", target->first },
											   { "reason", "target-destroyed" } } } } );
					target = sub.values.erase( target );
					continue;
				}
				json changes = json::object();
				for ( auto& [name, old] : target->second ) {
					json current = readProperty( widget, name );
					if ( current != old ) {
						changes[name] = { { "old", old }, { "new", current } };
						old = std::move( current );
					}
				}
				if ( !changes.empty() )
					enqueue( sub.client, { { "method", "watch.changed" },
										   { "params",
											 { { "subscription", it->first },
											   { "sequence", ++sub.sequence },
											   { "target", target->first },
											   { "changes", changes } } } } );
				++target;
			}
			if ( sub.values.empty() ) {
				enqueue( sub.client,
						 { { "method", "watch.ended" },
						   { "params",
							 { { "subscription", it->first }, { "reason", "no-targets" } } } } );
				it = subscriptions.erase( it );
			} else
				++it;
		}
	}

	void pump() {
		std::deque<Incoming> work;
		{
			std::lock_guard<std::mutex> lock( mutex );
			work.swap( incoming );
		}
		for ( const auto& item : work ) {
			if ( item.disconnected )
				disconnect( item.client );
			else
				handleRequest( item.client, item.line );
		}
		for ( auto& batch : pending ) {
			if ( batch->cancelled )
				continue;
			if ( batch->frames ) {
				if ( --batch->frames == 0 ) {
					batch->ready = true;
					advanceBatch( batch );
				}
			} else if ( batch->ready ) {
				advanceBatch( batch );
			}
		}
		pending.erase( std::remove_if( pending.begin(), pending.end(),
									   []( const auto& item ) { return item->cancelled; } ),
					   pending.end() );
		if ( !subscriptions.empty() )
			updateWatches();
		if ( authenticatedClients.load() && ++topologyPollFrames >= 30 ) {
			topologyPollFrames = 0;
			contexts();
		}
	}
};

UIInspectorServer* UIInspectorServer::sInstance = nullptr;

UIInspectorServer::UIInspectorServer() : mImpl( std::make_unique<Impl>() ) {}

UIInspectorServer::~UIInspectorServer() = default;

UIInspectorServer* UIInspectorServer::instance() {
	return sInstance;
}

bool UIInspectorServer::start( UISceneNode* defaultScene, UIInspectorServerSettings settings ) {
	if ( sInstance )
		return false;
	auto server = std::unique_ptr<UIInspectorServer>( new UIInspectorServer() );
	bool generated = settings.token.empty();
	if ( generated )
		settings.token = tokenFromOS();
	if ( settings.token.empty() )
		return false;
	server->mImpl->settings = std::move( settings );
	server->mImpl->inspector.setDefaultScene( defaultScene );
	if ( server->mImpl->listener.listen( server->mImpl->settings.port,
										 IpAddress( server->mImpl->settings.address ) ) !=
		 Socket::Done )
		return false;
	server->mImpl->port = server->mImpl->listener.getLocalPort();
	server->mImpl->listener.setBlocking( false );
	server->mImpl->running = true;
	server->mImpl->thread = std::thread( [&impl = *server->mImpl]() { impl.networkLoop(); } );
	json descriptor = { { "protocolVersion", 1 },
						{ "host", server->mImpl->settings.address },
						{ "port", server->mImpl->port },
						{ "pid", Sys::getProcessID() } };
	if ( generated )
		descriptor["token"] = server->mImpl->settings.token;
	std::fprintf( stderr, "EEPP_INSPECTOR=%s\n", descriptor.dump().c_str() );
	if ( server->mImpl->settings.address != "127.0.0.1" &&
		 server->mImpl->settings.address != "::1" )
		std::fprintf( stderr, "EEPP_INSPECTOR_WARNING=non-loopback raw TCP is plaintext\n" );
	sInstance = server.release();
	return true;
}

void UIInspectorServer::startFromEnvironment( UISceneNode* defaultScene ) {
	if ( sInstance || !envEnabled( std::getenv( "EEPP_INSPECTOR" ) ) )
		return;
	UIInspectorServerSettings settings;
	if ( const char* address = std::getenv( "EEPP_INSPECTOR_ADDRESS" ) )
		settings.address = address;
	if ( const char* port = std::getenv( "EEPP_INSPECTOR_PORT" ) ) {
		char* end = nullptr;
		unsigned long parsed = std::strtoul( port, &end, 10 );
		if ( !*port || *end || parsed > 65535 ) {
			std::fprintf( stderr, "EEPP_INSPECTOR_ERROR=invalid port\n" );
			return;
		}
		settings.port = static_cast<Uint16>( parsed );
	}
	settings.readOnly = envEnabled( std::getenv( "EEPP_INSPECTOR_READ_ONLY" ) );
	if ( const char* token = std::getenv( "EEPP_INSPECTOR_TOKEN" ) )
		settings.token = token;
	start( defaultScene, std::move( settings ) );
}

void UIInspectorServer::stop() {
	if ( !sInstance )
		return;
	auto* server = sInstance;
	sInstance = nullptr;
	server->mImpl->running = false;
	if ( server->mImpl->thread.joinable() )
		server->mImpl->thread.join();
	server->mImpl->listener.close();
	delete server;
}

void UIInspectorServer::pump() {
	if ( sInstance )
		sInstance->mImpl->pump();
}

void UIInspectorServer::notifyWindowDestroyed( EE::Window::Window* window ) {
	if ( sInstance )
		sInstance->mImpl->inspector.invalidateWindow( window );
}

bool UIInspectorServer::isRunning() const {
	return mImpl->running;
}

const std::string& UIInspectorServer::getAddress() const {
	return mImpl->settings.address;
}

Uint16 UIInspectorServer::getPort() const {
	return mImpl->port;
}

const std::string& UIInspectorServer::getToken() const {
	return mImpl->settings.token;
}

}} // namespace EE::UI
