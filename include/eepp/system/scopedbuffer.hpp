#ifndef EE_SYSTEM_SCOPEDBUFFER
#define EE_SYSTEM_SCOPEDBUFFER

#include <cstddef>
#include <eepp/config.hpp>
#include <eepp/core/debug.hpp>
#include <eepp/core/memorymanager.hpp>
#include <eepp/core/noncopyable.hpp>

namespace EE { namespace System {

/** @brief Keep a pointer to a buffer and release it when the ScopedBuffer goes out of scope.

The TScopedBuffer class template stores a pointer to a dynamically allocated array.
(Dynamically allocated arrays are allocated with the C++ new[] expression.)
The array pointed to is guaranteed to be deleted, either on destruction of the TScopedBuffer,
or via an explicit reset.

The TScopedBuffer template is a simple solution for simple needs.
It supplies a basic "resource acquisition is initialization" facility,
without shared-ownership or transfer-of-ownership semantics.
Both its name and enforcement of semantics (by being NonCopyable) signal its
intent to retain ownership solely within the current scope.
*/
template <typename T> class TScopedBuffer : NonCopyable {
  public:
	TScopedBuffer();

	TScopedBuffer( std::size_t length );

	TScopedBuffer( T* data, std::size_t length );

	/** @brief The destructor deletes the buffer */
	~TScopedBuffer();

	void clear();

	T& operator[]( std::size_t i ) const;

	bool operator()() const;

	T* get() const;

	void swap( TScopedBuffer<T>& b );

	void reset( T* p = 0, const std::size_t& size = 0 );

	void reset( const std::size_t& size = 0 );

	/** creates a copy of p */
	void reset( const T* p = 0, const std::size_t& size = 0 );

	bool isEmpty() const;

	const std::size_t& size() const;

	const std::size_t& length() const;

  private:
	/** Pointer to the buffer */
	T* mData;

	/** Buffer size */
	std::size_t mSize;
};

template <typename T> TScopedBuffer<T>::TScopedBuffer() : mData( NULL ), mSize( 0 ) {}

template <typename T>
TScopedBuffer<T>::TScopedBuffer( std::size_t length ) :
	mData( eeNewArray( T, length ) ), mSize( length ) {}

template <typename T>
TScopedBuffer<T>::TScopedBuffer( T* data, std::size_t length ) : mData( data ), mSize( length ) {}

template <typename T> TScopedBuffer<T>::~TScopedBuffer() {
	clear();
}

template <typename T> void TScopedBuffer<T>::clear() {
	eeSAFE_DELETE_ARRAY( mData );
	mSize = 0;
}

template <typename T> T& TScopedBuffer<T>::operator[]( std::size_t i ) const {
	eeASSERT( i < mSize && NULL != mData );
	return mData[i];
}

template <typename T> bool TScopedBuffer<T>::operator()() const {
	return NULL != mData;
}

template <typename T> T* TScopedBuffer<T>::get() const {
	return mData;
}

template <typename T> const std::size_t& TScopedBuffer<T>::size() const {
	return mSize;
}

template <typename T> const std::size_t& TScopedBuffer<T>::length() const {
	return mSize;
}

template <typename T> bool TScopedBuffer<T>::isEmpty() const {
	return mData == NULL;
}

template <typename T> void TScopedBuffer<T>::swap( TScopedBuffer<T>& b ) {
	std::swap( mData, b.mData );
	std::swap( mSize, b.mSize );
}

template <typename T> void TScopedBuffer<T>::reset( T* p, const std::size_t& size ) {
	eeASSERT( p == 0 || p != mData );
	clear();
	mData = p;
	mSize = size;
}

template <typename T> void TScopedBuffer<T>::reset( const T* p, const std::size_t& size ) {
	eeASSERT( p == 0 || p != mData );
	reset( size );
	memcpy( mData, p, size );
}

template <typename T> void TScopedBuffer<T>::reset( const std::size_t& size ) {
	clear();
	mData = eeNewArray( T, ( size ) );
	mSize = size;
}

typedef TScopedBuffer<Uint8> ScopedBuffer;

}} // namespace EE::System

#endif
