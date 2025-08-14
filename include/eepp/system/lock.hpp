#ifndef EE_SYSTEM_CLOCK_HPP
#define EE_SYSTEM_CLOCK_HPP

#include <eepp/config.hpp>
#include <eepp/core/noncopyable.hpp>

#ifdef EE_DEBUG
// #define EE_REGISTER_SLOW_LOCKS
#include <eepp/system/clock.hpp>
#endif

namespace EE { namespace System {

class Mutex;

/** @brief Automatic wrapper for locking and unlocking recursive mutexes */
class EE_API Lock : NonCopyable {
  public:
	/** @brief Construct the lock with a target mutex
	 *	The mutex passed to Lock is automatically locked.
	 *	@param mutex Mutex to lock */
	explicit Lock( Mutex& mutex );

	/**	@brief Destructor
	 *	The destructor of Lock automatically unlocks its mutex. */
	~Lock();

  private:
	Mutex& mMutex; ///< Mutex to lock / unlock

#ifdef EE_REGISTER_SLOW_LOCKS
	Clock mClock;
#endif
};

class EE_API ConditionalLock : NonCopyable {
  public:
	/** @brief Construct the lock with a target mutex
	 *	The mutex passed to Lock is automatically locked.
	 *  @param condition Only if condition is true the mutex will be locked/unlocked.
	 *	@param mutex Mutex to lock */
	explicit ConditionalLock( bool condition, Mutex* mutex );

	/**	@brief Destructor
	 *	The destructor of Lock automatically unlocks its mutex. */
	~ConditionalLock();

  private:
	Mutex* mMutex{ nullptr }; ///< Mutex to lock / unlock
	bool mCondition{ false };
};

}} // namespace EE::System

#endif
