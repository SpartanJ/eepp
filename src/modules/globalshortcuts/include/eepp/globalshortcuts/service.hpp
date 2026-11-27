#ifndef EE_GLOBALSHORTCUTS_SERVICE_HPP
#define EE_GLOBALSHORTCUTS_SERVICE_HPP

#include <eepp/config.hpp>
#include <eepp/core/cancellationtoken.hpp>
#include <eepp/core/result.hpp>
#include <eepp/core/task.hpp>
#include <eepp/window/keycodes.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// The module and core are separate libraries and can use different link modes.
#if defined( EE_GLOBALSHORTCUTS_STATIC )
#define EE_GLOBALSHORTCUTS_API
#elif defined( EE_GLOBALSHORTCUTS_EXPORTS )
#define EE_GLOBALSHORTCUTS_API EE_API_EXPORT
#else
#define EE_GLOBALSHORTCUTS_API EE_API_IMPORT
#endif

namespace EE::System::GlobalShortcuts {

enum class Backend : Uint8 { None, Windows, X11, WaylandPortal, MacOS, Haiku };

enum class State : Uint8 { Idle, Registering, Ready, Degraded, Unavailable, Stopping, Closed };

enum class ErrorCode : Uint8 {
	Unsupported,
	BackendUnavailable,
	InvalidShortcut,
	Cancelled,
	Superseded,
	Conflict,
	PermissionDenied,
	Disconnected,
	NativeFailure,
	Timeout,
	ShuttingDown,
	Busy
};

struct Error {
	ErrorCode code;
	std::string message;
	std::optional<int> nativeCode{};
	std::string nativeName{};
};

/** Logical letter, digit, F1–F12, navigation key, or common editing key. A backend may support
 * a subset. Left and right modifier bits normalize to one group per modifier. */
struct Trigger {
	Window::Keycode key{ Window::KEY_UNKNOWN };
	Uint32 modifiers{ Window::KEYMOD_NONE };

	bool operator==( const Trigger& ) const = default;
};

/** Stable UTF-8 command ID, app-localized description, and optional preferred trigger. */
struct Shortcut {
	std::string id;
	std::string description;
	std::optional<Trigger> preferred;

	bool operator==( const Shortcut& ) const = default;
};

enum class BindingStatus : Uint8 { Pending, Bound, Unbound, Unknown };

struct BoundShortcut {
	std::string id;
	std::string triggerDescription;
	std::optional<Trigger> effectiveTrigger;
	std::optional<Error> error;
	BindingStatus status{ BindingStatus::Unbound };
};

struct Capabilities {
	bool canRegisterShortcuts{ false };
	bool canOmitPreferredTrigger{ false };
	bool desktopWide{ false };
	bool canConfigureShortcuts{ false };
};

struct Options {
	/** Installed desktop entry ID for an unsandboxed Wayland portal client. */
	std::string appId;
};

struct ServiceSnapshot {
	std::vector<Shortcut> desired;
	std::vector<BoundShortcut> effective;
	std::optional<Error> error;
	/** Most recent unsolicited backend issue, retained across later snapshot revisions. */
	std::optional<Error> lastBackendIssue;
	Uint64 revision{ 0 };
	Uint64 generation{ 0 };
	Uint64 droppedActivations{ 0 };
	Capabilities capabilities;
	State state{ State::Idle };
	Backend backend{ Backend::None };
};

struct Activation {
	std::string id;
	std::optional<std::string> activationToken;
};

using ActivationCallback = std::function<void( const Activation& )>;
using StateChangedCallback = std::function<void( const ServiceSnapshot& )>;
using RegistrationTask = Task<Result<ServiceSnapshot, Error>>;
using OperationTask = Task<Result<void, Error>>;

namespace Private {

class ServiceTestAccess;
class ShortcutBackend;

} // namespace Private

/** Application-thread service for system-wide shortcuts.
 *
 * The executor must accept worker submissions without blocking and defer execution to the
 * owning application thread, even for submissions from that thread. macOS uses the main
 * application thread for native registration and event handling. Keep the executor and its
 * dispatcher alive through close and any pending continuations. Destruction can join a worker
 * or perform native cleanup on the owning application thread.
 */
class EE_GLOBALSHORTCUTS_API Service {
  public:
	explicit Service( Executor& applicationExecutor );

	Service( Executor& applicationExecutor, Options options );

	~Service();

	Service( const Service& ) = delete;

	Service& operator=( const Service& ) = delete;

	void setActivationCallback( ActivationCallback callback );

	/** Callback observes the latest snapshot. Intermediate revisions may coalesce. */
	void setStateChangedCallback( StateChangedCallback callback );

	/** Replace the full declaration. IDs determine the order of the snapshot. A cancelled
	 * accepted request keeps its desired snapshot, and a superseded request completes with
	 * Superseded. An identical declaration returns the cached result, or Busy while pending.
	 * A completed snapshot can contain partial grants. */
	RegistrationTask setShortcuts( std::vector<Shortcut> shortcuts, CancellationToken cancel = {} );

	RegistrationTask clear();

	/** Reconcile the current declaration explicitly, including after cancellation or failure. */
	RegistrationTask retry( CancellationToken cancel = {} );

	/** Ask the portal to open shortcut settings. Completes on acknowledgement. */
	OperationTask configureShortcuts( std::string activationToken = {} );

	/** Suppresses queued callbacks immediately. Completes after native cleanup and dispatches
	 * initiated by the service. Request close before executor shutdown and keep pumping it. */
	OperationTask closeAsync();

	ServiceSnapshot snapshot() const;

  private:
	friend class Private::ServiceTestAccess;

	Service( Executor& executor, std::unique_ptr<Private::ShortcutBackend> backend );

	struct Impl;
	std::unique_ptr<Impl> mImpl;
};

} // namespace EE::System::GlobalShortcuts

#endif
