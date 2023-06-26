#ifndef EE_SYSTEM_SCOPEDOP_HPP
#define EE_SYSTEM_SCOPEDOP_HPP

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

}} // namespace EE::System

#endif // EE_SYSTEM_SCOPEDOP_HPP
