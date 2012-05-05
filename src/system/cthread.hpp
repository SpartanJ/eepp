#ifndef EE_SYSTEMCTHREAD_H
#define EE_SYSTEMCTHREAD_H

#include "base.hpp"

namespace EE { namespace System {

namespace Platform { class cThreadImpl; }
namespace Private { struct ThreadFunc; }

/** @brief Thread manager class */
class EE_API cThread {
	public:
		typedef void (*FuncType)(void*);

		template <typename F>
		cThread( F function );

		template <typename F, typename A>
		cThread( F function, A argument );

		template <typename C>
		cThread( void(C::*function)(), C* object );

		virtual ~cThread();

		/** Launch the thread */
		virtual void	Launch();

		/** Wait the thread until end */
		void			Wait();

		/** Terminate the thread */
		void			Terminate();
	protected:
		cThread();
	private:
		friend class Platform::cThreadImpl;

		/** The virtual function to run in the thread */
		virtual void	Run();

		Platform::cThreadImpl *		mThreadImpl;       ///< OS-specific implementation of the thread
		Private::ThreadFunc *		mEntryPoint; ///< Abstraction of the function to run
};

//! Taken from SFML threads
namespace Private {

// Base class for abstract thread functions
struct ThreadFunc
{
	virtual ~ThreadFunc() {}
	virtual void Run() = 0;
};

// Specialization using a functor (including free functions) with no argument
template <typename T>
struct ThreadFunctor : ThreadFunc
{
	ThreadFunctor(T functor) : m_functor(functor) {}
	virtual void Run() {m_functor();}
	T m_functor;
};

// Specialization using a functor (including free functions) with one argument
template <typename F, typename A>
struct ThreadFunctorWithArg : ThreadFunc
{
	ThreadFunctorWithArg(F function, A arg) : m_function(function), m_arg(arg) {}
	virtual void Run() {m_function(m_arg);}
	F m_function;
	A m_arg;
};

// Specialization using a member function
template <typename C>
struct ThreadMemberFunc : ThreadFunc
{
	ThreadMemberFunc(void(C::*function)(), C* object) : m_function(function), m_object(object) {}
	virtual void Run() {(m_object->*m_function)();}
	void(C::*m_function)();
	C* m_object;
};

}

template <typename F>
cThread::cThread(F functor) :
	mThreadImpl      (NULL),
	mEntryPoint( eeNew( Private::ThreadFunctor<F>, (functor) ) )
{
}

template <typename F, typename A>
cThread::cThread(F function, A argument) :
	mThreadImpl(NULL),
	mEntryPoint( eeNew( Private::ThreadFunctorWithArg<F eeCOMMA A>, (function, argument) ) )
{
}

template <typename C>
cThread::cThread(void(C::*function)(), C* object) :
	mThreadImpl(NULL),
	mEntryPoint( eeNew( Private::ThreadMemberFunc<C>, (function, object) ) )
{
}

}}

#endif
