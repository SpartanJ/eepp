#ifndef EECLOG_H
#define EECLOG_H

#include <list>
#include <eepp/system/base.hpp>
#include <eepp/system/singleton.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/sys.hpp>

namespace EE { namespace System {

/** @brief The reader interface is useful if you want to keep track of what is write in the log, for example for a console. */
class LogReaderInterface {
	public:
		virtual void WriteLog( const std::string& Text ) = 0;
};

/** @brief Global log file. The engine will log everything in this file. */
class EE_API Log : protected Mutex {
	SINGLETON_DECLARE_HEADERS(Log)

	public:
		/** @brief Indicates that the log must be writed to a file when the Log instance is closed.
		**	@param filepath The path to the file to write the log.
		*/
		void Save( const std::string& filepath = "" );

		/** @brief Writes the text to the log
		**	@param Text The text to write
		**	@param newLine Indicates if a new line character is appended at the end of the text
		*/
		void Write( std::string Text, const bool& newLine = true);

		/** @brief Writes a formated string to the log */
		void Writef( const char* format, ... );

		/** @returns A copy of the current writed log. */
		std::string Buffer() const;

		/** @returns If the log Writes are outputed to the terminal. */
		const bool& ConsoleOutput() const;

		/** @brief Enabled or disables to output the Writes to the terminal. */
		void ConsoleOutput( const bool& output );

		/** @returns If the file is forced to flush the data on every Write call. */
		const bool& LiveWrite() const;

		/** @brief Activate or deactivate to flush the writed data to the log on every Write call. */
		void LiveWrite( const bool& lw );

		/** @brief Adds a reader interface.
		**	The reader interface is used to the informed for every writed text to the log.
		*/
		void AddLogReader( LogReaderInterface * reader );

		/** @brief Removes the reader interface */
		void RemoveLogReader( LogReaderInterface * reader );

		~Log();
	protected:
		Log();

		std::string				mData;
		std::string				mFilePath;
		bool					mSave;
		bool					mConsoleOutput;
		bool					mLiveWrite;
		IOStreamFile *			mFS;
		std::list<LogReaderInterface*>	mReaders;

		void OpenFS();

		void CloseFS();

		void WriteToReaders( std::string& text );
};

}}
#endif
