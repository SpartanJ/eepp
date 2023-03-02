#include <cstdarg>
#include <eepp/system/log.hpp>
#include <iostream>

#if EE_PLATFORM == EE_PLATFORM_ANDROID
#include <android/log.h>
#endif

#if defined( EE_COMPILER_MSVC )
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace EE { namespace System {

SINGLETON_DECLARE_IMPLEMENTATION( Log )

std::unordered_map<std::string, LogLevel> Log::getMapFlag() {
	return { { "debug", LogLevel::Debug },	 { "info", LogLevel::Info },
			 { "notice", LogLevel::Notice }, { "warning", LogLevel::Warning },
			 { "error", LogLevel::Error },	 { "critical", LogLevel::Critical },
			 { "assert", LogLevel::Assert } };
}

Log* Log::create( const std::string& logPath, const LogLevel& level, bool consoleOutput,
				  bool liveWrite ) {
	if ( NULL == ms_singleton ) {
		ms_singleton = eeNew( Log, ( logPath, level, consoleOutput, liveWrite ) );
	} else {
		ms_singleton->setLogLevelThreshold( level );
		ms_singleton->setConsoleOutput( consoleOutput );
		ms_singleton->setLiveWrite( liveWrite );
	}
	return ms_singleton;
}

Log* Log::create( const LogLevel& level, bool consoleOutput, bool liveWrite ) {
	if ( NULL == ms_singleton ) {
		ms_singleton = eeNew( Log, ( "", level, consoleOutput, liveWrite ) );
	} else {
		ms_singleton->setLogLevelThreshold( level );
		ms_singleton->setConsoleOutput( consoleOutput );
		ms_singleton->setLiveWrite( liveWrite );
	}
	return ms_singleton;
}

Log::Log() : mSave( false ), mConsoleOutput( false ), mLiveWrite( false ), mFS( NULL ) {
	writel( LogLevel::Info, "eepp initialized" );
}

Log::Log( const std::string& logPath, const LogLevel& level, bool consoleOutput, bool liveWrite ) :
	mFilePath( logPath ),
	mSave( false ),
	mConsoleOutput( consoleOutput ),
	mLiveWrite( liveWrite ),
	mLogLevelThreshold( level ),
	mFS( NULL ) {
	writel( LogLevel::Info, "eepp initialized" );
}

const std::string& Log::getFilePath() const {
	return mFilePath;
}

void Log::setFilePath( const std::string& filePath ) {
	if ( filePath != mFilePath ) {
		closeFS();
		mFilePath = filePath;
	}
}

Log::~Log() {
	writel( LogLevel::Info, "eepp stoped\n" );

	if ( mSave && !mLiveWrite ) {
		openFS();

		mFS->write( mData.c_str(), mData.size() );
	}

	closeFS();
}

const LogLevel& Log::getLogLevelThreshold() const {
	return mLogLevelThreshold;
}

void Log::setLogLevelThreshold( const LogLevel& logLevelThreshold ) {
	mLogLevelThreshold = logLevelThreshold;
}

void Log::save( const std::string& filepath ) {
	if ( !filepath.empty() ) {
		mFilePath = filepath;
	} else {
		mFilePath = Sys::getProcessPath() + "log.log";
	}

	mSave = true;
}

void Log::write( const std::string& text ) {
	lock();
	mData += text;
	unlock();

	writeToReaders( text );

	if ( mConsoleOutput ) {
#if EE_PLATFORM == EE_PLATFORM_ANDROID
		__android_log_print( ANDROID_LOG_INFO, "eepp", "%s", text.c_str() );
#elif defined( EE_COMPILER_MSVC )
#ifdef UNICODE
		OutputDebugString( String::fromUtf8( text ).toWideString().c_str() );
#else
		OutputDebugString( text.c_str() );
#endif
#else
		std::cout << text;
#endif
	}

	if ( mLiveWrite ) {
		openFS();

		mFS->write( text.c_str(), text.size() );

		mFS->flush();
	}
}

static std::string logLevelToString( const LogLevel& level ) {
	switch ( level ) {
		case LogLevel::Info:
			return "INFO";
		case LogLevel::Notice:
			return "NOTICE";
		case LogLevel::Warning:
			return "WARNING";
		case LogLevel::Error:
			return "ERROR";
		case LogLevel::Critical:
			return "CRITICAL";
		case LogLevel::Assert:
			return "ASSERT";
		default:
		case LogLevel::Debug:
			return "DEBUG";
	}
}

static std::string logLevelWithTimestamp( const LogLevel& level, const std::string& text,
										  bool appendNewLine ) {
	return String::format( appendNewLine ? "%s - %s: %s\n" : "%s - %s: %s",
						   Sys::getDateTimeStr().c_str(), logLevelToString( level ).c_str(),
						   text.c_str() );
}

void Log::write( const LogLevel& level, const std::string& text ) {
	if ( level >= mLogLevelThreshold )
		write( logLevelWithTimestamp( level, text, false ) );
}

void Log::writel( const std::string& text ) {
	lock();
	mData += text;
	mData += "\n";
	unlock();

	writeToReaders( text );
	writeToReaders( "\n" );

	if ( mConsoleOutput ) {
#if EE_PLATFORM == EE_PLATFORM_ANDROID
		__android_log_print( ANDROID_LOG_INFO, "eepp", "%s\n", text.c_str() );
#elif defined( EE_COMPILER_MSVC )
#ifdef UNICODE
		OutputDebugString( String::fromUtf8( text + "\n" ).toWideString().c_str() );
#else
		OutputDebugString( ( text + "\n" ).c_str() );
#endif
#else
		std::cout << text << std::endl;
#endif
	}

	if ( mLiveWrite ) {
		openFS();

		mFS->write( text.c_str(), text.size() );
		mFS->write( "\n", 1 );

		mFS->flush();
	}
}

void Log::writel( const LogLevel& level, const std::string& text ) {
	if ( level >= mLogLevelThreshold )
		write( logLevelWithTimestamp( level, text, true ) );
}

void Log::openFS() {
	if ( mFilePath.empty() )
		mFilePath = Sys::getProcessPath() + "log.log";

	if ( NULL == mFS )
		mFS = IOStreamFile::New( mFilePath, "a" );
}

void Log::closeFS() {
	lock();

	eeSAFE_DELETE( mFS );

	unlock();
}

void Log::writef( const char* format, ... ) {
	int n, size = 256;
	std::string tstr( size, '\0' );

	va_list args;

	while ( 1 ) {
		va_start( args, format );

		n = vsnprintf( &tstr[0], size, format, args );

		if ( n > -1 && n < size ) {
			tstr.resize( n );
			tstr += '\n';

			lock();
			mData += tstr;
			unlock();

			writeToReaders( tstr );

			if ( mConsoleOutput ) {
#if EE_PLATFORM == EE_PLATFORM_ANDROID
				__android_log_print( ANDROID_LOG_INFO, "eepp", "%s", tstr.c_str() );
#elif defined( EE_COMPILER_MSVC )
#ifdef UNICODE
				OutputDebugString( String::fromUtf8( tstr ).toWideString().c_str() );
#else
				OutputDebugString( tstr.c_str() );
#endif
#else
				std::cout << tstr;
#endif
			}

			if ( mLiveWrite ) {
				openFS();

				mFS->write( tstr.c_str(), tstr.size() );

				mFS->flush();
			}

			va_end( args );

			return;
		}

		if ( n > -1 )	  // glibc 2.1
			size = n + 1; // precisely what is needed
		else			  // glibc 2.0
			size *= 2;	  // twice the old size

		tstr.resize( size, '\0' );
	}
}

void Log::writef( const LogLevel& level, const char* format, ... ) {
	if ( mLogLevelThreshold > level )
		return;

	int n, size = 256;
	std::string tstr( size, '\0' );
	bool first = true;
	va_list args;

	while ( 1 ) {
		va_start( args, format );

		n = vsnprintf( &tstr[0], size, format, args );

		if ( n > -1 && n < size ) {
			tstr.resize( n );
			tstr += '\n';

			if ( first ) {
				tstr = logLevelWithTimestamp( level, tstr, false );
				first = false;
			}

			lock();
			mData += tstr;
			unlock();

			writeToReaders( tstr );

			if ( mConsoleOutput ) {
#if EE_PLATFORM == EE_PLATFORM_ANDROID
				__android_log_print( ANDROID_LOG_INFO, "eepp", "%s", tstr.c_str() );
#elif defined( EE_COMPILER_MSVC )
#ifdef UNICODE
				OutputDebugString( String::fromUtf8( tstr ).toWideString().c_str() );
#else
				OutputDebugString( tstr.c_str() );
#endif
#else
				std::cout << tstr;
#endif
			}

			if ( mLiveWrite ) {
				openFS();

				mFS->write( tstr.c_str(), tstr.size() );

				mFS->flush();
			}

			va_end( args );

			return;
		}

		if ( n > -1 )	  // glibc 2.1
			size = n + 1; // precisely what is needed
		else			  // glibc 2.0
			size *= 2;	  // twice the old size

		tstr.resize( size, '\0' );
	}
}

std::string Log::getBuffer() const {
	return mData;
}

const bool& Log::isConsoleOutput() const {
	return mConsoleOutput;
}

void Log::setConsoleOutput( const bool& output ) {
	bool OldOutput = mConsoleOutput;

	mConsoleOutput = output;

	if ( !OldOutput && output ) {
		lock();
		std::string data( mData );
		mData = "";
		unlock();
		write( data );
	}
}

const bool& Log::isLiveWrite() const {
	return mLiveWrite;
}

void Log::setLiveWrite( const bool& lw ) {
	mLiveWrite = lw;
}

void Log::addLogReader( LogReaderInterface* reader ) {
	mReaders.push_back( reader );
}

void Log::removeLogReader( LogReaderInterface* reader ) {
	mReaders.remove( reader );
}

void Log::writeToReaders( const std::string& text ) {
	for ( std::list<LogReaderInterface*>::iterator it = mReaders.begin(); it != mReaders.end();
		  ++it ) {
		( *it )->writeLog( text );
	}
}

}} // namespace EE::System
