#ifndef EE_CORE_RESULT_HPP
#define EE_CORE_RESULT_HPP

#include <eepp/core/debug.hpp>
#include <memory>
#include <type_traits>
#include <utility>

namespace EE {

/** Success payload produced by Ok(). It converts into any Result whose value type can be
 * constructed from T. */
template <typename T> struct OkValue {
	T value;
};

/** Success payload for Result<void, E>. */
template <> struct OkValue<void> {};

/** Error payload produced by Err(). It converts into any Result whose error type can be
 * constructed from E. */
template <typename E> struct ErrValue {
	E error;
};

/** Wraps a success value for implicit conversion into a Result. */
template <typename T> constexpr OkValue<std::decay_t<T>> Ok( T&& value ) {
	return OkValue<std::decay_t<T>>{ std::forward<T>( value ) };
}

/** Success value for Result<void, E>. */
constexpr OkValue<void> Ok() {
	return {};
}

/** Wraps an error value for implicit conversion into a Result. */
template <typename E> constexpr ErrValue<std::decay_t<E>> Err( E&& error ) {
	return ErrValue<std::decay_t<E>>{ std::forward<E>( error ) };
}

/** @brief A synchronous success-or-error value.
 *
 * Result<T, E> holds either a T or an E, stored inline in a union. It has no threading or async
 * semantics and never throws: accessing the inactive member is a contract violation reported by a
 * debug assertion. Move-only value and error types are supported; copy operations exist only when
 * both contained types are copyable.
 *
 * @code
 * Result<VFSFileInfo, VFSError> statResult = Ok( std::move( info ) );
 * if ( statResult )
 *     consume( statResult.value() );
 * else
 *     report( statResult.error() );
 * @endcode
 */
template <typename T, typename E> class Result {
	static_assert( !std::is_reference_v<T> && !std::is_reference_v<E>,
				   "Result does not store references" );

  public:
	using ValueType = T;
	using ErrorType = E;

	template <typename U, std::enable_if_t<std::is_constructible_v<T, U&&>, int> = 0>
	Result( OkValue<U>&& ok ) : mHasValue( true ) {
		std::construct_at( std::addressof( mValue ), std::move( ok.value ) );
	}

	template <typename U, std::enable_if_t<std::is_constructible_v<T, const U&>, int> = 0>
	Result( const OkValue<U>& ok ) : mHasValue( true ) {
		std::construct_at( std::addressof( mValue ), ok.value );
	}

	template <typename G, std::enable_if_t<std::is_constructible_v<E, G&&>, int> = 0>
	Result( ErrValue<G>&& err ) : mHasValue( false ) {
		std::construct_at( std::addressof( mError ), std::move( err.error ) );
	}

	template <typename G, std::enable_if_t<std::is_constructible_v<E, const G&>, int> = 0>
	Result( const ErrValue<G>& err ) : mHasValue( false ) {
		std::construct_at( std::addressof( mError ), err.error );
	}

	Result( const Result& other )
		requires( std::is_copy_constructible_v<T> && std::is_copy_constructible_v<E> )
		: mHasValue( other.mHasValue ) {
		if ( mHasValue )
			std::construct_at( std::addressof( mValue ), other.mValue );
		else
			std::construct_at( std::addressof( mError ), other.mError );
	}

	Result( Result&& other ) noexcept( std::is_nothrow_move_constructible_v<T> &&
									   std::is_nothrow_move_constructible_v<E> )
		requires( std::is_move_constructible_v<T> && std::is_move_constructible_v<E> )
		: mHasValue( other.mHasValue ) {
		if ( mHasValue )
			std::construct_at( std::addressof( mValue ), std::move( other.mValue ) );
		else
			std::construct_at( std::addressof( mError ), std::move( other.mError ) );
	}

	Result& operator=( const Result& other )
		requires( std::is_copy_constructible_v<T> && std::is_copy_constructible_v<E> &&
				  std::is_copy_assignable_v<T> && std::is_copy_assignable_v<E> )
	{
		if ( this != &other )
			assign( other );
		return *this;
	}

	Result& operator=( Result&& other ) noexcept( std::is_nothrow_move_constructible_v<T> &&
												  std::is_nothrow_move_constructible_v<E> &&
												  std::is_nothrow_move_assignable_v<T> &&
												  std::is_nothrow_move_assignable_v<E> )
		requires( std::is_move_constructible_v<T> && std::is_move_constructible_v<E> &&
				  std::is_move_assignable_v<T> && std::is_move_assignable_v<E> )
	{
		if ( this != &other )
			assign( std::move( other ) );
		return *this;
	}

	~Result() { destroy(); }

	bool hasValue() const noexcept { return mHasValue; }

	bool hasError() const noexcept { return !mHasValue; }

	explicit operator bool() const noexcept { return mHasValue; }

	T& value() & {
		eeASSERTM( mHasValue, "Result::value() called on an error result" );
		return mValue;
	}

	const T& value() const& {
		eeASSERTM( mHasValue, "Result::value() called on an error result" );
		return mValue;
	}

	T&& value() && {
		eeASSERTM( mHasValue, "Result::value() called on an error result" );
		return std::move( mValue );
	}

	E& error() & {
		eeASSERTM( !mHasValue, "Result::error() called on a success result" );
		return mError;
	}

	const E& error() const& {
		eeASSERTM( !mHasValue, "Result::error() called on a success result" );
		return mError;
	}

	E&& error() && {
		eeASSERTM( !mHasValue, "Result::error() called on a success result" );
		return std::move( mError );
	}

	T* operator->() { return &value(); }

	const T* operator->() const { return &value(); }

	T& operator*() & { return value(); }

	const T& operator*() const& { return value(); }

	T&& operator*() && { return std::move( *this ).value(); }

  private:
	void destroy() noexcept {
		if ( mHasValue )
			std::destroy_at( std::addressof( mValue ) );
		else
			std::destroy_at( std::addressof( mError ) );
	}

	template <typename Other> void assign( Other&& other ) {
		if ( mHasValue == other.mHasValue ) {
			if ( mHasValue )
				mValue = std::forward<Other>( other ).mValue;
			else
				mError = std::forward<Other>( other ).mError;
			return;
		}
		destroy();
		mHasValue = other.mHasValue;
		if ( mHasValue )
			std::construct_at( std::addressof( mValue ), std::forward<Other>( other ).mValue );
		else
			std::construct_at( std::addressof( mError ), std::forward<Other>( other ).mError );
	}

	union {
		T mValue;
		E mError;
	};
	bool mHasValue;
};

/** @brief Result specialization for operations that produce no value on success. */
template <typename E> class Result<void, E> {
	static_assert( !std::is_reference_v<E>, "Result does not store references" );

  public:
	using ValueType = void;
	using ErrorType = E;

	Result( OkValue<void> ) noexcept : mHasValue( true ) {}

	template <typename G, std::enable_if_t<std::is_constructible_v<E, G&&>, int> = 0>
	Result( ErrValue<G>&& err ) : mHasValue( false ) {
		std::construct_at( std::addressof( mError ), std::move( err.error ) );
	}

	template <typename G, std::enable_if_t<std::is_constructible_v<E, const G&>, int> = 0>
	Result( const ErrValue<G>& err ) : mHasValue( false ) {
		std::construct_at( std::addressof( mError ), err.error );
	}

	Result( const Result& other )
		requires( std::is_copy_constructible_v<E> )
		: mHasValue( other.mHasValue ) {
		if ( !mHasValue )
			std::construct_at( std::addressof( mError ), other.mError );
	}

	Result( Result&& other ) noexcept( std::is_nothrow_move_constructible_v<E> )
		requires( std::is_move_constructible_v<E> )
		: mHasValue( other.mHasValue ) {
		if ( !mHasValue )
			std::construct_at( std::addressof( mError ), std::move( other.mError ) );
	}

	Result& operator=( const Result& other )
		requires( std::is_copy_constructible_v<E> && std::is_copy_assignable_v<E> )
	{
		if ( this != &other )
			assign( other );
		return *this;
	}

	Result& operator=( Result&& other ) noexcept( std::is_nothrow_move_constructible_v<E> &&
												  std::is_nothrow_move_assignable_v<E> )
		requires( std::is_move_constructible_v<E> && std::is_move_assignable_v<E> )
	{
		if ( this != &other )
			assign( std::move( other ) );
		return *this;
	}

	~Result() { destroy(); }

	bool hasValue() const noexcept { return mHasValue; }

	bool hasError() const noexcept { return !mHasValue; }

	explicit operator bool() const noexcept { return mHasValue; }

	/** Validates success; provided for symmetry with the non-void Result. */
	void value() const { eeASSERTM( mHasValue, "Result::value() called on an error result" ); }

	E& error() & {
		eeASSERTM( !mHasValue, "Result::error() called on a success result" );
		return mError;
	}

	const E& error() const& {
		eeASSERTM( !mHasValue, "Result::error() called on a success result" );
		return mError;
	}

	E&& error() && {
		eeASSERTM( !mHasValue, "Result::error() called on a success result" );
		return std::move( mError );
	}

  private:
	void destroy() noexcept {
		if ( !mHasValue )
			std::destroy_at( std::addressof( mError ) );
	}

	template <typename Other> void assign( Other&& other ) {
		if ( !mHasValue && !other.mHasValue ) {
			mError = std::forward<Other>( other ).mError;
			return;
		}
		destroy();
		mHasValue = other.mHasValue;
		if ( !mHasValue )
			std::construct_at( std::addressof( mError ), std::forward<Other>( other ).mError );
	}

	union {
		E mError;
	};
	bool mHasValue;
};

} // namespace EE

#endif
