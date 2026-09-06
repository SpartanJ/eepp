#!/usr/bin/env python3

import argparse
import subprocess
import time

import gi

gi.require_version("Atspi", "2.0")
from gi.repository import Atspi


def wait_for_application(name, timeout):
	deadline = time.monotonic() + timeout
	while time.monotonic() < deadline:
		desktop = Atspi.get_desktop(0)
		for index in range(desktop.get_child_count()):
			application = desktop.get_child_at_index(index)
			if application and application.get_name() == name:
				return application
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


def action_index(node, name):
	for index in range(node.get_n_actions()):
		if Atspi.Action.get_localized_name(node, index) == name:
			return index
	raise RuntimeError(f'action "{name}" was not exposed by "{node.get_name()}"')


def check(condition, message):
	if not condition:
		raise RuntimeError(message)


def validate(application):
	nodes = descendants(application)
	check(len(nodes) >= 20, f"expected a complete accessibility tree, got {len(nodes)} nodes")

	project_name = find_named(nodes, "Project name")
	check(project_name.get_role_name() == "entry", "Project name must be an entry")
	check(project_name.get_character_count() == 4, "Project name character count must equal 4")
	check(Atspi.Text.get_text(project_name, 0, -1) == "eepp", "Project name text query failed")
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
	print(f"AT-SPI validation passed: {len(nodes)} nodes")


def main():
	parser = argparse.ArgumentParser(description="Validate the eepp accessibility example via AT-SPI")
	parser.add_argument("--application", default="eepp - Accessibility")
	parser.add_argument("--executable", default="bin/eepp-ui-accessibility")
	parser.add_argument("--no-launch", action="store_true")
	parser.add_argument("--timeout", type=float, default=5.0)
	args = parser.parse_args()

	process = None
	try:
		if not args.no_launch:
			process = subprocess.Popen([args.executable])
		validate(wait_for_application(args.application, args.timeout))
	finally:
		if process:
			process.terminate()
			try:
				process.wait(timeout=1)
			except subprocess.TimeoutExpired:
				process.kill()
				process.wait()


if __name__ == "__main__":
	main()
