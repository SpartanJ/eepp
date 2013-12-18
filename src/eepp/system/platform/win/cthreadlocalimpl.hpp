#ifndef EE_SYSTEMCTHREADLOCALIMPLWIN_HPP
#define EE_SYSTEMCTHREADLOCALIMPLWIN_HPP

#include <eepp/base.hpp>
#include <eepp/base/noncopyable.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <windows.h>

namespace EE { namespace System { namespace Private {

class cThreadLocalImpl : NonCopyable {
	public:
		cThreadLocalImpl();

		~cThreadLocalImpl();

		void Value(void* value);

		void* Value() const;
	private :
		DWORD mIndex;
};

}}}

#endif

#endif
