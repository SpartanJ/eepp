#ifndef ECODE_SCOPEDOP_HPP
#define ECODE_SCOPEDOP_HPP

#include <functional>

class ScopedOp {
  public:
	ScopedOp( std::function<void()> startOp,
			  std::function<void()> endOp = std::function<void()>() ) :
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

#endif // SCOPEDOP_HPP
