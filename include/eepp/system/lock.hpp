#ifndef EE_SYSTEM_CLOCK_HPP
#define EE_SYSTEM_CLOCK_HPP

#include <eepp/config.hpp>
#include <eepp/core/noncopyable.hpp>

namespace EE { namespace System {

class Mutex;

/** @brief Automatic wrapper for locking and unlocking mutexes */
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
};

}} // namespace EE::System

#endif
