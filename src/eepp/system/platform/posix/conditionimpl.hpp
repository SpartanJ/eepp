#ifndef EE_SYSTEMCCONDITIONIMPLPOSIX_HPP
#define EE_SYSTEMCCONDITIONIMPLPOSIX_HPP

#include <eepp/config.hpp>

#if defined( EE_PLATFORM_POSIX )

#include <pthread.h>

namespace EE { namespace System { namespace Platform {

class ConditionImpl {
  public:
	ConditionImpl( int var );

	~ConditionImpl();

	void lock();

	void unlock();

	bool waitAndRetain( int value );

	void release( int value );

	void setValue( int value );

	int value() const;

	void signal();

	void invalidate();

	void restore();

  private:
	int mIsInvalid;
	int mConditionnedVar;

	pthread_cond_t mCond;
	pthread_mutex_t mMutex;
};

}}} // namespace EE::System::Platform

#endif

#endif
