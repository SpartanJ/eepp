#include "ipc.hpp"

#include <filesystem>
#include <vector>

#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/uiscenenode.hpp>

#if defined( EE_PLATFORM_POSIX )
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#elif EE_PLATFORM == EE_PLATFORM_WIN
#ifndef WIN32LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

using namespace EE::System;

using json = nlohmann::json;

DiscordIPC::DiscordIPC()
#if EE_PLATFORM == EE_PLATFORM_WIN
	:
	mSocket( INVALID_HANDLE_VALUE )
#endif
{
	mPID = Sys::getProcessID();
}

bool DiscordIPC::tryConnect() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	std::string basePath = "\\\\.\\pipe\\";

	for ( int i = 0; i < 10; ++i ) {
		std::string ipcPath = basePath + "discord-ipc-" + std::to_string( i );

		// Check if exists
		DWORD attributes = GetFileAttributesA( ipcPath.c_str() );
		if ( attributes != INVALID_FILE_ATTRIBUTES ) {
			Log::debug( "dcIPC: IPC path found! - %s", ipcPath );
			mIpcPath = ipcPath;

			mSocket = CreateFileA( mIpcPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
								   OPEN_EXISTING, 0, nullptr );

			if ( mSocket != INVALID_HANDLE_VALUE ) {
				doHandshake();
				return true;
			}
		}
	}

	reconnect();
	return false;

#elif defined( EE_PLATFORM_POSIX )
	// Socket path can be in any of the following directories:
	// * `XDG_RUNTIME_DIR`
	// * `TMPDIR`
	// * `TMP`
	// * `TEMP`
	// * `/tmp`
	//
	// Possibly Followed by:
	// * `/app/com.discordapp.Discord` - for flatpak
	// * `/.flatpak/dev.vencord.Vesktop/xdg-run` - Vesktop flatpak
	// * `/snap.discord` - for snap
	//
	// Followed by:
	// * `/discord-ipc-{i}` - where `i` is a number from 0 to 9
	const std::string env[] = { "XDG_RUNTIME_DIR", "TMPDIR", "TMP", "TEMP" };
	const std::vector<std::string> additionalPaths = {
		"/app/com.discordapp.Discord",
		"/.flatpak/dev.vencord.Vesktop/xdg-run",
		"/snap.discord",
	};
	std::vector<std::string> validPaths;
	for ( const auto& eVar : env ) {
		const char* value = std::getenv( eVar.c_str() );
		if ( !value ) {
			continue;
		}
		if ( !std::filesystem::exists( value ) ) {
			continue;
		}
		validPaths.push_back( value );
	}

	std::vector<std::string> checkPaths;
	checkPaths.push_back( "/tmp" ); // Hard coded fallback

	for ( const auto& basePath : validPaths ) {
		for ( const auto& additionalPath : additionalPaths ) {
			std::string fullPath = basePath + additionalPath;
			if ( std::filesystem::exists( fullPath ) ) {
				checkPaths.push_back( fullPath );
			}
		}
	}
	checkPaths.insert( checkPaths.end(), validPaths.begin(), validPaths.end() );

	for ( const auto& basePath : checkPaths ) {
		if ( !std::filesystem::exists( basePath ) ) {
			continue;
		}
		for ( int i = 0; i < 10; ++i ) {
			std::string ipcPath = basePath + "/discord-ipc-" + std::to_string( i );

			if ( std::filesystem::exists( ipcPath ) ) {
				Log::debug( "dcIPC: IPC path found! - %s", ipcPath );
				mIpcPath = ipcPath;

				mSocket = socket( AF_UNIX, SOCK_STREAM, 0 );
				if ( mSocket == -1 ) {
					Log::error( "dcIPC: Discord IPC socket cold not be opened: %s", mIpcPath );
					mIpcPath.clear();
					continue;
				}

				sockaddr_un serverAddr;
				memset( &serverAddr, 0, sizeof( serverAddr ) );
				serverAddr.sun_family = AF_UNIX;
				mIpcPath.copy( serverAddr.sun_path, sizeof( serverAddr.sun_path ) - 1 );
				serverAddr.sun_path[sizeof( serverAddr.sun_path ) - 1] =
					'\0'; // Ensure null termination

				if ( connect( mSocket, reinterpret_cast<struct sockaddr*>( &serverAddr ),
							  sizeof( serverAddr ) ) == -1 ) {
					close( mSocket );
					mSocket = -1;
					mIpcPath.clear();
					return false;
				}

				doHandshake();
				return true;
			}
		}
		reconnect();
		return false;
	}
#endif
	return false; // Discord not supported by other OS (if it is, TBA)
}

void DiscordIPC::doHandshake() {
	json j = { { "v", 1 }, { "client_id", ClientID } };

	sendPacket( DiscordIPCOpcodes::Handshake, j );
}

void DiscordIPC::clearActivity() {
	json j = { { "cmd", "SET_ACTIVITY" },
			   { "args", { { "pid", 0 }, { "activity", nullptr } } },
			   { "nonce", "-" } };
	sendPacket( DiscordIPCOpcodes::Frame, j );
}

void DiscordIPC::setActivity( DiscordIPCActivity&& a ) {
	json aj = {
		{ "type", static_cast<int>( a.type ) },
		{ "instance", true },
	};

	if ( !a.state.empty() )
		aj["state"] = a.state;
	if ( !a.details.empty() )
		aj["details"] = a.details;

	json as;
	if ( !a.largeImage.empty() ) {
		as["large_image"] = a.largeImage;
	}
	if ( !a.largeText.empty() ) {
		as["large_text"] = a.largeText;
	}
	if ( !a.smallImage.empty() ) {
		as["small_image"] = a.smallImage;
	}
	if ( !a.smallText.empty() ) {
		as["small_text"] = a.smallText;
	}
	if ( !as.empty() ) {
		aj["assets"] = as;
	}

	json t;
	if ( a.start != 0 ) {
		t["start"] = a.start;
	}
	if ( a.end != 0 ) {
		t["end"] = a.end;
	}
	if ( !t.empty() ) {
		aj["timestamps"] = t;
	}

	json b;
	if ( !a.buttons[0].url.empty() ) {
		b[0]["label"] = a.buttons[0].label;
		b[0]["url"] = a.buttons[0].url;
	}
	if ( !a.buttons[1].url.empty() ) {
		b[1]["label"] = a.buttons[1].label;
		b[1]["url"] = a.buttons[1].url;
	}
	if ( !b.empty() ) {
		aj["buttons"] = b;
	}

	json j{ { "cmd", "SET_ACTIVITY" },
			{ "args",
			  {
				  { "pid", mPID },
				  { "activity", aj },
			  } },
			{ "nonce", "-" } };

	{
		Lock l( mActivityMutex );
		mActivity = std::move( a );
	}

	sendPacket( DiscordIPCOpcodes::Frame, j );
}

void DiscordIPC::sendPacket( DiscordIPCOpcodes opcode, json j ) {
	if ( !isConnected() ) {
		reconnect();
		return;
	}

	const std::string packet = j.dump();
	std::vector<uint8_t> data;

	// Add correct ammount of padding for the protocol
	union {
		uint32_t value;
		uint8_t bytes[4];
	} bytes;

	bytes.value = opcode;
	for ( int i = 0; i <= 3; ++i )
		data.push_back( bytes.bytes[i] );

	bytes.value = packet.length();
	for ( int i = 0; i <= 3; ++i )
		data.push_back( bytes.bytes[i] );

	for ( char c : packet )
		data.push_back( static_cast<uint8_t>( c ) );

	Log::debug( "dcIPC: sending packet: %s", packet );

#if defined( EE_PLATFORM_POSIX )

	ssize_t bytesSent = send( mSocket, data.data(), data.size(), 0 );
	if ( bytesSent != static_cast<ssize_t>( data.size() ) ) {
		Log::error(
			"dcIPC: Failed to send all data to Unix socket: %zu bytes sent, %zu bytes expected",
			bytesSent, data.size() );
		reconnect();
		return;
	}

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500000; // 0.5 seconds in microseconds
	setsockopt( mSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof( tv ) );

	char buffer[1024];
	recv( mSocket, buffer, sizeof( buffer ), 0 );

#elif EE_PLATFORM == EE_PLATFORM_WIN
	DWORD bytesSent;
	if ( !WriteFile( mSocket, data.data(), data.size(), &bytesSent, nullptr ) ) {
		Log::error( "dcIPC: Error writing to pipe!!" );
		reconnect();
		return;
	} else if ( bytesSent != data.size() ) {
		Log::error( "dcIPC: Incorrect ammount of data written: %zu bytes sent, %zu bytes expected",
					bytesSent, data.size() );
		reconnect();
		return;
	}

	DWORD bytesRead;
	char buffer[1024];
	if ( !ReadFile( mSocket, buffer, 1024, &bytesRead, nullptr ) ) {
		Log::error( "dcIPC: Error reading pipe!!" );
		reconnect();
		return;
	}

	Log::debug( "dcIPC - received sendPacket response: %s", buffer );

#endif
}

void DiscordIPC::reconnect() {
	if ( mReconnectLock ) {
		Log::warning( "dcIPC: Tried to call reconnect while locked" );
		return;
	}
	if ( !UIReady ) {
		Log::debug( "dcIPC: Scheduled a reconnect" );
		IsReconnectScheduled = true;
		return;
	}
	if ( mBackoffIndex < DISCORDIPC_BACKOFF_MAX ) {
		mBackoffIndex++;
	}
	int delay = 5 + pow( 2, mBackoffIndex );
	mReconnectLock = true;

	Log::info( "dcIPC: Waiting for reconnect delay of %us (%u/%u)", delay, mBackoffIndex,
			   DISCORDIPC_BACKOFF_MAX );
	SceneManager::instance()->getUISceneNode()->setTimeout(
		[this] {
			SceneManager::instance()->getUISceneNode()->getThreadPool()->run( [this] {
				Log::info( "dcIPC: Reconnecting..." );
				if ( tryConnect() ) {
					mReconnectLock = false;
					mBackoffIndex = 0;
				}
			} );
		},
		Seconds( delay ) );
}

DiscordIPC::~DiscordIPC() {
#if defined( EE_PLATFORM_POSIX )
	close( mSocket );
#elif EE_PLATFORM == EE_PLATFORM_WIN
	CloseHandle( mSocket );
#endif
}

bool DiscordIPC::isConnected() const {
#if defined( EE_PLATFORM_POSIX )
	return mSocket != -1;
#elif EE_PLATFORM == EE_PLATFORM_WIN
	return mSocket != INVALID_HANDLE_VALUE;
#endif
}
