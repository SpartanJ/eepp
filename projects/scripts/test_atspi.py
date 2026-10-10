#!/usr/bin/env python3

import argparse
import subprocess
import time

import gi

gi.require_version("Atspi", "2.0")
from gi.repository import Atspi, GLib


def pause(seconds):
	# libatspi's cache and event dispatch share state with synchronous queries. Keep both on
	# one thread; running event_main concurrently with these queries can corrupt the client.
	context = GLib.MainContext.default()
	while context.pending():
		context.iteration(False)
	time.sleep(seconds)
	while context.pending():
		context.iteration(False)


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
		pause(0.1)
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
		pause(0.05)


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
	text_events = []
	selection_events = []

	def on_children_changed(event):
		if event.type == "object:selection-changed" or event.type == "object:state-changed:selected":
			selection_events.append((event.type, event.source.get_name(), event.detail1))
			return
		if event.type.startswith("object:text-") or event.type == "object:announcement":
			text_events.append((event.type, event.detail1, event.detail2, event.any_data))
			return
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
	for event in ("object:text-changed", "object:text-caret-moved", "object:text-selection-changed",
			"object:announcement", "object:selection-changed", "object:state-changed:selected"):
		check(listener.register(event), f"could not register {event}")
	return listener, discovered_process_ids, text_events, selection_events


def wait_for_window_count(application, count, timeout=2):
	deadline = time.monotonic() + timeout
	while application.get_child_count() != count and time.monotonic() < deadline:
		pause(0.05)
	check(application.get_child_count() == count, f"expected {count} application windows")


def validate(application, multi_window=False, close_primary=False, process_id=None, text_events=None,
		selection_events=None, policy_reset=False):
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
	states = project_name.get_state_set()
	check(states.contains(Atspi.StateType.SENSITIVE), "enabled input must be sensitive")
	check(states.contains(Atspi.StateType.SINGLE_LINE), "input must be single-line")
	password = find_named(nodes, "Account password")
	check(password.get_role() == Atspi.Role.PASSWORD_TEXT, "password must use the protected role")
	check(project_name.get_accessible_id(), "Project name must expose a stable accessible ID")
	locale = project_name.get_object_locale()
	check(locale == "C", f"Project name locale query failed: {locale!r}")
	check(project_name.get_help_text() == "", "Project name help text query failed")
	check(project_name.get_attributes() is not None, "Project name attributes query failed")
	check(project_name.get_relation_set() is not None, "Project name relations query failed")
	check(project_name.get_layer() is not None, "Project name component layer query failed")
	check(project_name.get_mdi_z_order() == 0, "Project name component Z-order query failed")
	check(project_name.get_alpha() == 1, "Project name component alpha query failed")
	project_extents = project_name.get_extents(Atspi.CoordType.SCREEN)
	check(project_extents.width > 0 and project_extents.height > 0, "Project name extents are empty")
	window = application.get_child_at_index(0)
	if window.get_state_set().contains(Atspi.StateType.SHOWING):
		hit = window.get_accessible_at_point(
			project_extents.x + project_extents.width // 2,
			project_extents.y + project_extents.height // 2,
			Atspi.CoordType.SCREEN,
		)
		check(hit is not None, "visible window component hit test failed")
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
	check(Atspi.Text.set_caret_offset(project_name, 2), "setting the caret failed")
	check(project_name.get_caret_offset() == 2, "caret offset did not change")
	check(Atspi.Text.add_selection(project_name, 1, 4), "adding a text selection failed")
	check(Atspi.Text.set_selection(project_name, 0, 0, 3), "setting a text selection failed")
	selection = Atspi.Text.get_selection(project_name, 0)
	check((selection.start_offset, selection.end_offset) == (0, 3), "selection offsets are wrong")
	check(Atspi.Text.remove_selection(project_name, 0), "removing a text selection failed")
	check(project_name.get_n_selections() == 0, "selection was not removed")
	if text_events is not None:
		deadline = time.monotonic() + 2
		while time.monotonic() < deadline and not any(
			event[0] == "object:text-changed:insert" and event[1:4] == (4, 1, "2")
			for event in text_events
		):
			pause(0.01)
		check(any(event[0] == "object:text-changed:insert" and event[1:4] == (4, 1, "2")
			for event in text_events), f"missing text insertion event: {text_events}")
		check(any(event[0] == "object:text-caret-moved" and event[1] == 2
			for event in text_events), "missing caret event")
		check(any(event[0] == "object:text-selection-changed" for event in text_events),
			"missing text selection event")
	description = find_named(nodes, "Description")
	check(description.get_state_set().contains(Atspi.StateType.MULTI_LINE), "edit must be multiline")
	check(Atspi.EditableText.set_text_contents(description, "first\nsecond\nthird"),
		"multiline replacement failed")
	line = Atspi.Text.get_string_at_offset(description, 8, Atspi.TextGranularity.LINE)
	check((line.content, line.start_offset, line.end_offset) == ("second\n", 6, 13),
		f"line granularity returned the wrong range: {line}")
	last_line = Atspi.Text.get_string_at_offset(description, 15, Atspi.TextGranularity.LINE)
	check((last_line.content, last_line.start_offset, last_line.end_offset) == ("third", 13, 18),
		f"last line must exclude the final newline: {last_line}")
	word = Atspi.Text.get_string_at_offset(description, 8, Atspi.TextGranularity.WORD)
	check((word.content, word.start_offset, word.end_offset) == ("second", 6, 12),
		f"word granularity returned the wrong range: {word}")
	check(description.get_character_count() == 18, "multiline character count must equal 18")
	check(Atspi.Text.get_character_at_offset(description, 6) == ord("s"),
		"character at offset returned the wrong character")

	if text_events is not None:
		# An application edit arrives as one exact insertion, not as a diff of the whole text.
		append = find_named(nodes, "Append description line")
		del text_events[:]
		check(append.do_action(action_index(append, "click")), "append action failed")
		def appended():
			return [event for event in text_events if event[0] == "object:text-changed:insert"]
		deadline = time.monotonic() + 2
		while time.monotonic() < deadline and not appended():
			pause(0.01)
		check(appended() == [("object:text-changed:insert", 18, 9, "\nappended")],
			f"application edit was not reported exactly: {text_events}")
		check(Atspi.Text.get_text(description, 18, -1) == "\nappended", "appended text query failed")

		# Replacing a field's text reports the old contents removed and the new ones inserted.
		reset = find_named(nodes, "Reset project name")
		del text_events[:]
		check(reset.do_action(action_index(reset, "click")), "reset action failed")
		def replaced():
			return [(event[0], event[1], event[2]) for event in text_events
				if event[0].startswith("object:text-changed")]
		deadline = time.monotonic() + 2
		while time.monotonic() < deadline and len(replaced()) < 2:
			pause(0.01)
		check(replaced() == [("object:text-changed:delete", 0, 5),
			("object:text-changed:insert", 0, 4)],
			f"text replacement was not reported as delete and insert: {text_events}")
		check(Atspi.Text.get_text(project_name, 0, -1) == "eepp", "reset text query failed")

	checkbox = find_named(nodes, "Enable autosave")
	check(checkbox.get_role_name() == "check box", "autosave must be a check box")
	check(checkbox.get_state_set().contains(Atspi.StateType.CHECKABLE), "checkbox must be checkable")
	check(checkbox.do_action(action_index(checkbox, "toggle")), "checkbox toggle failed")
	pause(0.05)
	check(
		checkbox.get_state_set().contains(Atspi.StateType.CHECKED),
		"checkbox did not expose its checked state",
	)

	radio = find_named(nodes, "Light Theme")
	check(radio.get_role_name() == "radio button", "theme must be a radio button")
	check(radio.do_action(action_index(radio, "select")), "radio selection failed")
	pause(0.05)
	states = radio.get_state_set()
	check(states.contains(Atspi.StateType.CHECKED), "radio did not expose checked state")
	check(states.contains(Atspi.StateType.SELECTED), "radio did not expose selected state")
	check(states.contains(Atspi.StateType.SELECTABLE), "radio must be selectable")

	language = find_named(nodes, "Language")
	check(language.get_state_set().contains(Atspi.StateType.EXPANDABLE), "combo must be expandable")
	check(language.do_action(action_index(language, "expand")), "combo expansion failed")
	pause(0.05)
	check(language.get_state_set().contains(Atspi.StateType.EXPANDED), "combo must report expanded")
	check(language.do_action(action_index(language, "collapse")), "combo collapse failed")

	spin = find_named(nodes, "Retry count")
	check(spin.get_role_name() == "spin button", "Retry count must be a spin button")
	check(spin.get_current_value() == 3, "Retry count must initially equal 3")
	check(spin.get_minimum_value() == 0, "Retry count minimum must equal 0")
	check(spin.get_maximum_value() == 10, "Retry count maximum must equal 10")
	check(spin.grab_focus(), "spin focus action failed")
	pause(0.05)
	check(spin.get_state_set().contains(Atspi.StateType.FOCUSED), "spin must report internal input focus")

	colors = find_named(nodes, "Colors")
	check("Selection" in colors.get_interfaces(), "list must expose the Selection interface")
	check(colors.select_child(1), "list selection action failed")
	pause(0.05)
	check(colors.get_n_selected_children() == 1, "list must report one selected child")
	check(colors.get_selected_child(0).get_name() == "Green", "list selected-child query failed")
	green = colors.get_child_at_index(1)
	check(green.get_state_set().contains(Atspi.StateType.SELECTED), "selected row state is missing")
	# Repeated scalar/state reads must reuse the selection baseline. The next change must
	# still report both the old and new selected rows, rather than overwriting that baseline.
	for _ in range(3):
		check(colors.is_child_selected(1), "repeated selected-child query failed")
		green.clear_cache()
		check(green.get_state_set().contains(Atspi.StateType.SELECTED),
			"repeated selected-row state query failed")
	if selection_events is not None:
		selection_events.clear()
	check(colors.select_child(2), "second list selection action failed")
	pause(0.1)
	check(colors.is_child_selected(2), "new list selection state is missing")
	check(not colors.is_child_selected(1), "old list selection state stayed set")
	check(colors.get_selected_child(0).get_name() == "Blue", "selection did not follow new row")
	if selection_events is not None:
		check(("object:selection-changed", "Colors", 0) in selection_events,
			f"container selection event missing: {selection_events}")
		check(("object:state-changed:selected", "Green", 0) in selection_events,
			f"old selected row was not cleared: {selection_events}")
		check(("object:state-changed:selected", "Blue", 1) in selection_events,
			f"new selected row was not announced: {selection_events}")
	# Tabs report selected state on their individual widgets as well as on the container.
	tabs = next(node for node in nodes if node.get_role() == Atspi.Role.PAGE_TAB_LIST)
	check(tabs.get_n_selected_children() == 1, "tab selection baseline is missing")
	advanced = find_named(nodes, "Advanced")
	check(advanced.do_action(action_index(advanced, "select")), "tab selection action failed")
	pause(0.1)
	check(tabs.get_n_selected_children() == 1, "tab selection count is stale")
	check(tabs.get_selected_child(0).get_name() == "Advanced", "tab selection snapshot is stale")
	general = find_named(nodes, "General")
	check(general.do_action(action_index(general, "select")), "tab selection reset failed")
	pause(0.1)
	check(tabs.get_selected_child(0).get_name() == "General", "tab selection snapshot did not reset")
	projects = find_named(nodes, "Projects")
	if selection_events is not None:
		selection_events.clear()
	# The table's Selection interface has not been queried yet. Selecting a row via Action
	# sends the container hint, without allocating a previous-selection snapshot or row events.
	project = projects.get_child_at_index(1)
	check(project.do_action(action_index(project, "select")), "unqueried table selection failed")
	pause(0.1)
	if selection_events is not None:
		check(("object:selection-changed", "Projects", 0) in selection_events,
			f"unqueried container selection event missing: {selection_events}")
		check(not any(event[0] == "object:state-changed:selected" for event in selection_events),
			f"unqueried container emitted selected-row changes: {selection_events}")
	check(projects.get_n_selected_children() == 1, "first table selection query failed")
	check(projects.get_selected_child(0).get_name() == "ecode", "first table selection snapshot is stale")
	check(projects.select_child(0), "queried table selection failed")
	pause(0.1)
	check(projects.is_child_selected(0) and not projects.is_child_selected(1),
		"queried table selection snapshot did not update")
	check(projects.get_selected_child(0).get_name() == "eepp", "updated table selection snapshot is stale")
	if selection_events is not None:
		check(("object:state-changed:selected", "ecode", 0) in selection_events,
			f"queried table's old selected row was not cleared: {selection_events}")
		check(("object:state-changed:selected", "eepp", 1) in selection_events,
			f"queried table's new selected row was not announced: {selection_events}")

	# An icon-only button is named by its tooltip; the status line it updates is a live region.
	refresh = find_named(nodes, "Refresh status")
	check(refresh.get_role() == Atspi.Role.PUSH_BUTTON,
		"icon-only button must be named by its tooltip")
	check(refresh.do_action(action_index(refresh, "click")), "refresh action failed")
	if text_events is not None:
		def announced():
			return any(event[0] == "object:announcement" and event[3] == "Status refreshed"
				for event in text_events)
		deadline = time.monotonic() + 2
		while time.monotonic() < deadline and not announced():
			pause(0.01)
		check(announced(), "live region change was not announced: "
			f"{[event for event in text_events if event[0] == 'object:announcement']}")

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
			# Component extents require a live request, unlike a potentially cached name. Verify
			# that the I/O thread still wakes the application after the previous Input is destroyed.
			extents = remaining.get_extents(Atspi.CoordType.SCREEN)
			check(extents.width > 0 and extents.height > 0, "surviving window stopped responding")
			close_last = find_named(descendants(application), "Close secondary window")
			check(close_last.do_action(action_index(close_last, "click")), "last window close failed")
	if policy_reset:
		check(Atspi.EditableText.set_text_contents(project_name, "disable accessibility"),
			"policy reset during native SetTextContents did not finish its reply")
	print(f"AT-SPI validation passed: {len(nodes)} nodes")


def main():
	parser = argparse.ArgumentParser(description="Validate the eepp accessibility example via AT-SPI")
	parser.add_argument("--application", default="eepp - Accessibility")
	parser.add_argument("--executable", default="bin/eepp-ui-accessibility")
	parser.add_argument("--no-launch", action="store_true")
	parser.add_argument("--multi-window", action="store_true")
	parser.add_argument("--close-primary", action="store_true")
	parser.add_argument("--hidden", action="store_true", help="keep all example windows hidden")
	parser.add_argument("--policy-reset", action="store_true", help="disable backend inside SetTextContents")
	parser.add_argument("--timeout", type=float, default=5.0)
	parser.add_argument("--process-id", type=int)
	args = parser.parse_args()

	process = None
	discovery_listener = None
	text_events = None
	selection_events = None
	try:
		if not args.no_launch:
			discovery_listener, discovered_process_ids, text_events, selection_events = start_discovery_listener()
			command = [args.executable]
			if args.hidden:
				command.append("--hidden")
			if args.policy_reset:
				command.append("--test-policy-reset")
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
				pause(0.05)
			check(
				process.pid in discovered_process_ids,
				"the newly launched application was not announced through AT-SPI",
			)
		validate(
			application,
			args.multi_window or args.close_primary,
			args.close_primary,
			process.pid if process is not None else args.process_id,
			text_events,
			selection_events,
			args.policy_reset,
		)
		if process is not None and (args.close_primary or args.policy_reset):
			check(process.wait(timeout=3) == 0, "native backend teardown crashed on last-window closure")
	finally:
		if discovery_listener:
			try:
				for event in ("object:children-changed:add", "object:text-changed",
					"object:text-caret-moved", "object:text-selection-changed", "object:announcement",
					"object:selection-changed", "object:state-changed:selected"):
					discovery_listener.deregister(event)
			except GLib.Error:
				# A closed bus must not mask the original failure or skip child-process cleanup.
				pass
		if process and process.poll() is None:
			process.terminate()
			try:
				process.wait(timeout=1)
			except subprocess.TimeoutExpired:
				process.kill()
				process.wait()


if __name__ == "__main__":
	main()
