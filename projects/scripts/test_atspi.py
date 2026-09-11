#!/usr/bin/env python3

import argparse
import subprocess
import threading
import time

import gi

gi.require_version("Atspi", "2.0")
from gi.repository import Atspi


def wait_for_application(name, timeout, process_id=None):
	deadline = time.monotonic() + timeout
	while time.monotonic() < deadline:
		desktop = Atspi.get_desktop(0)
		for index in range(desktop.get_child_count() - 1, -1, -1):
			application = desktop.get_child_at_index(index)
			try:
				if (
					application
					and application.get_name() == name
					and (process_id is None or application.get_process_id() == process_id)
				):
					application.get_child_count()
					return application
			except Exception:
				continue
		time.sleep(0.1)
	raise RuntimeError(f'accessibility application "{name}" was not found')


def descendants(root):
	result = []
	pending = [root]
	while pending:
		node = pending.pop()
		result.append(node)
		for index in range(node.get_child_count() - 1, -1, -1):
			child = node.get_child_at_index(index)
			if child:
				pending.append(child)
	return result


def find_named(nodes, name):
	for node in nodes:
		if node.get_name() == name:
			return node
	raise RuntimeError(f'accessibility node "{name}" was not found')


def wait_for_nodes(application, names, timeout=2):
	deadline = time.monotonic() + timeout
	while True:
		nodes = descendants(application)
		available = {node.get_name() for node in nodes}
		if all(name in available for name in names):
			return nodes
		if time.monotonic() >= deadline:
			raise RuntimeError(f"accessibility tree did not expose {names}, got {sorted(available)}")
		time.sleep(0.05)


def action_index(node, name):
	for index in range(node.get_n_actions()):
		if Atspi.Action.get_localized_name(node, index) == name:
			return index
	raise RuntimeError(f'action "{name}" was not exposed by "{node.get_name()}"')


def check(condition, message):
	if not condition:
		raise RuntimeError(message)


def start_discovery_listener():
	discovered_process_ids = set()

	def on_children_changed(event):
		application = event.any_data
		try:
			if application:
				discovered_process_ids.add(application.get_process_id())
		except Exception:
			pass

	listener = Atspi.EventListener.new(on_children_changed)
	check(
		listener.register("object:children-changed:add"),
		"could not register the AT-SPI application discovery listener",
	)
	thread = threading.Thread(target=Atspi.event_main, daemon=True)
	thread.start()
	return listener, discovered_process_ids, thread


def wait_for_window_count(application, count, timeout=2):
	deadline = time.monotonic() + timeout
	while application.get_child_count() != count and time.monotonic() < deadline:
		time.sleep(0.05)
	check(application.get_child_count() == count, f"expected {count} application windows")


def validate(application, multi_window=False, close_primary=False, process_id=None):
	required_nodes = ["Project name", "Enable autosave"]
	if multi_window:
		required_nodes.append("Secondary accessibility window")
	nodes = wait_for_nodes(application, required_nodes)
	check(len(nodes) >= 20, f"expected a complete accessibility tree, got {len(nodes)} nodes")
	check(application.get_role_name() == "application", "root must be an application")
	expected_windows = 2 if multi_window else 1
	check(
		application.get_child_count() == expected_windows,
		f"expected {expected_windows} application windows, got {application.get_child_count()}",
	)
	for index in range(application.get_child_count()):
		check(
			application.get_child_at_index(index).get_role_name() == "window",
			"each application child must be a native window",
		)

	project_name = find_named(nodes, "Project name")
	check(project_name.get_role_name() == "entry", "Project name must be an entry")
	check(project_name.get_accessible_id(), "Project name must expose a stable accessible ID")
	locale = project_name.get_object_locale()
	check(locale == "C", f"Project name locale query failed: {locale!r}")
	check(project_name.get_help_text() == "", "Project name help text query failed")
	check(project_name.get_attributes() is not None, "Project name attributes query failed")
	check(project_name.get_relation_set() is not None, "Project name relations query failed")
	check(project_name.get_layer() is not None, "Project name component layer query failed")
	check(project_name.get_mdi_z_order() == 0, "Project name component Z-order query failed")
	check(project_name.get_alpha() == 1, "Project name component alpha query failed")
	check(project_name.get_character_count() == 4, "Project name character count must equal 4")
	check(Atspi.Text.get_text(project_name, 0, -1) == "eepp", "Project name text query failed")
	check(project_name.get_text_attributes(0) is not None, "Project name text attributes query failed")
	check(project_name.get_default_attributes() is not None, "Default text attributes query failed")
	check(
		Atspi.EditableText.set_text_contents(project_name, "eepp2"),
		"Project name text replacement failed",
	)
	check(
		Atspi.Text.get_text(project_name, 0, -1) == "eepp2",
		"Project name replacement was not visible",
	)

	checkbox = find_named(nodes, "Enable autosave")
	check(checkbox.get_role_name() == "check box", "autosave must be a check box")
	check(checkbox.do_action(action_index(checkbox, "toggle")), "checkbox toggle failed")
	time.sleep(0.05)
	check(
		checkbox.get_state_set().contains(Atspi.StateType.CHECKED),
		"checkbox did not expose its checked state",
	)

	radio = find_named(nodes, "Light Theme")
	check(radio.get_role_name() == "radio button", "theme must be a radio button")
	check(radio.do_action(action_index(radio, "select")), "radio selection failed")
	time.sleep(0.05)
	states = radio.get_state_set()
	check(states.contains(Atspi.StateType.CHECKED), "radio did not expose checked state")
	check(states.contains(Atspi.StateType.SELECTED), "radio did not expose selected state")

	spin = find_named(nodes, "Retry count")
	check(spin.get_role_name() == "spin button", "Retry count must be a spin button")
	check(spin.get_current_value() == 3, "Retry count must initially equal 3")
	check(spin.get_minimum_value() == 0, "Retry count minimum must equal 0")
	check(spin.get_maximum_value() == 10, "Retry count maximum must equal 10")

	find_named(nodes, "Colors")
	find_named(nodes, "Projects")
	if multi_window:
		find_named(nodes, "Secondary accessibility window")
		open_window = find_named(nodes, "Open accessibility window")
		check(
			open_window.do_action(action_index(open_window, "click")),
			"dynamic window open action failed",
		)
		application = wait_for_application("eepp - Accessibility", 2, process_id)
		wait_for_window_count(application, 3)
		dynamic_nodes = descendants(application)
		find_named(dynamic_nodes, "Dynamic accessibility window")
		close_dynamic = find_named(dynamic_nodes, "Close dynamic window")
		check(
			close_dynamic.do_action(action_index(close_dynamic, "click")),
			"dynamic window close action failed",
		)
		application = wait_for_application("eepp - Accessibility", 2, process_id)
		wait_for_window_count(application, 2)
		close_name = "Close primary window" if close_primary else "Close secondary window"
		close = find_named(nodes, close_name)
		check(close.do_action(action_index(close, "click")), f"{close_name} action failed")
		application = wait_for_application("eepp - Accessibility", 2, process_id)
		wait_for_window_count(application, 1)
		if close_primary:
			remaining = application.get_child_at_index(0)
			check(
				remaining.get_name() == "eepp - Accessibility Secondary",
				"secondary window did not become the surviving application root",
			)
	print(f"AT-SPI validation passed: {len(nodes)} nodes")


def main():
	parser = argparse.ArgumentParser(description="Validate the eepp accessibility example via AT-SPI")
	parser.add_argument("--application", default="eepp - Accessibility")
	parser.add_argument("--executable", default="bin/eepp-ui-accessibility")
	parser.add_argument("--no-launch", action="store_true")
	parser.add_argument("--multi-window", action="store_true")
	parser.add_argument("--close-primary", action="store_true")
	parser.add_argument("--timeout", type=float, default=5.0)
	parser.add_argument("--process-id", type=int)
	args = parser.parse_args()

	process = None
	discovery_listener = None
	discovery_thread = None
	try:
		if not args.no_launch:
			discovery_listener, discovered_process_ids, discovery_thread = start_discovery_listener()
			command = [args.executable]
			if args.multi_window or args.close_primary:
				command.append("--multi-window")
			if args.close_primary:
				command.append("--close-primary")
			process = subprocess.Popen(command)
		application = wait_for_application(
				args.application,
				args.timeout,
				process.pid if process is not None else args.process_id,
			)
		if process is not None:
			deadline = time.monotonic() + args.timeout
			while process.pid not in discovered_process_ids and time.monotonic() < deadline:
				time.sleep(0.05)
			check(
				process.pid in discovered_process_ids,
				"the newly launched application was not announced through AT-SPI",
			)
		validate(
			application,
			args.multi_window or args.close_primary,
			args.close_primary,
			process.pid if process is not None else args.process_id,
		)
	finally:
		if discovery_listener:
			discovery_listener.deregister("object:children-changed:add")
			Atspi.event_quit()
		if discovery_thread:
			discovery_thread.join(timeout=1)
		if process:
			process.terminate()
			try:
				process.wait(timeout=1)
			except subprocess.TimeoutExpired:
				process.kill()
				process.wait()


if __name__ == "__main__":
	main()
