#ifndef EE_SYSTEMCTIMEELAPSED_H
#define EE_SYSTEMCTIMEELAPSED_H

#include <eepp/system/base.hpp>
#include <eepp/system/ctimer.hpp>

namespace EE { namespace System {

/** @brief A very simple class to get the elapsed time between calls. */
class EE_API cTimeElapsed : private cTimer {
	public:
		cTimeElapsed();

		~cTimeElapsed();

		/** Time in milliseconds elapsed between this call and the last call to Elapsed() */
		eeDouble 	Elapsed();

		/** Time in milliseconds elapsed between the last Reset ( First Reset is when the class is instantiated ) */
		eeDouble 	ElapsedSinceStart();

		/** Restart the initial counter. ( set it as now ) */
		void 		Reset();
	protected:
		eeUint 		mFirstCheck;
		eeUint 		mLastCheck;
		eeDouble 	mElapsed;
};

}}

#endif
