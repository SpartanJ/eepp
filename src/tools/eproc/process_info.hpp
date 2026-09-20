#ifndef EPROC_PROCESS_INFO_HPP
#define EPROC_PROCESS_INFO_HPP

#include <eepp/core/core.hpp>
#include <string>

using namespace EE;

namespace eproc {

enum class ProcessStatus : Uint8 {
	Running,
	Sleeping,
	DiskSleep,
	Zombie,
	Stopped,
	Paging,
	Ended,
	Other
};

struct ProcessInfo {
	long pid{ 0 };
	long parentPid{ 0 };
	std::string name;
	std::string username;

	// Real, effective, saved and filesystem user ids (all four from the status "Uid:" line).
	long uid{ 0 };
	long euid{ 0 };
	long suid{ 0 };
	long fsuid{ 0 };
	// True when the account's shell is a real login shell (used to tell system users apart).
	bool canLogin{ false };
	// Same check for the effective uid, which the original's User Processes filter also consults.
	bool euidCanLogin{ false };

	ProcessStatus status{ ProcessStatus::Other };

	// CPU
	int userUsage{ 0 };
	int sysUsage{ 0 };
	long userTime{ 0 };
	long sysTime{ 0 };

	// Memory (kilobytes)
	long vmSize{ 0 };
	long vmRSS{ 0 };
	// Private memory: resident pages the process does not share with anyone else. This is what
	// ksysguard6 shows in its Memory column (VmRSS - shared, from statm), and -1 means the kernel
	// did not report the shared breakdown so the display falls back to vmRSS.
	long vmURSS{ -1 };
	long vmPSS{ 0 };
	long sharedMem{ 0 };
	// True when the kernel reported the shared-memory breakdown, so vmURSS could be derived.
	bool hasSharedInfo{ false };

	// Network (bytes, -1 if not available)
	long netDownload{ -1 };
	long netUpload{ -1 };

	// GPU (percentage, -1 if not available)
	int gpuUsage{ -1 };
	long gpuMemory{ -1 };

	// Disk I/O totals (bytes, -1 if not available)
	long long ioReadBytes{ -1 };
	long long ioWriteBytes{ -1 };

	// Other
	int niceLevel{ 0 };
	int numThreads{ 0 };
	// The complete command line, reconstructed from argv. The command column uses only the
	// executable name, while this value is used by actions that need the original arguments.
	std::string commandLine;
	std::string command;
	// Controlling terminal device number, 0 when the process has none.
	long ttyNr{ 0 };
	// Friendly Linux tty name (for example, "pts/2"). Empty when there is no controlling tty.
	std::string tty;
	// PID of the process tracing (debugging) this one, 0 when it is not traced.
	long tracerPid{ 0 };
	// Process start time in clock ticks since boot (stat field 22). Together with the pid it
	// identifies one incarnation of a process, which matters because pids are recycled.
	long long startTime{ 0 };
	// Resolved icon file for this process, empty when none could be found.
	std::string iconPath;

	// Sorting helpers
	/** Private memory when known, otherwise plain RSS. */
	long getMemoryForSort() const { return vmURSS >= 0 ? vmURSS : vmRSS; }

	int getCpuForSort() const { return userUsage + sysUsage; }

	// Formatted display strings
	std::string formatMemory() const;
	std::string formatSharedMem() const;
	std::string formatCpu() const;
	std::string formatGpuUsage() const;
	std::string formatGpuMemory() const;
	std::string formatDownload() const;
	std::string formatUpload() const;
	std::string formatCpuTime( long ticksPerSecond ) const;
	std::string formatRelativeStartTime( double uptimeSeconds, long ticksPerSecond ) const;
};

/** Formats a KiB amount using the largest fitting binary unit (K/M/G/T/P), matching the short
 *  form used by the original process table (e.g. "0 K", "853.8 M", "4.2 G").
 *  Returns an empty string for negative input. */
std::string formatKiB( long kib );

/** Formats a KiB amount using IEC units (KiB/MiB/GiB/TiB/PiB), used by the status bar.
 *  Returns an empty string for negative input. */
std::string formatKiBIEC( long kib );

/** Formats a byte-per-second amount using the largest fitting binary unit (B/K/M/G).
 *  Returns an empty string for zero or negative input. */
std::string formatBytesPerSecond( long bytes );

/** Formats a byte total using binary units. Returns an empty string for negative input. */
std::string formatBytes( long long bytes );

} // namespace eproc

#endif // EPROC_PROCESS_INFO_HPP
