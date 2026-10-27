#ifndef EEFM_DEVICEMONITOR_HPP
#define EEFM_DEVICEMONITOR_HPP

#include <atomic>
#include <eepp/config.hpp>
#include <eepp/core/cancellationtoken.hpp>
#include <eepp/core/noncopyable.hpp>
#include <memory>
#include <string>
#include <vector>

namespace eefm {

using namespace EE;

struct Place;

/** Native mount notifications and device ejection. Without native notifications, changed()
 * requests a background scan at the controller's polling interval. */
class DeviceMonitor : NonCopyable {
  public:
	DeviceMonitor();

	~DeviceMonitor();

	/** Consumes pending mount, unmount or volume rename notifications. Call on a worker;
	 * platform checks must never run on the UI thread. Serialize calls for this monitor. */
	bool changed();

	/** Unregisters push notifications. Passive polling resources remain valid until destruction
	 * so an in-flight changed() may finish. Join workers before destroying the monitor. */
	void stop();

	/** Updates native ejection capabilities in one snapshot. Call on a worker. The token
	 * interrupts in-flight UDisks requests on Linux. */
	static void updateCapabilities( std::vector<Place>& devices,
									const CancellationToken& token = {} );

	/** Unmounts and ejects normally (never forces). Call on a worker. Empty on success,
	 * otherwise the native error description. The token interrupts in-flight UDisks requests
	 * on Linux; other platforms check cancellation before starting native work. */
	static std::string eject( const std::string& path, const CancellationToken& token = {} );

  private:
#if EE_PLATFORM == EE_PLATFORM_MACOS
	static bool canEject( const std::string& path );

	// Notification blocks own the flag too, so an in-flight delivery cannot outlive its state.
	std::shared_ptr<std::atomic<bool>> mChanged;
	void* mObservers[3]{};
#elif EE_PLATFORM == EE_PLATFORM_LINUX
	// Immutable while worker checks may be running; closed only after the owner joins them.
	const int mMountsFd{ -1 };
#endif
};

} // namespace eefm

#endif
