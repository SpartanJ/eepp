#ifndef EE_VFS_WATCHERS_EARLYWATCHEVENTS_HPP
#define EE_VFS_WATCHERS_EARLYWATCHEVENTS_HPP

#include <eepp/vfs/watch.hpp>
#include <efsw/efsw.hpp>
#include <string>

namespace EE { namespace VFS { namespace detail {

/** Records loss of incremental ordering before a native watch registration is published. */
class EarlyWatchEvents {
  public:
	void push( const std::string&, const std::string&, efsw::Action, const std::string& ) {
		invalidate();
	}

	void invalidate() { mPending = true; }

	/** Live events can arrive as soon as registration is published. Invalidate instead of
	 * replaying older events after them; no early payloads or callbacks need to be retained. */
	void replay( WatchSink& sink ) const {
		if ( mPending )
			sink.rescanRequired();
	}

  private:
	bool mPending{ false };
};

}}} // namespace EE::VFS::detail

#endif
