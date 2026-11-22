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

}} // namespace EE::System

#endif // EE_SYSTEM_SCOPEDOP_HPP
