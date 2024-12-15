#include "debuggerclientdap.hpp"

namespace ecode {

DebuggerClientDap::DebuggerClientDap( std::unique_ptr<Bus>&& bus ) : mBus( std::move( bus ) ) {}

bool DebuggerClientDap::hasBreakpoint( const std::string& path, size_t line ) {
	return false;
}

bool DebuggerClientDap::addBreakpoint( const std::string& path, size_t line ) {
	return false;
}

bool DebuggerClientDap::removeBreakpoint( const std::string& path, size_t line ) {
	return false;
}

bool DebuggerClientDap::start() {
	return mBus->start();
}

bool DebuggerClientDap::attach() {
	return false;
}

bool DebuggerClientDap::started() const {
	return false;
}

bool DebuggerClientDap::cont() {
	return false;
}

bool DebuggerClientDap::stepInto() {
	return false;
}

bool DebuggerClientDap::stepOver() {
	return false;
}

bool DebuggerClientDap::stepOut() {
	return false;
}

bool DebuggerClientDap::halt() {
	return false;
}

bool DebuggerClientDap::terminate() {
	return false;
}

bool DebuggerClientDap::stopped() {
	return false;
}

bool DebuggerClientDap::completed() {
	return false;
}

} // namespace ecode
