#ifndef EE_AUDIO_SOUNDFILEFACTORY_HPP
#define EE_AUDIO_SOUNDFILEFACTORY_HPP

#include <eepp/config.hpp>
#include <string>
#include <vector>

namespace EE { namespace System {
class IOStream;
}}

using namespace EE::System;

namespace EE { namespace Audio {

class SoundFileReader;
class SoundFileWriter;

////////////////////////////////////////////////////////////
/// \brief Manages and instantiates sound file readers and writers
///
////////////////////////////////////////////////////////////
class EE_API SoundFileFactory {
	public:

		/// \brief Register a new reader
		/// \see unregisterReader
		template <typename T>
		static void registerReader();

		/// \brief Unregister a reader
		/// \see registerReader
		template <typename T>
		static void unregisterReader();

		/// \brief Register a new writer
		/// \see unregisterWriter
		template <typename T>
		static void registerWriter();

		/// \brief Unregister a writer
		/// \see registerWriter
		template <typename T>
		static void unregisterWriter();

		////////////////////////////////////////////////////////////
		/// \brief Instantiate the right reader for the given file on disk
		///
		/// It's up to the caller to release the returned reader
		///
		/// \param filename Path of the sound file
		///
		/// \return A new sound file reader that can read the given file, or null if no reader can handle it
		///
		/// \see createReaderFromMemory, createReaderFromStream
		///
		////////////////////////////////////////////////////////////
		static SoundFileReader* createReaderFromFilename(const std::string& filename);

		////////////////////////////////////////////////////////////
		/// \brief Instantiate the right codec for the given file in memory
		///
		/// It's up to the caller to release the returned reader
		///
		/// \param data		Pointer to the file data in memory
		/// \param sizeInBytes Total size of the file data, in bytes
		///
		/// \return A new sound file codec that can read the given file, or null if no codec can handle it
		///
		/// \see createReaderFromFilename, createReaderFromStream
		///
		////////////////////////////////////////////////////////////
		static SoundFileReader* createReaderFromMemory(const void* data, std::size_t sizeInBytes);

		////////////////////////////////////////////////////////////
		/// \brief Instantiate the right codec for the given file in stream
		///
		/// It's up to the caller to release the returned reader
		///
		/// \param stream Source stream to read from
		///
		/// \return A new sound file codec that can read the given file, or null if no codec can handle it
		///
		/// \see createReaderFromFilename, createReaderFromMemory
		///
		////////////////////////////////////////////////////////////
		static SoundFileReader* createReaderFromStream(IOStream& stream);

		////////////////////////////////////////////////////////////
		/// \brief Instantiate the right writer for the given file on disk
		///
		/// It's up to the caller to release the returned writer
		///
		/// \param filename Path of the sound file
		///
		/// \return A new sound file writer that can write given file, or null if no writer can handle it
		///
		////////////////////////////////////////////////////////////
		static SoundFileWriter* createWriterFromFilename(const std::string& filename);

	private:
		struct ReaderFactory
		{
			bool (*check)(IOStream&);
			SoundFileReader* (*create)();
		};
		typedef std::vector<ReaderFactory> ReaderFactoryArray;

		struct WriterFactory
		{
			bool (*check)(const std::string&);
			SoundFileWriter* (*create)();
		};
		typedef std::vector<WriterFactory> WriterFactoryArray;

		static ReaderFactoryArray s_readers; ///< List of all registered readers
		static WriterFactoryArray s_writers; ///< List of all registered writers
	};

}}

#include <eepp/audio/soundfilefactory.inl>

#endif


////////////////////////////////////////////////////////////
/// @class EE::Audio::SoundFileFactory
///
/// This class is where all the sound file readers and writers are
/// registered. You should normally only need to use its registration
/// and unregistration functions; readers/writers creation and manipulation
/// are wrapped into the higher-level classes InputSoundFile and
/// OutputSoundFile.
///
/// To register a new reader (writer) use the SoundFileFactory::registerReader
/// (registerWriter) static function. You don't have to call the unregisterReader
/// (unregisterWriter) function, unless you want to unregister a format before your
/// application ends (typically, when a plugin is unloaded).
///
/// Usage example:
/// \code
/// SoundFileFactory::registerReader<MySoundFileReader>();
/// SoundFileFactory::registerWriter<MySoundFileWriter>();
/// \endcode
///
/// \see InputSoundFile, OutputSoundFile, SoundFileReader, SoundFileWriter
///
////////////////////////////////////////////////////////////
