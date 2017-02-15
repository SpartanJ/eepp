#ifndef EE_AUDIOCLISTENER_H
#define EE_AUDIOCLISTENER_H

#include <eepp/audio/base.hpp>

namespace EE { namespace Audio {

/** @brief Listener is a global interface for defining the audio listener properties. */
class EE_API AudioListener {
	public:
		/** Change the global volume of all the sounds. ( default 100  )
		* @param Volume New global volume, in the range [0, 100]
		*/
		static void globalVolume( const float& Volume );

		/** Get the Global Volume */
		static float globalVolume();

		/** Change the position of the listener. \n The default position is (0, 0, 0) */
		static void position( const float& X, const float& Y, const float& Z );

		/** Change the position of the listener from a 3D vector. */
		static void position(const Vector3ff& position);

		/** Get the current position of the listener */
		static Vector3ff position();

		/** Change the orientation of the listener (the point he must look at). \n The default target is (0, 0, -1). */
		static void target( const float& X, const float& Y, const float& Z );

		/** Change the orientation of the listener from a 3D vector. */
		static void target(const Vector3ff& target);

		/** Get the current orientation of the listener (the point he's looking at) */
		static Vector3ff target();
};

}}

#endif
