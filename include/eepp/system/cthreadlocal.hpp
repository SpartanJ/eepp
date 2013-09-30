#ifndef EE_SYSTEMCTHREADLOCAL_HPP
#define EE_SYSTEMCTHREADLOCAL_HPP

#include <eepp/base.hpp>
#include <eepp/base/noncopyable.hpp>

namespace EE { namespace System { namespace Private {
class cThreadLocalImpl;
}}}

namespace EE { namespace System {

/** @brief Defines variables with thread-local storage */
class EE_API cThreadLocal : NonCopyable {
	public:
		/** @brief Default constructor
		**  @param value Optional value to initalize the variable */
		cThreadLocal(void* value = NULL);

		~cThreadLocal();

		/** @brief Set the thread-specific value of the variable
		**  @param value Value of the variable for the current thread */
		void Value(void* value);

		/** @brief Retrieve the thread-specific value of the variable
		**  @return Value of the variable for the current thread */
		void* Value() const;
	private :
		Private::cThreadLocalImpl* mImpl; ///< Pointer to the OS specific implementation
};

}}

#endif
