#ifndef EE_AUDIO_MUSIC_HPP
#define EE_AUDIO_MUSIC_HPP

#include <eepp/config.hpp>
#include <eepp/audio/soundstream.hpp>
#include <eepp/audio/inputsoundfile.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/time.hpp>
#include <eepp/system/safedatapointer.hpp>
#include <string>
#include <vector>

namespace EE { namespace System {
class IOStream;
class Pack;
}}

using namespace EE::System;

namespace EE { namespace Audio {

/// \brief Streamed music played from an audio file
class EE_API Music : public SoundStream
{
	public:
		/// \brief Structure defining a time range using the template type
		template <typename T>
		struct Span
		{
			Span()
			{

			}

			/// \brief Initialization constructor
			/// \param off Initial Offset
			/// \param len Initial Length
			Span(T off, T len):
				offset(off),
				length(len)
			{

			}

			T offset; ///< The beginning offset of the time range
			T length; ///< The length of the time range
		};

		// Define the relevant Span types
		typedef Span<Time> TimeSpan;

		Music();

		~Music();

		////////////////////////////////////////////////////////////
		/// \brief Open a music from an audio file
		///
		/// This function doesn't start playing the music (call play()
		/// to do so).
		/// See the documentation of InputSoundFile for the list
		/// of supported formats.
		///
		/// \warning Since the music is not loaded at once but rather
		/// streamed continuously, the file must remain accessible until
		/// the Music object loads a new music or is destroyed.
		///
		/// \param filename Path of the music file to open
		///
		/// \return True if loading succeeded, false if it failed
		///
		/// \see openFromMemory, openFromStream
		///
		////////////////////////////////////////////////////////////
		bool openFromFile(const std::string& filename);

		////////////////////////////////////////////////////////////
		/// \brief Open a music from an audio file in memory
		///
		/// This function doesn't start playing the music (call play()
		/// to do so).
		/// See the documentation of InputSoundFile for the list
		/// of supported formats.
		///
		/// \warning Since the music is not loaded at once but rather streamed
		/// continuously, the \a data buffer must remain accessible until
		/// the Music object loads a new music or is destroyed. That is,
		/// you can't deallocate the buffer right after calling this function.
		///
		/// \param data		Pointer to the file data in memory
		/// \param sizeInBytes Size of the data to load, in bytes
		///
		/// \return True if loading succeeded, false if it failed
		///
		/// \see openFromFile, openFromStream
		///
		////////////////////////////////////////////////////////////
		bool openFromMemory(const void* data, std::size_t sizeInBytes);

		////////////////////////////////////////////////////////////
		/// \brief Open a music from an audio file in a custom stream
		///
		/// This function doesn't start playing the music (call play()
		/// to do so).
		/// See the documentation of InputSoundFile for the list
		/// of supported formats.
		///
		/// \warning Since the music is not loaded at once but rather
		/// streamed continuously, the \a stream must remain accessible
		/// until the Music object loads a new music or is destroyed.
		///
		/// \param stream Source stream to read from
		///
		/// \return True if loading succeeded, false if it failed
		///
		/// \see openFromFile, openFromMemory
		///
		////////////////////////////////////////////////////////////
		bool openFromStream(IOStream& stream);

		bool openFromPack( Pack * pack, const std::string& filePackPath );

		////////////////////////////////////////////////////////////
		/// \brief Get the total duration of the music
		///
		/// \return Music duration
		///
		////////////////////////////////////////////////////////////
		Time getDuration() const;

		////////////////////////////////////////////////////////////
		/// \brief Get the positions of the of the sound's looping sequence
		///
		/// \return Loop Time position class.
		///
		/// \warning Since setLoopPoints() performs some adjustments on the
		/// provided values and rounds them to internal samples, a call to
		/// getLoopPoints() is not guaranteed to return the same times passed
		/// into a previous call to setLoopPoints(). However, it is guaranteed
		/// to return times that will map to the valid internal samples of
		/// this Music if they are later passed to setLoopPoints().
		///
		/// \see setLoopPoints
		///
		////////////////////////////////////////////////////////////
		TimeSpan getLoopPoints() const;

		////////////////////////////////////////////////////////////
		/// \brief Sets the beginning and end of the sound's looping sequence using Time
		///
		/// Loop points allow one to specify a pair of positions such that, when the music
		/// is enabled for looping, it will seamlessly seek to the beginning whenever it
		/// encounters the end. Valid ranges for timePoints.offset and timePoints.length are
		/// [0, Dur) and (0, Dur-offset] respectively, where Dur is the value returned by getDuration().
		/// Note that the EOF "loop point" from the end to the beginning of the stream is still honored,
		/// in case the caller seeks to a point after the end of the loop range. This function can be
		/// safely called at any point after a stream is opened, and will be applied to a playing sound
		/// without affecting the current playing offset.
		///
		/// \warning Setting the loop points while the stream's status is Paused
		/// will set its status to Stopped. The playing offset will be unaffected.
		///
		/// \param timePoints The definition of the loop. Can be any time points within the sound's length
		///
		/// \see getLoopPoints
		///
		////////////////////////////////////////////////////////////
		void setLoopPoints(TimeSpan timePoints);

	protected:

		////////////////////////////////////////////////////////////
		/// \brief Request a new chunk of audio samples from the stream source
		///
		/// This function fills the chunk from the next samples
		/// to read from the audio file.
		///
		/// \param data Chunk of data to fill
		///
		/// \return True to continue playback, false to stop
		///
		////////////////////////////////////////////////////////////
		virtual bool onGetData(Chunk& data);

		////////////////////////////////////////////////////////////
		/// \brief Change the current playing position in the stream source
		///
		/// \param timeOffset New playing position, from the beginning of the music
		///
		////////////////////////////////////////////////////////////
		virtual void onSeek(Time timeOffset);

		////////////////////////////////////////////////////////////
		/// \brief Change the current playing position in the stream source to the loop offset
		///
		/// This is called by the underlying SoundStream whenever it needs us to reset
		/// the seek position for a loop. We then determine whether we are looping on a
		/// loop point or the end-of-file, perform the seek, and return the new position.
		///
		/// \return The seek position after looping (or -1 if there's no loop)
		///
		////////////////////////////////////////////////////////////
		virtual Int64 onLoop();

	private:

		////////////////////////////////////////////////////////////
		/// \brief Initialize the internal state after loading a new music
		///
		////////////////////////////////////////////////////////////
		void initialize();

		////////////////////////////////////////////////////////////
		/// \brief Helper to convert an Time to a sample position
		///
		/// \param position Time to convert to samples
		///
		/// \return The number of samples elapsed at the given time
		///
		////////////////////////////////////////////////////////////
		Uint64 timeToSamples(Time position) const;

		////////////////////////////////////////////////////////////
		/// \brief Helper to convert a sample position to an Time
		///
		/// \param samples Sample count to convert to Time
		///
		/// \return The Time position of the given sample
		///
		////////////////////////////////////////////////////////////
		Time samplesToTime(Uint64 samples) const;

		////////////////////////////////////////////////////////////
		// Member data
		////////////////////////////////////////////////////////////
		InputSoundFile  mFile;	 ///< The streamed music file
		std::vector<Int16> mSamples;  ///< Temporary buffer of samples
		Mutex mMutex;	///< Mutex protecting the data
		Span<Uint64> mLoopSpan; ///< Loop Range Specifier
		SafeDataPointer	mData;
};

}}

#endif

////////////////////////////////////////////////////////////
/// \class Music
/// \ingroup audio
///
/// Musics are sounds that are streamed rather than completely
/// loaded in memory. This is especially useful for compressed
/// musics that usually take hundreds of MB when they are
/// uncompressed: by streaming it instead of loading it entirely,
/// you avoid saturating the memory and have almost no loading delay.
/// This implies that the underlying resource (file, stream or
/// memory buffer) must remain valid for the lifetime of the
/// Music object.
///
/// Apart from that, a Music has almost the same features as
/// the SoundBuffer / Sound pair: you can play/pause/stop
/// it, request its parameters (channels, sample rate), change
/// the way it is played (pitch, volume, 3D position, ...), etc.
///
/// As a sound stream, a music is played in its own thread in order
/// not to block the rest of the program. This means that you can
/// leave the music alone after calling play(), it will manage itself
/// very well.
///
/// Usage example:
/// \code
/// // Declare a new music
/// Music music;
///
/// // Open it from an audio file
/// if (!music.openFromFile("music.ogg"))
/// {
///	 // error...
/// }
///
/// // Change some parameters
/// music.setPosition(0, 1, 10); // change its 3D position
/// music.setPitch(2);		   // increase the pitch
/// music.setVolume(50);		 // reduce the volume
/// music.setLoop(true);		 // make it loop
///
/// // Play it
/// music.play();
/// \endcode
///
/// \see Sound, SoundStream
///
////////////////////////////////////////////////////////////
