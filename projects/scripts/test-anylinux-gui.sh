#!/bin/sh
# Run inside the Alpine smoke-test image. Verify native window and UI startup,
# including dlopened dependencies that --version does not exercise.
set -eu
image=$1
mode=${2:-software}
case "$mode" in
	software) ;;
	zink)
		# Select the host's CPU Vulkan ICD explicitly. Zink alone may select a GPU.
		icd=
		for manifest in /usr/share/vulkan/icd.d/*lvp*.json; do
			[ -f "$manifest" ] || continue
			icd=$manifest
			break
		done
		[ -n "$icd" ] || { echo 'Host Lavapipe ICD not found' >&2; exit 1; }
		export SHARUN_ALLOW_SYS_VKICD=1 MESA_LOADER_DRIVER_OVERRIDE=zink
		export LIBGL_KOPPER_DRI2=1 VK_DRIVER_FILES="$icd"
		export VK_LOADER_DRIVERS_SELECT='*lvp*' VK_LOADER_DEBUG=driver
		;;
	*) echo "Unknown smoke-test rendering mode: $mode" >&2; exit 1 ;;
esac
work=$(mktemp -d)
server_pid=
app_pid=
# Invoked indirectly by the EXIT trap.
# shellcheck disable=SC2329
cleanup() {
	# The crash handler can itself fail when foreign libc code was loaded. Keep
	# cleanup bounded even in that case; this test runs in a disposable container.
	[ -z "$app_pid" ] || { kill -KILL "$app_pid" 2>/dev/null || :; wait "$app_pid" 2>/dev/null || :; }
	[ -z "$server_pid" ] || { kill "$server_pid" 2>/dev/null || :; wait "$server_pid" 2>/dev/null || :; }
	# Run each invocation in its own disposable container. AppImage children can
	# outlive the runtime and still write Mesa caches; let container teardown
	# stop every descendant and discard the temporary directory together.
}
trap 'cleanup' EXIT
trap 'exit 1' HUP INT TERM

Xvfb -displayfd 3 -screen 0 1280x800x24 -nolisten tcp 3>"$work/display" >"$work/xvfb.log" 2>&1 &
server_pid=$!
attempt=0
while [ ! -s "$work/display" ]; do
	if ! kill -0 "$server_pid" 2>/dev/null || [ "$attempt" -ge 80 ]; then
		cat "$work/xvfb.log"
		echo 'Xvfb did not become ready' >&2
		exit 1
	fi
	attempt=$((attempt + 1))
	sleep 0.25
done

DISPLAY=":$(cat "$work/display")"
export DISPLAY XDG_RUNTIME_DIR="$work"
export XDG_CONFIG_HOME="$work/config" XDG_CACHE_HOME="$work/cache"
export EEPP_WINDOW_HIDDEN=1 EEPP_RUNTIME=native LIBGL_ALWAYS_SOFTWARE=1
"$image" --profile "$work/profile" --open-clean --verbose --log-level debug >"$work/ecode.log" 2>&1 &
app_pid=$!
attempt=0
while [ "$attempt" -lt 120 ]; do
	if grep -q 'Complete UI took:' "$work/ecode.log" && kill -0 "$app_pid" 2>/dev/null; then
		cat "$work/ecode.log"
		if [ "$mode" = zink ] && ! grep -Eq 'GL Renderer: zink.*(llvmpipe|lavapipe)' "$work/ecode.log"; then
			echo 'ecode did not select Zink with the host CPU Vulkan renderer' >&2
			exit 1
		fi
		exit 0
	fi
	kill -0 "$app_pid" 2>/dev/null || break
	grep -q 'Segmentation fault' "$work/ecode.log" && break
	attempt=$((attempt + 1))
	sleep 0.25
done
head -60 "$work/ecode.log"
tail -80 "$work/ecode.log"
echo 'ecode did not initialize its window and UI' >&2
exit 1
