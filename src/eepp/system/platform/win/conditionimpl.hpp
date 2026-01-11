#ifndef EE_SYSTEMCCONDITIONIMPLWIN_HPP
#define EE_SYSTEMCCONDITIONIMPLWIN_HPP

#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <eepp/system/mutex.hpp>

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
	int mIsValid;
	int mConditionedVar;

	HANDLE mCond;
	Mutex mMutex;
};

}}} // namespace EE::System::Platform

#endif

#endif
