
#include <iostream>
#include <filesystem>
#include <vector>
#include <unistd.h>

#if defined(__unix__) || defined(_APPLE__)
	#include <sys/socket.h>
	#include <sys/un.h>
#endif

class DiscordIPC {
	public:
	
		virtual ~DiscordIPC();
		DiscordIPC();

		// false - FileNotFound/OSNotSupported  
		// true  - Success
		bool tryConnect();
	protected:
		std::string mIpcPath;
		#if defined(__unix__) || defined(_APPLE__)
			int mSocket;
		#endif		
};