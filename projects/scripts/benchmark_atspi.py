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


def main():
	parser = argparse.ArgumentParser(description="Benchmark eepp's AT-SPI query path")
	parser.add_argument("--application", default="eepp - Accessibility")
	parser.add_argument("--executable", default="bin/eepp-ui-accessibility")
	parser.add_argument("--timeout", type=float, default=5.0)
	parser.add_argument("--sample-size", type=int, default=100)
	parser.add_argument("--max-discovery-ms", type=float, default=1000)
	parser.add_argument("--max-traversal-ms", type=float, default=1500)
	parser.add_argument("--max-property-batch-ms", type=float, default=250)
	parser.add_argument("--max-mutation-ms", type=float, default=250)
	parser.add_argument("--max-request-ms", type=float, default=100)
	parser.add_argument("--json", action="store_true")
	args = parser.parse_args()

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

	process = None
	try:
		start = time.perf_counter()
		process = subprocess.Popen([args.executable, "--benchmark"])
		target_process_id[0] = process.pid
		application = find_application(args.application, process.pid, args.timeout)
		discovery_ms = (time.perf_counter() - start) * 1000

		measurements = Measurements()
		start = time.perf_counter()
		nodes = traverse(application, measurements)
		traversal_ms = (time.perf_counter() - start) * 1000
		check(len(nodes) >= 1050, f"benchmark tree is incomplete: {len(nodes)} nodes")
		find_named(nodes, "Benchmark depth leaf")
		benchmark_list = find_named(nodes, "Benchmark items")
		check(benchmark_list.get_child_count() == 1000, "benchmark list must expose 1000 items")

		start = time.perf_counter()
		for node in nodes[: args.sample_size]:
			measurements.call("description", node.get_description)
			measurements.call("state", node.get_state_set)
			measurements.call("attributes", node.get_attributes)
			measurements.call("extents", lambda node=node: node.get_extents(Atspi.CoordType.SCREEN))
		property_batch_ms = (time.perf_counter() - start) * 1000

		mutate = find_named(nodes, "Replace benchmark model")
		model_changed.clear()
		start = time.perf_counter()
		check(
			mutate.do_action(action_index(mutate, "click")),
			"benchmark model replacement action failed",
		)
		check(model_changed.wait(args.timeout), "model replacement event was not received")
		while benchmark_list.get_child_count() != 1000 and time.monotonic() - start < args.timeout:
			time.sleep(0.001)
		mutation_ms = (time.perf_counter() - start) * 1000
		first_item = benchmark_list.get_child_at_index(0)
		check(first_item.get_name() == "Updated benchmark item 0", "model replacement stayed stale")
		check(model_change_count[0] == 1, "model-change burst was not coalesced")

		request_summary = measurements.summary()
		result = {
			"nodes": len(nodes),
			"discovery_ms": discovery_ms,
			"traversal_ms": traversal_ms,
			"property_batch_ms": property_batch_ms,
			"mutation_ms": mutation_ms,
			"model_change_events": model_change_count[0],
			**request_summary,
		}
		check(discovery_ms <= args.max_discovery_ms, "application discovery exceeded threshold")
		check(traversal_ms <= args.max_traversal_ms, "tree traversal exceeded threshold")
		check(property_batch_ms <= args.max_property_batch_ms, "property batch exceeded threshold")
		check(mutation_ms <= args.max_mutation_ms, "model mutation exceeded threshold")
		check(
			request_summary["maximum_ms"] <= args.max_request_ms,
			"an individual accessibility request exceeded threshold",
		)
		if args.json:
			print(json.dumps(result, sort_keys=True))
		else:
			print(
				"AT-SPI benchmark passed: "
				f"{result['nodes']} nodes, {result['calls']} calls, "
				f"discovery {discovery_ms:.1f}ms, traversal {traversal_ms:.1f}ms, "
				f"properties {property_batch_ms:.1f}ms, mutation {mutation_ms:.1f}ms, "
				f"median {result['median_ms']:.3f}ms, p95 {result['p95_ms']:.3f}ms, "
				f"maximum {result['maximum_ms']:.3f}ms"
			)
	finally:
		listener.deregister("object:model-changed")
		Atspi.event_quit()
		event_thread.join(timeout=1)
		if process:
			process.terminate()
			try:
				process.wait(timeout=1)
			except subprocess.TimeoutExpired:
				process.kill()
				process.wait()


if __name__ == "__main__":
	main()
