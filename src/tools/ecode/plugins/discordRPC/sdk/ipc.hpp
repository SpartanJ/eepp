#include <iostream>
#include <filesystem>
#include <vector>
#include <unistd.h>
#include <ctime>

#include <nlohmann/json.hpp>

#if defined(__unix__) || defined(_APPLE__)
    #include <sys/socket.h>
    #include <sys/un.h>
#endif

enum DiscordIPCActivityTypes {
    Playing = 0,
    Listening = 2,
    Watching = 3,
    Competing = 5,
};

struct DiscordIPCActivityButton {
    std::string label = "";
    std::string url = "";
};

struct DiscordIPCActivity {
    DiscordIPCActivityTypes type = DiscordIPCActivityTypes::Playing;
    std::string state = "";
    std::string details = "";
    
    time_t start = 0;
    time_t end = 0;
    
    std::string largeImage = "";
    std::string largeText = "";
    std::string smallImage = "";
    std::string smallText = "";
    
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
            
    protected:
        std::string mIpcPath;
        int mPID;
        
        DiscordIPCActivity mActivity;
        
        //Configurables
        std::string mcClientID;
        
        #if defined(__unix__) || defined(_APPLE__)
            int mSocket;
        #endif        
        
        void doHandshake();
        void reconnect();
        
        void sendPacket( DiscordIPCOpcodes opcode, nlohmann::json j );
};