#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "utest.hpp"
#include <array>
#include <eepp/core/containers.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/logfilefamily.hpp>
#include <filesystem>
#include <thread>

using namespace EE;
using namespace EE::System;

namespace {

class LogTestDirectory {
  public:
	LogTestDirectory() : path( FileSystem::createTempDirectory( "eepp-log-" ) ) {}

	~LogTestDirectory() {
		if ( !path.empty() )
			FileSystem::dirRemoveAll( path );
	}

	std::string file( std::string_view name = "log.txt" ) const {
		return path + FileSystem::getOSSlash() + std::string( name );
	}

	std::filesystem::path nativeFile( std::string_view name ) const {
#if EE_PLATFORM == EE_PLATFORM_WIN
		return String::fromUtf8( file( name ) ).toWideString();
#else
		return file( name );
#endif
	}

	const std::string path;
};

class TestFileLog : public Log {
  public:
	explicit TestFileLog( const std::string& path, bool live = true ) :
		Log( path, LogLevel::Critical, false, live ) {}
};

std::string readLog( const std::string& path ) {
	std::string data;
	FileSystem::fileGet( path, data );
	return data;
}

class ReentrantLogReader : public LogReaderInterface {
  public:
	explicit ReentrantLogReader( Log& log ) : mLog( log ) {}

	void writeLog( const std::string_view& text ) {
		mText += text;
		if ( text == "outer" )
			mLog.write( "inner" );
	}

	std::string mText;

  private:
	Log& mLog;
};

} // namespace

UTEST( IOStreamFile, reportsWriteFlushAndCloseResults ) {
	LogTestDirectory directory;
	ASSERT_FALSE( directory.path.empty() );
	const auto path = directory.file();
	{
		IOStreamFile output( path, "wb" );
		ASSERT_TRUE( output.isOpen() );
		EXPECT_EQ( output.write( "ignored", -1 ), 0 );
		EXPECT_EQ( output.write( "abc", 3 ), 3 );
		EXPECT_TRUE( output.flush() );
		EXPECT_TRUE( output.close() );
		EXPECT_EQ( output.write( "x", 1 ), 0 );
		EXPECT_FALSE( output.flush() );
		EXPECT_TRUE( output.close() );
	}
	IOStreamFile input( path );
	ASSERT_TRUE( input.isOpen() );
	EXPECT_EQ( input.write( "x", 1 ), 0 );
	EXPECT_TRUE( readLog( path ) == "abc" );
}

UTEST( LoggingConfig, validatesAndRoundTripsIniPreferences ) {
	LoggingConfig config;
	IniFile ini;
	config.load( ini );
	EXPECT_TRUE( config.rotationEnabled );
	EXPECT_EQ( config.rotation().maxFileSize, 5u * 1024 * 1024 );
	EXPECT_EQ( config.rotation().maxBackupFiles, 3u );

	ini.setValue( "logging", "rotation_enabled", "invalid" );
	ini.setValue( "logging", "max_file_size_mib", "1 MiB" );
	ini.setValueI( "logging", "max_backup_files", -5 );
	config.load( ini );
	EXPECT_TRUE( config.rotationEnabled );
	EXPECT_EQ( config.getMaxFileSizeMiB(), 5 );
	EXPECT_EQ( config.getMaxBackupFiles(), 0 );

	config.rotationEnabled = false;
	config.setMaxFileSizeMiB( 2048 );
	config.setMaxBackupFiles( 12 );
	config.save( ini );
	LoggingConfig loaded;
	loaded.load( ini );
	EXPECT_FALSE( loaded.rotationEnabled );
	EXPECT_EQ( loaded.rotation().maxFileSize, 0u );
	EXPECT_EQ( loaded.getMaxFileSizeMiB(), 1024 );
	EXPECT_EQ( loaded.getMaxBackupFiles(), 12 );
}

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_LINUX || \
	EE_PLATFORM == EE_PLATFORM_MACOS
UTEST( LogFileFamily, usesRequestedPathAndReusesReleasedSlots ) {
	LogTestDirectory directory;
	ASSERT_FALSE( directory.path.empty() );
	const auto requested = directory.file( "application.trace.log" );
	const auto secondary = directory.file( "application.trace.instance-2.log" );
	LogFileFamily owner;
	ASSERT_TRUE( owner.acquire( requested ) == requested );
	{
		LogFileFamily other;
		ASSERT_TRUE( other.acquire( requested ) == secondary );
		EXPECT_TRUE( FileSystem::fileExists( secondary + ".lock" ) );
	}
	EXPECT_TRUE( FileSystem::fileExists( secondary + ".lock" ) );
	LogFileFamily next;
	EXPECT_TRUE( next.acquire( requested ) == secondary );
	EXPECT_FALSE( FileSystem::fileExists( requested ) );
	EXPECT_FALSE( FileSystem::fileExists( secondary ) );
}
#endif

#if EE_PLATFORM == EE_PLATFORM_LINUX
UTEST( IOStreamFile, reportsShortWriteAndFlushFailure ) {
	class UnbufferedFile : public IOStreamFile {
	  public:
		UnbufferedFile() : IOStreamFile( "/dev/full", "wb" ) {
			if ( mFS )
				std::setvbuf( mFS, nullptr, _IONBF, 0 );
		}
	};
	UnbufferedFile shortWrite;
	if ( !shortWrite.isOpen() )
		UTEST_SKIP( "/dev/full is unavailable" );
	EXPECT_EQ( shortWrite.write( "abc", 3 ), 0 );
	IOStreamFile buffered( "/dev/full", "wb" );
	ASSERT_TRUE( buffered.isOpen() );
	EXPECT_EQ( buffered.write( "abc", 3 ), 3 );
	EXPECT_FALSE( buffered.flush() );
	IOStreamFile pending( "/dev/full", "wb" );
	ASSERT_TRUE( pending.isOpen() );
	EXPECT_EQ( pending.write( "abc", 3 ), 3 );
	EXPECT_FALSE( pending.close() );
	EXPECT_FALSE( pending.isOpen() );
}

UTEST( LogRotation, backsOffAfterFlushFailureWithoutReplayingTheRecord ) {
	LogTestDirectory directory;
	ASSERT_FALSE( directory.path.empty() );
	if ( !FileSystem::fileExists( "/dev/full" ) )
		UTEST_SKIP( "/dev/full is unavailable" );
	const auto path = directory.file();
	std::error_code error;
	std::filesystem::create_symlink( "/dev/full", directory.nativeFile( "log.txt" ), error );
	if ( error )
		UTEST_SKIP( "The filesystem does not support this symlink" );

	TestFileLog log( path );
	log.write( "failed" );
	ASSERT_TRUE( FileSystem::fileRemove( path ) );
	log.write( "during backoff" );
	EXPECT_FALSE( FileSystem::fileExists( path ) );
	log.setLiveWrite( false );
	log.setLiveWrite( true );
	log.write( "after backoff" );
	EXPECT_TRUE( readLog( path ) == "after backoff" );
}
#endif

UTEST( LogRotation, countsUtf8AndNewlinesAndKeepsOversizedRecordsWhole ) {
	LogTestDirectory directory;
	ASSERT_FALSE( directory.path.empty() );
	const auto path = directory.file( "log-\xC3\xA9.txt" );
	TestFileLog log( path );
	log.setRotationConfig( { 8, 3 } );

	log.write( "abc" );
	log.writel( "\xC3\xA9" );
	log.write( "de" );
	log.write( "" );
	EXPECT_TRUE( readLog( path ) == "abc\xC3\xA9\nde" );
	EXPECT_FALSE( FileSystem::fileExists( path + ".1" ) );

	log.writef( "%s", "oversized" );
	EXPECT_TRUE( readLog( path ) == "oversized\n" );
	EXPECT_TRUE( readLog( path + ".1" ) == "abc\xC3\xA9\nde" );
	log.write( "" );
	EXPECT_FALSE( FileSystem::fileExists( path + ".2" ) );
	log.writel( "" );
	EXPECT_TRUE( readLog( path ) == "\n" );
	EXPECT_TRUE( readLog( path + ".1" ) == "oversized\n" );
	EXPECT_TRUE( readLog( path + ".2" ) == "abc\xC3\xA9\nde" );
}

UTEST( LogRotation, appendsAcrossRestartsAndArchivesLegacyOversizedFile ) {
	LogTestDirectory directory;
	ASSERT_FALSE( directory.path.empty() );
	const auto path = directory.file();
	{
		TestFileLog log( path );
		EXPECT_EQ( log.getRotationConfig().maxFileSize, 0u );
		log.write( "6bytes" );
	}
	{
		TestFileLog log( path );
		log.setRotationConfig( { 8, 3 } );
		log.write( "78" );
		EXPECT_TRUE( readLog( path ) == "6bytes78" );
		EXPECT_FALSE( FileSystem::fileExists( path + ".1" ) );
		log.setRotationConfig( { 0, 3 } );
		log.write( "legacy" );
	}
	{
		TestFileLog log( path );
		log.setRotationConfig( { 8, 3 } );
		log.write( "new" );
		EXPECT_TRUE( readLog( path ) == "new" );
		EXPECT_TRUE( readLog( path + ".1" ) == "6bytes78legacy" );
		log.setRotationConfig( { 0, 0 } );
		log.write( "unlimited" );
		EXPECT_TRUE( readLog( path ) == "newunlimited" );
		EXPECT_TRUE( readLog( path + ".1" ) == "6bytes78legacy" );
	}
}

UTEST( LogRotation, prunesExactArchivesAndHandlesGapsAndZeroBackups ) {
	LogTestDirectory directory;
	ASSERT_FALSE( directory.path.empty() );
	const auto path = directory.file();
	TestFileLog log( path );
	log.setRotationConfig( { 3, 5 } );
	log.write( "one" );
	log.write( "two" );
	log.write( "tri" );
	ASSERT_TRUE( FileSystem::fileRemove( path + ".1" ) );
	ASSERT_TRUE( FileSystem::fileWrite( path + ".8", "stale" ) );
	ASSERT_TRUE( FileSystem::fileWrite( path + ".900000000000000000000", "stale" ) );
	ASSERT_TRUE( FileSystem::fileWrite( path + ".4.txt", "related name" ) );
	ASSERT_TRUE( FileSystem::makeDir( path + ".9" ) );
	std::error_code symlinkError;
	std::filesystem::create_symlink( directory.nativeFile( "log.txt" ),
									 directory.nativeFile( "log.txt.10" ), symlinkError );
	log.setRotationConfig( { 3, 4 } );
	log.write( "for" );
	EXPECT_TRUE( readLog( path + ".1" ) == "tri" );
	EXPECT_FALSE( FileSystem::fileExists( path + ".2" ) );
	EXPECT_TRUE( readLog( path + ".3" ) == "one" );
	EXPECT_FALSE( FileSystem::fileExists( path + ".8" ) );
	EXPECT_FALSE( FileSystem::fileExists( path + ".900000000000000000000" ) );
	EXPECT_TRUE( readLog( path + ".4.txt" ) == "related name" );
	EXPECT_TRUE( FileSystem::isDirectory( path + ".9" ) );
	if ( !symlinkError )
		EXPECT_TRUE( std::filesystem::is_symlink( directory.nativeFile( "log.txt.10" ) ) );

	log.setRotationConfig( { 3, 0 } );
	log.write( "fiv" );
	EXPECT_TRUE( readLog( path ) == "fiv" );
	EXPECT_FALSE( FileSystem::fileExists( path + ".1" ) );
	EXPECT_FALSE( FileSystem::fileExists( path + ".3" ) );

	log.setRotationConfig( { 3, 2 } );
	log.write( "six" );
	log.write( "sev" );
	log.write( "ate" );
	EXPECT_TRUE( readLog( path ) == "ate" );
	EXPECT_TRUE( readLog( path + ".1" ) == "sev" );
	EXPECT_TRUE( readLog( path + ".2" ) == "six" );
	EXPECT_FALSE( FileSystem::fileExists( path + ".3" ) );
}

UTEST( LogRotation, backsOffAfterBlockedArchiveAndContinuesAppending ) {
	LogTestDirectory directory;
	ASSERT_FALSE( directory.path.empty() );
	const auto path = directory.file();
	TestFileLog log( path );
	log.setRotationConfig( { 3, 2 } );
	log.write( "one" );
	log.write( "two" );
	ASSERT_TRUE( FileSystem::makeDir( path + ".2" ) );
	log.write( "tri" );
	EXPECT_TRUE( FileSystem::isDirectory( path + ".2" ) );
	EXPECT_TRUE( readLog( path + ".1" ) == "one" );
	EXPECT_TRUE( readLog( path ) == "twotri" );
	ASSERT_TRUE( FileSystem::dirRemoveAll( path + ".2" ) );
	log.write( "for" );
	EXPECT_TRUE( readLog( path ) == "twotrifor" );
	EXPECT_FALSE( FileSystem::fileExists( path + ".2" ) );

	log.setRotationConfig( { 4, 2 } );
	log.write( "next" );
	EXPECT_TRUE( readLog( path ) == "next" );
	EXPECT_TRUE( readLog( path + ".1" ) == "twotrifor" );
	EXPECT_TRUE( readLog( path + ".2" ) == "one" );
}

#if EE_PLATFORM == EE_PLATFORM_WIN
UTEST( LogRotation, appendsWhenWindowsReaderBlocksActiveRename ) {
	LogTestDirectory directory;
	ASSERT_FALSE( directory.path.empty() );
	const auto path = directory.file();
	TestFileLog log( path );
	log.setRotationConfig( { 3, 2 } );
	log.write( "one" );

	const HANDLE reader =
		CreateFileW( String::fromUtf8( path ).toWideString().c_str(), GENERIC_READ,
					 FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr );
	ASSERT_TRUE( reader != INVALID_HANDLE_VALUE );
	log.write( "two" );
	CloseHandle( reader );
	EXPECT_TRUE( readLog( path ) == "onetwo" );
	EXPECT_FALSE( FileSystem::fileExists( path + ".1" ) );

	log.setRotationConfig( { 4, 2 } );
	log.write( "new" );
	EXPECT_TRUE( readLog( path ) == "new" );
	EXPECT_TRUE( readLog( path + ".1" ) == "onetwo" );
}
#endif

UTEST( LogRotation, retriesOpenWithNewPathAndResetsSize ) {
	LogTestDirectory directory;
	ASSERT_FALSE( directory.path.empty() );
	const auto path = directory.file();
	const auto next = directory.file( "next.log" );
	ASSERT_TRUE( FileSystem::makeDir( path ) );
	TestFileLog log( path );
	log.setRotationConfig( { 4, 2 } );
	log.write( "lost" );
	ASSERT_TRUE( FileSystem::dirRemoveAll( path ) );
	log.write( "wait" );
	EXPECT_FALSE( FileSystem::fileExists( path ) );
	log.setFilePath( next );
	log.write( "data" );
	log.setFilePath( path );
	log.write( "ok" );
	EXPECT_TRUE( readLog( next ) == "data" );
	EXPECT_TRUE( readLog( path ) == "ok" );
	EXPECT_FALSE( FileSystem::fileExists( path + ".1" ) );
}

UTEST( LogRotation, preservesReaderCallsFilteringAndBufferedSave ) {
	LogTestDirectory directory;
	ASSERT_FALSE( directory.path.empty() );
	const auto path = directory.file();
	ASSERT_TRUE( FileSystem::fileWrite( path, "previous" ) );
	{
		TestFileLog log( path, false );
		log.setKeepLog( true );
		log.setRotationConfig( { 8, 1 } );
		log.save( path );
		ReentrantLogReader reader( log );
		log.addLogReader( &reader );
		log.writel( LogLevel::Info, "filtered" );
		log.writef( "%s", "outer" );
		EXPECT_TRUE( reader.mText == "outerinner\n" );
		EXPECT_TRUE( log.getBuffer() == "outer\ninner" );
		EXPECT_TRUE( readLog( path ) == "previous" );
		log.removeLogReader( &reader );
	}
	EXPECT_TRUE( readLog( path ) == "outer\ninner" );
	EXPECT_TRUE( readLog( path + ".1" ) == "previous" );
}

UTEST( LogRotation, serializesCompleteRecordsDuringPathAndPolicyChanges ) {
	LogTestDirectory directory;
	ASSERT_FALSE( directory.path.empty() );
	const auto path = directory.file();
	const auto next = directory.file( "next.log" );
	TestFileLog log( path );
	log.setRotationConfig( { 32, 100 } );
	constexpr int Records = 36;
	std::array<std::thread, 3> writers;
	for ( size_t index = 0; index < writers.size(); ++index ) {
		writers[index] = std::thread( [&log, index] {
			for ( int record = 0; record < Records; ++record ) {
				auto line = std::to_string( index ) + "/" + std::to_string( record );
				if ( index == 0 )
					log.write( line + "\n" );
				else if ( index == 1 )
					log.writel( line );
				else
					log.writef( "%s", line );
			}
		} );
	}
	for ( int iteration = 0; iteration < 12; ++iteration ) {
		log.setRotationConfig( { iteration % 2 == 0 ? 31u : 32u, 100 } );
		log.setFilePath( iteration % 2 == 0 ? next : path );
	}
	for ( auto& writer : writers )
		writer.join();
	log.setLiveWrite( false );

	UnorderedSet<std::string> lines;
	for ( const auto* family : { &path, &next } ) {
		for ( unsigned archive = 0; archive <= LogRotationConfig::MaxBackupFiles; ++archive ) {
			auto file = archive == 0 ? *family : *family + "." + std::to_string( archive );
			auto contents = readLog( file );
			if ( contents.empty() )
				continue;
			EXPECT_TRUE( contents.back() == '\n' );
			for ( auto& line : String::split( contents, '\n' ) )
				EXPECT_TRUE( lines.emplace( std::move( line ) ).second );
		}
	}
	EXPECT_EQ( lines.size(), writers.size() * Records );
	for ( size_t index = 0; index < writers.size(); ++index ) {
		for ( int record = 0; record < Records; ++record )
			EXPECT_EQ( lines.count( std::to_string( index ) + "/" + std::to_string( record ) ),
					   1u );
	}
}

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_MACOS
UTEST( LogRotation, sizesExistingSparseFileAboveFourGiB ) {
	LogTestDirectory directory;
	ASSERT_FALSE( directory.path.empty() );
	const auto path = directory.file();
	ASSERT_TRUE( FileSystem::fileWrite( path, "legacy" ) );
	constexpr Uint64 Size = ( 1ull << 32 ) + 1;
	std::error_code error;
	std::filesystem::resize_file( directory.nativeFile( "log.txt" ), Size, error );
	if ( error )
		UTEST_SKIP( "The test filesystem could not extend a sparse file" );
	TestFileLog log( path );
	log.setRotationConfig( { 16, 2 } );
	log.write( "first" );
	EXPECT_TRUE( readLog( path ) == "first" );
	EXPECT_EQ( std::filesystem::file_size( directory.nativeFile( "log.txt.1" ), error ), Size );
	EXPECT_FALSE( error );
}
#endif
