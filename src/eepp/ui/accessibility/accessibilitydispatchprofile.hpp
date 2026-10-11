#ifndef EE_UI_ACCESSIBILITY_ACCESSIBILITYDISPATCHPROFILE_HPP
#define EE_UI_ACCESSIBILITY_ACCESSIBILITYDISPATCHPROFILE_HPP

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <eepp/config.hpp>
#include <memory>

namespace EE { namespace UI {

/** Opt-in UI-thread dispatch measurements, distinct from client round-trip latency. The
 * backend owns this only when EEPP_ACCESSIBILITY_PROFILE is set; dormant updates never read
 * the clock or allocate profiling storage. Reports logarithmically, and on a new maximum,
 * rather than adding console I/O to every native request. */
class AccessibilityDispatchProfile {
  public:
	using Clock = std::chrono::steady_clock;

	explicit AccessibilityDispatchProfile( const char* backend ) : mBackend( backend ) {}

	~AccessibilityDispatchProfile() { report(); }

	static std::unique_ptr<AccessibilityDispatchProfile> create( const char* backend ) {
		return std::getenv( "EEPP_ACCESSIBILITY_PROFILE" )
				   ? std::make_unique<AccessibilityDispatchProfile>( backend )
				   : nullptr;
	}

	void batch( size_t messages, Clock::time_point start, double maximumMessage, bool backlog ) {
		const double elapsed = microseconds( start );
		const bool maximum = elapsed > mMaximumBatch || maximumMessage > mMaximumMessage;
		++mBatches;
		mMessages += messages;
		mTotal += elapsed;
		mMaximumBatch = std::max( mMaximumBatch, elapsed );
		mMaximumMessage = std::max( mMaximumMessage, maximumMessage );
		mBacklogs += backlog;
		if ( maximum || ( mBatches & ( mBatches - 1 ) ) == 0 )
			report();
	}

	static double microseconds( Clock::time_point start ) {
		return std::chrono::duration<double, std::micro>( Clock::now() - start ).count();
	}

  private:
	const char* mBackend;
	size_t mBatches{};
	size_t mMessages{};
	size_t mBacklogs{};
	double mTotal{};
	double mMaximumBatch{};
	double mMaximumMessage{};

	void report() const {
		if ( !mBatches )
			return;
		std::fprintf( stderr,
					  "EEPP_A11Y_PROFILE backend=%s batches=%zu messages=%zu backlog_batches=%zu "
					  "total_ui_us=%.3f max_batch_ui_us=%.3f max_message_ui_us=%.3f\n",
					  mBackend, mBatches, mMessages, mBacklogs, mTotal, mMaximumBatch,
					  mMaximumMessage );
	}
};

}} // namespace EE::UI

#endif
