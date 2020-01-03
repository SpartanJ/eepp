#ifndef EE_SYSTEMCTHREADLOCALPTR_HPP
#define EE_SYSTEMCTHREADLOCALPTR_HPP

#include <eepp/system/threadlocal.hpp>

namespace EE { namespace System {

/** @brief Pointer to a thread-local variable */
template <typename T>
class ThreadLocalPtr : private ThreadLocal {
	public :
		/** @brief Default constructor
		**  @param value Optional value to initalize the variable */
		ThreadLocalPtr(T* value = NULL);

		/** @brief Overload of unary operator *
		**  Like raw pointers, applying the * operator returns a
		**  reference to the pointed object.
		**  @return Reference to the pointed object */
		T& operator *() const;

		/** @brief Overload of operator ->
		**  Like raw pointers, applying the -> operator returns the
		**  pointed object.
		**  @return Pointed object */
		T* operator ->() const;

		/** @brief Cast operator to implicitely convert the pointer to its raw pointer type (T*)
		**  @return Pointer to the actual object */
		operator T*() const;

		/** @brief Assignment operator for a raw pointer parameter
		**  @param value Pointer to assign
		**  @return Reference to self */
		ThreadLocalPtr<T>& operator =(T* value);

		/** @brief Assignment operator for a ThreadLocalPtr parameter
		**  @param right ThreadLocalPtr to assign
		**  @return Reference to self */
		ThreadLocalPtr<T>& operator =(const ThreadLocalPtr<T>& right);
};

template <typename T>
ThreadLocalPtr<T>::ThreadLocalPtr(T* value) :
	ThreadLocal(value)
{
}

template <typename T>
T& ThreadLocalPtr<T>::operator *() const {
	return *static_cast<T*>(getValue());
}

template <typename T>
T* ThreadLocalPtr<T>::operator ->() const {
	return static_cast<T*>(getValue());
}

template <typename T>
ThreadLocalPtr<T>::operator T*() const {
	return static_cast<T*>(getValue());
}

template <typename T>
ThreadLocalPtr<T>& ThreadLocalPtr<T>::operator =(T* val) {
	setValue(val);
	return *this;
}

template <typename T>
ThreadLocalPtr<T>& ThreadLocalPtr<T>::operator =(const ThreadLocalPtr<T>& right) {
	setValue(right.getValue());
	return *this;
}

}}

#endif

/**
@class EE::System::ThreadLocalPtr

ThreadLocalPtr is a type-safe wrapper for storing
pointers to thread-local variables. A thread-local
variable holds a different value for each different
thread, unlike normal variable that are shared.

Its usage is completely transparent, so that it is similar
to manipulating the raw pointer directly (like any smart pointer).

Usage example:
@code
MyClass object1;
MyClass object2;
ThreadLocalPtr<MyClass> objectPtr;

void thread1() {
	objectPtr = &object1; // doesn't impact thread2
	...
}

void thread2() {
	objectPtr = &object2; // doesn't impact thread1
	...
}

int main() {
	// Create and launch the two threads
	Thread t1(&thread1);
	Thread t2(&thread2);
	t1.launch();
	t2.launch();

	return 0;
}
@endcode
*/
