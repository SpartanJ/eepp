#ifndef EE_AUDIODEVICE_HPP
#define EE_AUDIODEVICE_HPP

#include <eepp/math/vector3.hpp>
#include <set>
#include <string>
using namespace EE::Math;

namespace EE { namespace Audio { namespace Private {

////////////////////////////////////////////////////////////
/// \brief High-level wrapper around the audio API, it manages
///		the creation and destruction of the audio device and
///		context and stores the device capabilities
///
////////////////////////////////////////////////////////////
class AudioDevice {
	public:
		static AudioDevice * New();

		AudioDevice();

		~AudioDevice();

		////////////////////////////////////////////////////////////
		/// \brief Check if an OpenAL extension is supported
		///
		/// This functions automatically finds whether it
		/// is an AL or ALC extension, and calls the corresponding
		/// function.
		///
		/// \param extension Name of the extension to test
		///
		/// \return True if the extension is supported, false if not
		///
		////////////////////////////////////////////////////////////
		static bool isExtensionSupported(const std::string& extension);

		////////////////////////////////////////////////////////////
		/// \brief Get the OpenAL format that matches the given number of channels
		///
		/// \param channelCount Number of channels
		///
		/// \return Corresponding format
		///
		////////////////////////////////////////////////////////////
		static int getFormatFromChannelCount(unsigned int channelCount);

		////////////////////////////////////////////////////////////
		/// \brief Change the global volume of all the sounds and musics
		///
		/// The volume is a number between 0 and 100; it is combined with
		/// the individual volume of each sound / music.
		/// The default value for the volume is 100 (maximum).
		///
		/// \param volume New global volume, in the range [0, 100]
		///
		/// \see getGlobalVolume
		///
		////////////////////////////////////////////////////////////
		static void setGlobalVolume(float volume);

		////////////////////////////////////////////////////////////
		/// \brief Get the current value of the global volume
		///
		/// \return Current global volume, in the range [0, 100]
		///
		/// \see setGlobalVolume
		///
		////////////////////////////////////////////////////////////
		static float getGlobalVolume();

		////////////////////////////////////////////////////////////
		/// \brief Set the position of the listener in the scene
		///
		/// The default listener's position is (0, 0, 0).
		///
		/// \param position New listener's position
		///
		/// \see getPosition, setDirection
		///
		////////////////////////////////////////////////////////////
		static void setPosition(const Vector3f& position);

		////////////////////////////////////////////////////////////
		/// \brief Get the current position of the listener in the scene
		///
		/// \return Listener's position
		///
		/// \see setPosition
		///
		////////////////////////////////////////////////////////////
		static Vector3f getPosition();

		////////////////////////////////////////////////////////////
		/// \brief Set the forward vector of the listener in the scene
		///
		/// The direction (also called "at vector") is the vector
		/// pointing forward from the listener's perspective. Together
		/// with the up vector, it defines the 3D orientation of the
		/// listener in the scene. The direction vector doesn't
		/// have to be normalized.
		/// The default listener's direction is (0, 0, -1).
		///
		/// \param direction New listener's direction
		///
		/// \see getDirection, setUpVector, setPosition
		///
		////////////////////////////////////////////////////////////
		static void setDirection(const Vector3f& direction);

		////////////////////////////////////////////////////////////
		/// \brief Get the current forward vector of the listener in the scene
		///
		/// \return Listener's forward vector (not normalized)
		///
		/// \see setDirection
		///
		////////////////////////////////////////////////////////////
		static Vector3f getDirection();

		////////////////////////////////////////////////////////////
		/// \brief Set the upward vector of the listener in the scene
		///
		/// The up vector is the vector that points upward from the
		/// listener's perspective. Together with the direction, it
		/// defines the 3D orientation of the listener in the scene.
		/// The up vector doesn't have to be normalized.
		/// The default listener's up vector is (0, 1, 0). It is usually
		/// not necessary to change it, especially in 2D scenarios.
		///
		/// \param upVector New listener's up vector
		///
		/// \see getUpVector, setDirection, setPosition
		///
		////////////////////////////////////////////////////////////
		static void setUpVector(const Vector3f& upVector);

		////////////////////////////////////////////////////////////
		/// \brief Get the current upward vector of the listener in the scene
		///
		/// \return Listener's upward vector (not normalized)
		///
		/// \see setUpVector
		///
		////////////////////////////////////////////////////////////
		static Vector3f getUpVector();
};

}}}

#endif
