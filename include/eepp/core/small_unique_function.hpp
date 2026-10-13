#ifndef EE_CORE_SMALL_UNIQUE_FUNCTION_HPP
#define EE_CORE_SMALL_UNIQUE_FUNCTION_HPP

#include <cstddef>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>

namespace EE {

template <typename Signature, std::size_t InlineSize = 64> class SmallUniqueFunction;

/** Move-only type-erased callable with inline storage and a heap fallback.
 *
 * Unlike SmallFunction, the stored callable only needs to be move-constructible, which allows
 * closures that own move-only state (task sources, unique pointers, file handles) to be queued
 * on executors without wrapping them in shared heap holders. Callables larger than InlineSize, or
 * whose alignment or move constructor make inline relocation unsafe, are stored on the heap.
 *
 * The callable is invoked as a non-const lvalue: it may mutate its captures, and the object can
 * be invoked more than once unless the stored callable itself consumes its state.
 */
template <typename R, typename... Args, std::size_t InlineSize>
class SmallUniqueFunction<R( Args... ), InlineSize> {
  public:
	SmallUniqueFunction() noexcept = default;

	SmallUniqueFunction( std::nullptr_t ) noexcept {}

	SmallUniqueFunction( const SmallUniqueFunction& ) = delete;

	SmallUniqueFunction& operator=( const SmallUniqueFunction& ) = delete;

	SmallUniqueFunction( SmallUniqueFunction&& other ) noexcept {
		if ( other.mVTable )
			other.mVTable->move( *this, other );
	}

	template <typename Callable, typename Decayed = std::decay_t<Callable>,
			  std::enable_if_t<!std::is_same_v<Decayed, SmallUniqueFunction> &&
								   std::is_invocable_r_v<R, Decayed&, Args...>,
							   int> = 0>
	SmallUniqueFunction( Callable&& callable ) {
		emplace<Decayed>( std::forward<Callable>( callable ) );
	}

	~SmallUniqueFunction() { reset(); }

	SmallUniqueFunction& operator=( SmallUniqueFunction&& other ) noexcept {
		if ( this != &other ) {
			reset();
			if ( other.mVTable )
				other.mVTable->move( *this, other );
		}
		return *this;
	}

	template <typename Callable, typename Decayed = std::decay_t<Callable>,
			  std::enable_if_t<!std::is_same_v<Decayed, SmallUniqueFunction> &&
								   std::is_invocable_r_v<R, Decayed&, Args...>,
							   int> = 0>
	SmallUniqueFunction& operator=( Callable&& callable ) {
		reset();
		emplace<Decayed>( std::forward<Callable>( callable ) );
		return *this;
	}

	SmallUniqueFunction& operator=( std::nullptr_t ) noexcept {
		reset();
		return *this;
	}

	explicit operator bool() const noexcept { return nullptr != mVTable; }

	R operator()( Args... args ) {
		if ( !mVTable )
			throw std::bad_function_call();
		return mVTable->invoke( *this, std::forward<Args>( args )... );
	}

	void reset() noexcept {
		if ( mVTable ) {
			mVTable->destroy( *this );
			mVTable = nullptr;
		}
	}

	/** @return True when the stored callable lives in the inline buffer. */
	bool isInline() const noexcept { return mVTable && mVTable->isInline; }

	static constexpr std::size_t inlineSize() noexcept { return InlineSize; }

	/** @return True if a callable of this type would be stored without a heap allocation. */
	template <typename Callable> static constexpr bool fitsInline() noexcept {
		return FitsInline<std::decay_t<Callable>>;
	}

  private:
	static constexpr std::size_t StorageSize =
		InlineSize < sizeof( void* ) ? sizeof( void* ) : InlineSize;
	// Pointer/double alignment covers ordinary closures without inflating the object to
	// max_align_t; over-aligned callables take the heap path.
	static constexpr std::size_t StorageAlign = alignof( void* ) > alignof( double )
													? alignof( void* )
													: alignof( double );

	struct VTable {
		R ( *invoke )( SmallUniqueFunction&, Args&&... );
		void ( *move )( SmallUniqueFunction&, SmallUniqueFunction& ) noexcept;
		void ( *destroy )( SmallUniqueFunction& ) noexcept;
		bool isInline;
	};

	template <typename Callable>
	static constexpr bool FitsInline =
		sizeof( Callable ) <= StorageSize && alignof( Callable ) <= StorageAlign &&
		std::is_nothrow_move_constructible_v<Callable>;

	template <typename Callable> static Callable*& heapPointer( SmallUniqueFunction& function ) {
		return *reinterpret_cast<Callable**>( function.mStorage );
	}

	template <typename Callable> static Callable* inlinePointer( SmallUniqueFunction& function ) {
		return std::launder( reinterpret_cast<Callable*>( function.mStorage ) );
	}

	template <typename Callable, typename Value> void emplace( Value&& value ) {
		static_assert( std::is_move_constructible_v<Callable>,
					   "SmallUniqueFunction requires a move-constructible callable" );
		if constexpr ( FitsInline<Callable> ) {
			new ( mStorage ) Callable( std::forward<Value>( value ) );
			mVTable = &inlineVTable<Callable>();
		} else {
			heapPointer<Callable>( *this ) = new Callable( std::forward<Value>( value ) );
			mVTable = &heapVTable<Callable>();
		}
	}

	template <typename Callable> static const VTable& inlineVTable() {
		static const VTable table{
			[]( SmallUniqueFunction& function, Args&&... args ) -> R {
				return std::invoke( *inlinePointer<Callable>( function ),
									std::forward<Args>( args )... );
			},
			[]( SmallUniqueFunction& destination, SmallUniqueFunction& source ) noexcept {
				new ( destination.mStorage )
					Callable( std::move( *inlinePointer<Callable>( source ) ) );
				inlinePointer<Callable>( source )->~Callable();
				destination.mVTable = source.mVTable;
				source.mVTable = nullptr;
			},
			[]( SmallUniqueFunction& function ) noexcept {
				inlinePointer<Callable>( function )->~Callable();
			},
			true };
		return table;
	}

	template <typename Callable> static const VTable& heapVTable() {
		static const VTable table{
			[]( SmallUniqueFunction& function, Args&&... args ) -> R {
				return std::invoke( *heapPointer<Callable>( function ),
									std::forward<Args>( args )... );
			},
			[]( SmallUniqueFunction& destination, SmallUniqueFunction& source ) noexcept {
				heapPointer<Callable>( destination ) = heapPointer<Callable>( source );
				heapPointer<Callable>( source ) = nullptr;
				destination.mVTable = source.mVTable;
				source.mVTable = nullptr;
			},
			[]( SmallUniqueFunction& function ) noexcept {
				delete heapPointer<Callable>( function );
			},
			false };
		return table;
	}

	alignas( StorageAlign ) unsigned char mStorage[StorageSize];
	const VTable* mVTable{ nullptr };
};

} // namespace EE

#endif
