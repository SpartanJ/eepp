#ifndef EE_SYSTEMCTHREADLOCALPTR_HPP
#define EE_SYSTEMCTHREADLOCALPTR_HPP

#include <eepp/system/cthreadlocal.hpp>

namespace EE { namespace System {

/** @brief Pointer to a thread-local variable */
template <typename T>
class tThreadLocalPtr : private cThreadLocal
{
	public :
		/** @brief Default constructor
		**  @param value Optional value to initalize the variable */
		tThreadLocalPtr(T* value = NULL);

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
		tThreadLocalPtr<T>& operator =(T* value);

		/** @brief Assignment operator for a tThreadLocalPtr parameter
		**  @param right tThreadLocalPtr to assign
		**  @return Reference to self */
		tThreadLocalPtr<T>& operator =(const tThreadLocalPtr<T>& right);
};

template <typename T>
tThreadLocalPtr<T>::tThreadLocalPtr(T* value) :
	cThreadLocal(value)
{
}

template <typename T>
T& tThreadLocalPtr<T>::operator *() const {
	return *static_cast<T*>(Value());
}

template <typename T>
T* tThreadLocalPtr<T>::operator ->() const {
	return static_cast<T*>(Value());
}

template <typename T>
tThreadLocalPtr<T>::operator T*() const {
	return static_cast<T*>(Value());
}

template <typename T>
tThreadLocalPtr<T>& tThreadLocalPtr<T>::operator =(T* value) {
	Value(value);
	return *this;
}

template <typename T>
tThreadLocalPtr<T>& tThreadLocalPtr<T>::operator =(const tThreadLocalPtr<T>& right) {
	Value(right.Value());
	return *this;
}

}}

#endif

/**
@class tThreadLocalPtr
@ingroup System

tThreadLocalPtr is a type-safe wrapper for storing
pointers to thread-local variables. A thread-local
variable holds a different value for each different
thread, unlike normal variable that are shared.

Its usage is completely transparent, so that it is similar
to manipulating the raw pointer directly (like any smart pointer).

Usage example:
@code
MyClass object1;
MyClass object2;
tThreadLocalPtr<MyClass> objectPtr;

void thread1()
{
	objectPtr = &object1; // doesn't impact thread2
	...
}

void thread2()
{
	objectPtr = &object2; // doesn't impact thread1
	...
}

int main()
{
	// Create and launch the two threads
	cThread t1(&thread1);
	cThread t2(&thread2);
	t1.launch();
	t2.launch();

	return 0;
}
@endcode
*/
