#ifndef EPROC_PROCESS_NETWORK_MONITOR_HPP
#define EPROC_PROCESS_NETWORK_MONITOR_HPP

#include "../../process_info.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string_view>
#include <thread>
#include <vector>

namespace eproc {

/** Collects per-process network traffic on Linux.
 *
 * Linux does not expose network byte counters per process. The implementation therefore captures
 * TCP/UDP packet headers and joins their local endpoints with socket inodes from procfs. Packet
 * capture runs continuously in its own thread; the process collector only refreshes the procfs
 * ownership map and consumes the counters at snapshot time.
 */
class ProcessNetworkMonitor {
  public:
	ProcessNetworkMonitor();
	~ProcessNetworkMonitor();

	/** Refreshes the socket endpoint to PID map. This is intended for the collection worker. */
	void refreshMapping();

	/** Copies the current byte rates into @p processes and starts a new measurement interval. */
	void applyRates( std::vector<ProcessInfo>& processes );

  private:
	struct Endpoint {
		Uint8 family{ 0 };
		Uint16 port{ 0 };
		std::array<Uint8, 16> address{};

		bool operator==( const Endpoint& other ) const {
			return family == other.family && port == other.port && address == other.address;
		}
	};

	struct EndpointHash {
		std::size_t operator()( const Endpoint& endpoint ) const noexcept;
	};

	struct Traffic {
		Uint64 download{ 0 };
		Uint64 upload{ 0 };
	};

	using EndpointMap = UnorderedMap<Endpoint, long, EndpointHash>;
	using TrafficMap = UnorderedMap<long, Traffic>;

	void captureLoop();
	void processPacket( int dataLink, const Uint8* data, size_t length, size_t wireLength );

	static bool parsePacket( int dataLink, const Uint8* data, size_t length, Endpoint& source,
							 Endpoint& destination );
	static bool parseProcEndpoint( std::string_view token, Endpoint& endpoint );
	static bool parseSocketLine( std::string_view line, Endpoint& endpoint, Uint64& inode );
	static bool parseUnsigned( std::string_view token, Uint64& value, int base );

	static void collectSocketOwners( UnorderedMap<Uint64, long>& owners );
	static void collectSocketTable( const char* path, Uint8 family,
									const UnorderedMap<Uint64, long>& owners,
									EndpointMap& endpoints );

	long findPid( const Endpoint& endpoint ) const;
	static long rateFor( Uint64 bytes, std::chrono::steady_clock::duration elapsed );

	std::atomic<bool> mRunning{ true };
	std::atomic<bool> mAvailable{ false };
	std::thread mCaptureThread;
	int mDataLink{ -1 };

	mutable std::mutex mMutex;
	EndpointMap mEndpoints;
	TrafficMap mTraffic;
	std::chrono::steady_clock::time_point mLastSnapshot;
};

} // namespace eproc

#endif // EPROC_PROCESS_NETWORK_MONITOR_HPP
