#include <eepp/network/ftp.hpp>
#include <eepp/network/ipaddress.hpp>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>

namespace EE { namespace Network {

class Ftp::DataChannel : NonCopyable {
	public :

		DataChannel(Ftp& owner);

		Ftp::Response Open(Ftp::TransferMode mode);

		void Send(std::istream& stream);

		void Receive(std::ostream& stream);
	private:
		// Member data
		Ftp&		mFtp;		///< Reference to the owner Ftp instance
		TcpSocket	mDataSocket; ///< Socket used for data transfers
};

Ftp::Response::Response(Status code, const std::string& message) :
	mStatus (code),
	mMessage(message)
{
}

bool Ftp::Response::isOk() const {
	return mStatus < 400;
}

Ftp::Response::Status Ftp::Response::getStatus() const {
	return mStatus;
}

const std::string& Ftp::Response::getMessage() const {
	return mMessage;
}

Ftp::DirectoryResponse::DirectoryResponse(const Ftp::Response& response) :
	Ftp::Response(response)
{
	if ( isOk() ) {
		// Extract the directory from the server response
		std::string::size_type begin = getMessage().find('"', 0);
		std::string::size_type end   = getMessage().find('"', begin + 1);
		mDirectory = getMessage().substr(begin + 1, end - begin - 1);
	}
}

const std::string& Ftp::DirectoryResponse::getDirectory() const {
	return mDirectory;
}

Ftp::ListingResponse::ListingResponse(const Ftp::Response& response, const std::string& data) :
	Ftp::Response(response)
{
	if ( isOk() ) {
		// Fill the array of strings
		std::string::size_type lastPos = 0;
		for (std::string::size_type pos = data.find("\r\n"); pos != std::string::npos; pos = data.find("\r\n", lastPos)) {
			mListing.push_back(data.substr(lastPos, pos - lastPos));
			lastPos = pos + 2;
		}
	}
}

const std::vector<std::string>& Ftp::ListingResponse::getListing() const {
	return mListing;
}

Ftp::~Ftp() {
	disconnect();
}

Ftp::Response Ftp::connect(const IpAddress& server, unsigned short port, Time timeout) {
	// Connect to the server
	if (mCommandSocket.connect(server, port, timeout) != Socket::Done)
		return Response(Response::ConnectionFailed);

	// Get the response to the connection
	return getResponse();
}

Ftp::Response Ftp::login() {
	return login("anonymous", "user@eepp.com.ar");
}

Ftp::Response Ftp::login(const std::string& name, const std::string& password) {
	Response response = sendCommand("USER", name);
	if (response.isOk())
		response = sendCommand("PASS", password);

	return response;
}

Ftp::Response Ftp::disconnect() {
	// Send the exit command
	Response response = sendCommand("QUIT");
	if (response.isOk())
		mCommandSocket.disconnect();

	return response;
}

Ftp::Response Ftp::keepAlive() {
	return sendCommand("NOOP");
}

Ftp::DirectoryResponse Ftp::getWorkingDirectory() {
	return DirectoryResponse(sendCommand("PWD"));
}

Ftp::ListingResponse Ftp::getDirectoryListing(const std::string& directory) {
	// Open a data channel on default port (20) using ASCII transfer mode
	std::ostringstream directoryData;
	DataChannel data(*this);
	Response response = data.Open(Ascii);

	if (response.isOk()) {
		// Tell the server to send us the listing
		response = sendCommand("NLST", directory);
		if (response.isOk()) {
			// Receive the listing
			data.Receive(directoryData);

			// Get the response from the server
			response = getResponse();
		}
	}

	return ListingResponse(response, directoryData.str());
}

Ftp::Response Ftp::changeDirectory(const std::string& directory) {
	return sendCommand("CWD", directory);
}

Ftp::Response Ftp::parentDirectory() {
	return sendCommand("CDUP");
}

Ftp::Response Ftp::createDirectory(const std::string& name) {
	return sendCommand("MKD", name);
}

Ftp::Response Ftp::deleteDirectory(const std::string& name) {
	return sendCommand("RMD", name);
}

Ftp::Response Ftp::renameFile(const std::string& file, const std::string& newName) {
	Response response = sendCommand("RNFR", file);
	if (response.isOk())
		response = sendCommand("RNTO", newName);

	return response;
}

Ftp::Response Ftp::deleteFile(const std::string& name) {
	return sendCommand("DELE", name);
}

Ftp::Response Ftp::download(const std::string& remoteFile, const std::string& localPath, TransferMode mode) {
	// Open a data channel using the given transfer mode
	DataChannel data(*this);
	Response response = data.Open(mode);

	if ( response.isOk() ) {
		// Tell the server to start the transfer
		response = sendCommand("RETR", remoteFile);

		if ( response.isOk() )
		{
			// Extract the filename from the file path
			std::string filename = remoteFile;
			std::string::size_type pos = filename.find_last_of("/\\");
			if (pos != std::string::npos)
				filename = filename.substr(pos + 1);

			// Make sure the destination path ends with a slash
			std::string path = localPath;
			if (!path.empty() && (path[path.size() - 1] != '\\') && (path[path.size() - 1] != '/'))
				path += "/";

			// Create the file and truncate it if necessary
			std::ofstream file((path + filename).c_str(), std::ios_base::binary | std::ios_base::trunc);
			if (!file)
				return Response(Response::InvalidFile);

			// Receive the file data
			data.Receive(file);

			// Close the file
			file.close();

			// Get the response from the server
			response = getResponse();

			// If the download was unsuccessful, delete the partial file
			if (!response.isOk())
				std::remove((path + filename).c_str());
		}
	}

	return response;
}

Ftp::Response Ftp::upload(const std::string& localFile, const std::string& remotePath, TransferMode mode, bool append) {
	// Get the contents of the file to send
	std::ifstream file(localFile.c_str(), std::ios_base::binary);
	if (!file)
		return Response(Response::InvalidFile);

	// Extract the filename from the file path
	std::string filename = localFile;
	std::string::size_type pos = filename.find_last_of("/\\");
	if (pos != std::string::npos)
		filename = filename.substr(pos + 1);

	// Make sure the destination path ends with a slash
	std::string path = remotePath;
	if (!path.empty() && (path[path.size() - 1] != '\\') && (path[path.size() - 1] != '/'))
		path += "/";

	// Open a data channel using the given transfer mode
	DataChannel data(*this);
	Response response = data.Open(mode);
	if (response.isOk()) {
		// Tell the server to start the transfer
		response = sendCommand(append ? "APPE" : "STOR", path + filename);
		if (response.isOk()) {
			// Send the file data
			data.Send(file);

			// Get the response from the server
			response = getResponse();
		}
	}

	return response;
}

Ftp::Response Ftp::sendCommand(const std::string& command, const std::string& parameter) {
	// Build the command string
	std::string commandStr;
	if (parameter != "")
		commandStr = command + " " + parameter + "\r\n";
	else
		commandStr = command + "\r\n";

	// Send it to the server
	if (mCommandSocket.send(commandStr.c_str(), commandStr.length()) != Socket::Done)
		return Response(Response::ConnectionClosed);

	// Get the response
	return getResponse();
}

Ftp::Response Ftp::getResponse() {
	// We'll use a variable to keep track of the last valid code.
	// It is useful in case of multi-lines responses, because the end of such a response
	// will start by the same code
	unsigned int lastCode  = 0;
	bool isInsideMultiline = false;
	std::string message;

	for (;;) {
		// Receive the response from the server
		char buffer[1024];
		std::size_t length;

		if (mReceiveBuffer.empty()) {
			if (mCommandSocket.receive(buffer, sizeof(buffer), length) != Socket::Done)
				return Response(Response::ConnectionClosed);
		} else {
			std::copy(mReceiveBuffer.begin(), mReceiveBuffer.end(), buffer);
			length = mReceiveBuffer.size();
			mReceiveBuffer.clear();
		}

		// There can be several lines inside the received buffer, extract them all
		std::istringstream in(std::string(buffer, length), std::ios_base::binary);
		while (in) {
			// Try to extract the code
			unsigned int code;

			if (in >> code) {
				// Extract the separator
				char separator = 0;
				in.get(separator);

				// The '-' character means a multiline response
				if ((separator == '-') && !isInsideMultiline) {
					// Set the multiline flag
					isInsideMultiline = true;

					// Keep track of the code
					if (lastCode == 0)
						lastCode = code;

					// Extract the line
					std::getline(in, message);

					// Remove the ending '\r' (all lines are terminated by "\r\n")
					message.erase(message.length() - 1);
					message = separator + message + "\n";
				} else {
					// We must make sure that the code is the same, otherwise it means
					// we haven't reached the end of the multiline response
					if ((separator != '-') && ((code == lastCode) || (lastCode == 0))) {
						// Extract the line
						std::string line;
						std::getline(in, line);

						// Remove the ending '\r' (all lines are terminated by "\r\n")
						line.erase(line.length() - 1);

						// Append it to the message
						if (code == lastCode) {
							std::ostringstream out;
							out << code << separator << line;
							message += out.str();
						} else {
							message = separator + line;
						}

						// Return the response code and message
						return Response(static_cast<Response::Status>(code), message);
					} else {
						// The line we just read was actually not a response,
						// only a new part of the current multiline response

						// Extract the line
						std::string line;
						std::getline(in, line);

						if (!line.empty()) {
							// Remove the ending '\r' (all lines are terminated by "\r\n")
							line.erase(line.length() - 1);

							// Append it to the current message
							std::ostringstream out;
							out << code << separator << line << "\n";
							message += out.str();
						}
					}
				}
			} else if (lastCode != 0) {
				// It seems we are in the middle of a multiline response

				// Clear the error bits of the stream
				in.clear();

				// Extract the line
				std::string line;
				std::getline(in, line);

				if (!line.empty()) {
					// Remove the ending '\r' (all lines are terminated by "\r\n")
					line.erase(line.length() - 1);

					// Append it to the current message
					message += line + "\n";
				}
			} else {
				// Error : cannot extract the code, and we are not in a multiline response
				return Response(Response::InvalidResponse);
			}
		}
	}

	// We never reach there
}

Ftp::DataChannel::DataChannel(Ftp& owner) :
	mFtp(owner)
{
}

Ftp::Response Ftp::DataChannel::Open(Ftp::TransferMode mode) {
	// Open a data connection in active mode (we connect to the server)
	Ftp::Response response = mFtp.sendCommand("PASV");

	if (response.isOk()) {
		// Extract the connection address and port from the response
		std::string::size_type begin = response.getMessage().find_first_of("0123456789");

		if (begin != std::string::npos) {
			Uint8 data[6] = {0, 0, 0, 0, 0, 0};
			std::string str = response.getMessage().substr(begin);
			std::size_t index = 0;
			std::locale loc;

			for (int i = 0; i < 6; ++i) {
				// Extract the current number
				while (std::isdigit(str[index],loc)) {
					data[i] = data[i] * 10 + (str[index] - '0');
					index++;
				}

				// Skip separator
				index++;
			}

			// Reconstruct connection port and address
			unsigned short port = data[4] * 256 + data[5];
			IpAddress address(static_cast<Uint8>(data[0]),
							  static_cast<Uint8>(data[1]),
							  static_cast<Uint8>(data[2]),
							  static_cast<Uint8>(data[3]));

			// Connect the data channel to the server
			if (mDataSocket.connect(address, port) == Socket::Done) {
				// Translate the transfer mode to the corresponding FTP parameter
				std::string modeStr;
				switch (mode) {
					case Ftp::Binary	: modeStr = "I"; break;
					case Ftp::Ascii	:  modeStr = "A"; break;
					case Ftp::Ebcdic	: modeStr = "E"; break;
				}

				// Set the transfer mode
				response = mFtp.sendCommand("TYPE", modeStr);
			} else {
				// Failed to connect to the server
				response = Ftp::Response(Ftp::Response::ConnectionFailed);
			}
		}
	}

	return response;
}

void Ftp::DataChannel::Receive(std::ostream& stream) {
	// Receive data
	char buffer[1024];
	std::size_t received;

	while (mDataSocket.receive(buffer, sizeof(buffer), received) == Socket::Done) {
		stream.write(buffer, static_cast<std::streamsize>(received));

		if (!stream.good()) {
			eePRINTL( "FTP Error: Writing to the file has failed" );
			break;
		}
	}

	// Close the data socket
	mDataSocket.disconnect();
}

void Ftp::DataChannel::Send( std::istream& stream ) {
	// Send data
	char buffer[1024];
	std::size_t count;

	for (;;) {
		// read some data from the stream
		stream.read(buffer, sizeof(buffer));

		if (!stream.good() && !stream.eof()) {
			eePRINTL( "FTP Error: Reading from the file has failed" );
			break;
		}

		count = stream.gcount();

		if (count > 0) {
			// we could read more data from the stream: send them
			if (mDataSocket.send(buffer, count) != Socket::Done)
				break;
		} else {
			// no more data: exit the loop
			break;
		}
	}

	// Close the data socket
	mDataSocket.disconnect();
}

}}
