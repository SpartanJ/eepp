#include <algorithm>
#include <cstdarg>
#include <eepp/system/log.hpp>
#include <iostream>

#if EE_PLATFORM == EE_PLATFORM_ANDROID
#include <android/log.h>
#elif EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
#include <emscripten/console.h>
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

Log* Log::create( const std::string& logPath, const LogLevel& level, bool stdOutLog,
				  bool liveWrite ) {
	if ( NULL == ms_singleton ) {
		ms_singleton = eeNew( Log, ( logPath, level, stdOutLog, liveWrite ) );
	} else {
		ms_singleton->setLogLevelThreshold( level );
		ms_singleton->setLogToStdOut( stdOutLog );
		ms_singleton->setLiveWrite( liveWrite );
	}
	return ms_singleton;
}

Log* Log::create( const LogLevel& level, bool stdOutLog, bool liveWrite ) {
	if ( NULL == ms_singleton ) {
		ms_singleton = eeNew( Log, ( "", level, stdOutLog, liveWrite ) );
	} else {
		ms_singleton->setLogLevelThreshold( level );
		ms_singleton->setLogToStdOut( stdOutLog );
		ms_singleton->setLiveWrite( liveWrite );
	}
	return ms_singleton;
}

Log::Log() : mSave( false ), mStdOutEnabled( false ), mLiveWrite( false ), mFS( NULL ) {
	writel( LogLevel::Info, "eepp initialized" );
}

Log::Log( const std::string& logPath, const LogLevel& level, bool stdOutLog, bool liveWrite ) :
	mFilePath( logPath ),
	mSave( false ),
	mStdOutEnabled( stdOutLog ),
	mLiveWrite( liveWrite ),
	mLogLevelThreshold( level ),
	mFS( NULL ) {
	writel( LogLevel::Info, "eepp initialized" );
}

bool Log::getKeepLog() const {
	return mKeepLog;
}

void Log::setKeepLog( bool keepLog ) {
	mKeepLog = keepLog;
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
	writel( LogLevel::Info, "eepp stopped\n" );

	if ( mSave && !mLiveWrite && mKeepLog ) {
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

void Log::write( const std::string_view& text ) {
	if ( mKeepLog ) {
		lock();
		mData += text;
		unlock();
	}

	writeToReaders( text );

	if ( mStdOutEnabled ) {
#if EE_PLATFORM == EE_PLATFORM_ANDROID
		__android_log_print( ANDROID_LOG_INFO, "eepp", "%s", text.data() );
#elif defined( EE_COMPILER_MSVC )
#ifdef UNICODE
		OutputDebugString( String::fromUtf8( text ).toWideString().c_str() );
#else
		OutputDebugString( text.c_str() );
#endif
#elif EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
		emscripten_console_log( text.data() );
#else
		std::cout << text << std::flush;
#endif
	}

	if ( mLiveWrite ) {
		openFS();

		mFS->write( text.data(), text.size() );

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

std::string Log::logLevelWithTimestamp( const LogLevel& level, const std::string_view& text,
										bool appendNewLine ) {
	return String::format( appendNewLine ? "%s - %s: %s\n" : "%s - %s: %s",
						   Sys::getDateTimeStr().c_str(), logLevelToString( level ).c_str(),
						   text.data() );
}

void Log::write( const LogLevel& level, const std::string_view& text ) {
	if ( level >= mLogLevelThreshold )
		write( logLevelWithTimestamp( level, text, false ) );
}

void Log::writel( const std::string_view& text ) {
	if ( mKeepLog ) {
		lock();
		mData += text;
		mData += "\n";
		unlock();
	}

	writeToReaders( text );
	writeToReaders( "\n" );

	if ( mStdOutEnabled ) {
#if EE_PLATFORM == EE_PLATFORM_ANDROID
		__android_log_print( ANDROID_LOG_INFO, "eepp", "%s\n", text.data() );
#elif defined( EE_COMPILER_MSVC )
#ifdef UNICODE
		OutputDebugString( String::fromUtf8( text ).toWideString().c_str() );
		OutputDebugString( String::fromUtf8( std::string_view{ "\n" } ).toWideString().c_str() );
#else
		OutputDebugString( text.data() );
		OutputDebugString( "\n" );
#endif
#elif EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
		emscripten_console_log( text.data() );
#else
		std::cout << text << std::endl;
#endif
	}

	if ( mLiveWrite ) {
		openFS();

		mFS->write( text.data(), text.size() );
		mFS->write( "\n", 1 );

		mFS->flush();
	}
}

void Log::writel( const LogLevel& level, const std::string_view& text ) {
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

const std::string& Log::getBuffer() const {
	return mData;
}

const bool& Log::isLoggingToStdOut() const {
	return mStdOutEnabled;
}

void Log::setLogToStdOut( const bool& output ) {
	bool OldOutput = mStdOutEnabled;

	mStdOutEnabled = output;

	if ( !OldOutput && output && !mData.empty() ) {
		lock();
		std::string data( std::move( mData ) );
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
	auto found = std::find( mReaders.begin(), mReaders.end(), reader );
	if ( found != mReaders.end() )
		mReaders.erase( found );
}

void Log::writeToReaders( const std::string_view& text ) {
	for ( const auto& reader : mReaders )
		reader->writeLog( text );
}

}} // namespace EE::System
