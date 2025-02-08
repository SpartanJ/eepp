#include <eepp/config.hpp>

#if defined( EE_PLATFORM_POSIX ) 
    #include <sys/socket.h>
    #include <sys/un.h>
    #include <unistd.h>
#elif EE_PLATFORM == EE_PLATFORM_WIN
    #include <windows.h>
#endif

#include <nlohmann/json.hpp>

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
    
    DiscordIPCActivityButton buttons[2];
};

enum DiscordIPCOpcodes {
    Handshake = 0,
    Frame,
    Close,
    Ping,
    Pong
};

class DiscordIPC {
    public:
    
        virtual ~DiscordIPC();
        DiscordIPC();

        // false - FileNotFound/OSNotSupported  
        // true  - Success
        bool tryConnect();
        
        void setActivity( DiscordIPCActivity a );
        DiscordIPCActivity *getActivity() { return &mActivity; }
        void clearActivity();
            
        bool mUIReady;
        bool mIsReconnectScheduled = false; // If we fail to load bofore UI initialises we call reconnect after init
    protected:
        std::string mIpcPath;
        int mPID;
        
        int mBackoffIndex;
        int mReconnectLock = false; // Not quite a mutex because I want to lock any attempts if one is already waiting
        
        DiscordIPCActivity mActivity;
        
        //Configurables
        std::string mcClientID;
        
         #if defined( EE_PLATFORM_POSIX ) 
            int mSocket;
         #elif EE_PLATFORM == EE_PLATFORM_WIN
             HANDLE mSocket;
         #endif
        
        void doHandshake();
        void reconnect();
        
        void sendPacket( DiscordIPCOpcodes opcode, nlohmann::json j );
};