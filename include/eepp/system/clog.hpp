#ifndef EECLOG_H
#define EECLOG_H

#include <list>
#include <eepp/system/base.hpp>
#include <eepp/system/tsingleton.hpp>
#include <eepp/system/ciostreamfile.hpp>
#include <eepp/system/cmutex.hpp>
#include <eepp/system/sys.hpp>

namespace EE { namespace System {

/** @brief The reader interface is usefull if you want to keep track of what is write in the log, for example for a console. */
class iLogReader {
	public:
		virtual void WriteLog( const std::string& Text ) = 0;
};

class EE_API cLog : protected cMutex {
	SINGLETON_DECLARE_HEADERS(cLog)

	public:
		void Save(const std::string& filepath = "" );

		void Write( std::string Text, const bool& newLine = true);

		void Writef( const char* format, ... );

		std::string Buffer() const;

		const bool& ConsoleOutput() const;

		void ConsoleOutput( const bool& output );

		const bool& LiveWrite() const;

		void LiveWrite( const bool& lw );

		void AddLogReader( iLogReader * reader );

		void RemoveLogReader( iLogReader * reader );

		~cLog();
	protected:
		cLog();

		std::string				mData;
		std::string				mFilePath;
		bool					mSave;
		bool					mConsoleOutput;
		bool					mLiveWrite;
		cIOStreamFile *			mFS;
		std::list<iLogReader*>	mReaders;

		void OpenFS();

		void CloseFS();

		void WriteToReaders( std::string& text );
};

}}
#endif
