#!/usr/bin/env python3

import argparse
import json
import statistics
import subprocess
import threading
import time

import gi

gi.require_version("Atspi", "2.0")
from gi.repository import Atspi


def check(condition, message):
	if not condition:
		raise RuntimeError(message)


class Measurements:
	def __init__(self):
		self.calls = []

	def call(self, name, function):
		start = time.perf_counter()
		result = function()
		self.calls.append((name, (time.perf_counter() - start) * 1000))
		return result

	def summary(self):
		durations = sorted(duration for _, duration in self.calls)
		if not durations:
			return {"calls": 0, "median_ms": 0, "p95_ms": 0, "maximum_ms": 0}
		p95_index = min(len(durations) - 1, int(len(durations) * 0.95))
		return {
			"calls": len(durations),
			"median_ms": statistics.median(durations),
			"p95_ms": durations[p95_index],
			"maximum_ms": durations[-1],
		}


def find_application(name, process_id, timeout):
	deadline = time.monotonic() + timeout
	while time.monotonic() < deadline:
		desktop = Atspi.get_desktop(0)
		for index in range(desktop.get_child_count() - 1, -1, -1):
			application = desktop.get_child_at_index(index)
			try:
				if (
					application
					and application.get_name() == name
					and application.get_process_id() == process_id
				):
					return application
			except Exception:
				continue
		time.sleep(0.01)
	raise RuntimeError(f'accessibility application "{name}" was not discovered')


def traverse(root, measurements):
	nodes = []
	pending = [root]
	while pending:
		node = pending.pop()
		nodes.append(node)
		measurements.call("name", node.get_name)
		measurements.call("role", node.get_role_name)
		count = measurements.call("child-count", node.get_child_count)
		for index in range(count - 1, -1, -1):
			child = measurements.call(
				"child-at-index", lambda node=node, index=index: node.get_child_at_index(index)
			)
			if child:
				pending.append(child)
	return nodes


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


def run_sample(args, item_count, target_process_id, model_changed, model_change_count):
	process = None
	try:
		start = time.perf_counter()
		process = subprocess.Popen(
			[args.executable, "--benchmark", "--benchmark-items", str(item_count)],
			stdout=subprocess.DEVNULL,
		)
		target_process_id[0] = process.pid
		application = find_application(args.application, process.pid, args.timeout)
		discovery_ms = (time.perf_counter() - start) * 1000

		measurements = Measurements()
		start = time.perf_counter()
		nodes = traverse(application, measurements)
		traversal_ms = (time.perf_counter() - start) * 1000
		check(len(nodes) >= item_count + 50, f"benchmark tree is incomplete: {len(nodes)} nodes")
		find_named(nodes, "Benchmark depth leaf")
		benchmark_list = find_named(nodes, "Benchmark items")
		check(
			benchmark_list.get_child_count() == item_count,
			f"benchmark list must expose {item_count} items",
		)

		start = time.perf_counter()
		for node in nodes[: min(args.sample_size, len(nodes))]:
			measurements.call("description", node.get_description)
			measurements.call("state", node.get_state_set)
			measurements.call("attributes", node.get_attributes)
			measurements.call("extents", lambda node=node: node.get_extents(Atspi.CoordType.SCREEN))
		property_batch_ms = (time.perf_counter() - start) * 1000

		project_name = find_named(nodes, "Project name")
		check(
			measurements.call("text-character-count", project_name.get_character_count) == 4,
			"text character count query failed",
		)
		check(
			measurements.call(
				"text-content", lambda: Atspi.Text.get_text(project_name, 0, -1)
			) == "eepp",
			"text content query failed",
		)
		check(
			measurements.call(
				"editable-text", lambda: Atspi.EditableText.set_text_contents(project_name, "eepp2")
			),
			"editable text action failed",
		)
		check(
			measurements.call(
				"focus", lambda: project_name.do_action(action_index(project_name, "focus"))
			),
			"focus action failed",
		)

		checkbox = find_named(nodes, "Enable autosave")
		check(
			measurements.call(
				"toggle", lambda: checkbox.do_action(action_index(checkbox, "toggle"))
			),
			"toggle action failed",
		)
		check(
			measurements.call("toggle-state", checkbox.get_state_set).contains(
				Atspi.StateType.CHECKED
			),
			"toggle state query failed",
		)

		spin = find_named(nodes, "Retry count")
		check(measurements.call("range-value", spin.get_current_value) == 3, "range value query failed")
		check(
			measurements.call(
				"set-range-value", lambda: Atspi.Value.set_current_value(spin, 4)
			),
			"range value action failed",
		)

		window = measurements.call("application-window", lambda: application.get_child_at_index(0))
		window_extents = measurements.call(
			"window-extents", lambda: window.get_extents(Atspi.CoordType.SCREEN)
		)
		hit = measurements.call(
			"component-hit-test",
			lambda: window.get_accessible_at_point(
				window_extents.x + window_extents.width // 2,
				window_extents.y + window_extents.height // 2,
				Atspi.CoordType.SCREEN,
			),
		)
		window_states = window.get_state_set()
		if (
			window_states.contains(Atspi.StateType.SHOWING)
			and window_extents.width > 0
			and window_extents.height > 0
		):
			check(hit is not None, "component hit test failed for a showing window")

		mutate = find_named(nodes, "Replace benchmark model")
		model_changed.clear()
		model_change_count[0] = 0
		start = time.perf_counter()
		check(
			mutate.do_action(action_index(mutate, "click")),
			"benchmark model replacement action failed",
		)
		check(model_changed.wait(args.timeout), "model replacement event was not received")
		while benchmark_list.get_child_count() != item_count and time.monotonic() - start < args.timeout:
			time.sleep(0.001)
		mutation_ms = (time.perf_counter() - start) * 1000
		first_item = benchmark_list.get_child_at_index(0)
		check(first_item.get_name() == "Updated benchmark item 0", "model replacement stayed stale")
		check(model_change_count[0] == 1, "model-change burst was not coalesced")

		return {
			"nodes": len(nodes),
			"item_count": item_count,
			"discovery_ms": discovery_ms,
			"traversal_ms": traversal_ms,
			"property_batch_ms": property_batch_ms,
			"mutation_ms": mutation_ms,
			"model_change_events": model_change_count[0],
			**measurements.summary(),
		}
	finally:
		target_process_id[0] = None
		if process:
			process.terminate()
			try:
				process.wait(timeout=1)
			except subprocess.TimeoutExpired:
				process.kill()
				process.wait()


def median_result(samples):
	result = {"item_count": samples[0]["item_count"], "iterations": len(samples)}
	for key in samples[0]:
		if key not in {"item_count", "model_change_events"}:
			result[key] = statistics.median(sample[key] for sample in samples)
	result["model_change_events"] = max(sample["model_change_events"] for sample in samples)
	result["traversal_ms_per_1k"] = result["traversal_ms"] * 1000 / result["nodes"]
	result["mutation_ms_per_1k"] = result["mutation_ms"] * 1000 / result["item_count"]
	return result


def parse_item_counts(value):
	counts = sorted(set(int(count) for count in value.split(",")))
	check(counts and counts[0] > 0, "item counts must be positive")
	return counts


def main():
	parser = argparse.ArgumentParser(description="Benchmark eepp's AT-SPI query path")
	parser.add_argument("--application", default="eepp - Accessibility")
	parser.add_argument("--executable", default="bin/eepp-ui-accessibility")
	parser.add_argument("--timeout", type=float, default=5.0)
	parser.add_argument("--sample-size", type=int, default=100)
	parser.add_argument("--item-counts", default="250,1000,4000")
	parser.add_argument("--iterations", type=int, default=3)
	parser.add_argument("--max-discovery-ms", type=float, default=2000)
	parser.add_argument("--max-traversal-ms-per-1k", type=float, default=2000)
	parser.add_argument("--max-property-batch-ms", type=float, default=500)
	parser.add_argument("--max-mutation-ms-per-1k", type=float, default=1000)
	parser.add_argument("--max-request-ms", type=float, default=250)
	parser.add_argument("--max-p95-ms", type=float, default=5)
	parser.add_argument("--max-scaling-ratio", type=float, default=2.5)
	parser.add_argument("--json", action="store_true")
	args = parser.parse_args()
	check(args.iterations > 0, "iterations must be positive")
	item_counts = parse_item_counts(args.item_counts)

	model_changed = threading.Event()
	model_change_count = [0]
	target_process_id = [None]

	def on_model_changed(event):
		try:
			if event.source.get_process_id() == target_process_id[0]:
				model_change_count[0] += 1
				model_changed.set()
		except Exception:
			pass

	listener = Atspi.EventListener.new(on_model_changed)
	check(listener.register("object:model-changed"), "could not register model-change listener")
	event_thread = threading.Thread(target=Atspi.event_main, daemon=True)
	event_thread.start()

	try:
		results = []
		for item_count in item_counts:
			samples = [
				run_sample(
					args, item_count, target_process_id, model_changed, model_change_count
				)
				for _ in range(args.iterations)
			]
			results.append(median_result(samples))

		for result in results:
			check(result["discovery_ms"] <= args.max_discovery_ms, "application discovery exceeded threshold")
			check(
				result["traversal_ms_per_1k"] <= args.max_traversal_ms_per_1k,
				"normalized tree traversal exceeded threshold",
			)
			check(result["property_batch_ms"] <= args.max_property_batch_ms, "property batch exceeded threshold")
			check(
				result["mutation_ms_per_1k"] <= args.max_mutation_ms_per_1k,
				"normalized model mutation exceeded threshold",
			)
			check(result["maximum_ms"] <= args.max_request_ms, "an individual accessibility request exceeded threshold")
			check(result["p95_ms"] <= args.max_p95_ms, "p95 accessibility request latency exceeded threshold")

		baseline_cost = results[0]["traversal_ms_per_1k"]
		scaling_ratio = max(
			result["traversal_ms_per_1k"] / baseline_cost for result in results[1:]
		) if len(results) > 1 else 1
		check(scaling_ratio <= args.max_scaling_ratio, "tree traversal scaling became nonlinear")
		output = {"worst_scaling_ratio": scaling_ratio, "samples": results}
		if args.json:
			print(json.dumps(output, sort_keys=True))
		else:
			for result in results:
				print(
					f"{result['item_count']} items: {result['nodes']:.0f} nodes, "
					f"traversal {result['traversal_ms']:.1f}ms "
					f"({result['traversal_ms_per_1k']:.1f}ms/1k), "
					f"p95 {result['p95_ms']:.3f}ms, maximum {result['maximum_ms']:.3f}ms"
				)
			print(f"AT-SPI benchmark passed: worst normalized growth {scaling_ratio:.2f}x")
	finally:
		listener.deregister("object:model-changed")
		Atspi.event_quit()
		event_thread.join(timeout=1)


if __name__ == "__main__":
	main()
