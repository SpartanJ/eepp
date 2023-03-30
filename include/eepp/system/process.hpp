#ifndef EE_SYSTEM_PROCESS_HPP
#define EE_SYSTEM_PROCESS_HPP

#include <eepp/config.hpp>
#include <eepp/system/mutex.hpp>
#include <functional>
#include <map>
#include <string>
#include <thread>

namespace EE { namespace System {

class EE_API Process {
  public:
	enum Options {
		// stdout and stderr are the same FILE.
		CombinedStdoutStderr = 0x1,

		// The child process should inherit the environment variables of the parent.
		InheritEnvironment = 0x2,

		// Enable asynchronous reading of stdout/stderr before it has completed.
		EnableAsync = 0x4,

		// Enable the child process to be spawned with no window visible if supported
		// by the platform.
		NoWindow = 0x8,

		// Search for program names in the PATH variable. Always enabled on Windows.
		// Note: this will **not** search for paths in any provided custom environment
		// and instead uses the PATH of the spawning process.
		SearchUserPath = 0x10
	};

	static inline constexpr Uint32 getDefaultOptions() {
		return Options::SearchUserPath | Options::InheritEnvironment | Options::NoWindow;
	}

	struct Config {
		size_t bufferSize = 131072;
		Uint32 options = getDefaultOptions();
	};

	typedef std::function<void( const char* bytes, size_t n )> ReadFn;

	Process();

	/** @brief Create a process.
	 ** @param command Command line to execute for this process.
	 ** @param options A bit field of Options's to pass. */
	Process( const std::string& command, const Uint32& options = getDefaultOptions(),
			 const std::unordered_map<std::string, std::string>& environment = {},
			 const std::string& workingDirectory = "", const size_t& bufferSize = 132072 );

	~Process();

	/** @brief Create a process.
	 ** @param command Command line to execute for this process.
	 ** @param options A bit field of Options's to pass.
	 ** @return On success true is returned. */
	bool create( const std::string& command, const Uint32& options = getDefaultOptions(),
				 const std::unordered_map<std::string, std::string>& environment = {},
				 const std::string& workingDirectory = "" );

	/** @brief Starts a new thread to receive all stdout and stderr data */
	void startAsyncRead( ReadFn readStdOut = nullptr, ReadFn readStdErr = nullptr );

	/** @brief Read all standard output from the child process.
	 ** @param buffer The buffer to read into.
	 ** @return The number of bytes actually read into buffer. Can only be 0 if the
	 ** process has complete.
	 **
	 ** The only safe way to read from the standard output of a process during it's
	 ** execution is to use the `Option::EnableAsync` option in
	 ** conjuction with this method. */
	size_t readAllStdOut( std::string& buffer );

	/** @brief Read the standard output from the child process.
	 ** @param buffer The buffer to read into.
	 ** @return The number of bytes actually read into buffer. Can only be 0 if the
	 ** process has complete.
	 **
	 ** The only safe way to read from the standard output of a process during it's
	 ** execution is to use the `Option::EnableAsync` option in
	 ** conjuction with this method. */
	size_t readStdOut( std::string& buffer );

	/** @brief Read the standard output from the child process.
	 ** @param buffer The buffer to read into.
	 ** @param size The maximum number of bytes to read.
	 ** @return The number of bytes actually read into buffer. Can only be 0 if the
	 ** process has complete.
	 **
	 ** The only safe way to read from the standard output of a process during it's
	 ** execution is to use the `Option::EnableAsync` option in
	 ** conjuction with this method. */
	size_t readStdOut( char* const buffer, const size_t& size );

	/** @brief Read all the standard error from the child process.
	 ** @param buffer The buffer to read into.
	 ** @return The number of bytes actually read into buffer. Can only be 0 if the
	 ** process has complete.
	 **
	 ** The only safe way to read from the standard error of a process during it's
	 ** execution is to use the `Option::EnableAsync` option in
	 ** conjuction with this method. */
	size_t readAllStdErr( std::string& buffer );

	/** @brief Read the standard error from the child process.
	 ** @param buffer The buffer to read into.
	 ** @return The number of bytes actually read into buffer. Can only be 0 if the
	 ** process has complete.
	 **
	 ** The only safe way to read from the standard error of a process during it's
	 ** execution is to use the `Option::EnableAsync` option in
	 ** conjuction with this method. */
	size_t readStdErr( std::string& buffer );

	/** @brief Read the standard error from the child process.
	 ** @param buffer The buffer to read into.
	 ** @param size The maximum number of bytes to read.
	 ** @return The number of bytes actually read into buffer. Can only be 0 if the
	 ** process has complete.
	 **
	 ** The only safe way to read from the standard error of a process during it's
	 ** execution is to use the `Option::EnableAsync` option in
	 ** conjuction with this method. */
	size_t readStdErr( char* const buffer, const size_t& size );

	/** @brief Write the standard output from the child process.
	 ** @param buffer The buffer to write into.
	 ** @param size The number of bytes to write.
	 ** @return The number of bytes actually written into buffer. */
	size_t write( const char* buffer, const size_t& size );

	/** @brief Write the standard output from the child process.
	 ** @param buffer The buffer to write into.
	 ** @return The number of bytes actually written into buffer. */
	size_t write( const std::string& buffer );

	/** @brief Wait for a process to finish execution.
	 ** @param returnCodeOut The return code of the returned process (can be nullptr).
	 ** @return On success true is returned.
	 **
	 ** Joining a process will close the stdin pipe to the process. */
	bool join( int* const returnCodeOut );

	/** @brief Terminate a previously created process.
	 ** @return On success true is returned.
	 **
	 ** If the process to be destroyed had not finished execution, it will be
	 ** terminated (i.e killed). */
	bool kill();

	/** @brief Destroy a previously created process.
	 ** @return On success true is returned.
	 **
	 ** If the process to be destroyed had not finished execution, it may out live
	 ** the parent process. */
	bool destroy();

	/** @brief Returns if the subprocess is currently still alive and executing.
	 ** @return If the process is still alive true returned. */
	bool isAlive();

	/** @brief Get the standard input file for a process.
	 ** @return The file for standard input of the process.
	 **
	 ** The file returned can be written to by the parent process to feed data to
	 ** the standard input of the process. */
	FILE* getStdIn() const;

	/** @brief Get the standard output file for a process.
	 ** @return The file for standard output of the process.
	 **
	 ** The file returned can be read from by the parent process to read data from
	 ** the standard output of the child process. */
	FILE* getStdOut() const;

	/** @brief Get the standard error file for a process.
	 ** @return The file for standard error of the process.
	 **
	 ** The file returned can be read from by the parent process to read data from
	 ** the standard error of the child process.
	 **
	 ** If the process was created with the Option::CombinedStdoutStderr
	 ** option bit set, this function will return NULL, and the getStdOut
	 ** function should be used for both the standard output and error combined. */
	FILE* getStdErr() const;

	/** Indicates that the process must start its shutdown */
	void startShutdown();

	/** Indicates if the process started its shutdown */
	const bool& isShuttingDown();

  protected:
	void* mProcess{ nullptr };
	bool mShuttingDown{ false };
	bool mIsAsync{ false };
	size_t mBufferSize{ 131072 };
	std::thread mStdOutThread;
	std::thread mStdErrThread;
	Mutex mStdInMutex;
	ReadFn mReadStdOutFn;
	ReadFn mReadStdErrFn;
};

}} // namespace EE::System

#endif // EE_SYSTEM_PROCESS_HPP
