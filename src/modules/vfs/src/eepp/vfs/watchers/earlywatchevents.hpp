#ifndef EE_VFS_WATCHERS_EARLYWATCHEVENTS_HPP
#define EE_VFS_WATCHERS_EARLYWATCHEVENTS_HPP

#include <eepp/vfs/watch.hpp>
#include <efsw/efsw.hpp>
#include <string>
#include <vector>

namespace EE { namespace VFS { namespace detail {

/** Callbacks received before efsw returns the id that the adapter must register. */
class EarlyWatchEvents {
  public:
	static constexpr size_t MaxEvents = 4096;

	struct Event {
		std::string dir;
		std::string filename;
		std::string oldFilename;
		efsw::Action action;
	};

	void push( const std::string& dir, const std::string& filename, efsw::Action action,
			   const std::string& oldFilename ) {
		if ( mOverflow )
			return;
		if ( mEvents.size() == MaxEvents ) {
			invalidate();
			return;
		}
		mEvents.push_back( { dir, filename, oldFilename, action } );
	}

	void invalidate() {
		mEvents.clear();
		mOverflow = true;
	}

	/** An invalidation replaces the truncated stream; it must never look complete. */
	template <typename Dispatch> void replay( WatchSink& sink, Dispatch&& dispatch ) const {
		if ( mOverflow ) {
			sink.rescanRequired();
		} else {
			for ( const auto& event : mEvents )
				dispatch( event );
		}
	}

  private:
	std::vector<Event> mEvents;
	bool mOverflow{ false };
};

}}} // namespace EE::VFS::detail

#endif
