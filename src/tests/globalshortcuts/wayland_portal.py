"""Run the Wayland shortcut integration harness on a private bus with a mock portal.

Usage: dbus-run-session -- python3 src/tests/globalshortcuts/wayland_portal.py path/to/test
Requires PyGObject with Gio, and a fresh dbus-run-session bus.
"""

import os
from pathlib import Path
import subprocess
import sys
import tempfile

from gi.repository import Gio, GLib


PORTAL = "org.freedesktop.portal.Desktop"
SHORTCUTS = "org.freedesktop.portal.GlobalShortcuts"
REQUEST = "org.freedesktop.portal.Request"
SESSION = "org.freedesktop.portal.Session"
REGISTRY = "org.freedesktop.host.portal.Registry"
DESKTOP = "/org/freedesktop/portal/desktop"


def variant(signature, value):
    return GLib.Variant(signature, value)


class Portal:
    def __init__(self, marker):
        self.bus = Gio.bus_get_sync(Gio.BusType.SESSION)
        self.marker = marker
        self.sessions = {}
        self.registered = set()
        self.loop = GLib.MainLoop()
        self.errors = []
        self.objects = []
        self.process = None
        self.previous_bus = None

    def install(self, path, interface):
        for entry in Gio.DBusNodeInfo.new_for_xml("<node>" + interface + "</node>").interfaces:
            self.objects.append(self.bus.register_object(
                path, entry, self.handle, self.property, None))

    def install_interfaces(self):
        self.install(DESKTOP, f"""
        <interface name='{SHORTCUTS}'>
          <property name='version' type='u' access='read'/>
          <method name='CreateSession'><arg type='a{{sv}}' direction='in'/><arg type='o' direction='out'/></method>
          <method name='BindShortcuts'>
            <arg type='o' direction='in'/><arg type='a(sa{{sv}})' direction='in'/>
            <arg type='s' direction='in'/><arg type='a{{sv}}' direction='in'/><arg type='o' direction='out'/>
          </method>
          <method name='ConfigureShortcuts'>
            <arg type='o' direction='in'/><arg type='s' direction='in'/><arg type='a{{sv}}' direction='in'/>
          </method>
          <method name='ListShortcuts'>
            <arg type='o' direction='in'/><arg type='a{{sv}}' direction='in'/><arg type='o' direction='out'/>
          </method>
        </interface>
        <interface name='{REGISTRY}'>
          <method name='Register'><arg type='s' direction='in'/><arg type='a{{sv}}' direction='in'/></method>
        </interface>
        <interface name='org.freedesktop.impl.eepp.Test'><method name='Hang'/></interface>
        """)

    def replace_owner(self):
        self.previous_bus = self.bus
        self.previous_bus.call_sync("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                    "org.freedesktop.DBus", "ReleaseName", variant("(s)", (PORTAL,)),
                                    None, Gio.DBusCallFlags.NONE, 2000, None)
        self.bus = Gio.DBusConnection.new_for_address_sync(
            os.environ["DBUS_SESSION_BUS_ADDRESS"],
            Gio.DBusConnectionFlags.AUTHENTICATION_CLIENT | Gio.DBusConnectionFlags.MESSAGE_BUS_CONNECTION,
            None, None)
        self.install_interfaces()
        self.acquire_name()
        self.marker.with_name("owner-replaced").touch()

    def acquire_name(self):
        result = self.bus.call_sync("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                    "org.freedesktop.DBus", "RequestName",
                                    variant("(su)", (PORTAL, 0)), None, Gio.DBusCallFlags.NONE, 2000, None)
        assert result.unpack() == (1,)

    def activate_from_old_owner(self, sender, session):
        self.previous_bus.emit_signal(sender, DESKTOP, SHORTCUTS, "Activated",
                                      variant("(osta{sv})", (session, "toggle", 456, {})))
        return False

    def check_client(self):
        if self.process.poll() is None:
            return True
        self.loop.quit()
        return False

    def property(self, bus, sender, path, interface, name):
        assert interface == SHORTCUTS and name == "version"
        return variant("u", 2)

    def handle(self, bus, sender, path, interface, method, parameters, invocation):
        try:
            self.method(sender, path, interface, method, parameters.unpack(), invocation)
        except Exception as error:
            self.errors.append(str(error))
            invocation.return_dbus_error("org.freedesktop.DBus.Error.Failed", str(error))

    def method(self, sender, path, interface, method, args, invocation):
        if method == "Hang":
            return
        if method == "Register":
            assert args[0] == "org.eepp.PortalTest" and sender not in self.registered
            self.registered.add(sender)
            invocation.return_value(None)
            return
        assert sender in self.registered, "Portal method was sent before Registry.Register"
        peer = sender[1:].replace(".", "_")
        if method == "Close":
            assert self.sessions.pop(path)["sender"] == sender
            if self.previous_bus is not None:
                self.marker.with_name("active-close").touch()
            invocation.return_value(None)
            return
        if method == "ConfigureShortcuts":
            if args[2]["activation_token"] == "replace-owner":
                invocation.return_value(None)
                self.replace_owner()
                return
            assert args[2]["activation_token"] == "settings-token"
            session = self.sessions[args[0]]
            session["shortcuts"] = [("toggle", {"trigger_description": variant("s", "Alt+F2")})]
            self.bus.emit_signal(sender, DESKTOP, SHORTCUTS, "ShortcutsChanged",
                                 variant("(oa(sa{sv}))", (args[0], session["shortcuts"])))
            invocation.return_value(None)
            return

        token = args[-1]["handle_token"]
        request = DESKTOP + "/request/" + peer + "/" + token
        if method == "CreateSession":
            session = DESKTOP + "/session/" + peer + "/" + args[0]["session_handle_token"]
            self.sessions[session] = {"sender": sender}
            self.install(session, f"<interface name='{SESSION}'><method name='Close'/></interface>")
            # The portal signal may precede the method reply.
            self.response(sender, request, {"session_handle": variant("s", session)})
        elif method == "BindShortcuts":
            session = self.sessions[args[0]]
            assert "shortcuts" not in session, "BindShortcuts called twice on one session"
            declared = dict(args[1])
            if "hold" in declared:
                self.marker.touch()
                invocation.return_value(variant("(o)", (request,)))
                return
            assert declared["toggle"]["preferred_trigger"] == "ALT+F1"
            assert "second" not in declared or "preferred_trigger" not in declared["second"]
            session["shortcuts"] = [("toggle", {"trigger_description": variant("s", "Alt+F1")})]
            self.response(sender, request, {"shortcuts": variant("a(sa{sv})", session["shortcuts"])})
            if self.previous_bus is not None:
                GLib.timeout_add(30, self.activate_from_old_owner, sender, args[0])
        elif method == "ListShortcuts":
            session = self.sessions[args[0]]
            self.response(sender, request, {"shortcuts": variant("a(sa{sv})", session["shortcuts"])})
            invocation.return_value(variant("(o)", (request,)))
            for shortcut in ("unknown", "toggle"):
                self.bus.emit_signal(sender, DESKTOP, SHORTCUTS, "Activated",
                                     variant("(osta{sv})", (args[0], shortcut, 123,
                                                            {"activation_token": variant("s", "window-token")})))
            return
        else:
            raise AssertionError(f"Unexpected method {method}")
        invocation.return_value(variant("(o)", (request,)))

    def response(self, sender, path, results):
        self.bus.emit_signal(sender, path, REQUEST, "Response", variant("(ua{sv})", (0, results)))

    def run(self):
        self.install_interfaces()
        self.acquire_name()
        env = dict(os.environ, XDG_SESSION_TYPE="wayland", EEPP_RUNTIME="native")
        self.process = subprocess.Popen([sys.argv[1], str(self.marker)], env=env)
        GLib.timeout_add(20, self.check_client)
        GLib.timeout_add_seconds(18, self.loop.quit)
        try:
            self.loop.run()
            return self.errors or self.process.poll() != 0
        finally:
            if self.process.poll() is None:
                self.process.kill()
                self.process.wait()


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    with tempfile.TemporaryDirectory(prefix="eepp-wayland-tests-") as directory:
        result = Portal(Path(directory) / "consent-pending").run()
        if result:
            sys.exit(str(result))
