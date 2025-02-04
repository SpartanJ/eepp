#include "ipc.hpp"

#include <eepp/system/log.hpp>
#include <netinet/in.h>
using namespace EE::System;

using json = nlohmann::json;

DiscordIPC::DiscordIPC() {
	mPID = Sys::getProcessID();
	
	mcClientID = "1335730393948749898"; // TODO: Implement actual config reading
	
	#if defined(__unix__) || defined(_APPLE__)
		mSocket = -1;
	#endif
}

bool DiscordIPC::tryConnect() {
	#if defined(_WIN32) || defined(_WIN64)
		// TODO
		return false;
	#elif defined(__unix__) || defined(_APPLE__)
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
				    
				    doHandshake();
	                return true;
            	}
        	}
        	return false;
   	 }		
	#endif
	return false; // Discord not supported by other OS (if it is, TBA)
}

void DiscordIPC::doHandshake() {
	json j = {
		{"v", 1},
		{"client_id", mcClientID}
	};
	
	sendPacket(DiscordIPCOpcodes::Handshake, j);
}

void DiscordIPC::clearActivity() {
	json j = {
        {"cmd", "SET_ACTIVITY"},
        {"args", {
            {"pid", 0},
            {"activity", nullptr} 
        }},
        {"nonce", "-"} // TODO: Null nonce for dev purposes, change to UUIDV4 in finished product
    };
	sendPacket(DiscordIPCOpcodes::Frame, j);
}

void DiscordIPC::setActivity( DiscordIPCActivity a ) {
	json aj = {
        {"type", static_cast<int>(a.type)},
        {"instance", true},
    };
    
    if (!a.state.empty())
    	aj["state"] = a.state;
     if (!a.details.empty())
    	aj["details"] = a.details;

    json as;
    if (!a.largeImage.empty()) {
        as["large_image"] = a.largeImage;
    }
    if (!a.largeText.empty()) {
        as["large_text"] = a.largeText;
    }
    if (!a.smallImage.empty()) {
        as["small_image"] = a.smallImage;
    }
    if (!a.smallText.empty()) {
        as["small_text"] = a.smallText;
    }
    if (!as.empty()) {
        aj["assets"] = as;
    }

    json t;
    if (a.start != 0) {
        t["start"] = a.start;
    }
    if (a.end != 0) {
        t["end"] = a.end;
    }
    if (!t.empty()) {
        aj["timestamps"] = t;
    }
    
	json b;
    if (!a.buttons[0].url.empty()){
		b[0]["label"] = a.buttons[0].label;
		b[0]["url"] = a.buttons[0].url;
    }
    if (!a.buttons[1].url.empty()){
		b[1]["label"] = a.buttons[1].label;
		b[1]["url"] = a.buttons[1].url;
    }
    if (!b.empty()) {
    	aj["buttons"] = b;
    }
    
    json j{
    {"cmd", "SET_ACTIVITY"},
    {"args", {
        {"pid", mPID},
        {"activity", aj},
    }},
    {"nonce", "-"}
	};
	
	sendPacket(DiscordIPCOpcodes::Frame, j);
	mActivity = a;
}

void DiscordIPC::sendPacket(DiscordIPCOpcodes opcode, json j) {
	Log::debug("Packet is: %s", j.dump(4)); // Packet json logging before we start sending to ipc
    #if defined(__unix__) || defined(_APPLE__)
        const std::string packet = j.dump();
        std::vector<uint8_t> data;
		
		// Conversion to little endian
        union {
            uint32_t value;
            uint8_t bytes[4];
        } bytes;

        bytes.value = htonl(opcode);
        for (int i = 3; i >= 0; --i) {
            data.push_back(bytes.bytes[i]);
        }

        bytes.value = htonl(packet.length());
        for (int i = 3; i >= 0; --i) {
            data.push_back(bytes.bytes[i]);
        }

        for (char c : packet) {
            data.push_back(static_cast<uint8_t>(c));
        }
        
        long unsigned int bytesSent = send(mSocket, data.data(), data.size(), 0);
        if (bytesSent != data.size()) {
            Log::error("Failed to send all data to Unix socket: %zu bytes sent, %zu bytes expected", bytesSent, data.size());
        }

        
        struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 500000; // 0.5 seconds in microseconds
		setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));   
        
        char buffer[1024];
		ssize_t bytesRead = recv(mSocket, buffer, sizeof(buffer), 0);
		
		// TODO: Implement nonce checking? (does it even really matter?)
		
		//return bytesRead;
    #endif
}

void DiscordIPC::reconnect() {
	// TODO. Might change the API here!!
}

DiscordIPC::~DiscordIPC() {
	#if defined(__unix__) || defined(_APPLE__) // Windows apparently uses named pipes. TODO
		close(mSocket);
	#endif
}