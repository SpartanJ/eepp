#ifndef EECLOG_H
#define EECLOG_H

#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/singleton.hpp>
#include <eepp/system/sys.hpp>
#include <list>

namespace EE { namespace System {

/** @brief The reader interface is useful if you want to keep track of what is write in the log, for
 * example for a console. */
class LogReaderInterface {
  public:
	virtual void writeLog( const std::string& Text ) = 0;
};

/** @brief Global log file. The engine will log everything in this file. */
class EE_API Log : protected Mutex {
	SINGLETON_DECLARE_HEADERS( Log )

  public:
	/** @brief Indicates that the log must be writed to a file when the Log instance is closed.
	**	@param filepath The path to the file to write the log.
	*/
	void save( const std::string& filepath = "" );

	/** @brief Writes the text to the log
	**	@param Text The text to write
	**	@param newLine Indicates if a new line character is appended at the end of the text
	*/
	void write( std::string Text, const bool& newLine = true );

	/** @brief Writes a formated string to the log */
	void writef( const char* format, ... );

	/** @returns A copy of the current writed log. */
	std::string getBuffer() const;

	/** @returns If the log Writes are outputed to the terminal. */
	const bool& isConsoleOutput() const;

	/** @brief Enabled or disables to output the Writes to the terminal. */
	void setConsoleOutput( const bool& output );

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

	virtual ~Log();

  protected:
	Log();

	std::string mData;
	std::string mFilePath;
	bool mSave;
	bool mConsoleOutput;
	bool mLiveWrite;
	IOStreamFile* mFS;
	std::list<LogReaderInterface*> mReaders;

	void openFS();

	void closeFS();

	void writeToReaders( std::string& text );
};

}} // namespace EE::System
#endif
