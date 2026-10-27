#include <eepp/config.hpp>

#if EE_PLATFORM != EE_PLATFORM_MACOS && EE_PLATFORM != EE_PLATFORM_LINUX

#include "devicemonitor.hpp"

namespace eefm {

DeviceMonitor::DeviceMonitor() = default;

DeviceMonitor::~DeviceMonitor() = default;

bool DeviceMonitor::changed() {
	return true;
}

void DeviceMonitor::stop() {}

void DeviceMonitor::updateCapabilities( std::vector<Place>&, const CancellationToken& ) {}

std::string DeviceMonitor::eject( const std::string&, const CancellationToken& ) {
	return "Device ejection is not supported on this platform.";
}

} // namespace eefm

#endif
