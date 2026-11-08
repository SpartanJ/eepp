#ifndef EE_NETWORK_LOCALSOCKET_HPP
#define EE_NETWORK_LOCALSOCKET_HPP
#include <eepp/network/socket.hpp>
#include <eepp/system/time.hpp>
#include <string>

namespace EE::Network {

class LocalListener;

/** Filesystem local byte stream (AF_UNIX). Native POSIX and Windows 10 1803+.
 * No IP addresses, TCP options or TLS methods apply. Emscripten returns Error. */
class EE_API LocalSocket : public Socket {
  public:
	LocalSocket();

	Status connect( const std::string& path, System::Time timeout = System::Seconds( 5 ) );

	/** Check a nonblocking connect after write readiness. Done confirms a connected peer,
	 * NotReady means still pending. On failure disconnect before starting another connect. */
	Status finishConnect() const;

	/** Interrupt pending I/O without releasing the handle. Safe to call while another thread is
	 * blocked in send/receive; join that thread before disconnecting or destroying this socket. */
	void shutdown();

	void disconnect();

	Status send( const void* data, size_t size );

	Status send( const void* data, size_t size, size_t& sent );

	Status receive( void* data, size_t size, size_t& received );

  private:
	friend class LocalListener;
};

} // namespace EE::Network

#endif
