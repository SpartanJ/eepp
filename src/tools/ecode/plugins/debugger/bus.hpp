#pragma once

#include <functional>

namespace ecode {

class Bus {
  public:
	enum class State { None, Running, Closed };

	typedef std::function<void( const char* bytes, size_t n )> ReadFn;

	State state() const;

	virtual bool start() = 0;

	virtual bool close() = 0;

	virtual void startAsyncRead( ReadFn readFn ) = 0;

	virtual size_t write( const char* buffer, const size_t& size ) = 0;

	virtual bool hasProcess() { return false; }

	virtual ~Bus() {}

  protected:
	void setState( State state );

	void onStateChanged( State state );

	State mState{ State::None };
};

} // namespace ecode
