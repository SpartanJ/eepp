#include <eepp/config.hpp>
#include <eepp/system/mutex.hpp>

#include <nlohmann/json_fwd.hpp>

using namespace EE::System;

// 2^8 = 256s ~4.3min
#define DISCORDIPC_BACKOFF_MAX 8

enum DiscordIPCActivityTypes {
	Playing = 0,
	Listening = 2,
	Watching = 3,
	Competing = 5,
};

struct DiscordIPCActivityButton {
	std::string label;
	std::string url;
};

struct DiscordIPCActivity {
	DiscordIPCActivityTypes type = DiscordIPCActivityTypes::Playing;
	std::string state;
	std::string details;

	time_t start = 0;
	time_t end = 0;

	std::string largeImage;
	std::string largeText;
	std::string smallImage;
	std::string smallText;

	// IMPORTANT: For some reason, you do not see the buttons of your own account on the discord
	// client (intended discord behavior)
	DiscordIPCActivityButton buttons[2];
};

enum DiscordIPCOpcodes { Handshake = 0, Frame, Close, Ping, Pong };

class DiscordIPC {
  public:
	virtual ~DiscordIPC();
	DiscordIPC();

	// false - FileNotFound/OSNotSupported
	// true  - Success
	bool tryConnect();
	void reconnect();
	bool isConnected() const;

	void setActivity( DiscordIPCActivity&& a );
	const DiscordIPCActivity& getActivity() { return mActivity; }
	void clearActivity();

	bool UIReady = false;
	bool IsReconnectScheduled =
		false; // If we fail to load bofore UI initialises we call reconnect after init

	// Configurables
	std::string ClientID;

  protected:
	std::string mIpcPath;
	int mPID = 0;

	int mBackoffIndex = 0;
	int mReconnectLock =
		false; // Not quite a mutex because I want to lock any attempts if one is already waiting

	DiscordIPCActivity mActivity;
	Mutex mActivityMutex;

#if defined( EE_PLATFORM_POSIX )
	int mSocket = -1;
#elif EE_PLATFORM == EE_PLATFORM_WIN
	void* mSocket;
#endif

	void doHandshake();

	void sendPacket( DiscordIPCOpcodes opcode, nlohmann::json j );
};
