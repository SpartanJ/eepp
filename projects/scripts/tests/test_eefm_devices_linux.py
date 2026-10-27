#!/usr/bin/env python3
"""Exercise eefm's Linux removal backend against a private, fake UDisks service.

Run on Linux after building release tests:
  python3 projects/scripts/tests/test_eefm_devices_linux.py bin/unit_tests/eepp-unit_tests
Requires dbus-daemon, python3-dbus, python3-gi and the optional libudisks2 runtime.
No real devices, mount operations, system bus or desktop session are used.
"""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from contextlib import contextmanager


ROOT = "/org/freedesktop/UDisks2"
BLOCK = "org.freedesktop.UDisks2.Block"
FS = "org.freedesktop.UDisks2.Filesystem"
DRIVE = "org.freedesktop.UDisks2.Drive"
ENCRYPTED = "org.freedesktop.UDisks2.Encrypted"


def serve(address, log, stall_discovery="0"):
    import dbus
    import dbus.service
    from dbus.mainloop.glib import DBusGMainLoop
    from gi.repository import GLib

    DBusGMainLoop(set_as_default=True)
    bus = dbus.bus.BusConnection(address)
    _name = dbus.service.BusName("org.freedesktop.UDisks2", bus)
    registry = {}
    exported = []

    def record(action, path, options):
        assert not options, f"Removal should never force: {options}"
        with open(log, "a", encoding="utf-8") as output:
            output.write(json.dumps([action, path]) + "\n")

    def drive_for(path):
        block = registry[path].get(BLOCK, {})
        drive = block.get("Drive", "/")
        backing = block.get("CryptoBackingDevice", "/")
        return drive_for(backing) if drive == "/" and backing != "/" else drive

    def mount_points(properties):
        return [
            bytes(point).rstrip(b"\0")
            for point in properties.get(FS, {}).get("MountPoints", [])
        ]

    def busy(message):
        return dbus.exceptions.DBusException(
            message, name="org.freedesktop.UDisks2.Error.DeviceBusy"
        )

    def remove(action, path, options, reply, error):
        record(action, path, options)
        if path.endswith(("/stall_eject", "/stall_poweroff")):
            return  # Deliberately never reply: only GCancellable can release the worker.
        for block_path, properties in registry.items():
            if drive_for(block_path) != path:
                continue
            if mount_points(properties):
                error(busy("Drive has a mounted filesystem"))
                return
            backing = properties.get(BLOCK, {}).get("CryptoBackingDevice", "/")
            if backing != "/":
                error(busy("Encrypted volume is unlocked"))
                return
        reply()

    class Manager(dbus.service.Object):
        @dbus.service.method(
            "org.freedesktop.DBus.ObjectManager",
            "",
            "a{oa{sa{sv}}}",
            async_callbacks=("reply", "error"),
        )
        def GetManagedObjects(self, reply, error):
            if stall_discovery == "1":
                record("GetManagedObjects", ROOT, {})
                return
            reply(registry)

    class Drive(dbus.service.Object):
        @dbus.service.method(DRIVE, "a{sv}", "", async_callbacks=("reply", "error"))
        def Eject(self, options, reply, error):
            remove("Eject", self.__dbus_object_path__, options, reply, error)

        @dbus.service.method(DRIVE, "a{sv}", "", async_callbacks=("reply", "error"))
        def PowerOff(self, options, reply, error):
            remove("PowerOff", self.__dbus_object_path__, options, reply, error)

    class Filesystem(dbus.service.Object):
        @dbus.service.method(ENCRYPTED, "a{sv}", "", async_callbacks=("reply", "error"))
        def Lock(self, options, reply, error):
            record("Lock", self.__dbus_object_path__, options)
            if self.__dbus_object_path__.endswith("/stall_lock"):
                return
            for path, properties in registry.items():
                block = properties.get(BLOCK, {})
                if block.get("CryptoBackingDevice") == self.__dbus_object_path__:
                    if mount_points(properties) or any(
                        child.get(BLOCK, {}).get("CryptoBackingDevice") == path
                        for child in registry.values()
                    ):
                        error(busy("Encrypted mapping has an active child"))
                        return
            for properties in registry.values():
                block = properties.get(BLOCK, {})
                if block.get("CryptoBackingDevice") == self.__dbus_object_path__:
                    block["CryptoBackingDevice"] = dbus.ObjectPath("/")
            reply()

        @dbus.service.method(FS, "a{sv}", "", async_callbacks=("reply", "error"))
        def Unmount(self, options, reply, error):
            record("Unmount", self.__dbus_object_path__, options)
            if self.__dbus_object_path__.endswith("/stall_unmount"):
                return
            if self.__dbus_object_path__.endswith("/busy"):
                error(busy("The volume is busy"))
                return
            for parent in mount_points(registry[self.__dbus_object_path__]):
                if any(
                    point.startswith(parent + b"/")
                    for properties in registry.values()
                    for point in mount_points(properties)
                ):
                    error(busy("Parent has a mounted child"))
                    return
            registry[self.__dbus_object_path__][FS]["MountPoints"] = dbus.Array(
                [], signature="ay"
            )
            reply()

    def drive(key, *, power=False, eject=False):
        path = ROOT + "/drives/" + key
        registry[path] = {
            DRIVE: {
                "CanPowerOff": dbus.Boolean(power),
                "Ejectable": dbus.Boolean(eject),
            }
        }
        exported.append(Drive(bus, path))
        return path

    def volume(key, drive_path, mount=None, *, system=False, backing="/"):
        path = ROOT + "/block_devices/" + key
        properties = {
            BLOCK: {
                "Drive": dbus.ObjectPath(drive_path),
                "CryptoBackingDevice": dbus.ObjectPath(backing),
                "HintSystem": dbus.Boolean(system),
                "HintIgnore": dbus.Boolean(False),
            }
        }
        if mount:
            properties[FS] = {
                "MountPoints": dbus.Array(
                    [dbus.ByteArray(os.fsencode(mount) + b"\0")], signature="ay"
                )
            }
        registry[path] = properties
        exported.append(Filesystem(bus, path))
        return path

    exported.append(Manager(bus, ROOT))
    usb = drive("usb", power=True)
    volume("usb_a", usb, "/fixture/usb/a")
    volume("usb_b", usb, "/fixture/usb/b")
    # An unmounted metadata/firmware partition must not hide removal of the USB drive.
    volume("usb_metadata", usb, system=True)
    volume("disc", drive("disc", power=True, eject=True), "/fixture/disc")
    volume("busy", drive("busy", power=True), "/fixture/busy")
    # A capability flag alone must never make the system disk removable.
    protected = drive("protected", power=True)
    volume("root", protected, "/", system=True)
    volume("protected", protected, "/fixture/protected")
    encrypted = drive("encrypted", power=True)
    backing = volume("crypto", encrypted)
    registry[backing][ENCRYPTED] = {}
    volume("cleartext", "/", "/fixture/encrypted", backing=backing)
    nested = drive("nested", power=True)
    volume("nested_parent", nested, "/fixture/nested")
    volume("nested_child", nested, "/fixture/nested/child")
    layered = drive("layered", power=True)
    outer = volume("layer_outer", layered)
    inner = volume("layer_inner", "/", backing=outer)
    registry[outer][ENCRYPTED] = {}
    registry[inner][ENCRYPTED] = {}
    volume("layer_cleartext", "/", "/fixture/layered", backing=inner)
    volume(
        "stall_unmount", drive("stall_unmount", power=True), "/fixture/stall-unmount"
    )
    volume("stall_eject", drive("stall_eject", eject=True), "/fixture/stall-eject")
    volume(
        "stall_poweroff", drive("stall_poweroff", power=True), "/fixture/stall-poweroff"
    )
    stalled_crypto = volume("stall_lock", drive("stall_lock", power=True))
    registry[stalled_crypto][ENCRYPTED] = {}
    volume("stall_cleartext", "/", "/fixture/stall-lock", backing=stalled_crypto)
    print("ready", flush=True)
    GLib.MainLoop().run()


@contextmanager
def fixture(log, discovery=False):
    daemon = subprocess.Popen(
        ["dbus-daemon", "--session", "--nofork", "--print-address=1"],
        stdout=subprocess.PIPE,
        text=True,
    )
    server = None
    try:
        address = daemon.stdout.readline().strip()
        assert address, "Private D-Bus daemon did not start"
        server = subprocess.Popen(
            [
                sys.executable,
                __file__,
                "--serve",
                address,
                str(log),
                "1" if discovery else "0",
            ],
            stdout=subprocess.PIPE,
            text=True,
        )
        assert server.stdout.readline().strip() == "ready", "Mock UDisks did not start"
        yield dict(
            os.environ,
            DBUS_SYSTEM_BUS_ADDRESS=address,
            EEFM_TEST_UDISKS="0" if discovery else "1",
            EEFM_TEST_UDISKS_DISCOVERY="1" if discovery else "0",
            EEFM_TEST_UDISKS_LOG=str(log),
        )
    finally:
        for process in (server, daemon):
            if process is not None:
                process.terminate()
                process.wait(timeout=10)


def run(binary):
    with tempfile.TemporaryDirectory(prefix="eefm-udisks-test-") as temporary:
        log = Path(temporary) / "calls.jsonl"
        with fixture(log) as environment:
            subprocess.run(
                [str(Path(binary).resolve()), "--filter=EEFMDevices.*"],
                env=environment,
                check=True,
                timeout=30,
            )
            calls = [json.loads(line) for line in log.read_text().splitlines()]
            usb_unmounts = {
                ("Unmount", ROOT + "/block_devices/usb_a"),
                ("Unmount", ROOT + "/block_devices/usb_b"),
            }
            assert {tuple(call) for call in calls[:2]} == usb_unmounts, calls
            assert calls[2:] == [
                ["PowerOff", ROOT + "/drives/usb"],
                ["Unmount", ROOT + "/block_devices/disc"],
                ["Eject", ROOT + "/drives/disc"],
                ["Unmount", ROOT + "/block_devices/busy"],
                ["Unmount", ROOT + "/block_devices/cleartext"],
                ["Lock", ROOT + "/block_devices/crypto"],
                ["PowerOff", ROOT + "/drives/encrypted"],
                ["Unmount", ROOT + "/block_devices/nested_child"],
                ["Unmount", ROOT + "/block_devices/nested_parent"],
                ["PowerOff", ROOT + "/drives/nested"],
                ["Unmount", ROOT + "/block_devices/layer_cleartext"],
                ["Lock", ROOT + "/block_devices/layer_inner"],
                ["Lock", ROOT + "/block_devices/layer_outer"],
                ["PowerOff", ROOT + "/drives/layered"],
                ["Unmount", ROOT + "/block_devices/stall_unmount"],
                ["Unmount", ROOT + "/block_devices/stall_cleartext"],
                ["Lock", ROOT + "/block_devices/stall_lock"],
                ["Unmount", ROOT + "/block_devices/stall_eject"],
                ["Eject", ROOT + "/drives/stall_eject"],
                ["Unmount", ROOT + "/block_devices/stall_poweroff"],
                ["PowerOff", ROOT + "/drives/stall_poweroff"],
            ], calls
            print(
                "PASS: removal semantics, nested dependencies, context cleanup and cancellation"
            )
        with fixture(
            Path(temporary) / "discovery.jsonl", discovery=True
        ) as environment:
            subprocess.run(
                [
                    str(Path(binary).resolve()),
                    "--filter=EEFMDevices.UDisksDiscoveryCancellationReleasesTheWorkerPool",
                ],
                env=environment,
                check=True,
                timeout=15,
            )
            print("PASS: stalled discovery is cancelled before joining its worker pool")


if __name__ == "__main__":
    if sys.argv[1:2] == ["--serve"]:
        serve(*sys.argv[2:])
    elif len(sys.argv) == 2:
        run(sys.argv[1])
    else:
        raise SystemExit(__doc__)
