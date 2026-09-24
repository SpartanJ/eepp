#ifndef EPROC_PROCESS_INFO_HPP
#define EPROC_PROCESS_INFO_HPP

#include <eepp/core/core.hpp>
#include <eepp/graphics/drawable.hpp>
#include <string>

using namespace EE;
using namespace EE::Graphics;

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
	Int64 pid{ 0 };
	Int64 parentPid{ 0 };
	std::string name;
	std::string username;

	// Real, effective, saved and filesystem user ids (all four from the status "Uid:" line).
	Int64 uid{ -1 };
	Int64 euid{ -1 };
	Int64 suid{ -1 };
	Int64 fsuid{ -1 };
	// True when the account's shell is a real login shell (used to tell system users apart).
	bool canLogin{ false };
	// Same check for the effective uid, which the original's User Processes filter also consults.
	bool euidCanLogin{ false };
	bool ownedByCurrentUser{ false };
	bool systemProcess{ false };
	bool userProcess{ false };

	ProcessStatus status{ ProcessStatus::Other };

	// CPU
	int userUsage{ 0 };
	int sysUsage{ 0 };
	Int64 userTime{ 0 };
	Int64 sysTime{ 0 };

	// Memory (kilobytes)
	Int64 vmSize{ -1 };
	Int64 vmRSS{ -1 };
	// Preferred memory for the main column. Linux derives private resident pages from RSS minus
	// shared pages; Windows provides private commit. -1 falls back to RSS.
	Int64 vmURSS{ -1 };
	Int64 vmPSS{ -1 };
	Int64 sharedMem{ -1 };
	// True when the kernel reported the shared-memory breakdown, so vmURSS could be derived.
	bool hasSharedInfo{ false };

	// Network (bytes, -1 if not available)
	Int64 netDownload{ -1 };
	Int64 netUpload{ -1 };

	// GPU (percentage, -1 if not available)
	int gpuUsage{ -1 };
	Int64 gpuMemory{ -1 };

	// Disk I/O totals (bytes, -1 if not available)
	Int64 ioReadBytes{ -1 };
	Int64 ioWriteBytes{ -1 };

	// Other
	int niceLevel{ 0 };
	int numThreads{ 0 };
	// Linux reconstructs argv here. Platforms without a public argv API may provide just the
	// executable path. Shown in the command column and used by Copy Command Line.
	std::string commandLine;
	// Controlling terminal device number, 0 when the process has none.
	Int64 ttyNr{ 0 };
	// Friendly Linux tty name (for example, "pts/2"). Empty when there is no controlling tty.
	std::string tty;
	// PID of the process tracing (debugging) this one, 0 when it is not traced.
	Int64 tracerPid{ 0 };
	// Process start time since boot, in SystemInfo::clockTicksPerSecond units. Together with PID
	// it identifies one incarnation of a process, which matters because PIDs are recycled.
	Int64 startTime{ 0 };
	// Resolved icon file for this process, empty when none could be found.
	std::string iconPath;
	// X11 window icon used only when no desktop-entry icon was found.
	DrawablePtr windowIcon;

	// Sorting helpers
	/** Private memory when known, otherwise plain RSS. */
	Int64 getMemoryForSort() const { return vmURSS >= 0 ? vmURSS : vmRSS; }

	int getCpuForSort() const { return userUsage + sysUsage; }

	// Formatted display strings
	std::string formatMemory() const;
	std::string formatSharedMem() const;
	std::string formatCpu() const;
	std::string formatGpuUsage() const;
	std::string formatGpuMemory() const;
	std::string formatDownload() const;
	std::string formatUpload() const;
	std::string formatCpuTime( Int64 ticksPerSecond ) const;
	std::string formatRelativeStartTime( double uptimeSeconds, Int64 ticksPerSecond ) const;
};

/** Formats a KiB amount using the largest fitting binary unit (K/M/G/T/P), matching the short
 *  form used by the original process table (e.g. "0 K", "853.8 M", "4.2 G").
 *  Returns an empty string for negative input. */
std::string formatKiB( Int64 kib );

/** Formats a KiB amount using IEC units (KiB/MiB/GiB/TiB/PiB), used by the status bar.
 *  Returns an empty string for negative input. */
std::string formatKiBIEC( Int64 kib );

/** Formats a byte-per-second amount using the largest fitting binary unit (B/K/M/G).
 *  Returns an empty string for zero or negative input. */
std::string formatBytesPerSecond( Int64 bytes );

/** Formats a byte total using binary units. Returns an empty string for negative input. */
std::string formatBytes( Int64 bytes );

} // namespace eproc

#endif // EPROC_PROCESS_INFO_HPP
