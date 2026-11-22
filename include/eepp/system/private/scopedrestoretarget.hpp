#ifndef EE_SYSTEM_PRIVATE_SCOPEDRESTORETARGET_HPP
#define EE_SYSTEM_PRIVATE_SCOPEDRESTORETARGET_HPP

namespace EE { namespace System { namespace Private {

// Library-internal only. Keep instantiations in eepp so each thread uses the same list in DLL
// builds. Tracks a previous context binding on its owning thread. Destruction can invalidate saved
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

}}} // namespace EE::System::Private

#endif // EE_SYSTEM_PRIVATE_SCOPEDRESTORETARGET_HPP
