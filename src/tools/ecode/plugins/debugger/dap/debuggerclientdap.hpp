#pragma once

#include "../bus.hpp"
#include "../debuggerclient.hpp"

namespace ecode {

class DebuggerClientDap : public DebuggerClient {
  public:
	DebuggerClientDap( std::unique_ptr<Bus>&& bus );

	bool hasBreakpoint( const std::string& path, size_t line );

	bool addBreakpoint( const std::string& path, size_t line );

	bool removeBreakpoint( const std::string& path, size_t line );

	bool start();

	bool attach();

	bool started() const;

	bool cont();

	bool stepInto();

	bool stepOver();

	bool stepOut();

	bool halt();

	bool terminate();

	bool stopped();

	bool completed();

  protected:
	std::unique_ptr<Bus> mBus;
};

} // namespace ecode
