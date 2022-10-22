#ifndef EE_SYSTEMCTHREADLOCAL_HPP
#define EE_SYSTEMCTHREADLOCAL_HPP

#include <eepp/core.hpp>
#include <eepp/core/noncopyable.hpp>

namespace EE { namespace System { namespace Private {
class ThreadLocalImpl;
}}} // namespace EE::System::Private

namespace EE { namespace System {

/** @brief Defines variables with thread-local storage */
class EE_API ThreadLocal : NonCopyable {
  public:
	/** @brief Default constructor
	**  @param value Optional value to initalize the variable */
	ThreadLocal( void* value = NULL );

	~ThreadLocal();

	/** @brief Set the thread-specific value of the variable
	**  @param value Value of the variable for the current thread */
	void setValue( void* value );

	/** @brief Retrieve the thread-specific value of the variable
	**  @return Value of the variable for the current thread */
	void* getValue() const;

  private:
	Private::ThreadLocalImpl* mImpl; ///< Pointer to the OS specific implementation
};

}} // namespace EE::System

#endif
