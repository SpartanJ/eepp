#ifndef EE_SYSTEM_SCOPEDOP_HPP
#define EE_SYSTEM_SCOPEDOP_HPP

#include <atomic>
#include <functional>

namespace EE { namespace System {

class ScopedOp {
  public:
	explicit ScopedOp( std::function<void()> startOp, std::function<void()> endOp = nullptr ) :
		mEndOp( endOp ) {
		if ( startOp )
			startOp();
	}

	~ScopedOp() {
		if ( mEndOp )
			mEndOp();
	}

  private:
	std::function<void()> mEndOp;
};

class ScopedOpOptional {
  public:
	explicit ScopedOpOptional( bool cond, std::function<void()> startOp,
							   std::function<void()> endOp = nullptr ) :
		cond( cond ), mEndOp( endOp ) {
		if ( cond && startOp )
			startOp();
	}

	~ScopedOpOptional() {
		if ( cond && mEndOp )
			mEndOp();
	}

  private:
	bool cond;
	std::function<void()> mEndOp;
};

class BoolScopedOp {
  public:
	explicit BoolScopedOp( bool& boolRef ) : boolRef( boolRef ) {}

	explicit BoolScopedOp( bool& boolRef, bool initialVal ) : boolRef( boolRef ) {
		boolRef = initialVal;
	}

	~BoolScopedOp() { boolRef = !boolRef; }

  private:
	bool& boolRef;
};

class AtomicBoolScopedOp {
  public:
	explicit AtomicBoolScopedOp( std::atomic<bool>& boolRef ) : boolRef( boolRef ) {}

	explicit AtomicBoolScopedOp( std::atomic<bool>& boolRef, bool initialVal ) :
		boolRef( boolRef ) {
		boolRef = initialVal;
	}

	~AtomicBoolScopedOp() { boolRef = !boolRef; }

  private:
	std::atomic<bool>& boolRef;
};

class BoolScopedOpOptional {
  public:
	explicit BoolScopedOpOptional( bool cond, bool& boolRef ) : cond( cond ), boolRef( boolRef ) {}

	explicit BoolScopedOpOptional( bool cond, bool& boolRef, bool initialVal ) :
		cond( cond ), boolRef( boolRef ) {
		if ( cond )
			boolRef = initialVal;
	}

	~BoolScopedOpOptional() {
		if ( cond )
			boolRef = !boolRef;
	}

  private:
	bool cond;
	bool& boolRef;
};

namespace Private {

// Tracks a previous context binding on its owning thread. Destruction can invalidate saved
// targets without allocating or checking an object registry at every scope exit.
template <typename T> class ScopedRestoreTarget {
  public:
	explicit ScopedRestoreTarget( T* target ) : mTarget( target ), mNext( sHead ) { sHead = this; }

	~ScopedRestoreTarget() {
		if ( mState != State::Moved )
			findIncomingLink() = mNext;
	}

	ScopedRestoreTarget( const ScopedRestoreTarget& ) = delete;
	ScopedRestoreTarget& operator=( const ScopedRestoreTarget& ) = delete;

	ScopedRestoreTarget( ScopedRestoreTarget&& other ) noexcept :
		mTarget( other.mTarget ), mNext( other.mNext ), mState( other.mState ) {
		if ( mState != State::Moved )
			other.findIncomingLink() = this;
		other.mState = State::Moved;
	}

	bool isValid() const { return mState == State::Valid; }

	T* get() const { return mTarget; }

	void invalidate() {
		if ( mState == State::Valid ) {
			mTarget = nullptr;
			mState = State::Invalid;
		}
	}

	static void invalidate( T* target ) {
		for ( auto* entry = sHead; entry; entry = entry->mNext ) {
			if ( entry->mTarget == target )
				entry->invalidate();
		}
	}

	static void invalidateAll() {
		for ( auto* entry = sHead; entry; entry = entry->mNext )
			entry->invalidate();
	}

  private:
	enum class State : unsigned char { Valid, Invalid, Moved };

	inline static thread_local ScopedRestoreTarget* sHead = nullptr;
	T* mTarget;
	ScopedRestoreTarget* mNext;
	State mState{ State::Valid };

	ScopedRestoreTarget*& findIncomingLink() {
		if ( sHead == this )
			return sHead;
		auto* newer = sHead;
		while ( newer->mNext != this )
			newer = newer->mNext;
		return newer->mNext;
	}
};

} // namespace Private

}} // namespace EE::System

#endif // EE_SYSTEM_SCOPEDOP_HPP
