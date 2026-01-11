#include <eepp/audio/soundfilefactory.hpp>
#include <eepp/audio/soundfilereaderflac.hpp>
#include <eepp/audio/soundfilereadermp3.hpp>
#include <eepp/audio/soundfilereaderogg.hpp>
#include <eepp/audio/soundfilereaderwav.hpp>
#include <eepp/audio/soundfilewriterogg.hpp>
#include <eepp/audio/soundfilewriterwav.hpp>
#include <eepp/core/debug.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/mutex.hpp>
using namespace EE::System;

namespace {
// Register all the built-in readers and writers if not already done
void ensureDefaultReadersWritersRegistered() {
	static Mutex registerMutex;
	static bool registered = false;

	Lock lock( registerMutex );

	if ( !registered ) {
		EE::Audio::SoundFileFactory::registerReader<EE::Audio::Private::SoundFileReaderWav>();
		EE::Audio::SoundFileFactory::registerReader<EE::Audio::Private::SoundFileReaderOgg>();
		EE::Audio::SoundFileFactory::registerReader<EE::Audio::Private::SoundFileReaderFlac>();
		EE::Audio::SoundFileFactory::registerReader<EE::Audio::Private::SoundFileReaderMp3>();
		EE::Audio::SoundFileFactory::registerWriter<EE::Audio::Private::SoundFileWriterWav>();
		EE::Audio::SoundFileFactory::registerWriter<EE::Audio::Private::SoundFileWriterOgg>();
		registered = true;
	}
}
} // namespace

namespace EE { namespace Audio {

SoundFileFactory::ReaderFactoryArray SoundFileFactory::s_readers;
SoundFileFactory::WriterFactoryArray SoundFileFactory::s_writers;

bool SoundFileFactory::isKnownFileExtension( const std::string& path ) {
	ensureDefaultReadersWritersRegistered();

	auto ext = FileSystem::fileExtension( path, true );
	for ( const auto& reader : s_readers ) {
		if ( reader.usesFileExtension( ext ) )
			return true;
	}
	return false;

}

bool SoundFileFactory::isValidAudio( IOStream& stream ) {
	ensureDefaultReadersWritersRegistered();

	for ( const auto& reader : s_readers ) {
		stream.seek( 0 );
		if ( reader.check( stream ) )
			return true;
	}
	return false;
}

bool SoundFileFactory::isValidAudioFile( const std::string& path ) {
	IOStreamFile f( path );
	if ( !f.isOpen() )
		return false;
	return isValidAudio( f );
}

SoundFileReader* SoundFileFactory::createReaderFromFilename( const std::string& filename ) {
	// Register the built-in readers/writers on first call
	ensureDefaultReadersWritersRegistered();

	// Wrap the input file into a file stream
	IOStreamFile stream( filename );
	if ( !stream.isOpen() ) {
		Log::error( "Failed to open sound file \"%s\" (couldn't open stream)", filename.c_str() );
		return NULL;
	}

	// Test the filename in all the registered factories
	for ( ReaderFactoryArray::const_iterator it = s_readers.begin(); it != s_readers.end(); ++it ) {
		stream.seek( 0 );
		if ( it->check( stream ) )
			return it->create();
	}

	// No suitable reader found
	Log::error( "Failed to open sound file \"%s\" (format not supported)", filename.c_str() );

	return NULL;
}

SoundFileReader* SoundFileFactory::createReaderFromMemory( const void* data,
														   std::size_t sizeInBytes ) {
	// Register the built-in readers/writers on first call
	ensureDefaultReadersWritersRegistered();

	// Wrap the memory file into a file stream
	IOStreamMemory stream( (char*)data, sizeInBytes );

	// Test the stream for all the registered factories
	for ( ReaderFactoryArray::const_iterator it = s_readers.begin(); it != s_readers.end(); ++it ) {
		stream.seek( 0 );
		if ( it->check( stream ) )
			return it->create();
	}

	// No suitable reader found
	Log::error( "Failed to open sound file from memory (format not supported)" );
	return NULL;
}

SoundFileReader* SoundFileFactory::createReaderFromStream( IOStream& stream ) {
	// Register the built-in readers/writers on first call
	ensureDefaultReadersWritersRegistered();

	// Test the stream for all the registered factories
	for ( ReaderFactoryArray::const_iterator it = s_readers.begin(); it != s_readers.end(); ++it ) {
		stream.seek( 0 );
		if ( it->check( stream ) )
			return it->create();
	}

	// No suitable reader found
	Log::error( "Failed to open sound file from stream (format not supported)" );
	return NULL;
}

SoundFileWriter* SoundFileFactory::createWriterFromFilename( const std::string& filename ) {
	// Register the built-in readers/writers on first call
	ensureDefaultReadersWritersRegistered();

	// Test the filename in all the registered factories
	for ( WriterFactoryArray::const_iterator it = s_writers.begin(); it != s_writers.end(); ++it ) {
		if ( it->check( filename ) )
			return it->create();
	}

	// No suitable writer found
	Log::error( "Failed to open sound file \"%s\" (format not supported)", filename.c_str() );
	return NULL;
}

}} // namespace EE::Audio
