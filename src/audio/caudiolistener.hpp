#ifndef EE_AUDIOCLISTENER_H
#define EE_AUDIOCLISTENER_H

#include "base.hpp"

namespace EE { namespace Audio {

typedef Vector3<ALfloat> Vector3AL; //! Use this special vector because OpenAL doesn't support doubles.

/** @brief Listener is a global interface for defining the audio listener properties. */
class EE_API cAudioListener {
	public:
		/** Change the global volume of all the sounds. ( default 100  )
		* @param Volume New global volume, in the range [0, 100]
		*/
		static void GlobalVolume( const ALfloat& Volume );

		/** Get the Global Volume */
		static ALfloat GlobalVolume();

		/** Change the position of the listener. \n The default position is (0, 0, 0) */
		static void Position( const ALfloat& X, const ALfloat& Y, const ALfloat& Z );

		/** Change the position of the listener from a 3D vector. */
		static void Position(const Vector3AL& Position);

		/** Get the current position of the listener */
		static Vector3AL Position();

		/** Change the orientation of the listener (the point he must look at). \n The default target is (0, 0, -1). */
		static void Target( const ALfloat& X, const ALfloat& Y, const ALfloat& Z );

		/** Change the orientation of the listener from a 3D vector. */
		static void Target(const Vector3AL& Target);

		/** Get the current orientation of the listener (the point he's looking at) */
		static Vector3AL Target();
};

}}

#endif
