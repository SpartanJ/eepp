# eproc

eproc is eepp's native system monitor. It collects process and system statistics on a worker
thread and displays them in a process table/tree and performance charts.

## GPU monitoring

The Performance tab shows each detected adapter as GPU 0, GPU 1, and so on, with its model
when available. Each adapter has its own utilization history and memory figures. Numbering follows
the current adapter list and is shared with the process table's GPU Device column. Hover a device
cell to see the corresponding models. A process can use several adapters; a listed device may be idle.

Linux uses NVIDIA's NVML and DRM/sysfs counters for AMD and other DRM drivers. Device-wide
counters are preferred; fallbacks based on accessible DRM clients are marked as tracked usage or
tracked memory. First-sample rates and unavailable counters stay unavailable. Windows uses WDDM
counters grouped by adapter and DXGI model names. macOS shows separate IOAccelerator adapters
when the driver exposes them; per-process GPU attribution is unavailable there.

## Windows monitoring

Windows supports system and per-process CPU, memory and I/O counters, per-core CPU charts,
system network rates, GPU engine usage and GPU memory, native executable icons, and the
Programs Only filter for processes with visible top-level windows.

GPU statistics use Windows' WDDM performance counters when the installed driver exposes them.
Usage is the busiest engine's percentage; GPU memory includes dedicated and shared memory.
The system memory figure comes from adapter counters, since summing per-process allocations
would count memory shared between processes multiple times. Unavailable counters remain blank.

System network rates use active non-loopback interfaces and work without tracing privileges.
Per-process TCP/UDP rates (IPv4 and IPv6) use a private real-time ETW session with the
permissions already granted to the running account. Administrator access is not required when
Performance Log Users membership and the provider's permissions allow tracing. eproc does not
request elevation or change account permissions automatically.

The process Download/Upload column tooltips report the tracing status. If Windows denies
tracing or a session fails, collection retries every 30 seconds while system-wide network rates
remain available. Idle and unavailable process rates remain blank;
the monitoring status explains whether accounting is available. Incomplete intervals caused by
lost ETW events are discarded rather than displayed as accurate measurements. Initial samples
establish a baseline; ETW delivery can lag by about one second.

Some fields have different platform semantics. Windows' main Memory column shows private
commit, Resident Memory shows the working set, and Swap represents commit capacity rather
than Linux swap. The command column currently contains the executable path. Linux-only
details such as PSS, controlling TTY and Unix niceness are not collected on Windows.
