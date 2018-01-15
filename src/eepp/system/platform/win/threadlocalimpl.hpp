#ifndef EE_SYSTEMCTHREADLOCALIMPLWIN_HPP
#define EE_SYSTEMCTHREADLOCALIMPLWIN_HPP

#include <eepp/core.hpp>
#include <eepp/core/noncopyable.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <windows.h>

namespace EE { namespace System { namespace Private {

class ThreadLocalImpl : NonCopyable {
	public:
		ThreadLocalImpl();

		~ThreadLocalImpl();

		void setValue(void* val);

		void* getValue() const;
	private :
		DWORD mIndex;
};

}}}

#endif

#endif
