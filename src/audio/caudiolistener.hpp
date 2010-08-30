#ifndef EE_AUDIOCLISTENER_H
#define EE_AUDIOCLISTENER_H

#include "base.hpp"

namespace EE { namespace Audio {
typedef Vector3<ALfloat> Vector3AL; //! Use this special vector because OpenAL doesn't support doubles.

/** @brief Listener is a global interface for defining the audio listener properties. */
class EE_API cAudioListener : public tSingleton<cAudioListener> {
	public:
		/** Change the global volume of all the sounds. ( default 100  )
		* @param Volume New global volume, in the range [0, 100]
		*/
		void SetGlobalVolume( const ALfloat& Volume );

		/** Get the Global Volume */
		ALfloat GetGlobalVolume();

		/** Change the position of the listener. \n The default position is (0, 0, 0) */
		void SetPosition( const ALfloat& X, const ALfloat& Y, const ALfloat& Z );

		/** Change the position of the listener from a 3D vector. */
		void SetPosition(const Vector3AL& Position);

		/** Get the current position of the listener */
		Vector3AL GetPosition();

		/** Change the orientation of the listener (the point he must look at). \n The default target is (0, 0, -1). */
		void SetTarget( const ALfloat& X, const ALfloat& Y, const ALfloat& Z );

		/** Change the orientation of the listener from a 3D vector. */
		void SetTarget(const Vector3AL& Target);

		/** Get the current orientation of the listener (the point he's looking at) */
		Vector3AL GetTarget();
};

}}

#endif
