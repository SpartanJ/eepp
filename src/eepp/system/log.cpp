#include <algorithm>
#include <cstdio>
#include <eepp/system/inifile.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/log.hpp>
#include <filesystem>
#include <iostream>
#include <limits>

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

namespace {

namespace fs = std::filesystem;
constexpr Time RetryDelay = Seconds( 5 );

bool retryReady( MonotonicClock::TimePoint retry ) {
	return retry == MonotonicClock::TimePoint{} || MonotonicClock::now() >= retry;
}

void reportFileError( const char* error ) {
	if ( error )
		std::fprintf( stderr, "eepp log: %s\n", error );
}

fs::path nativeLogPath( const std::string& path ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return String::fromUtf8( path ).toWideString();
#else
	return path;
#endif
}

fs::file_type fileType( const fs::path& path ) {
	std::error_code error;
	const auto status = fs::symlink_status( path, error );
	return !error || error == std::errc::no_such_file_or_directory ? status.type()
																   : fs::file_type::none;
}

fs::path archivePath( const fs::path& active, Uint32 index ) {
	auto path = active;
	path += '.';
	path += std::to_string( index );
	return path;
}

bool moveLogFile( const fs::path& from, const fs::path& to ) {
	const auto sourceType = fileType( from );
	if ( sourceType == fs::file_type::not_found )
		return true;
	if ( sourceType != fs::file_type::regular || fileType( to ) != fs::file_type::not_found )
		return false;
	std::error_code error;
	fs::rename( from, to, error );
	return !error;
}

} // namespace

SINGLETON_DECLARE_IMPLEMENTATION( Log )

void LoggingConfig::load( const IniFile& ini ) {
	const LoggingConfig defaults;
	auto enabled = String::toLower( ini.getValue( "logging", "rotation_enabled" ) );
	String::trimInPlace( enabled, " \t\r" );
	rotationEnabled = enabled != "false" && enabled != "0" && enabled != "no";

	Int32 value;
	auto size = ini.getValue( "logging", "max_file_size_mib" );
	String::trimInPlace( size, " \t\r" );
	setMaxFileSizeMiB( String::fromString( value, size ) ? value : defaults.mMaxFileSizeMiB );
	auto backups = ini.getValue( "logging", "max_backup_files" );
	String::trimInPlace( backups, " \t\r" );
	setMaxBackupFiles( String::fromString( value, backups ) ? value : defaults.mMaxBackupFiles );
}

void LoggingConfig::save( IniFile& ini ) const {
	ini.setValue( "logging", "rotation_enabled", rotationEnabled ? "true" : "false" );
	ini.setValueI( "logging", "max_file_size_mib", mMaxFileSizeMiB );
	ini.setValueI( "logging", "max_backup_files", mMaxBackupFiles );
}

void LoggingConfig::setMaxFileSizeMiB( int size ) {
	mMaxFileSizeMiB = std::clamp( size, MinFileSizeMiB, MaxFileSizeMiB );
}

void LoggingConfig::setMaxBackupFiles( int count ) {
	mMaxBackupFiles = std::clamp( count, MinBackupFiles, MaxBackupFiles );
}

LogRotationConfig LoggingConfig::rotation() const {
	return { rotationEnabled ? static_cast<Uint64>( mMaxFileSizeMiB ) * 1024 * 1024 : 0,
			 static_cast<Uint32>( mMaxBackupFiles ) };
}

std::unordered_map<std::string, LogLevel> Log::getMapFlag() {
	return { { "debug", LogLevel::Debug },	 { "info", LogLevel::Info },
			 { "notice", LogLevel::Notice }, { "warning", LogLevel::Warning },
			 { "error", LogLevel::Error },	 { "critical", LogLevel::Critical },
			 { "assert", LogLevel::Assert } };
}

Log* Log::create( const std::string& logPath, const LogLevel& level, bool stdOutLog,
				  bool liveWrite ) {
	return createImpl( logPath, level, stdOutLog, liveWrite, nullptr );
}

Log* Log::create( const std::string& logPath, const LogLevel& level, bool stdOutLog, bool liveWrite,
				  const LogRotationConfig& rotation ) {
	return createImpl( logPath, level, stdOutLog, liveWrite, &rotation );
}

Log* Log::create( const LogLevel& level, bool stdOutLog, bool liveWrite ) {
	return createImpl( "", level, stdOutLog, liveWrite, nullptr );
}

Log* Log::createImpl( const std::string& logPath, const LogLevel& level, bool stdOutLog,
					  bool liveWrite, const LogRotationConfig* rotation ) {
	Log* singleton = ms_singleton.load( std::memory_order_acquire );
	if ( NULL == singleton ) {
		Lock l( ms_mutex );
		singleton = ms_singleton.load( std::memory_order_acquire );
		if ( NULL == singleton ) {
			singleton = eeNew( Log, ( logPath, level, stdOutLog, liveWrite,
									  rotation ? *rotation : LogRotationConfig{} ) );
			ms_singleton.store( singleton, std::memory_order_release );
			return singleton;
		}
	}

	singleton->setLogLevelThreshold( level );
	if ( rotation ) {
		bool closed;
		{
			Lock lock( singleton->mFileMutex );
			closed = singleton->setFilePathLocked( logPath );
			closed = singleton->setRotationConfigLocked( *rotation ) && closed;
			singleton->mLiveWrite = liveWrite;
			if ( !liveWrite )
				closed = singleton->closeFSLocked() && closed;
		}
		if ( !closed )
			reportFileError( "could not close the active file" );
		singleton->setLogToStdOut( stdOutLog );
	} else {
		// The legacy overloads keep the path and replay any stdout buffer as before.
		singleton->setLogToStdOut( stdOutLog );
		singleton->setLiveWrite( liveWrite );
	}
	return singleton;
}

Log::Log() : Log( "", getDefaultLogLevel(), false, false, {} ) {}

Log::Log( const std::string& logPath, const LogLevel& level, bool stdOutLog, bool liveWrite ) :
	Log( logPath, level, stdOutLog, liveWrite, {} ) {}

Log::Log( const std::string& logPath, const LogLevel& level, bool stdOutLog, bool liveWrite,
		  const LogRotationConfig& rotation ) :
	mFilePath( logPath ),
	mSave( false ),
	mStdOutEnabled( stdOutLog ),
	mLiveWrite( liveWrite ),
	mLogLevelThreshold( level ),
	mFS( nullptr ) {
	setRotationConfigLocked( rotation );
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
	bool closed;
	{
		Lock lock( mFileMutex );
		closed = setFilePathLocked( filePath );
	}
	if ( !closed )
		reportFileError( "could not close the active file" );
}

bool Log::setFilePathLocked( const std::string& filePath ) {
	if ( filePath == mFilePath )
		return true;
	const bool closed = closeFSLocked();
	mFilePath = filePath;
	mOpenRetry = mRotationRetry = mCleanupRetry = {};
	mCleanupPending = mRotation.maxFileSize != 0;
	return closed;
}

void Log::setRotationConfig( const LogRotationConfig& config ) {
	bool closed;
	{
		Lock lock( mFileMutex );
		closed = setRotationConfigLocked( config );
	}
	if ( !closed )
		reportFileError( "could not close the active file" );
}

LogRotationConfig Log::getRotationConfig() const {
	Lock lock( mFileMutex );
	return mRotation;
}

bool Log::setRotationConfigLocked( const LogRotationConfig& config ) {
	const auto backups = std::min( config.maxBackupFiles, LogRotationConfig::MaxBackupFiles );
	if ( config.maxFileSize == mRotation.maxFileSize && backups == mRotation.maxBackupFiles )
		return true;

	const bool wasEnabled = mRotation.maxFileSize != 0;
	const bool enabled = config.maxFileSize != 0;
	const bool closed = enabled == wasEnabled || closeFSLocked();
	if ( enabled != wasEnabled )
		mOpenRetry = {};
	if ( enabled && ( !wasEnabled || backups < mRotation.maxBackupFiles ) ) {
		mCleanupPending = true;
		mCleanupRetry = {};
	} else if ( !enabled ) {
		mCleanupPending = false;
	}
	mRotationRetry = {};
	mRotation = { config.maxFileSize, backups };
	return closed;
}

Log::~Log() {
	writel( LogLevel::Info, "eepp stopped\n" );

	if ( mKeepLog )
		writeToFile( mData, false, true );
	closeFS();
}

const LogLevel& Log::getLogLevelThreshold() const {
	return mLogLevelThreshold;
}

void Log::setLogLevelThreshold( const LogLevel& logLevelThreshold ) {
	mLogLevelThreshold = logLevelThreshold;
}

void Log::save( const std::string& filepath ) {
	bool closed;
	{
		Lock lock( mFileMutex );
		closed =
			setFilePathLocked( !filepath.empty() ? filepath : Sys::getProcessPath() + "log.log" );
		mSave = true;
	}
	if ( !closed )
		reportFileError( "could not close the active file" );
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

	writeToFile( text, false );
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

	writeToFile( text, true );
}

void Log::writel( const LogLevel& level, const std::string_view& text ) {
	if ( level >= mLogLevelThreshold )
		write( logLevelWithTimestamp( level, text, true ) );
}

void Log::openFS() {
	bool opened;
	{
		Lock lock( mFileMutex );
		opened = openFSLocked();
	}
	if ( !opened )
		reportFileError( "could not open the active file" );
}

bool Log::openFSLocked() {
	if ( mFS )
		return true;
	if ( mFilePath.empty() )
		mFilePath = Sys::getProcessPath() + "log.log";

	if ( mRotation.maxFileSize ) {
		const auto type = fileType( nativeLogPath( mFilePath ) );
		if ( type != fs::file_type::not_found && type != fs::file_type::regular )
			return false;
	}
	mFS = IOStreamFile::New( mFilePath, mRotation.maxFileSize ? "ab" : "a" );
	if ( mFS->isOpen() )
		return true;
	eeSAFE_DELETE( mFS );
	return false;
}

void Log::closeFS() {
	bool closed;
	{
		Lock lock( mFileMutex );
		closed = closeFSLocked();
	}
	if ( !closed )
		reportFileError( "could not close the active file" );
}

bool Log::closeFSLocked() {
	const bool closed = !mFS || mFS->close();
	eeSAFE_DELETE( mFS );
	mFileSizeValid = false;
	mCurrentFileSize = 0;
	mSizeRetry = {};
	return closed;
}

bool Log::updateFileSizeLocked() {
	std::error_code error;
	const auto size = fs::file_size( nativeLogPath( mFilePath ), error );
	mFileSizeValid = !error && size <= std::numeric_limits<Uint64>::max();
	if ( mFileSizeValid )
		mCurrentFileSize = static_cast<Uint64>( size );
	else
		mSizeRetry = MonotonicClock::now() + RetryDelay;
	return mFileSizeValid;
}

bool Log::rotateFilesLocked() {
	const bool flushed = mFS->flush();
	if ( !closeFSLocked() || !flushed )
		return false;

	const auto backups = mRotation.maxBackupFiles;
	if ( backups == 0 ) {
		IOStreamFile truncated( mFilePath, "wb" );
		return truncated.isOpen() && truncated.close();
	}

	const auto active = nativeLogPath( mFilePath );
	const auto oldest = archivePath( active, backups );
	const auto type = fileType( oldest );
	if ( type != fs::file_type::not_found ) {
		std::error_code error;
		if ( type != fs::file_type::regular || !fs::remove( oldest, error ) || error )
			return false;
	}
	for ( auto index = backups; index > 1; --index ) {
		if ( !moveLogFile( archivePath( active, index - 1 ), archivePath( active, index ) ) )
			return false;
	}
	return moveLogFile( active, archivePath( active, 1 ) );
}

bool Log::pruneArchivesLocked() {
	const auto active = nativeLogPath( mFilePath );
	auto archivePrefix = active.filename().native();
	archivePrefix += '.';
	const auto parent = active.has_parent_path() ? active.parent_path() : fs::path( "." );
	std::error_code error;
	bool success = true;
	for ( fs::directory_iterator entry( parent, error ), end; !error && entry != end;
		  entry.increment( error ) ) {
		const auto name = entry->path().filename().native();
		if ( name.size() <= archivePrefix.size() ||
			 name.compare( 0, archivePrefix.size(), archivePrefix ) != 0 ||
			 name[archivePrefix.size()] == '0' )
			continue;
		Uint32 index = 0;
		bool numeric = true;
		for ( auto pos = archivePrefix.size(); pos < name.size(); ++pos ) {
			if ( name[pos] < '0' || name[pos] > '9' ) {
				numeric = false;
				break;
			}
			// Once this is beyond the limit, only the remaining suffix syntax matters.
			if ( index <= mRotation.maxBackupFiles )
				index = index * 10 + ( name[pos] - '0' );
		}
		if ( !numeric || index <= mRotation.maxBackupFiles )
			continue;
		std::error_code entryError;
		const auto type = entry->symlink_status( entryError ).type();
		if ( !entryError && type == fs::file_type::regular ) {
			if ( !fs::remove( entry->path(), entryError ) )
				success = false;
		}
		success = success && !entryError;
	}
	return success && !error;
}

void Log::writeToFile( std::string_view text, bool appendNewLine, bool saving ) {
	if ( text.empty() && !appendNewLine )
		return;
	const char* error = nullptr;
	{
		Lock lock( mFileMutex );
		if ( saving ? mSave && !mLiveWrite : mLiveWrite )
			error = writeToFileLocked( text, appendNewLine );
	}
	reportFileError( error );
}

const char* Log::writeToFileLocked( std::string_view text, bool appendNewLine ) {
	if ( !mFS ) {
		if ( !retryReady( mOpenRetry ) )
			return nullptr;
		if ( !openFSLocked() ) {
			mOpenRetry = MonotonicClock::now() + RetryDelay;
			return "could not open the active file";
		}
		mOpenRetry = {};
	}

	const char* error = nullptr;
	const auto limit = mRotation.maxFileSize;
	const auto incoming = static_cast<Uint64>( text.size() ) + appendNewLine;
	if ( limit ) {
		if ( mCleanupPending && retryReady( mCleanupRetry ) ) {
			mCleanupPending = !pruneArchivesLocked();
			if ( mCleanupPending ) {
				mCleanupRetry = MonotonicClock::now() + RetryDelay;
				error = "could not prune old archives";
			}
		}
		if ( !mFileSizeValid && retryReady( mSizeRetry ) && !updateFileSizeLocked() )
			error = "could not read the active file size";

		if ( mFileSizeValid && mCurrentFileSize != 0 &&
			 ( mCurrentFileSize >= limit || incoming > limit - mCurrentFileSize ) &&
			 retryReady( mRotationRetry ) ) {
			if ( !rotateFilesLocked() ) {
				mRotationRetry = MonotonicClock::now() + RetryDelay;
				error = "could not rotate the active file";
			} else {
				mRotationRetry = {};
			}
			if ( !openFSLocked() ) {
				mOpenRetry = MonotonicClock::now() + RetryDelay;
				return "could not reopen the active file after rotation";
			}
			if ( !updateFileSizeLocked() )
				error = "could not read the active file size after rotation";
		}
	}

	const auto size = static_cast<ios_size>( text.size() );
	const bool written = ( size == 0 || mFS->write( text.data(), size ) == size ) &&
						 ( !appendNewLine || mFS->write( "\n", 1 ) == 1 );
	const bool flushed = mFS->flush();
	if ( !written || !flushed ) {
		closeFSLocked();
		mOpenRetry = MonotonicClock::now() + RetryDelay;
		return "could not write or flush the active file";
	}
	if ( mFileSizeValid ) {
		const auto available = std::numeric_limits<Uint64>::max() - mCurrentFileSize;
		mCurrentFileSize += std::min( incoming, available );
	}
	return error;
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
	bool closed = true;
	{
		Lock lock( mFileMutex );
		if ( mLiveWrite != lw ) {
			mLiveWrite = lw;
			mOpenRetry = {};
			if ( !mLiveWrite )
				closed = closeFSLocked();
		}
	}
	if ( !closed )
		reportFileError( "could not close the active file" );
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
