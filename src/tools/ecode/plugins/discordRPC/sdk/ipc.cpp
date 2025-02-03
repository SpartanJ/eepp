#include "ipc.hpp"

#include <eepp/system/log.hpp>
using namespace EE::System;

DiscordIPC::DiscordIPC() {
	#if defined(__unix__) || defined(_APPLE__)
		mSocket = -1;
	#endif
}

bool DiscordIPC::tryConnect() {
	#if defined(_WIN32) || defined(_WIN64)
		// TODO
		return false;
	#elif defined(__unix__) || defined(_APPLE__)
		// Pipe path can be in any of the following directories:
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
		const std::string env[] = {"XDG_RUNTIME_DIR", "TMPDIR", "TMP", "TEMP"};
		const std::vector<std::string> additionalPaths = {
	        "/app/com.discordapp.Discord",
	        "/.flatpak/dev.vencord.Vesktop/xdg-run",
	        "/snap.discord",
    	};
		std::vector<std::string> validPaths;
		for ( const auto& eVar : env ){
			const char* value = std::getenv(eVar.c_str());
			if (!value) { continue; }
			if (!std::filesystem::exists(value)) { continue; }
			validPaths.push_back(value);
		}
	
		validPaths.push_back("/tmp"); // Hard coded fallback
		
		std::vector<std::string> checkPaths;
		
		for ( const auto& basePath : validPaths ) {
			for (const auto& additionalPath : additionalPaths) {
	            std::string fullPath = basePath + additionalPath;
	            if (std::filesystem::exists(fullPath)) {
	                checkPaths.push_back(fullPath);
	            }
        	}
		}
		checkPaths.insert(checkPaths.end(), validPaths.begin(), validPaths.end());
		
		for (const auto& basePath : validPaths) {
	        if (!std::filesystem::exists(basePath)) { continue; }
	        for (int i = 0; i < 10; ++i) {
	            std::string ipcPath = basePath + "/discord-ipc-" + std::to_string(i);
	            
	            if (std::filesystem::exists(ipcPath)) {
	                Log::info("IPC path found! - %s", ipcPath);
	                mIpcPath = ipcPath;
	                
	                mSocket = socket(AF_UNIX, SOCK_STREAM, 0);
	                if (mSocket == -1) {
	                	Log::error("Discord IPC socket cold not be opened: %s", mIpcPath);
	                	mIpcPath = "";
	                	continue;
	                }
	                
	                sockaddr_un serverAddr;
				    memset(&serverAddr, 0, sizeof(serverAddr));
				    serverAddr.sun_family = AF_UNIX;
				    mIpcPath.copy(serverAddr.sun_path, sizeof(serverAddr.sun_path) - 1);
				    serverAddr.sun_path[sizeof(serverAddr.sun_path) - 1] = '\0'; // Ensure null termination
				    
				    if (connect(mSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
				    	close(mSocket);
				    	return false;
				    }
	                
	                return true;
            	}
        	}
        	return false;
   	 }		
	#endif
	return false; // Discord not supported by other OS (if it is, TBA)
}

DiscordIPC::~DiscordIPC() {
	#if defined(__unix__) || defined(_APPLE__) // Windows apparently uses named pipes. TODO
		close(mSocket);
	#endif
}