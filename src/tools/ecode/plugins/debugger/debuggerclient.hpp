
#include <cstddef>
#include <string>

namespace ecode {

class DebuggerClient {
  public:
	virtual bool hasBreakpoint( const std::string& path, size_t line ) = 0;

	virtual bool addBreakpoint( const std::string& path, size_t line ) = 0;

	virtual bool removeBreakpoint( const std::string& path, size_t line ) = 0;

	virtual bool start() = 0;

	virtual bool attach() = 0;

	virtual bool started() const = 0;

	virtual bool cont() = 0;

	virtual bool stepInto() = 0;

	virtual bool stepOver() = 0;

	virtual bool stepOut() = 0;

	virtual bool halt() = 0;

	virtual bool terminate() = 0;

	virtual bool stopped() = 0;

	virtual bool completed() = 0;
};

} // namespace ecode
