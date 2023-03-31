#ifndef EECLOG_H
#define EECLOG_H

#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/singleton.hpp>
#include <eepp/system/sys.hpp>
#include <list>
#include <unordered_map>

namespace EE { namespace System {

/** @brief The reader interface is useful if you want to keep track of what is write in the log, for
 * example for a console. */
class LogReaderInterface {
  public:
	virtual void writeLog( const std::string& Text ) = 0;
};

enum class LogLevel : int {
	Debug,	 ///< Detailed debug information.
	Info,	 ///< Interesting events in your application.
	Notice,	 ///< Normal, but significant events in your application.
	Warning, ///< Exceptional occurrences that are not errors.
	Error,	 ///< Runtime errors that do not require immediate action but should typically be
		   ///< logged and monitored.
	Critical, ///< Critical conditions.
	Assert,	  ///< Asserted critical condition.
};

/** @brief Global log file. The engine will log everything in this file. */
class EE_API Log : protected Mutex {
	SINGLETON_DECLARE_HEADERS( Log )

  public:
	static std::unordered_map<std::string, LogLevel> getMapFlag();

	static constexpr LogLevel getDefaultLogLevel() {
#ifdef EE_DEBUG
		return LogLevel::Debug;
#else
		return LogLevel::Info;
#endif
	}

	static Log* create( const std::string& logPath, const LogLevel& level, bool consoleOutput,
						bool liveWrite );

	static Log* create( const LogLevel& level, bool consoleOutput, bool liveWrite );

	virtual ~Log();

	/** @brief Indicates that the log must be writed to a file when the Log instance is closed.
	**	@param filepath The path to the file to write the log.
	*/
	void save( const std::string& filepath = "" );

	/** @brief Writes the text to the log
	**	@param text The text to write */
	void write( const std::string& text );

	/** @brief Writes the text to the log with a log level.
	 ** @param level The log level that will try to write.
	 ** @param text The text to write */
	void write( const LogLevel& level, const std::string& text );

	/** @brief Writes the text to the log and appends a new line character at the end.
	**	@param text The text to write */
	void writel( const std::string& text );

	/** @brief Writes the text to the log and appends a new line character at the end with a log
	 *level.
	 ** @param levelThe log level that will try to write.
	 ** @param text The text to write */
	void writel( const LogLevel& level, const std::string& text );

	/** @brief Writes a formated string to the log with a log level.
	 ** @param level The log level that will try to write.
	 ** @param format The Text format.
	 */
	void writef( const LogLevel& level, const char* format, ... );

	/** @brief Writes a formated string to the log */
	void writef( const char* format, ... );

	/** @returns A reference of the current writed log. */
	const std::string& getBuffer() const;

	/** @returns If the log Writes are outputed to stdout. */
	const bool& isLoggingToStdOut() const;

	/** @brief Enabled or disables to output the Writes to stdout. */
	void setLogToStdOut( const bool& output );

	/** @returns If the file is forced to flush the data on every Write call. */
	const bool& isLiveWrite() const;

	/** @brief Activate or deactivate to flush the writed data to the log on every Write call. */
	void setLiveWrite( const bool& lw );

	/** @brief Adds a reader interface.
	**	The reader interface is used to the informed for every writed text to the log.
	*/
	void addLogReader( LogReaderInterface* reader );

	/** @brief Removes the reader interface */
	void removeLogReader( LogReaderInterface* reader );

	/** @return The log level threshold. */
	const LogLevel& getLogLevelThreshold() const;

	/** Sets the log level threshold. This is the minimum level that message will actually be
	 * logged. */
	void setLogLevelThreshold( const LogLevel& logLevelThreshold );

	/** @return The file path of the log file (if any). */
	const std::string& getFilePath() const;

	/** Sets the file path of the log file. */
	void setFilePath( const std::string& filePath );

	/** @return True if the logs are being buffered in memory */
	bool getKeepLog() const;

	/** Enable/Disable to keep a copy of the logs into memory (disabled by default) */
	void setKeepLog( bool keepLog );

	static void debug( const std::string& text ) {
		Log::instance()->writel( LogLevel::Debug, text );
	}

	static void info( const std::string& text ) { Log::instance()->writel( LogLevel::Info, text ); }

	static void notice( const std::string& text ) {
		Log::instance()->writel( LogLevel::Notice, text );
	}

	static void warning( const std::string& text ) {
		Log::instance()->writel( LogLevel::Warning, text );
	}

	static void error( const std::string& text ) {
		Log::instance()->writel( LogLevel::Error, text );
	}

	static void critical( const std::string& text ) {
		Log::instance()->writel( LogLevel::Critical, text );
	}

	static void assertLog( const std::string& text ) {
		Log::instance()->writel( LogLevel::Assert, text );
	}

	template <class... Args> static void debug( const char* format, Args&&... args ) {
		Log::instance()->writef( LogLevel::Debug, format, std::forward<Args>( args )... );
	}

	template <class... Args> static void info( const char* format, Args&&... args ) {
		Log::instance()->writef( LogLevel::Info, format, std::forward<Args>( args )... );
	}

	template <class... Args> static void notice( const char* format, Args&&... args ) {
		Log::instance()->writef( LogLevel::Notice, format, std::forward<Args>( args )... );
	}

	template <class... Args> static void warning( const char* format, Args&&... args ) {
		Log::instance()->writef( LogLevel::Warning, format, std::forward<Args>( args )... );
	}

	template <class... Args> static void error( const char* format, Args&&... args ) {
		Log::instance()->writef( LogLevel::Error, format, std::forward<Args>( args )... );
	}

	template <class... Args> static void critical( const char* format, Args&&... args ) {
		Log::instance()->writef( LogLevel::Critical, format, std::forward<Args>( args )... );
	}

	template <class... Args> static void assertLog( const char* format, Args&&... args ) {
		Log::instance()->writef( LogLevel::Assert, format, std::forward<Args>( args )... );
	}

  protected:
	Log();

	Log( const std::string& logPath, const LogLevel& level, bool consoleOutput, bool liveWrite );

	std::string mData;
	std::string mFilePath;
	bool mSave;
	bool mConsoleOutput;
	bool mLiveWrite;
	bool mKeepLog{ false };
	LogLevel mLogLevelThreshold{ getDefaultLogLevel() };
	IOStreamFile* mFS;
	std::list<LogReaderInterface*> mReaders;

	void openFS();

	void closeFS();

	void writeToReaders( const std::string& text );
};

}} // namespace EE::System
#endif
