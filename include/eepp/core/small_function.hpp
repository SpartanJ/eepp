#ifndef EE_CORE_SMALL_FUNCTION_HPP
#define EE_CORE_SMALL_FUNCTION_HPP

#include <cstddef>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>

namespace EE {

/** Copyable type-erased void callable with inline storage and a heap fallback.
 *
 * This intentionally implements only the signature used by the scene scheduler. Ordinary lambdas
 * stay inside the object; unusually large or over-aligned callables retain std::function-like
 * fallback behavior.
 */
template <std::size_t InlineSize = 64> class SmallFunction {
  public:
	SmallFunction() noexcept = default;
	SmallFunction( std::nullptr_t ) noexcept {}

	SmallFunction( const SmallFunction& other ) {
		if ( other.mVTable )
			other.mVTable->copy( *this, other );
	}

	SmallFunction( SmallFunction&& other ) {
		if ( other.mVTable )
			other.mVTable->move( *this, other );
	}

	template <typename Callable, typename Decayed = std::decay_t<Callable>,
			  std::enable_if_t<!std::is_same_v<Decayed, SmallFunction> &&
								   std::is_invocable_r_v<void, Decayed&>,
							   int> = 0>
	SmallFunction( Callable&& callable ) {
		emplace<Decayed>( std::forward<Callable>( callable ) );
	}

	~SmallFunction() { reset(); }

	SmallFunction& operator=( const SmallFunction& other ) {
		if ( this != &other ) {
			reset();
			if ( other.mVTable )
				other.mVTable->copy( *this, other );
		}
		return *this;
	}

	SmallFunction& operator=( SmallFunction&& other ) {
		if ( this != &other ) {
			reset();
			if ( other.mVTable )
				other.mVTable->move( *this, other );
		}
		return *this;
	}

	template <typename Callable, typename Decayed = std::decay_t<Callable>,
			  std::enable_if_t<!std::is_same_v<Decayed, SmallFunction> &&
								   std::is_invocable_r_v<void, Decayed&>,
							   int> = 0>
	SmallFunction& operator=( Callable&& callable ) {
		reset();
		emplace<Decayed>( std::forward<Callable>( callable ) );
		return *this;
	}

	SmallFunction& operator=( std::nullptr_t ) noexcept {
		reset();
		return *this;
	}

	explicit operator bool() const noexcept { return nullptr != mVTable; }

	void operator()() const {
		if ( !mVTable )
			throw std::bad_function_call();
		mVTable->invoke( const_cast<SmallFunction&>( *this ) );
	}

	void reset() noexcept {
		if ( mVTable ) {
			mVTable->destroy( *this );
			mVTable = nullptr;
		}
	}

	static constexpr std::size_t inlineSize() noexcept { return InlineSize; }

  private:
	static constexpr std::size_t StorageSize =
		InlineSize < sizeof( void* ) ? sizeof( void* ) : InlineSize;
	using Storage = std::aligned_storage_t<StorageSize, alignof( std::max_align_t )>;

	struct VTable {
		void ( *invoke )( SmallFunction& );
		void ( *copy )( SmallFunction&, const SmallFunction& );
		void ( *move )( SmallFunction&, SmallFunction& );
		void ( *destroy )( SmallFunction& ) noexcept;
	};

	template <typename Callable>
	static constexpr bool FitsInline =
		sizeof( Callable ) <= StorageSize && alignof( Callable ) <= alignof( Storage );

	template <typename Callable> static Callable*& heapPointer( SmallFunction& function ) {
		return *reinterpret_cast<Callable**>( &function.mStorage );
	}

	template <typename Callable> static Callable* inlinePointer( SmallFunction& function ) {
		return reinterpret_cast<Callable*>( &function.mStorage );
	}

	template <typename Callable, typename Value> void emplace( Value&& value ) {
		static_assert( std::is_copy_constructible_v<Callable>,
					   "SmallFunction requires a copy-constructible callable" );
		if constexpr ( FitsInline<Callable> ) {
			new ( &mStorage ) Callable( std::forward<Value>( value ) );
			mVTable = &inlineVTable<Callable>();
		} else {
			heapPointer<Callable>( *this ) = new Callable( std::forward<Value>( value ) );
			mVTable = &heapVTable<Callable>();
		}
	}

	template <typename Callable> static const VTable& inlineVTable() {
		static const VTable table{
			[]( SmallFunction& function ) { ( *inlinePointer<Callable>( function ) )(); },
			[]( SmallFunction& destination, const SmallFunction& source ) {
				new ( &destination.mStorage )
					Callable( *inlinePointer<Callable>( const_cast<SmallFunction&>( source ) ) );
				destination.mVTable = &inlineVTable<Callable>();
			},
			[]( SmallFunction& destination, SmallFunction& source ) {
				new ( &destination.mStorage )
					Callable( std::move( *inlinePointer<Callable>( source ) ) );
				inlinePointer<Callable>( source )->~Callable();
				destination.mVTable = &inlineVTable<Callable>();
				source.mVTable = nullptr;
			},
			[]( SmallFunction& function ) noexcept {
				inlinePointer<Callable>( function )->~Callable();
			} };
		return table;
	}

	template <typename Callable> static const VTable& heapVTable() {
		static const VTable table{
			[]( SmallFunction& function ) { ( *heapPointer<Callable>( function ) )(); },
			[]( SmallFunction& destination, const SmallFunction& source ) {
				heapPointer<Callable>( destination ) =
					new Callable( *heapPointer<Callable>( const_cast<SmallFunction&>( source ) ) );
				destination.mVTable = &heapVTable<Callable>();
			},
			[]( SmallFunction& destination, SmallFunction& source ) {
				heapPointer<Callable>( destination ) = heapPointer<Callable>( source );
				heapPointer<Callable>( source ) = nullptr;
				destination.mVTable = &heapVTable<Callable>();
				source.mVTable = nullptr;
			},
			[]( SmallFunction& function ) noexcept { delete heapPointer<Callable>( function ); } };
		return table;
	}

	Storage mStorage;
	const VTable* mVTable{ nullptr };
};

} // namespace EE

#endif
