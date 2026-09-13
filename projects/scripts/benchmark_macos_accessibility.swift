#!/usr/bin/env swift

import ApplicationServices
import Darwin
import Foundation

struct BenchmarkFailure: Error, CustomStringConvertible {
	let description: String
}

struct Arguments {
	var executable = "bin/eepp-ui-accessibility"
	var items = 1_000
	var traversals = 5
	var timeout: TimeInterval = 15
}

func fail(_ message: String) throws -> Never {
	throw BenchmarkFailure(description: message)
}

func parseArguments() throws -> Arguments {
	var result = Arguments()
	var index = 1
	while index < CommandLine.arguments.count {
		switch CommandLine.arguments[index] {
		case "--executable":
			index += 1
			guard index < CommandLine.arguments.count else { try fail("missing --executable value") }
			result.executable = CommandLine.arguments[index]
		case "--items":
			index += 1
			guard index < CommandLine.arguments.count,
				let value = Int(CommandLine.arguments[index]), value > 0
			else { try fail("invalid --items value") }
			result.items = value
		case "--traversals":
			index += 1
			guard index < CommandLine.arguments.count,
				let value = Int(CommandLine.arguments[index]), value > 0
			else { try fail("invalid --traversals value") }
			result.traversals = value
		case "--timeout":
			index += 1
			guard index < CommandLine.arguments.count,
				let value = TimeInterval(CommandLine.arguments[index]), value > 0
			else { try fail("invalid --timeout value") }
			result.timeout = value
		default:
			try fail("unknown argument: \(CommandLine.arguments[index])")
		}
		index += 1
	}
	return result
}

func elapsedMilliseconds(since start: UInt64) -> Double {
	Double(DispatchTime.now().uptimeNanoseconds - start) / 1_000_000
}

func optionalAttribute(_ element: AXUIElement, _ attribute: CFString) -> CFTypeRef? {
	var value: CFTypeRef?
	return AXUIElementCopyAttributeValue(element, attribute, &value) == .success ? value : nil
}

func elementsAttribute(_ element: AXUIElement, _ attribute: CFString) -> [AXUIElement] {
	optionalAttribute(element, attribute) as? [AXUIElement] ?? []
}

func stringAttribute(_ element: AXUIElement, _ attribute: CFString) -> String? {
	optionalAttribute(element, attribute) as? String
}

func descendants(_ root: AXUIElement, limit: Int) -> [AXUIElement] {
	var result: [AXUIElement] = []
	var pending = elementsAttribute(root, kAXChildrenAttribute as CFString)
	while !pending.isEmpty && result.count < limit {
		let element = pending.removeFirst()
		result.append(element)
		pending.append(contentsOf: elementsAttribute(element, kAXChildrenAttribute as CFString))
	}
	return result
}

func findElement(in root: AXUIElement, role: String, name: String, limit: Int) -> AXUIElement? {
	for element in descendants(root, limit: limit) {
		guard stringAttribute(element, kAXRoleAttribute as CFString) == role else { continue }
		let names = [
			stringAttribute(element, kAXTitleAttribute as CFString),
			stringAttribute(element, kAXDescriptionAttribute as CFString),
			stringAttribute(element, kAXHelpAttribute as CFString),
			stringAttribute(element, kAXValueAttribute as CFString),
		]
		if names.contains(where: { $0 == name }) { return element }
	}
	return nil
}

func waitFor<T>(
	_ description: String, timeout: TimeInterval, process: Process, operation: () -> T?
) throws -> T {
	let deadline = Date().addingTimeInterval(timeout)
	repeat {
		if let result = operation() { return result }
		if !process.isRunning {
			try fail("process exited with status \(process.terminationStatus) while waiting for \(description)")
		}
		CFRunLoopRunInMode(CFRunLoopMode.defaultMode, 0.01, true)
	} while Date() < deadline
	try fail("timed out waiting for \(description)")
}

func statistics(_ values: [Double]) -> [String: Any] {
	let sorted = values.sorted()
	let median: Double
	if sorted.count.isMultiple(of: 2) {
		median = (sorted[sorted.count / 2 - 1] + sorted[sorted.count / 2]) / 2
	} else {
		median = sorted[sorted.count / 2]
	}
	let p95Index = min(sorted.count - 1, max(0, Int(ceil(Double(sorted.count) * 0.95)) - 1))
	return [
		"count": sorted.count,
		"median": median,
		"p95": sorted[p95Index],
		"maximum": sorted.last!,
	]
}

func setAttribute(_ element: AXUIElement, _ attribute: CFString, _ value: CFTypeRef) throws {
	let error = AXUIElementSetAttributeValue(element, attribute, value)
	if error != .success {
		try fail("setting \(attribute) failed with AX error \(error.rawValue)")
	}
}

func perform(_ element: AXUIElement, _ action: CFString) throws {
	let error = AXUIElementPerformAction(element, action)
	if error != .success {
		try fail("performing \(action) failed with AX error \(error.rawValue)")
	}
}

func run() throws {
	guard AXIsProcessTrusted() else {
		try fail("the benchmark requires macOS Accessibility permission")
	}
	let arguments = try parseArguments()
	let executableURL = URL(fileURLWithPath: arguments.executable).standardizedFileURL
	guard FileManager.default.isExecutableFile(atPath: executableURL.path) else {
		try fail("benchmark executable is not available at \(executableURL.path)")
	}

	let process = Process()
	process.executableURL = executableURL
	process.arguments = [
		"--benchmark", "--benchmark-visible", "--benchmark-items", String(arguments.items),
	]
	var environment = ProcessInfo.processInfo.environment
	let executableDirectory = executableURL.deletingLastPathComponent().path
	if let existingLibraryPath = environment["DYLD_LIBRARY_PATH"], !existingLibraryPath.isEmpty {
		environment["DYLD_LIBRARY_PATH"] = "\(executableDirectory):\(existingLibraryPath)"
	} else {
		environment["DYLD_LIBRARY_PATH"] = executableDirectory
	}
	process.environment = environment
	let startup = DispatchTime.now().uptimeNanoseconds
	try process.run()
	defer {
		if process.isRunning {
			process.terminate()
			let deadline = Date().addingTimeInterval(2)
			while process.isRunning && Date() < deadline { Thread.sleep(forTimeInterval: 0.02) }
			if process.isRunning { Darwin.kill(process.processIdentifier, SIGKILL) }
		}
	}

	let application = AXUIElementCreateApplication(process.processIdentifier)
	AXUIElementSetMessagingTimeout(application, 2)
	let window: AXUIElement = try waitFor(
		"visible benchmark window", timeout: arguments.timeout, process: process
	) {
		elementsAttribute(application, kAXWindowsAttribute as CFString).first
	}
	let discoveryMilliseconds = elapsedMilliseconds(since: startup)
	let searchLimit = max(4_096, arguments.items * 2)
	guard let list = findElement(
		in: window, role: kAXListRole, name: "Benchmark items", limit: searchLimit)
	else { try fail("could not find benchmark list") }

	var rowEnumerationMilliseconds: [Double] = []
	var valueQueryMicroseconds: [Double] = []
	for _ in 0..<arguments.traversals {
		let enumerationStart = DispatchTime.now().uptimeNanoseconds
		let rows = elementsAttribute(list, kAXRowsAttribute as CFString)
		rowEnumerationMilliseconds.append(elapsedMilliseconds(since: enumerationStart))
		guard rows.count == arguments.items else {
			try fail("expected \(arguments.items) rows, received \(rows.count)")
		}
		for row in rows {
			let queryStart = DispatchTime.now().uptimeNanoseconds
			guard stringAttribute(row, kAXValueAttribute as CFString) != nil else {
				try fail("benchmark row did not expose a value")
			}
			valueQueryMicroseconds.append(elapsedMilliseconds(since: queryStart) * 1_000)
		}
	}

	let traversalStart = DispatchTime.now().uptimeNanoseconds
	let allElements = descendants(window, limit: searchLimit)
	let completeTraversalMilliseconds = elapsedMilliseconds(since: traversalStart)
	guard allElements.count >= arguments.items else {
		try fail("semantic traversal ended early at \(allElements.count) elements")
	}

	guard let projectName = findElement(
		in: window, role: kAXTextFieldRole, name: "Project name", limit: searchLimit)
	else { try fail("could not find focus benchmark target") }
	let focusStart = DispatchTime.now().uptimeNanoseconds
	try setAttribute(projectName, kAXFocusedAttribute as CFString, kCFBooleanTrue)
	let _: Bool = try waitFor(
		"focused element", timeout: arguments.timeout, process: process
	) {
		guard let focused = optionalAttribute(application, kAXFocusedUIElementAttribute as CFString)
		else { return nil }
		return CFEqual(focused, projectName) ? true : nil
	}
	let focusMilliseconds = elapsedMilliseconds(since: focusStart)

	guard let slider = findElement(
		in: window, role: kAXSliderRole, name: "Volume", limit: searchLimit)
	else { try fail("could not find action benchmark target") }
	let actionStart = DispatchTime.now().uptimeNanoseconds
	try setAttribute(slider, kAXValueAttribute as CFString, 60 as CFNumber)
	let _: Bool = try waitFor(
		"slider value", timeout: arguments.timeout, process: process
	) {
		(optionalAttribute(slider, kAXValueAttribute as CFString) as? NSNumber)?.intValue == 60
			? true : nil
	}
	let actionMilliseconds = elapsedMilliseconds(since: actionStart)

	guard let replaceModel = findElement(
		in: window, role: kAXButtonRole, name: "Replace benchmark model", limit: searchLimit)
	else { try fail("could not find model replacement benchmark action") }
	let modelStart = DispatchTime.now().uptimeNanoseconds
	try perform(replaceModel, kAXPressAction as CFString)
	let _: Bool = try waitFor(
		"replacement model", timeout: arguments.timeout, process: process
	) {
		let rows = elementsAttribute(list, kAXRowsAttribute as CFString)
		return rows.count == arguments.items
			&& stringAttribute(rows[0], kAXValueAttribute as CFString)
				== "Updated benchmark item 0" ? true : nil
	}
	let modelReplacementMilliseconds = elapsedMilliseconds(since: modelStart)

	guard let openWindow = findElement(
		in: window, role: kAXButtonRole, name: "Open accessibility window", limit: searchLimit)
	else { try fail("could not find dynamic window benchmark action") }
	let createWindowStart = DispatchTime.now().uptimeNanoseconds
	try perform(openWindow, kAXPressAction as CFString)
	let dynamicWindow: AXUIElement = try waitFor(
		"dynamic window", timeout: arguments.timeout, process: process
	) {
		elementsAttribute(application, kAXWindowsAttribute as CFString).first {
			stringAttribute($0, kAXTitleAttribute as CFString) == "eepp - Accessibility Dynamic"
		}
	}
	let createWindowMilliseconds = elapsedMilliseconds(since: createWindowStart)
	let closeWindow: AXUIElement = try waitFor(
		"dynamic window semantic hierarchy", timeout: arguments.timeout, process: process
	) {
		findElement(
			in: dynamicWindow, role: kAXButtonRole, name: "Close dynamic window", limit: 128)
	}
	let closeWindowStart = DispatchTime.now().uptimeNanoseconds
	try perform(closeWindow, kAXPressAction as CFString)
	let _: Bool = try waitFor(
		"dynamic window removal", timeout: arguments.timeout, process: process
	) {
		elementsAttribute(application, kAXWindowsAttribute as CFString).count == 1 ? true : nil
	}
	let closeWindowMilliseconds = elapsedMilliseconds(since: closeWindowStart)

	let output: [String: Any] = [
		"items": arguments.items,
		"traversals": arguments.traversals,
		"discovery_ms": discoveryMilliseconds,
		"complete_traversal_ms": completeTraversalMilliseconds,
		"element_count": allElements.count,
		"row_enumeration_ms": statistics(rowEnumerationMilliseconds),
		"row_value_query_us": statistics(valueQueryMicroseconds),
		"focus_ms": focusMilliseconds,
		"value_action_ms": actionMilliseconds,
		"model_replacement_ms": modelReplacementMilliseconds,
		"window_create_ms": createWindowMilliseconds,
		"window_close_ms": closeWindowMilliseconds,
		"raw_row_enumeration_ms": rowEnumerationMilliseconds,
		"raw_row_value_query_us": valueQueryMicroseconds,
	]
	let data = try JSONSerialization.data(withJSONObject: output, options: [.sortedKeys])
	print(String(decoding: data, as: UTF8.self))
}

do {
	try run()
} catch {
	FileHandle.standardError.write(Data("macOS accessibility benchmark failed: \(error)\n".utf8))
	exit(EXIT_FAILURE)
}
