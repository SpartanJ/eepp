#include "codexsessionrelay.hpp"
#include "codexinvocation.hpp"
#include "harnesssessionrecovery.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <eepp/core/small_vector.hpp>
#include <eepp/core/string.hpp>
#include <eepp/network/localsocket.hpp>
#include <eepp/network/socketselector.hpp>
#include <eepp/network/streamrelay.hpp>
#include <eepp/network/tcplistener.hpp>
#include <eepp/network/websocket.hpp>
#include <eepp/system/base64.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/sys.hpp>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

using namespace EE;
using namespace EE::System;
using namespace EE::Network;

namespace eterm {

namespace {

// A pane has one selected conversation even when the TUI uses several connections. IDs remain
// connection-local. Every request that may switch threads gets a generation; the saved record is
// only republished once no selection in the pane is unresolved, and any outcome that may have
// switched threads unseen drops it.
struct SelectionState {
	const std::string& directory;
	const std::string& home;
	const std::string& upstream;
	// The last conversation known to be selected, and the generation that confirmed it.
	nlohmann::json current;
	Uint64 confirmed{ 0 };
	Uint64 generation{ 0 };
	// Captured selections still awaiting a response, across all connections.
	unsigned unresolved{ 0 };
	// Connections whose requests can no longer be observed: any of them may switch threads unseen.
	unsigned blind{ 0 };

	// Dropping the known conversation clears the saved record at once; only republishing a known
	// record waits for unresolved selections.
	void forget() {
		current = nullptr;
		HarnessSessionRecovery::publish( directory, {} );
	}

	// Save the known conversation once nothing can still change it unseen.
	void settle() {
		if ( !unresolved )
			HarnessSessionRecovery::publish( directory, blind ? nlohmann::json{} : current );
	}
};

struct Selection {
	struct Pending {
		std::string id;
		Uint64 generation;
	};
	SelectionState& state;
	SmallVector<Pending, 4> pending;
	bool blind{ false };

	// Save no session rather than one that might be stale.
	void clear( const char* reason ) {
		state.forget();
		std::cerr << "eterm: Codex recovery cleared the session: " << reason << ".\n";
	}

	// Without a response the request may or may not have switched threads. A record confirmed by
	// a newer selection stays valid; an older one does not.
	void cancelPending( const char* reason ) {
		bool uncertain = false;
		for ( const auto& request : pending ) {
			--state.unresolved;
			uncertain = uncertain || request.generation > state.confirmed;
		}
		pending.clear();
		if ( uncertain )
			clear( reason );
		else
			state.settle();
	}

	// A request that could not be observed may have been a selection, captured or not. It acts as
	// an unseen newer selection: responses to earlier requests can no longer be trusted.
	void requestLost( const char* reason, bool permanent ) {
		if ( permanent && !blind ) {
			blind = true;
			++state.blind;
		}
		++state.generation;
		clear( reason );
	}

	void close() {
		cancelPending( "connection closed before selection completed" );
		if ( blind ) {
			blind = false;
			--state.blind;
		}
	}

	void consume( const nlohmann::json& value, bool request ) {
		if ( !value.is_object() || !value.contains( "id" ) ||
			 !( value["id"].is_string() || value["id"].is_number_integer() ) )
			return;
		const auto id = value["id"].dump();
		if ( request ) {
			const auto method = value.value( "method", nlohmann::json{} );
			if ( method == "thread/start" || method == "thread/resume" ||
				 method == "thread/fork" ) {
				if ( pending.size() >= 64 ) {
					requestLost( "too many concurrent selections", false );
					return;
				}
				pending.push_back( { id, ++state.generation } );
				++state.unresolved;
				// Never save the old conversation while a different selection is unconfirmed.
				HarnessSessionRecovery::publish( state.directory, {} );
			}
			return;
		}
		// Server requests occupy the opposite JSON-RPC ID namespace, even on one connection.
		if ( value.contains( "method" ) ||
			 ( !value.contains( "result" ) && !value.contains( "error" ) ) )
			return;
		for ( auto it = pending.begin(); it != pending.end(); ++it ) {
			if ( it->id != id )
				continue;
			const auto generation = it->generation;
			pending.erase( it );
			--state.unresolved;
			// An error proves this request kept the previous thread: nothing changed.
			if ( value.contains( "error" ) ) {
				state.settle();
				return;
			}
			// An older request succeeded after a newer one was sent. The daemon may be on either
			// thread, so no record is certain until a newer selection confirms one.
			if ( generation != state.generation ) {
				state.forget();
				return;
			}
			if ( state.blind ) {
				clear( "another connection's requests are not observable" );
				return;
			}
			const auto result = value.find( "result" );
			if ( result == value.end() || !result->is_object() ) {
				clear( "selection response has no result" );
				return;
			}
			const auto thread = result->find( "thread" );
			if ( thread == result->end() || !thread->is_object() ) {
				clear( "selection response has no thread" );
				return;
			}
			auto record = HarnessSessionRecovery::descriptor(
				"codex",
				{ { "session_id", thread->value( "id", nlohmann::json{} ) },
				  { "cwd", thread->value( "cwd", nlohmann::json{} ) },
				  { "transcript_path", thread->value( "path", nlohmann::json{} ) } },
				state.home );
			if ( thread->value( "ephemeral", nlohmann::json{} ) == true ) {
				record = nullptr;
			} else if ( record.is_null() ) {
				clear( "selection response has invalid session metadata" );
				return;
			}
			if ( !record.is_null() ) {
				record["mode"] = "daemon";
				record["socket"] = state.upstream;
				record["owner_pid"] = Sys::getProcessID();
				record["owner_started"] = Sys::getProcessCreationTime( Sys::getProcessID() );
			}
			// The newest selection is confirmed. A still-unresolved older one that succeeds later
			// clears it again.
			state.current = std::move( record );
			state.confirmed = generation;
			HarnessSessionRecovery::publish( state.directory, state.current );
			return;
		}
	}
};

// Removes every `name:` field line from an HTTP header whose blank line starts at end.
void removeHeaderField( std::string& header, size_t end, std::string_view name ) {
	size_t line = header.find( "\r\n" );
	while ( line != std::string::npos && line < end ) {
		const size_t start = line + 2;
		const size_t next = header.find( "\r\n", start );
		const std::string_view field( header.data() + start, next - start );
		if ( field.size() > name.size() && field[name.size()] == ':' &&
			 String::iequals( field.substr( 0, name.size() ), name ) ) {
			header.erase( start, next + 2 - start );
			end -= next + 2 - start;
			line = start - 2;
			continue;
		}
		line = next;
	}
}

struct Direction {
	WebSocket::Decoder frames;
	std::string header;
	bool observing{ true };
	bool upgraded{ false };

	Direction( Selection& selection, bool request ) :
		frames(
			WebSocket::Mask::Any,
			[&selection, request]( WebSocket::Opcode opcode, std::string_view data ) {
				if ( opcode != WebSocket::Opcode::Text )
					return;
				// Prompts and pasted images can be megabytes: only parse a request that can name
				// a thread/* method, written plainly or with JSON escapes.
				if ( request && data.find( "\"thread/" ) == std::string_view::npos &&
					 data.find( "\\/" ) == std::string_view::npos &&
					 data.find( "\\u" ) == std::string_view::npos )
					return;
				selection.consume( nlohmann::json::parse( data, nullptr, false ), request );
			},
			8 * 1024 * 1024,
			[&selection, request] { return request || !selection.pending.empty(); }, true ) {}

	void observe( std::string_view bytes, Selection& selection, bool request ) {
		if ( observing && !frames.feed( bytes ) ) {
			observing = false;
			if ( request )
				selection.requestLost( "request observation stopped", true );
			else
				selection.cancelPending( "protocol observation stopped" );
		}
		if ( frames.takeOversizedMessage() ) {
			if ( request )
				selection.requestLost( "request exceeds the 8 MiB observation limit", false );
			else
				selection.cancelPending( "selection message exceeds the 8 MiB observation limit" );
		}
	}

	bool filter( std::string& data, Selection& selection, bool request, const std::string& token ) {
		if ( !upgraded ) {
			header.append( data );
			const auto end = header.find( "\r\n\r\n" );
			if ( end == std::string::npos ) {
				data.clear();
				return header.size() <= 65536;
			}
			if ( end + 4 > 65536 )
				return false;
			if ( request &&
				 !String::constantTimeEquals( WebSocket::headerValue( header, "authorization" ),
											  "Bearer " + token ) )
				return false;
			if ( !request ) {
				std::istringstream status( header.substr( 0, header.find( "\r\n" ) ) );
				std::string version;
				unsigned code = 0;
				status >> version >> code;
				if ( version != "HTTP/1.1" || code != 101 )
					observing = false;
			}
			observe( std::string_view( header ).substr( end + 4 ), selection, request );
			// The token authorizes this relay only; the daemon never receives it.
			if ( request )
				removeHeaderField( header, end, "authorization" );
			data = std::move( header );
			upgraded = true;
		} else {
			observe( data, selection, request );
		}
		return true;
	}
};

struct Connection {
	Selection selection;
	Direction outgoing;
	Direction incoming;
	const std::string& token;
	StreamRelay<TcpSocket, LocalSocket> transport;
	std::chrono::steady_clock::time_point opened{ std::chrono::steady_clock::now() };
	bool connected{ false };

	Connection( SelectionState& state, const std::string& token ) :
		selection{ state, {} },
		outgoing( selection, true ),
		incoming( selection, false ),
		token( token ),
		transport( [this]( bool request, std::string& bytes ) {
			return ( request ? outgoing : incoming )
				.filter( bytes, selection, request, this->token );
		} ) {}

	~Connection() { selection.close(); }
};

/** One worker multiplexes bounded connections. A slow reader applies backpressure to its peer;
 * no unbounded queues, per-connection threads, or UI-thread I/O are introduced. */
struct Relay {
	std::string directory;
	std::string home;
	std::string upstream;
	std::string token;
	SelectionState selection{ directory, home, upstream, {}, 0 };
	TcpListener listener;
	std::array<std::unique_ptr<Connection>, 8> connections;
	std::thread worker;
	std::atomic<bool> stopped{ false };

	Relay( std::string directory, std::string home, std::string upstream ) :
		directory( std::move( directory ) ),
		home( std::move( home ) ),
		upstream( std::move( upstream ) ) {}

	~Relay() {
		stopped = true;
		if ( worker.joinable() )
			worker.join();
	}

	std::string listen() {
		std::array<char, 32> random;
		if ( !Sys::getRandomBytes( random.data(), random.size() ) )
			return {};
		Base64::encode( std::string_view( random.data(), random.size() ), token );
		if ( listener.listen( Socket::AnyPort, IpAddress::LocalHost ) != Socket::Done )
			return {};
		listener.setBlocking( false );
		return "ws://127.0.0.1:" + std::to_string( listener.getLocalPort() );
	}

	void pump() {
		SocketSelector selector;
		while ( !stopped ) {
			selector.clear();
			selector.add( listener );
			for ( auto& connection : connections ) {
				if ( !connection )
					continue;
				auto& c = *connection;
				if ( !c.incoming.upgraded &&
					 std::chrono::steady_clock::now() - c.opened > std::chrono::seconds( 5 ) ) {
					connection.reset();
					continue;
				}
				if ( c.connected )
					c.transport.watch( selector );
				else
					selector.add( c.transport.right(), SocketSelector::Event::Write );
			}
			if ( !selector.wait( Milliseconds( 100 ) ) )
				continue;
			if ( selector.isReady( listener ) ) {
				// When full, a new connection replaces the oldest one that has not authenticated,
				// so idle local connections cannot lock the TUI out.
				if ( std::none_of( connections.begin(), connections.end(),
								   []( const auto& slot ) { return !slot; } ) ) {
					std::unique_ptr<Connection>* oldest = nullptr;
					for ( auto& slot : connections ) {
						if ( !slot->outgoing.upgraded &&
							 ( !oldest || slot->opened < ( *oldest )->opened ) )
							oldest = &slot;
					}
					if ( oldest )
						oldest->reset();
				}
				auto candidate = std::make_unique<Connection>( selection, token );
				if ( listener.accept( candidate->transport.left() ) == Socket::Done ) {
					for ( auto& slot : connections ) {
						if ( slot )
							continue;
						candidate->transport.left().setBlocking( false );
						candidate->transport.right().setBlocking( false );
						const auto status = candidate->transport.right().connect( upstream );
						if ( status == Socket::Done || status == Socket::NotReady ) {
							candidate->connected = status == Socket::Done;
							slot = std::move( candidate );
						} else {
							std::cerr
								<< "eterm: Codex relay could not connect to the local daemon.\n";
						}
						break;
					}
				}
			}
			for ( auto& connection : connections ) {
				if ( !connection )
					continue;
				auto& c = *connection;
				if ( !c.connected ) {
					if ( selector.isReady( c.transport.right(), SocketSelector::Event::Write ) ) {
						const auto status = c.transport.right().finishConnect();
						if ( status == Socket::Done ) {
							c.connected = true;
						} else if ( status != Socket::NotReady ) {
							std::cerr
								<< "eterm: Codex relay could not connect to the local daemon.\n";
							connection.reset();
						}
					}
					continue;
				}
				if ( !c.transport.poll( selector ) )
					connection.reset();
			}
		}
	}
};

bool bypass( const std::vector<std::string>& arguments ) {
	const auto shell = Sys::getEnv( "ETERM_HARNESS_SHELL_PID" );
	if ( Sys::getEnv( "ETERM_HARNESS_DIRECTORY" ).empty() || shell.empty() ||
		 std::to_string( Sys::getProcessInfo( Sys::getProcessID() ).parentID ) != shell )
		return true;
	return !CodexInvocation::relay( arguments );
}

} // namespace

int CodexSessionRelay::run( const std::vector<std::string>& arguments ) {
	const auto executable = Sys::which( "codex" );
	if ( executable.empty() ) {
		std::cerr << "eterm: codex executable is unavailable.\n";
		return 127;
	}
	if ( bypass( arguments ) )
		return Process::runForeground( executable, arguments );
	const auto home = Sys::getEnv( "CODEX_HOME" );
	const auto defaultSocket = ( home.empty() ? Sys::getUserDirectory() + "/.codex" : home ) +
							   "/app-server-control/app-server-control.sock";
	auto upstream = Sys::getEnv( "ETERM_CODEX_SOCKET" );
	if ( upstream.empty() )
		upstream = defaultSocket;
	LocalSocket probe;
	if ( probe.connect( upstream, Seconds( 1 ) ) != Socket::Done ) {
		if ( upstream != defaultSocket ) {
			std::cerr << "eterm: saved Codex daemon socket is unavailable; start that daemon and "
						 "retry.\n";
			return 1;
		}
		const int result = Process::runForeground( executable, { "app-server", "daemon", "start" },
												   { { "ETERM_HARNESS_DIRECTORY", "" } } );
		if ( result )
			return result;
		if ( probe.connect( upstream, Seconds( 5 ) ) != Socket::Done ) {
			std::cerr << "eterm: Codex daemon is unavailable after start.\n";
			return 1;
		}
	}
	probe.disconnect();
	Relay relay( Sys::getEnv( "ETERM_HARNESS_DIRECTORY" ), home, upstream );
	const auto remote = relay.listen();
	if ( remote.empty() ) {
		std::cerr << "eterm: could not create the private Codex relay.\n";
		return 1;
	}
	relay.worker = std::thread( [&relay] {
		try {
			relay.pump();
		} catch ( const std::exception& ) {
			std::cerr << "eterm: Codex relay transport failed.\n";
		}
	} );
	std::vector<std::string> args{ "--remote", remote };
	std::unordered_map<std::string, std::string> environment{ { "ETERM_HARNESS_DIRECTORY", "" } };
	if ( !relay.token.empty() ) {
		environment["ETERM_CODEX_RELAY_TOKEN"] = relay.token;
		args.emplace_back( "--remote-auth-token-env" );
		args.emplace_back( "ETERM_CODEX_RELAY_TOKEN" );
	}
	args.insert( args.end(), arguments.begin(), arguments.end() );
	return Process::runForeground( executable, args, environment );
}

} // namespace eterm
