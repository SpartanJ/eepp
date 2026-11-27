#ifndef EE_GLOBALSHORTCUTS_SHORTCUTBACKEND_HPP
#define EE_GLOBALSHORTCUTS_SHORTCUTBACKEND_HPP

#include <eepp/globalshortcuts/service.hpp>

namespace EE::System::GlobalShortcuts::Private {

/** Native backend calls run on the worker or on the owning application thread. wake() can be
 * called from any thread for worker backends. */
class ShortcutBackend {
  public:
	struct Binding {
		BoundShortcut shortcut;
		/** Native identity for this registration. A replacement gets a new identity. */
		Uint64 incarnation{ 0 };
	};

	class Client {
	  public:
		virtual void complete( Uint64 request, Result<std::vector<Binding>, Error> result ) = 0;

		virtual void bindingsChanged( std::vector<Binding> effective,
									  std::optional<Error> issue ) = 0;

		virtual void activate( Uint64 incarnation, std::optional<std::string> token ) = 0;

		virtual void capabilitiesChanged( Capabilities capabilities ) = 0;
		virtual void scheduleNativeWork() = 0;

	  protected:
		~Client() = default;
	};

	virtual ~ShortcutBackend() = default;

	virtual Backend type() const = 0;

	virtual bool usesApplicationThread() const { return false; }

	/** Processes coalesced work requested by application-thread native callbacks. */
	virtual void processEvents( Client& ) {}

	/** Discover the backend and open its worker-owned resources. May be retried after failure. */
	virtual Result<Capabilities, Error> open() = 0;

	/** Reconcile a validated declaration sorted by shortcut ID. */
	virtual void reconcile( Client& client, Uint64 request, std::vector<Shortcut> shortcuts,
							bool retry ) = 0;

	/** Cancel provisional work and report any change to retained registrations. */
	virtual void cancel( Client& client, Uint64 request ) = 0;

	virtual void configure( std::string, TaskResolver<Result<void, Error>> resolve ) {
		resolve( Err(
			Error{ ErrorCode::Unsupported, "This shortcut backend has no settings dialog" } ) );
	}

	/** Wait for a durable wake or native events and handle a bounded batch of work. The command
	 * wake mechanism also works before open(), after open() fails and while the backend is idle. */
	virtual void wait( Client& client ) = 0;

	/** Thread-safe, nonblocking, and durable across the decision to enter wait(). */
	virtual void wake() noexcept = 0;

	/** Quiesces native resources. Safe to call before open() and after failed initialization. */
	virtual Result<void, Error> close() = 0;
};

} // namespace EE::System::GlobalShortcuts::Private

#endif
