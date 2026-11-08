#ifndef EE_NETWORK_LOCALLISTENER_HPP
#define EE_NETWORK_LOCALLISTENER_HPP
#include <eepp/network/localsocket.hpp>

namespace EE::Network {

/** Owns the bound endpoint until close/destruction. Never unlinks an existing endpoint on listen;
 * callers should put listeners in a private directory. The path must fit the native sun_path. */
class EE_API LocalListener : public Socket {
  public:
	LocalListener();

	virtual ~LocalListener();

	Status listen( const std::string& path, int backlog = 16 );

	Status accept( LocalSocket& socket );

	void close();

  private:
	std::string mPath;
	Uint64 mDevice{ 0 };
	Uint64 mInode{ 0 };
};

} // namespace EE::Network

#endif
