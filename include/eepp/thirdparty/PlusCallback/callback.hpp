/*
 * PlusCallback 1.7
 * Copyright (c) 2009-2010 Lewis Van Winkle
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 */

#ifndef __CALLBACK_HPP__
#define __CALLBACK_HPP__

#include <string.h>
#include <string>

#if !defined( __ANDROID__ ) && !defined( ANDROID )
#define EXCEPTIONS_SUPPORTED
#endif

#ifdef EXCEPTIONS_SUPPORTED
#include <stdexcept>

#define THROW_RUNTIME_ERROR( R ) throw std::runtime_error( unset_call_error );
#else
#include <cstdio>

#define THROW_RUNTIME_ERROR( R )                            \
	printf( "throw std::runtime_error(unset_call_error)" ); \
	return (R)NULL;
#endif

// PlusCallback 1.7
// This library was built on 12.10.2010 to support
// functions with a maximum of 9 parameters.
#define CALLBACK_VERSION 1.7

namespace cb {

static const std::string unset_call_error( "Attempting to invoke null callback." );

/// Stores a callback for a function taking 0 parameters.
///\tparam R Callback function return type.
template <typename R> class Callback0 {
  public:
	/// Constructs the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C>
	Callback0( C* object, R ( C::*function )() ) :
		mCallback( new ( &mMem ) ChildMethod<C>( object, function ) ) {}

	/// Constructs the callback to a free function or static member function.
	///\param function Free function address to call.
	Callback0( R ( *function )() ) : mCallback( new ( &mMem ) ChildFree( function ) ) {}

	/// Constructs a callback that can later be set.
	Callback0() : mCallback( 0 ) {}

	Callback0( const Callback0& c ) : mCallback( c.mCallback ) {
		if ( mCallback ) {
			memcpy( mMem, c.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}
	}

	Callback0& operator=( const Callback0& rhs ) {
		mCallback = rhs.mCallback;
		if ( mCallback ) {
			memcpy( mMem, rhs.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}

		return *this;
	}

	~Callback0() {}

	/// Sets the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C> void Reset( C* object, R ( C::*function )() ) {
		mCallback = new ( &mMem ) ChildMethod<C>( object, function );
	}

	/// Sets the callback to a free function or static member function.
	///\param function Free function address to call.
	void Reset( R ( *function )() ) { mCallback = new ( &mMem ) ChildFree( function ); }

	/// Resests to callback to nothing.
	void Reset() { mCallback = 0; }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator==( const Callback0& rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) == ( *( rhs.mCallback ) );
		else
			return mCallback == rhs.mCallback;
	}

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator!=( const Callback0& rhs ) const { return !( *this == rhs ); }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator<( const Callback0 rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) < ( *( rhs.mCallback ) );
		else
			return mCallback < rhs.mCallback;
	}

	/// Returns true if the callback has been set, or false if the callback is not set and is
	/// invalid.
	bool IsSet() const { return 0 != mCallback; }

	/// Invokes the callback.
	R operator()() const {
		if ( mCallback )
			return ( *mCallback )();
		else
			THROW_RUNTIME_ERROR( R )
	}

	/// Invokes the callback. This function can sometimes be more convenient than the operator(),
	/// which does the same thing.
	R Call() const {
		if ( mCallback )
			return ( *mCallback )();
		else
			THROW_RUNTIME_ERROR( R )
	}

  private:
	class Base {
	  public:
		Base() {}
		virtual R operator()() = 0;
		virtual bool operator==( const Base& rhs ) const = 0;
		virtual bool operator<( const Base& rhs ) const = 0;
		virtual void* Comp() const = 0; // Returns a pointer used in comparisons.
	};

	class ChildFree : public Base {
	  public:
		ChildFree( R ( *function )() ) : mFunc( function ) {}

		virtual R operator()() { return mFunc(); }

		virtual bool operator==( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return mFunc < r->mFunc;
			else
				return true; // Free functions will always be less than methods (because comp
							 // returns 0).
		}

		virtual void* Comp() const { return 0; }

	  private:
		R ( *const mFunc )();
	};

	template <typename C> class ChildMethod : public Base {
	  public:
		ChildMethod( C* object, R ( C::*function )() ) : mObj( object ), mFunc( function ) {}

		virtual R operator()() { return ( mObj->*mFunc )(); }

		virtual bool operator==( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r )
				return ( mObj == r->mObj ) && ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r ) {
				if ( mObj != r->mObj )
					return mObj < r->mObj;
				else
					return 0 > memcmp( (void*)&mFunc, (void*)&( r->mFunc ), sizeof( mFunc ) );
			} else
				return mObj < rhs.Comp();
		}

		virtual void* Comp() const { return mObj; }

	  private:
		C* const mObj;
		R ( C::*const mFunc )();
	};

	/// This class is only to find the worst case method pointer size.
	class unknown;

	char mMem[sizeof( ChildMethod<unknown> )]; // Reserve memory for creating useful objects later.
	Base* mCallback;
};

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename C, typename R> Callback0<R> Make0( C* object, R ( C::*function )() ) {
	return Callback0<R>( object, function );
}

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename R> Callback0<R> Make0( R ( *function )() ) {
	return Callback0<R>( function );
}

/// Stores a callback for a function taking 1 parameters.
///\tparam R Callback function return type.
template <typename R, typename T0> class Callback1 {
  public:
	/// Constructs the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C>
	Callback1( C* object, R ( C::*function )( T0 t0 ) ) :
		mCallback( new ( &mMem ) ChildMethod<C>( object, function ) ) {}

	/// Constructs the callback to a free function or static member function.
	///\param function Free function address to call.
	Callback1( R ( *function )( T0 t0 ) ) : mCallback( new ( &mMem ) ChildFree( function ) ) {}

	/// Constructs a callback that can later be set.
	Callback1() : mCallback( 0 ) {}

	Callback1( const Callback1& c ) : mCallback( c.mCallback ) {
		if ( mCallback ) {
			memcpy( mMem, c.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}
	}

	Callback1& operator=( const Callback1& rhs ) {
		mCallback = rhs.mCallback;
		if ( mCallback ) {
			memcpy( mMem, rhs.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}

		return *this;
	}

	~Callback1() {}

	/// Sets the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C> void Reset( C* object, R ( C::*function )( T0 t0 ) ) {
		mCallback = new ( &mMem ) ChildMethod<C>( object, function );
	}

	/// Sets the callback to a free function or static member function.
	///\param function Free function address to call.
	void Reset( R ( *function )( T0 t0 ) ) { mCallback = new ( &mMem ) ChildFree( function ); }

	/// Resests to callback to nothing.
	void Reset() { mCallback = 0; }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator==( const Callback1& rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) == ( *( rhs.mCallback ) );
		else
			return mCallback == rhs.mCallback;
	}

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator!=( const Callback1& rhs ) const { return !( *this == rhs ); }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator<( const Callback1 rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) < ( *( rhs.mCallback ) );
		else
			return mCallback < rhs.mCallback;
	}

	/// Returns true if the callback has been set, or false if the callback is not set and is
	/// invalid.
	bool IsSet() const { return 0 != mCallback; }

	/// Invokes the callback.
	R operator()( T0 t0 ) const {
		if ( mCallback )
			return ( *mCallback )( t0 );
		else
			THROW_RUNTIME_ERROR( R )
	}

	/// Invokes the callback. This function can sometimes be more convenient than the operator(),
	/// which does the same thing.
	R Call( T0 t0 ) const {
		if ( mCallback )
			return ( *mCallback )( t0 );
		else
			THROW_RUNTIME_ERROR( R )
	}

  private:
	class Base {
	  public:
		Base() {}
		virtual R operator()( T0 t0 ) = 0;
		virtual bool operator==( const Base& rhs ) const = 0;
		virtual bool operator<( const Base& rhs ) const = 0;
		virtual void* Comp() const = 0; // Returns a pointer used in comparisons.
	};

	class ChildFree : public Base {
	  public:
		ChildFree( R ( *function )( T0 t0 ) ) : mFunc( function ) {}

		virtual R operator()( T0 t0 ) { return mFunc( t0 ); }

		virtual bool operator==( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return mFunc < r->mFunc;
			else
				return true; // Free functions will always be less than methods (because comp
							 // returns 0).
		}

		virtual void* Comp() const { return 0; }

	  private:
		R ( *const mFunc )( T0 t0 );
	};

	template <typename C> class ChildMethod : public Base {
	  public:
		ChildMethod( C* object, R ( C::*function )( T0 t0 ) ) : mObj( object ), mFunc( function ) {}

		virtual R operator()( T0 t0 ) { return ( mObj->*mFunc )( t0 ); }

		virtual bool operator==( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r )
				return ( mObj == r->mObj ) && ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r ) {
				if ( mObj != r->mObj )
					return mObj < r->mObj;
				else
					return 0 > memcmp( (void*)&mFunc, (void*)&( r->mFunc ), sizeof( mFunc ) );
			} else
				return mObj < rhs.Comp();
		}

		virtual void* Comp() const { return mObj; }

	  private:
		C* const mObj;
		R ( C::*const mFunc )( T0 t0 );
	};

	/// This class is only to find the worst case method pointer size.
	class unknown;

	char mMem[sizeof( ChildMethod<unknown> )]; // Reserve memory for creating useful objects later.
	Base* mCallback;
};

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename C, typename R, typename T0>
Callback1<R, T0> Make1( C* object, R ( C::*function )( T0 t0 ) ) {
	return Callback1<R, T0>( object, function );
}

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename R, typename T0> Callback1<R, T0> Make1( R ( *function )( T0 t0 ) ) {
	return Callback1<R, T0>( function );
}

/// Stores a callback for a function taking 2 parameters.
///\tparam R Callback function return type.
template <typename R, typename T0, typename T1> class Callback2 {
  public:
	/// Constructs the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C>
	Callback2( C* object, R ( C::*function )( T0 t0, T1 t1 ) ) :
		mCallback( new ( &mMem ) ChildMethod<C>( object, function ) ) {}

	/// Constructs the callback to a free function or static member function.
	///\param function Free function address to call.
	Callback2( R ( *function )( T0 t0, T1 t1 ) ) :
		mCallback( new ( &mMem ) ChildFree( function ) ) {}

	/// Constructs a callback that can later be set.
	Callback2() : mCallback( 0 ) {}

	Callback2( const Callback2& c ) : mCallback( c.mCallback ) {
		if ( mCallback ) {
			memcpy( mMem, c.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}
	}

	Callback2& operator=( const Callback2& rhs ) {
		mCallback = rhs.mCallback;
		if ( mCallback ) {
			memcpy( mMem, rhs.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}

		return *this;
	}

	~Callback2() {}

	/// Sets the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C> void Reset( C* object, R ( C::*function )( T0 t0, T1 t1 ) ) {
		mCallback = new ( &mMem ) ChildMethod<C>( object, function );
	}

	/// Sets the callback to a free function or static member function.
	///\param function Free function address to call.
	void Reset( R ( *function )( T0 t0, T1 t1 ) ) {
		mCallback = new ( &mMem ) ChildFree( function );
	}

	/// Resests to callback to nothing.
	void Reset() { mCallback = 0; }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator==( const Callback2& rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) == ( *( rhs.mCallback ) );
		else
			return mCallback == rhs.mCallback;
	}

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator!=( const Callback2& rhs ) const { return !( *this == rhs ); }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator<( const Callback2 rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) < ( *( rhs.mCallback ) );
		else
			return mCallback < rhs.mCallback;
	}

	/// Returns true if the callback has been set, or false if the callback is not set and is
	/// invalid.
	bool IsSet() const { return 0 != mCallback; }

	/// Invokes the callback.
	R operator()( T0 t0, T1 t1 ) const {
		if ( mCallback )
			return ( *mCallback )( t0, t1 );
		else
			THROW_RUNTIME_ERROR( R )
	}

	/// Invokes the callback. This function can sometimes be more convenient than the operator(),
	/// which does the same thing.
	R Call( T0 t0, T1 t1 ) const {
		if ( mCallback )
			return ( *mCallback )( t0, t1 );
		else
			THROW_RUNTIME_ERROR( R )
	}

  private:
	class Base {
	  public:
		Base() {}
		virtual R operator()( T0 t0, T1 t1 ) = 0;
		virtual bool operator==( const Base& rhs ) const = 0;
		virtual bool operator<( const Base& rhs ) const = 0;
		virtual void* Comp() const = 0; // Returns a pointer used in comparisons.
	};

	class ChildFree : public Base {
	  public:
		ChildFree( R ( *function )( T0 t0, T1 t1 ) ) : mFunc( function ) {}

		virtual R operator()( T0 t0, T1 t1 ) { return mFunc( t0, t1 ); }

		virtual bool operator==( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return mFunc < r->mFunc;
			else
				return true; // Free functions will always be less than methods (because comp
							 // returns 0).
		}

		virtual void* Comp() const { return 0; }

	  private:
		R ( *const mFunc )( T0 t0, T1 t1 );
	};

	template <typename C> class ChildMethod : public Base {
	  public:
		ChildMethod( C* object, R ( C::*function )( T0 t0, T1 t1 ) ) :
			mObj( object ), mFunc( function ) {}

		virtual R operator()( T0 t0, T1 t1 ) { return ( mObj->*mFunc )( t0, t1 ); }

		virtual bool operator==( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r )
				return ( mObj == r->mObj ) && ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r ) {
				if ( mObj != r->mObj )
					return mObj < r->mObj;
				else
					return 0 > memcmp( (void*)&mFunc, (void*)&( r->mFunc ), sizeof( mFunc ) );
			} else
				return mObj < rhs.Comp();
		}

		virtual void* Comp() const { return mObj; }

	  private:
		C* const mObj;
		R ( C::*const mFunc )( T0 t0, T1 t1 );
	};

	/// This class is only to find the worst case method pointer size.
	class unknown;

	char mMem[sizeof( ChildMethod<unknown> )]; // Reserve memory for creating useful objects later.
	Base* mCallback;
};

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename C, typename R, typename T0, typename T1>
Callback2<R, T0, T1> Make2( C* object, R ( C::*function )( T0 t0, T1 t1 ) ) {
	return Callback2<R, T0, T1>( object, function );
}

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename R, typename T0, typename T1>
Callback2<R, T0, T1> Make2( R ( *function )( T0 t0, T1 t1 ) ) {
	return Callback2<R, T0, T1>( function );
}

/// Stores a callback for a function taking 3 parameters.
///\tparam R Callback function return type.
template <typename R, typename T0, typename T1, typename T2> class Callback3 {
  public:
	/// Constructs the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C>
	Callback3( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2 ) ) :
		mCallback( new ( &mMem ) ChildMethod<C>( object, function ) ) {}

	/// Constructs the callback to a free function or static member function.
	///\param function Free function address to call.
	Callback3( R ( *function )( T0 t0, T1 t1, T2 t2 ) ) :
		mCallback( new ( &mMem ) ChildFree( function ) ) {}

	/// Constructs a callback that can later be set.
	Callback3() : mCallback( 0 ) {}

	Callback3( const Callback3& c ) : mCallback( c.mCallback ) {
		if ( mCallback ) {
			memcpy( mMem, c.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}
	}

	Callback3& operator=( const Callback3& rhs ) {
		mCallback = rhs.mCallback;
		if ( mCallback ) {
			memcpy( mMem, rhs.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}

		return *this;
	}

	~Callback3() {}

	/// Sets the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C> void Reset( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2 ) ) {
		mCallback = new ( &mMem ) ChildMethod<C>( object, function );
	}

	/// Sets the callback to a free function or static member function.
	///\param function Free function address to call.
	void Reset( R ( *function )( T0 t0, T1 t1, T2 t2 ) ) {
		mCallback = new ( &mMem ) ChildFree( function );
	}

	/// Resests to callback to nothing.
	void Reset() { mCallback = 0; }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator==( const Callback3& rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) == ( *( rhs.mCallback ) );
		else
			return mCallback == rhs.mCallback;
	}

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator!=( const Callback3& rhs ) const { return !( *this == rhs ); }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator<( const Callback3 rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) < ( *( rhs.mCallback ) );
		else
			return mCallback < rhs.mCallback;
	}

	/// Returns true if the callback has been set, or false if the callback is not set and is
	/// invalid.
	bool IsSet() const { return 0 != mCallback; }

	/// Invokes the callback.
	R operator()( T0 t0, T1 t1, T2 t2 ) const {
		if ( mCallback )
			return ( *mCallback )( t0, t1, t2 );
		else
			THROW_RUNTIME_ERROR( R )
	}

	/// Invokes the callback. This function can sometimes be more convenient than the operator(),
	/// which does the same thing.
	R Call( T0 t0, T1 t1, T2 t2 ) const {
		if ( mCallback )
			return ( *mCallback )( t0, t1, t2 );
		else
			THROW_RUNTIME_ERROR( R )
	}

  private:
	class Base {
	  public:
		Base() {}
		virtual R operator()( T0 t0, T1 t1, T2 t2 ) = 0;
		virtual bool operator==( const Base& rhs ) const = 0;
		virtual bool operator<( const Base& rhs ) const = 0;
		virtual void* Comp() const = 0; // Returns a pointer used in comparisons.
	};

	class ChildFree : public Base {
	  public:
		ChildFree( R ( *function )( T0 t0, T1 t1, T2 t2 ) ) : mFunc( function ) {}

		virtual R operator()( T0 t0, T1 t1, T2 t2 ) { return mFunc( t0, t1, t2 ); }

		virtual bool operator==( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return mFunc < r->mFunc;
			else
				return true; // Free functions will always be less than methods (because comp
							 // returns 0).
		}

		virtual void* Comp() const { return 0; }

	  private:
		R ( *const mFunc )( T0 t0, T1 t1, T2 t2 );
	};

	template <typename C> class ChildMethod : public Base {
	  public:
		ChildMethod( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2 ) ) :
			mObj( object ), mFunc( function ) {}

		virtual R operator()( T0 t0, T1 t1, T2 t2 ) { return ( mObj->*mFunc )( t0, t1, t2 ); }

		virtual bool operator==( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r )
				return ( mObj == r->mObj ) && ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r ) {
				if ( mObj != r->mObj )
					return mObj < r->mObj;
				else
					return 0 > memcmp( (void*)&mFunc, (void*)&( r->mFunc ), sizeof( mFunc ) );
			} else
				return mObj < rhs.Comp();
		}

		virtual void* Comp() const { return mObj; }

	  private:
		C* const mObj;
		R ( C::*const mFunc )( T0 t0, T1 t1, T2 t2 );
	};

	/// This class is only to find the worst case method pointer size.
	class unknown;

	char mMem[sizeof( ChildMethod<unknown> )]; // Reserve memory for creating useful objects later.
	Base* mCallback;
};

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename C, typename R, typename T0, typename T1, typename T2>
Callback3<R, T0, T1, T2> Make3( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2 ) ) {
	return Callback3<R, T0, T1, T2>( object, function );
}

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename R, typename T0, typename T1, typename T2>
Callback3<R, T0, T1, T2> Make3( R ( *function )( T0 t0, T1 t1, T2 t2 ) ) {
	return Callback3<R, T0, T1, T2>( function );
}

/// Stores a callback for a function taking 4 parameters.
///\tparam R Callback function return type.
template <typename R, typename T0, typename T1, typename T2, typename T3> class Callback4 {
  public:
	/// Constructs the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C>
	Callback4( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3 ) ) :
		mCallback( new ( &mMem ) ChildMethod<C>( object, function ) ) {}

	/// Constructs the callback to a free function or static member function.
	///\param function Free function address to call.
	Callback4( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3 ) ) :
		mCallback( new ( &mMem ) ChildFree( function ) ) {}

	/// Constructs a callback that can later be set.
	Callback4() : mCallback( 0 ) {}

	Callback4( const Callback4& c ) : mCallback( c.mCallback ) {
		if ( mCallback ) {
			memcpy( mMem, c.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}
	}

	Callback4& operator=( const Callback4& rhs ) {
		mCallback = rhs.mCallback;
		if ( mCallback ) {
			memcpy( mMem, rhs.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}

		return *this;
	}

	~Callback4() {}

	/// Sets the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C>
	void Reset( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3 ) ) {
		mCallback = new ( &mMem ) ChildMethod<C>( object, function );
	}

	/// Sets the callback to a free function or static member function.
	///\param function Free function address to call.
	void Reset( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3 ) ) {
		mCallback = new ( &mMem ) ChildFree( function );
	}

	/// Resests to callback to nothing.
	void Reset() { mCallback = 0; }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator==( const Callback4& rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) == ( *( rhs.mCallback ) );
		else
			return mCallback == rhs.mCallback;
	}

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator!=( const Callback4& rhs ) const { return !( *this == rhs ); }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator<( const Callback4 rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) < ( *( rhs.mCallback ) );
		else
			return mCallback < rhs.mCallback;
	}

	/// Returns true if the callback has been set, or false if the callback is not set and is
	/// invalid.
	bool IsSet() const { return 0 != mCallback; }

	/// Invokes the callback.
	R operator()( T0 t0, T1 t1, T2 t2, T3 t3 ) const {
		if ( mCallback )
			return ( *mCallback )( t0, t1, t2, t3 );
		else
			THROW_RUNTIME_ERROR( R )
	}

	/// Invokes the callback. This function can sometimes be more convenient than the operator(),
	/// which does the same thing.
	R Call( T0 t0, T1 t1, T2 t2, T3 t3 ) const {
		if ( mCallback )
			return ( *mCallback )( t0, t1, t2, t3 );
		else
			THROW_RUNTIME_ERROR( R )
	}

  private:
	class Base {
	  public:
		Base() {}
		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3 ) = 0;
		virtual bool operator==( const Base& rhs ) const = 0;
		virtual bool operator<( const Base& rhs ) const = 0;
		virtual void* Comp() const = 0; // Returns a pointer used in comparisons.
	};

	class ChildFree : public Base {
	  public:
		ChildFree( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3 ) ) : mFunc( function ) {}

		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3 ) { return mFunc( t0, t1, t2, t3 ); }

		virtual bool operator==( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return mFunc < r->mFunc;
			else
				return true; // Free functions will always be less than methods (because comp
							 // returns 0).
		}

		virtual void* Comp() const { return 0; }

	  private:
		R ( *const mFunc )( T0 t0, T1 t1, T2 t2, T3 t3 );
	};

	template <typename C> class ChildMethod : public Base {
	  public:
		ChildMethod( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3 ) ) :
			mObj( object ), mFunc( function ) {}

		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3 ) {
			return ( mObj->*mFunc )( t0, t1, t2, t3 );
		}

		virtual bool operator==( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r )
				return ( mObj == r->mObj ) && ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r ) {
				if ( mObj != r->mObj )
					return mObj < r->mObj;
				else
					return 0 > memcmp( (void*)&mFunc, (void*)&( r->mFunc ), sizeof( mFunc ) );
			} else
				return mObj < rhs.Comp();
		}

		virtual void* Comp() const { return mObj; }

	  private:
		C* const mObj;
		R ( C::*const mFunc )( T0 t0, T1 t1, T2 t2, T3 t3 );
	};

	/// This class is only to find the worst case method pointer size.
	class unknown;

	char mMem[sizeof( ChildMethod<unknown> )]; // Reserve memory for creating useful objects later.
	Base* mCallback;
};

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename C, typename R, typename T0, typename T1, typename T2, typename T3>
Callback4<R, T0, T1, T2, T3> Make4( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3 ) ) {
	return Callback4<R, T0, T1, T2, T3>( object, function );
}

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename R, typename T0, typename T1, typename T2, typename T3>
Callback4<R, T0, T1, T2, T3> Make4( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3 ) ) {
	return Callback4<R, T0, T1, T2, T3>( function );
}

/// Stores a callback for a function taking 5 parameters.
///\tparam R Callback function return type.
template <typename R, typename T0, typename T1, typename T2, typename T3, typename T4>
class Callback5 {
  public:
	/// Constructs the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C>
	Callback5( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4 ) ) :
		mCallback( new ( &mMem ) ChildMethod<C>( object, function ) ) {}

	/// Constructs the callback to a free function or static member function.
	///\param function Free function address to call.
	Callback5( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4 ) ) :
		mCallback( new ( &mMem ) ChildFree( function ) ) {}

	/// Constructs a callback that can later be set.
	Callback5() : mCallback( 0 ) {}

	Callback5( const Callback5& c ) : mCallback( c.mCallback ) {
		if ( mCallback ) {
			memcpy( mMem, c.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}
	}

	Callback5& operator=( const Callback5& rhs ) {
		mCallback = rhs.mCallback;
		if ( mCallback ) {
			memcpy( mMem, rhs.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}

		return *this;
	}

	~Callback5() {}

	/// Sets the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C>
	void Reset( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4 ) ) {
		mCallback = new ( &mMem ) ChildMethod<C>( object, function );
	}

	/// Sets the callback to a free function or static member function.
	///\param function Free function address to call.
	void Reset( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4 ) ) {
		mCallback = new ( &mMem ) ChildFree( function );
	}

	/// Resests to callback to nothing.
	void Reset() { mCallback = 0; }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator==( const Callback5& rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) == ( *( rhs.mCallback ) );
		else
			return mCallback == rhs.mCallback;
	}

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator!=( const Callback5& rhs ) const { return !( *this == rhs ); }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator<( const Callback5 rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) < ( *( rhs.mCallback ) );
		else
			return mCallback < rhs.mCallback;
	}

	/// Returns true if the callback has been set, or false if the callback is not set and is
	/// invalid.
	bool IsSet() const { return 0 != mCallback; }

	/// Invokes the callback.
	R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4 ) const {
		if ( mCallback )
			return ( *mCallback )( t0, t1, t2, t3, t4 );
		else
			THROW_RUNTIME_ERROR( R )
	}

	/// Invokes the callback. This function can sometimes be more convenient than the operator(),
	/// which does the same thing.
	R Call( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4 ) const {
		if ( mCallback )
			return ( *mCallback )( t0, t1, t2, t3, t4 );
		else
			THROW_RUNTIME_ERROR( R )
	}

  private:
	class Base {
	  public:
		Base() {}
		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4 ) = 0;
		virtual bool operator==( const Base& rhs ) const = 0;
		virtual bool operator<( const Base& rhs ) const = 0;
		virtual void* Comp() const = 0; // Returns a pointer used in comparisons.
	};

	class ChildFree : public Base {
	  public:
		ChildFree( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4 ) ) : mFunc( function ) {}

		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4 ) {
			return mFunc( t0, t1, t2, t3, t4 );
		}

		virtual bool operator==( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return mFunc < r->mFunc;
			else
				return true; // Free functions will always be less than methods (because comp
							 // returns 0).
		}

		virtual void* Comp() const { return 0; }

	  private:
		R ( *const mFunc )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4 );
	};

	template <typename C> class ChildMethod : public Base {
	  public:
		ChildMethod( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4 ) ) :
			mObj( object ), mFunc( function ) {}

		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4 ) {
			return ( mObj->*mFunc )( t0, t1, t2, t3, t4 );
		}

		virtual bool operator==( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r )
				return ( mObj == r->mObj ) && ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r ) {
				if ( mObj != r->mObj )
					return mObj < r->mObj;
				else
					return 0 > memcmp( (void*)&mFunc, (void*)&( r->mFunc ), sizeof( mFunc ) );
			} else
				return mObj < rhs.Comp();
		}

		virtual void* Comp() const { return mObj; }

	  private:
		C* const mObj;
		R ( C::*const mFunc )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4 );
	};

	/// This class is only to find the worst case method pointer size.
	class unknown;

	char mMem[sizeof( ChildMethod<unknown> )]; // Reserve memory for creating useful objects later.
	Base* mCallback;
};

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4>
Callback5<R, T0, T1, T2, T3, T4> Make5( C* object,
										R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4 ) ) {
	return Callback5<R, T0, T1, T2, T3, T4>( object, function );
}

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename R, typename T0, typename T1, typename T2, typename T3, typename T4>
Callback5<R, T0, T1, T2, T3, T4> Make5( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4 ) ) {
	return Callback5<R, T0, T1, T2, T3, T4>( function );
}

/// Stores a callback for a function taking 6 parameters.
///\tparam R Callback function return type.
template <typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
class Callback6 {
  public:
	/// Constructs the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C>
	Callback6( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5 ) ) :
		mCallback( new ( &mMem ) ChildMethod<C>( object, function ) ) {}

	/// Constructs the callback to a free function or static member function.
	///\param function Free function address to call.
	Callback6( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5 ) ) :
		mCallback( new ( &mMem ) ChildFree( function ) ) {}

	/// Constructs a callback that can later be set.
	Callback6() : mCallback( 0 ) {}

	Callback6( const Callback6& c ) : mCallback( c.mCallback ) {
		if ( mCallback ) {
			memcpy( mMem, c.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}
	}

	Callback6& operator=( const Callback6& rhs ) {
		mCallback = rhs.mCallback;
		if ( mCallback ) {
			memcpy( mMem, rhs.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}

		return *this;
	}

	~Callback6() {}

	/// Sets the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C>
	void Reset( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5 ) ) {
		mCallback = new ( &mMem ) ChildMethod<C>( object, function );
	}

	/// Sets the callback to a free function or static member function.
	///\param function Free function address to call.
	void Reset( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5 ) ) {
		mCallback = new ( &mMem ) ChildFree( function );
	}

	/// Resests to callback to nothing.
	void Reset() { mCallback = 0; }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator==( const Callback6& rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) == ( *( rhs.mCallback ) );
		else
			return mCallback == rhs.mCallback;
	}

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator!=( const Callback6& rhs ) const { return !( *this == rhs ); }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator<( const Callback6 rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) < ( *( rhs.mCallback ) );
		else
			return mCallback < rhs.mCallback;
	}

	/// Returns true if the callback has been set, or false if the callback is not set and is
	/// invalid.
	bool IsSet() const { return 0 != mCallback; }

	/// Invokes the callback.
	R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5 ) const {
		if ( mCallback )
			return ( *mCallback )( t0, t1, t2, t3, t4, t5 );
		else
			THROW_RUNTIME_ERROR( R )
	}

	/// Invokes the callback. This function can sometimes be more convenient than the operator(),
	/// which does the same thing.
	R Call( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5 ) const {
		if ( mCallback )
			return ( *mCallback )( t0, t1, t2, t3, t4, t5 );
		else
			THROW_RUNTIME_ERROR( R )
	}

  private:
	class Base {
	  public:
		Base() {}
		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5 ) = 0;
		virtual bool operator==( const Base& rhs ) const = 0;
		virtual bool operator<( const Base& rhs ) const = 0;
		virtual void* Comp() const = 0; // Returns a pointer used in comparisons.
	};

	class ChildFree : public Base {
	  public:
		ChildFree( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5 ) ) :
			mFunc( function ) {}

		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5 ) {
			return mFunc( t0, t1, t2, t3, t4, t5 );
		}

		virtual bool operator==( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return mFunc < r->mFunc;
			else
				return true; // Free functions will always be less than methods (because comp
							 // returns 0).
		}

		virtual void* Comp() const { return 0; }

	  private:
		R ( *const mFunc )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5 );
	};

	template <typename C> class ChildMethod : public Base {
	  public:
		ChildMethod( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5 ) ) :
			mObj( object ), mFunc( function ) {}

		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5 ) {
			return ( mObj->*mFunc )( t0, t1, t2, t3, t4, t5 );
		}

		virtual bool operator==( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r )
				return ( mObj == r->mObj ) && ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r ) {
				if ( mObj != r->mObj )
					return mObj < r->mObj;
				else
					return 0 > memcmp( (void*)&mFunc, (void*)&( r->mFunc ), sizeof( mFunc ) );
			} else
				return mObj < rhs.Comp();
		}

		virtual void* Comp() const { return mObj; }

	  private:
		C* const mObj;
		R ( C::*const mFunc )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5 );
	};

	/// This class is only to find the worst case method pointer size.
	class unknown;

	char mMem[sizeof( ChildMethod<unknown> )]; // Reserve memory for creating useful objects later.
	Base* mCallback;
};

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4,
		  typename T5>
Callback6<R, T0, T1, T2, T3, T4, T5> Make6( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2,
																		   T3 t3, T4 t4, T5 t5 ) ) {
	return Callback6<R, T0, T1, T2, T3, T4, T5>( object, function );
}

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
Callback6<R, T0, T1, T2, T3, T4, T5> Make6( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4,
															 T5 t5 ) ) {
	return Callback6<R, T0, T1, T2, T3, T4, T5>( function );
}

/// Stores a callback for a function taking 7 parameters.
///\tparam R Callback function return type.
template <typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
		  typename T6>
class Callback7 {
  public:
	/// Constructs the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C>
	Callback7( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6 ) ) :
		mCallback( new ( &mMem ) ChildMethod<C>( object, function ) ) {}

	/// Constructs the callback to a free function or static member function.
	///\param function Free function address to call.
	Callback7( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6 ) ) :
		mCallback( new ( &mMem ) ChildFree( function ) ) {}

	/// Constructs a callback that can later be set.
	Callback7() : mCallback( 0 ) {}

	Callback7( const Callback7& c ) : mCallback( c.mCallback ) {
		if ( mCallback ) {
			memcpy( mMem, c.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}
	}

	Callback7& operator=( const Callback7& rhs ) {
		mCallback = rhs.mCallback;
		if ( mCallback ) {
			memcpy( mMem, rhs.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}

		return *this;
	}

	~Callback7() {}

	/// Sets the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C>
	void Reset( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6 ) ) {
		mCallback = new ( &mMem ) ChildMethod<C>( object, function );
	}

	/// Sets the callback to a free function or static member function.
	///\param function Free function address to call.
	void Reset( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6 ) ) {
		mCallback = new ( &mMem ) ChildFree( function );
	}

	/// Resests to callback to nothing.
	void Reset() { mCallback = 0; }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator==( const Callback7& rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) == ( *( rhs.mCallback ) );
		else
			return mCallback == rhs.mCallback;
	}

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator!=( const Callback7& rhs ) const { return !( *this == rhs ); }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator<( const Callback7 rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) < ( *( rhs.mCallback ) );
		else
			return mCallback < rhs.mCallback;
	}

	/// Returns true if the callback has been set, or false if the callback is not set and is
	/// invalid.
	bool IsSet() const { return 0 != mCallback; }

	/// Invokes the callback.
	R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6 ) const {
		if ( mCallback )
			return ( *mCallback )( t0, t1, t2, t3, t4, t5, t6 );
		else
			THROW_RUNTIME_ERROR( R )
	}

	/// Invokes the callback. This function can sometimes be more convenient than the operator(),
	/// which does the same thing.
	R Call( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6 ) const {
		if ( mCallback )
			return ( *mCallback )( t0, t1, t2, t3, t4, t5, t6 );
		else
			THROW_RUNTIME_ERROR( R )
	}

  private:
	class Base {
	  public:
		Base() {}
		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6 ) = 0;
		virtual bool operator==( const Base& rhs ) const = 0;
		virtual bool operator<( const Base& rhs ) const = 0;
		virtual void* Comp() const = 0; // Returns a pointer used in comparisons.
	};

	class ChildFree : public Base {
	  public:
		ChildFree( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6 ) ) :
			mFunc( function ) {}

		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6 ) {
			return mFunc( t0, t1, t2, t3, t4, t5, t6 );
		}

		virtual bool operator==( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return mFunc < r->mFunc;
			else
				return true; // Free functions will always be less than methods (because comp
							 // returns 0).
		}

		virtual void* Comp() const { return 0; }

	  private:
		R ( *const mFunc )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6 );
	};

	template <typename C> class ChildMethod : public Base {
	  public:
		ChildMethod( C* object,
					 R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6 ) ) :
			mObj( object ), mFunc( function ) {}

		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6 ) {
			return ( mObj->*mFunc )( t0, t1, t2, t3, t4, t5, t6 );
		}

		virtual bool operator==( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r )
				return ( mObj == r->mObj ) && ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r ) {
				if ( mObj != r->mObj )
					return mObj < r->mObj;
				else
					return 0 > memcmp( (void*)&mFunc, (void*)&( r->mFunc ), sizeof( mFunc ) );
			} else
				return mObj < rhs.Comp();
		}

		virtual void* Comp() const { return mObj; }

	  private:
		C* const mObj;
		R ( C::*const mFunc )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6 );
	};

	/// This class is only to find the worst case method pointer size.
	class unknown;

	char mMem[sizeof( ChildMethod<unknown> )]; // Reserve memory for creating useful objects later.
	Base* mCallback;
};

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4,
		  typename T5, typename T6>
Callback7<R, T0, T1, T2, T3, T4, T5, T6>
Make7( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6 ) ) {
	return Callback7<R, T0, T1, T2, T3, T4, T5, T6>( object, function );
}

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
		  typename T6>
Callback7<R, T0, T1, T2, T3, T4, T5, T6> Make7( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4,
																 T5 t5, T6 t6 ) ) {
	return Callback7<R, T0, T1, T2, T3, T4, T5, T6>( function );
}

/// Stores a callback for a function taking 8 parameters.
///\tparam R Callback function return type.
template <typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
		  typename T6, typename T7>
class Callback8 {
  public:
	/// Constructs the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C>
	Callback8( C* object,
			   R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7 ) ) :
		mCallback( new ( &mMem ) ChildMethod<C>( object, function ) ) {}

	/// Constructs the callback to a free function or static member function.
	///\param function Free function address to call.
	Callback8( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7 ) ) :
		mCallback( new ( &mMem ) ChildFree( function ) ) {}

	/// Constructs a callback that can later be set.
	Callback8() : mCallback( 0 ) {}

	Callback8( const Callback8& c ) : mCallback( c.mCallback ) {
		if ( mCallback ) {
			memcpy( mMem, c.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}
	}

	Callback8& operator=( const Callback8& rhs ) {
		mCallback = rhs.mCallback;
		if ( mCallback ) {
			memcpy( mMem, rhs.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}

		return *this;
	}

	~Callback8() {}

	/// Sets the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C>
	void Reset( C* object,
				R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7 ) ) {
		mCallback = new ( &mMem ) ChildMethod<C>( object, function );
	}

	/// Sets the callback to a free function or static member function.
	///\param function Free function address to call.
	void Reset( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7 ) ) {
		mCallback = new ( &mMem ) ChildFree( function );
	}

	/// Resests to callback to nothing.
	void Reset() { mCallback = 0; }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator==( const Callback8& rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) == ( *( rhs.mCallback ) );
		else
			return mCallback == rhs.mCallback;
	}

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator!=( const Callback8& rhs ) const { return !( *this == rhs ); }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator<( const Callback8 rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) < ( *( rhs.mCallback ) );
		else
			return mCallback < rhs.mCallback;
	}

	/// Returns true if the callback has been set, or false if the callback is not set and is
	/// invalid.
	bool IsSet() const { return 0 != mCallback; }

	/// Invokes the callback.
	R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7 ) const {
		if ( mCallback )
			return ( *mCallback )( t0, t1, t2, t3, t4, t5, t6, t7 );
		else
			THROW_RUNTIME_ERROR( R )
	}

	/// Invokes the callback. This function can sometimes be more convenient than the operator(),
	/// which does the same thing.
	R Call( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7 ) const {
		if ( mCallback )
			return ( *mCallback )( t0, t1, t2, t3, t4, t5, t6, t7 );
		else
			THROW_RUNTIME_ERROR( R )
	}

  private:
	class Base {
	  public:
		Base() {}
		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7 ) = 0;
		virtual bool operator==( const Base& rhs ) const = 0;
		virtual bool operator<( const Base& rhs ) const = 0;
		virtual void* Comp() const = 0; // Returns a pointer used in comparisons.
	};

	class ChildFree : public Base {
	  public:
		ChildFree( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7 ) ) :
			mFunc( function ) {}

		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7 ) {
			return mFunc( t0, t1, t2, t3, t4, t5, t6, t7 );
		}

		virtual bool operator==( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return mFunc < r->mFunc;
			else
				return true; // Free functions will always be less than methods (because comp
							 // returns 0).
		}

		virtual void* Comp() const { return 0; }

	  private:
		R ( *const mFunc )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7 );
	};

	template <typename C> class ChildMethod : public Base {
	  public:
		ChildMethod( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6,
													T7 t7 ) ) :
			mObj( object ), mFunc( function ) {}

		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7 ) {
			return ( mObj->*mFunc )( t0, t1, t2, t3, t4, t5, t6, t7 );
		}

		virtual bool operator==( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r )
				return ( mObj == r->mObj ) && ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r ) {
				if ( mObj != r->mObj )
					return mObj < r->mObj;
				else
					return 0 > memcmp( (void*)&mFunc, (void*)&( r->mFunc ), sizeof( mFunc ) );
			} else
				return mObj < rhs.Comp();
		}

		virtual void* Comp() const { return mObj; }

	  private:
		C* const mObj;
		R ( C::*const mFunc )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7 );
	};

	/// This class is only to find the worst case method pointer size.
	class unknown;

	char mMem[sizeof( ChildMethod<unknown> )]; // Reserve memory for creating useful objects later.
	Base* mCallback;
};

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4,
		  typename T5, typename T6, typename T7>
Callback8<R, T0, T1, T2, T3, T4, T5, T6, T7>
Make8( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7 ) ) {
	return Callback8<R, T0, T1, T2, T3, T4, T5, T6, T7>( object, function );
}

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
		  typename T6, typename T7>
Callback8<R, T0, T1, T2, T3, T4, T5, T6, T7>
Make8( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7 ) ) {
	return Callback8<R, T0, T1, T2, T3, T4, T5, T6, T7>( function );
}

/// Stores a callback for a function taking 9 parameters.
///\tparam R Callback function return type.
template <typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
		  typename T6, typename T7, typename T8>
class Callback9 {
  public:
	/// Constructs the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C>
	Callback9( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6,
											  T7 t7, T8 t8 ) ) :
		mCallback( new ( &mMem ) ChildMethod<C>( object, function ) ) {}

	/// Constructs the callback to a free function or static member function.
	///\param function Free function address to call.
	Callback9( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8 ) ) :
		mCallback( new ( &mMem ) ChildFree( function ) ) {}

	/// Constructs a callback that can later be set.
	Callback9() : mCallback( 0 ) {}

	Callback9( const Callback9& c ) : mCallback( c.mCallback ) {
		if ( mCallback ) {
			memcpy( mMem, c.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}
	}

	Callback9& operator=( const Callback9& rhs ) {
		mCallback = rhs.mCallback;
		if ( mCallback ) {
			memcpy( mMem, rhs.mMem, sizeof( mMem ) );
			mCallback = reinterpret_cast<Base*>( &mMem );
		}

		return *this;
	}

	~Callback9() {}

	/// Sets the callback to a specific object and member function.
	///\param object Pointer to the object to call upon. Care should be taken that this object
	/// remains valid as long as the callback may be invoked. \param function Member function
	/// address to call.
	template <typename C>
	void Reset( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6,
											   T7 t7, T8 t8 ) ) {
		mCallback = new ( &mMem ) ChildMethod<C>( object, function );
	}

	/// Sets the callback to a free function or static member function.
	///\param function Free function address to call.
	void Reset( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8 ) ) {
		mCallback = new ( &mMem ) ChildFree( function );
	}

	/// Resests to callback to nothing.
	void Reset() { mCallback = 0; }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator==( const Callback9& rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) == ( *( rhs.mCallback ) );
		else
			return mCallback == rhs.mCallback;
	}

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator!=( const Callback9& rhs ) const { return !( *this == rhs ); }

	/// Note that comparison operators may not work with virtual function callbacks.
	bool operator<( const Callback9 rhs ) const {
		if ( mCallback && rhs.mCallback )
			return ( *mCallback ) < ( *( rhs.mCallback ) );
		else
			return mCallback < rhs.mCallback;
	}

	/// Returns true if the callback has been set, or false if the callback is not set and is
	/// invalid.
	bool IsSet() const { return 0 != mCallback; }

	/// Invokes the callback.
	R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8 ) const {
		if ( mCallback )
			return ( *mCallback )( t0, t1, t2, t3, t4, t5, t6, t7, t8 );
		else
			THROW_RUNTIME_ERROR( R )
	}

	/// Invokes the callback. This function can sometimes be more convenient than the operator(),
	/// which does the same thing.
	R Call( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8 ) const {
		if ( mCallback )
			return ( *mCallback )( t0, t1, t2, t3, t4, t5, t6, t7, t8 );
		else
			THROW_RUNTIME_ERROR( R )
	}

  private:
	class Base {
	  public:
		Base() {}
		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8 ) = 0;
		virtual bool operator==( const Base& rhs ) const = 0;
		virtual bool operator<( const Base& rhs ) const = 0;
		virtual void* Comp() const = 0; // Returns a pointer used in comparisons.
	};

	class ChildFree : public Base {
	  public:
		ChildFree( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7,
									T8 t8 ) ) :
			mFunc( function ) {}

		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8 ) {
			return mFunc( t0, t1, t2, t3, t4, t5, t6, t7, t8 );
		}

		virtual bool operator==( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildFree* const r = dynamic_cast<const ChildFree*>( &rhs );
			if ( r )
				return mFunc < r->mFunc;
			else
				return true; // Free functions will always be less than methods (because comp
							 // returns 0).
		}

		virtual void* Comp() const { return 0; }

	  private:
		R ( *const mFunc )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8 );
	};

	template <typename C> class ChildMethod : public Base {
	  public:
		ChildMethod( C* object, R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6,
													T7 t7, T8 t8 ) ) :
			mObj( object ), mFunc( function ) {}

		virtual R operator()( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8 ) {
			return ( mObj->*mFunc )( t0, t1, t2, t3, t4, t5, t6, t7, t8 );
		}

		virtual bool operator==( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r )
				return ( mObj == r->mObj ) && ( mFunc == r->mFunc );
			else
				return false;
		}

		virtual bool operator<( const Base& rhs ) const {
			const ChildMethod<C>* const r = dynamic_cast<const ChildMethod<C>*>( &rhs );
			if ( r ) {
				if ( mObj != r->mObj )
					return mObj < r->mObj;
				else
					return 0 > memcmp( (void*)&mFunc, (void*)&( r->mFunc ), sizeof( mFunc ) );
			} else
				return mObj < rhs.Comp();
		}

		virtual void* Comp() const { return mObj; }

	  private:
		C* const mObj;
		R ( C::*const mFunc )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8 );
	};

	/// This class is only to find the worst case method pointer size.
	class unknown;

	char mMem[sizeof( ChildMethod<unknown> )]; // Reserve memory for creating useful objects later.
	Base* mCallback;
};

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4,
		  typename T5, typename T6, typename T7, typename T8>
Callback9<R, T0, T1, T2, T3, T4, T5, T6, T7, T8>
Make9( C* object,
	   R ( C::*function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8 ) ) {
	return Callback9<R, T0, T1, T2, T3, T4, T5, T6, T7, T8>( object, function );
}

/// Helper function to construct a callback without bothering to specify template parameters.
template <typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
		  typename T6, typename T7, typename T8>
Callback9<R, T0, T1, T2, T3, T4, T5, T6, T7, T8>
Make9( R ( *function )( T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8 ) ) {
	return Callback9<R, T0, T1, T2, T3, T4, T5, T6, T7, T8>( function );
}

} // namespace cb
#endif /*__CALLBACK_HPP__*/
