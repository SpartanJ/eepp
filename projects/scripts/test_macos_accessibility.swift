#!/usr/bin/env swift

import ApplicationServices
import Darwin
import Foundation

struct HarnessFailure: Error, CustomStringConvertible {
	let description: String
}

struct RecordedNotification {
	let element: AXUIElement
	let name: String
}

final class NotificationRecorder {
	var events: [RecordedNotification] = []

	func contains(_ name: CFString, target: AXUIElement, since index: Int) -> Bool {
		count(name, target: target, since: index) > 0
	}

	func count(_ name: CFString, target: AXUIElement, since index: Int) -> Int {
		events.dropFirst(index).filter {
			$0.name == name as String && CFEqual($0.element, target)
		}.count
	}
}

let notificationCallback: AXObserverCallback = { _, element, notification, context in
	guard let context else { return }
	let recorder = Unmanaged<NotificationRecorder>.fromOpaque(context).takeUnretainedValue()
	recorder.events.append(RecordedNotification(element: element, name: notification as String))
}

func progress(_ message: String) {
	FileHandle.standardError.write(Data("[macOS accessibility] \(message)\n".utf8))
}

func fail(_ message: String) throws -> Never {
	throw HarnessFailure(description: message)
}

func check(_ condition: @autoclosure () -> Bool, _ message: String) throws {
	if !condition() {
		try fail(message)
	}
}

func optionalAttribute(_ element: AXUIElement, _ attribute: CFString) -> CFTypeRef? {
	var value: CFTypeRef?
	return AXUIElementCopyAttributeValue(element, attribute, &value) == .success ? value : nil
}

func stringAttribute(_ element: AXUIElement, _ attribute: CFString) -> String? {
	optionalAttribute(element, attribute) as? String
}

func numberAttribute(_ element: AXUIElement, _ attribute: CFString) -> NSNumber? {
	optionalAttribute(element, attribute) as? NSNumber
}

func elementsAttribute(_ element: AXUIElement, _ attribute: CFString) -> [AXUIElement] {
	optionalAttribute(element, attribute) as? [AXUIElement] ?? []
}

func elementsAttributeResult(
	_ element: AXUIElement, _ attribute: CFString
) -> ([AXUIElement], AXError) {
	var value: CFTypeRef?
	let error = AXUIElementCopyAttributeValue(element, attribute, &value)
	return (value as? [AXUIElement] ?? [], error)
}

func pointAttribute(_ element: AXUIElement, _ attribute: CFString) -> CGPoint? {
	guard let value = optionalAttribute(element, attribute),
		CFGetTypeID(value) == AXValueGetTypeID()
	else {
		return nil
	}
	var point = CGPoint.zero
	return AXValueGetValue(value as! AXValue, .cgPoint, &point) ? point : nil
}

func sizeAttribute(_ element: AXUIElement, _ attribute: CFString) -> CGSize? {
	guard let value = optionalAttribute(element, attribute),
		CFGetTypeID(value) == AXValueGetTypeID()
	else {
		return nil
	}
	var size = CGSize.zero
	return AXValueGetValue(value as! AXValue, .cgSize, &size) ? size : nil
}

func rangeAttribute(_ element: AXUIElement, _ attribute: CFString) -> CFRange? {
	guard let value = optionalAttribute(element, attribute),
		CFGetTypeID(value) == AXValueGetTypeID()
	else {
		return nil
	}
	var range = CFRange()
	return AXValueGetValue(value as! AXValue, .cfRange, &range) ? range : nil
}

func rangeValue(location: Int, length: Int) throws -> AXValue {
	var range = CFRange(location: location, length: length)
	guard let value = AXValueCreate(.cfRange, &range) else {
		try fail("could not create AX range value")
	}
	return value
}

func rangeEquals(_ range: CFRange?, location: Int, length: Int) -> Bool {
	range?.location == location && range?.length == length
}

func parameterizedAttribute(
	_ element: AXUIElement, _ attribute: CFString, _ parameter: CFTypeRef
) throws -> CFTypeRef {
	var value: CFTypeRef?
	let error = AXUIElementCopyParameterizedAttributeValue(
		element, attribute, parameter, &value)
	guard error == .success, let value else {
		try fail("querying \(attribute) failed with AX error \(error.rawValue)")
	}
	return value
}

func observe(
	_ observer: AXObserver, _ element: AXUIElement, _ notification: CFString,
	_ recorder: NotificationRecorder
) throws {
	let error = AXObserverAddNotification(
		observer, element, notification, Unmanaged.passUnretained(recorder).toOpaque())
	if error != .success {
		try fail("observing \(notification) failed with AX error \(error.rawValue)")
	}
}

func checkSingleNotification(
	_ recorder: NotificationRecorder, _ notification: CFString, target: AXUIElement,
	since index: Int
) throws {
	let deadline = Date().addingTimeInterval(0.1)
	repeat {
		CFRunLoopRunInMode(CFRunLoopMode.defaultMode, 0.01, true)
	} while Date() < deadline
	let count = recorder.count(notification, target: target, since: index)
	try check(
		count == 1,
		"expected one \(notification) notification for \(describe(target)), received \(count)")
}

func descendants(_ root: AXUIElement, limit: Int = 4_096) -> [AXUIElement] {
	var result: [AXUIElement] = []
	var pending = elementsAttribute(root, kAXChildrenAttribute as CFString)
	while !pending.isEmpty && result.count < limit {
		let element = pending.removeFirst()
		result.append(element)
		pending.append(contentsOf: elementsAttribute(element, kAXChildrenAttribute as CFString))
	}
	return result
}

func findElement(in root: AXUIElement, role: String, name: String) -> AXUIElement? {
	for element in descendants(root) {
		guard stringAttribute(element, kAXRoleAttribute as CFString) == role else { continue }
		let names = [
			stringAttribute(element, kAXTitleAttribute as CFString),
			stringAttribute(element, kAXDescriptionAttribute as CFString),
			stringAttribute(element, kAXHelpAttribute as CFString),
			stringAttribute(element, kAXValueAttribute as CFString),
		]
		if names.contains(where: { $0 == name }) {
			return element
		}
	}
	return nil
}

func describe(_ element: AXUIElement?) -> String {
	guard let element else { return "nil" }
	let attributes: [(String, CFString)] = [
		("role", kAXRoleAttribute as CFString),
		("title", kAXTitleAttribute as CFString),
		("description", kAXDescriptionAttribute as CFString),
		("identifier", kAXIdentifierAttribute as CFString),
	]
	return attributes.map { name, attribute in
		"\(name)=\(stringAttribute(element, attribute) ?? "nil")"
	}.joined(separator: " ")
}

func waitFor<T>(
	_ description: String, timeout: TimeInterval, operation: () -> T?
) throws -> T {
	let deadline = Date().addingTimeInterval(timeout)
	repeat {
		if let result = operation() {
			return result
		}
		CFRunLoopRunInMode(CFRunLoopMode.defaultMode, 0.05, true)
	} while Date() < deadline
	try fail("timed out waiting for \(description)")
}

func actionNames(_ element: AXUIElement) throws -> [String] {
	var names: CFArray?
	let error = AXUIElementCopyActionNames(element, &names)
	guard error == .success else {
		try fail("copying action names failed with AX error \(error.rawValue)")
	}
	return names as? [String] ?? []
}

func perform(_ element: AXUIElement, _ action: CFString) throws {
	let error = AXUIElementPerformAction(element, action)
	if error != .success {
		try fail("\(action) failed with AX error \(error.rawValue)")
	}
}

func stressQueriesDuringAction(
	staleElement: AXUIElement, actionElement: AXUIElement, timeout: TimeInterval
) throws {
	AXUIElementSetMessagingTimeout(staleElement, 0.25)
	let started = DispatchSemaphore(value: 0)
	let completed = DispatchGroup()
	let resultLock = NSLock()
	var unexpectedErrors: [Int32] = []
	completed.enter()
	DispatchQueue.global(qos: .userInitiated).async {
		defer { completed.leave() }
		for index in 0..<64 {
			var role: CFTypeRef?
			let roleError = AXUIElementCopyAttributeValue(
				staleElement, kAXRoleAttribute as CFString, &role)
			var children: CFTypeRef?
			let childrenError = AXUIElementCopyAttributeValue(
				staleElement, kAXChildrenAttribute as CFString, &children)
			if index == 0 { started.signal() }
			for error in [roleError, childrenError] {
				if error != .success && error != .invalidUIElement && error != .cannotComplete
					&& error != .noValue && error != .attributeUnsupported
				{
					resultLock.lock()
					unexpectedErrors.append(error.rawValue)
					resultLock.unlock()
				}
			}
			usleep(1_000)
		}
	}
	guard started.wait(timeout: .now() + 1) == .success else {
		try fail("concurrent AX query worker did not start")
	}
	try perform(actionElement, kAXPressAction as CFString)
	guard completed.wait(timeout: .now() + timeout) == .success else {
		try fail("concurrent AX queries did not finish after the destructive action")
	}
	resultLock.lock()
	let errors = unexpectedErrors
	resultLock.unlock()
	try check(errors.isEmpty, "concurrent AX queries returned unexpected errors: \(errors)")
}

func setAttribute(_ element: AXUIElement, _ attribute: CFString, _ value: CFTypeRef) throws {
	let error = AXUIElementSetAttributeValue(element, attribute, value)
	if error != .success {
		try fail("setting \(attribute) failed with AX error \(error.rawValue)")
	}
}

struct Arguments {
	var executable = "bin/eepp-ui-accessibility"
	var timeout: TimeInterval = 10
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
		case "--timeout":
			index += 1
			guard index < CommandLine.arguments.count,
				let timeout = TimeInterval(CommandLine.arguments[index]), timeout > 0
			else {
				try fail("invalid --timeout value")
			}
			result.timeout = timeout
		default:
			try fail("unknown argument: \(CommandLine.arguments[index])")
		}
		index += 1
	}
	return result
}

func run() throws {
	try check(AXIsProcessTrusted(), "the harness requires macOS Accessibility permission")
	let arguments = try parseArguments()
	let executableURL = URL(fileURLWithPath: arguments.executable).standardizedFileURL
	try check(
		FileManager.default.isExecutableFile(atPath: executableURL.path),
		"accessibility example is not executable at \(executableURL.path)")

	let process = Process()
	process.executableURL = executableURL
	// This mode also selects QuitPolicy::OnLastWindowClosed, which is required to exercise
	// primary-window removal while the secondary window remains alive.
	process.arguments = ["--close-primary"]
	var environment = ProcessInfo.processInfo.environment
	let executableDirectory = executableURL.deletingLastPathComponent().path
	if let existing = environment["DYLD_LIBRARY_PATH"], !existing.isEmpty {
		environment["DYLD_LIBRARY_PATH"] = "\(executableDirectory):\(existing)"
	} else {
		environment["DYLD_LIBRARY_PATH"] = executableDirectory
	}
	process.environment = environment
	try process.run()
	defer {
		if process.isRunning {
			process.terminate()
			let deadline = Date().addingTimeInterval(2)
			while process.isRunning && Date() < deadline {
				Thread.sleep(forTimeInterval: 0.05)
			}
			if process.isRunning {
				Darwin.kill(process.processIdentifier, SIGKILL)
			}
		}
	}

	let application = AXUIElementCreateApplication(process.processIdentifier)
	AXUIElementSetMessagingTimeout(application, 2)
	progress("waiting for application windows")
	var lastWindowStatus = ""
	let windows: [AXUIElement] = try waitFor(
		"two accessibility windows", timeout: arguments.timeout
	) {
		let (found, error) = elementsAttributeResult(application, kAXWindowsAttribute as CFString)
		let status = "AXWindows error=\(error.rawValue) count=\(found.count)"
		if status != lastWindowStatus {
			progress(status)
			lastWindowStatus = status
		}
		return found.count == 2 ? found : nil
	}
	try check(
		stringAttribute(application, kAXRoleAttribute as CFString) == kAXApplicationRole,
		"application role was not AXApplication")
	let windowsByTitle = Dictionary(
		uniqueKeysWithValues: windows.map {
			(stringAttribute($0, kAXTitleAttribute as CFString) ?? "", $0)
		})
	try check(
		Set(windowsByTitle.keys)
			== Set(["eepp - Accessibility", "eepp - Accessibility Secondary"]),
		"window titles were incorrect")

	progress("checking hierarchy and actions")
	guard let primary = windowsByTitle["eepp - Accessibility"] else {
		try fail("could not find the primary window by title")
	}
	let recorder = NotificationRecorder()
	var observer: AXObserver?
	let observerError = AXObserverCreate(
		process.processIdentifier, notificationCallback, &observer)
	guard observerError == .success, let observer else {
		try fail("creating AX notification observer failed with AX error \(observerError.rawValue)")
	}
	let observerSource = AXObserverGetRunLoopSource(observer)
	CFRunLoopAddSource(CFRunLoopGetCurrent(), observerSource, CFRunLoopMode.defaultMode)
	defer {
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), observerSource, CFRunLoopMode.defaultMode)
	}
	guard let save = findElement(in: primary, role: kAXButtonRole, name: "Save settings") else {
		try fail("could not find Save settings button")
	}
	let saveActions = try actionNames(save)
	try check(saveActions.contains(kAXPressAction), "save button did not expose AXPress")
	try perform(save, kAXPressAction as CFString)
	guard let metadataButton = findElement(
		in: primary, role: kAXButtonRole, name: "Update accessible metadata")
	else {
		try fail("could not find accessible-metadata update button")
	}
	try observe(observer, metadataButton, kAXTitleChangedNotification as CFString, recorder)
	try observe(observer, metadataButton, kAXLayoutChangedNotification as CFString, recorder)
	let metadataEventStart = recorder.events.count
	try perform(metadataButton, kAXPressAction as CFString)
	try check(
		stringAttribute(metadataButton, kAXDescriptionAttribute as CFString)
			== "Updated metadata control",
		"updated accessible name was not exposed as AXDescription")
	try check(
		stringAttribute(metadataButton, kAXHelpAttribute as CFString)
			== "Updated accessibility help",
		"updated accessible help was incorrect")
	let _: Bool = try waitFor("metadata title notification", timeout: arguments.timeout) {
		recorder.contains(
			kAXTitleChangedNotification as CFString, target: metadataButton,
			since: metadataEventStart) ? true : nil
	}
	let _: Bool = try waitFor("metadata help notification", timeout: arguments.timeout) {
		recorder.contains(
			kAXLayoutChangedNotification as CFString, target: metadataButton,
			since: metadataEventStart) ? true : nil
	}
	try checkSingleNotification(
		recorder, kAXTitleChangedNotification as CFString, target: metadataButton,
		since: metadataEventStart)
	try checkSingleNotification(
		recorder, kAXLayoutChangedNotification as CFString, target: metadataButton,
		since: metadataEventStart)

	guard let checkbox = findElement(
		in: primary, role: kAXCheckBoxRole, name: "Enable autosave")
	else {
		try fail("could not find autosave checkbox")
	}
	try check(
		numberAttribute(checkbox, kAXValueAttribute as CFString)?.intValue == 0,
		"checkbox initial value was not unchecked")
	try observe(observer, checkbox, kAXValueChangedNotification as CFString, recorder)
	let checkboxEventStart = recorder.events.count
	try perform(checkbox, kAXPressAction as CFString)
	try check(
		numberAttribute(checkbox, kAXValueAttribute as CFString)?.intValue == 1,
		"checkbox did not toggle through AXPress")
	let _: Bool = try waitFor("checkbox value notification", timeout: arguments.timeout) {
		recorder.contains(
			kAXValueChangedNotification as CFString, target: checkbox, since: checkboxEventStart)
			? true : nil
	}
	try checkSingleNotification(
		recorder, kAXValueChangedNotification as CFString, target: checkbox,
		since: checkboxEventStart)

	guard let projectName = findElement(
		in: primary, role: kAXTextFieldRole, name: "Project name")
	else {
		try fail("could not find Project name text field")
	}
	try check(
		stringAttribute(projectName, kAXValueAttribute as CFString) == "eepp",
		"text field value was incorrect")
	try setAttribute(projectName, kAXValueAttribute as CFString, "A😀e\u{301}" as CFString)
	try check(
		stringAttribute(projectName, kAXValueAttribute as CFString) == "A😀e\u{301}",
		"text field value did not update")
	try observe(
		observer, projectName, kAXSelectedTextChangedNotification as CFString, recorder)
	let textSelectionEventStart = recorder.events.count
	try setAttribute(
		projectName, kAXSelectedTextRangeAttribute as CFString,
		try rangeValue(location: 1, length: 4))
	try check(
		rangeEquals(
			rangeAttribute(projectName, kAXSelectedTextRangeAttribute as CFString), location: 1,
			length: 4),
		"Unicode text selection did not round-trip as UTF-16")
	let _: Bool = try waitFor("text selection notification", timeout: arguments.timeout) {
		recorder.contains(
			kAXSelectedTextChangedNotification as CFString, target: projectName,
			since: textSelectionEventStart) ? true : nil
	}
	try checkSingleNotification(
		recorder, kAXSelectedTextChangedNotification as CFString, target: projectName,
		since: textSelectionEventStart)
	try check(
		numberAttribute(projectName, kAXNumberOfCharactersAttribute as CFString)?.intValue == 5,
		"Unicode text field did not report its UTF-16 character count")
	let emojiRangeValue = try parameterizedAttribute(
		projectName, kAXRangeForIndexParameterizedAttribute as CFString, 1 as CFNumber)
	var emojiRange = CFRange()
	try check(
		CFGetTypeID(emojiRangeValue) == AXValueGetTypeID()
			&& AXValueGetValue(emojiRangeValue as! AXValue, .cfRange, &emojiRange)
			&& rangeEquals(emojiRange, location: 1, length: 2),
		"AXRangeForIndex did not preserve the surrogate pair")

	guard let description = findElement(
		in: primary, role: kAXTextFieldRole, name: "Description")
	else {
		try fail("could not find Description text editor")
	}
	try setAttribute(description, kAXValueAttribute as CFString, "one\n😀x" as CFString)
	try setAttribute(
		description, kAXSelectedTextRangeAttribute as CFString,
		try rangeValue(location: 4, length: 2))
	try check(
		stringAttribute(description, kAXSelectedTextAttribute as CFString) == "😀",
		"multiline editor selected text was incorrect")
	try check(
		numberAttribute(description, kAXInsertionPointLineNumberAttribute as CFString)?.intValue
			== 1,
		"multiline editor insertion point line was incorrect")
	let selectedSubstring = try parameterizedAttribute(
		description, kAXStringForRangeParameterizedAttribute as CFString,
		try rangeValue(location: 4, length: 2))
	try check(
		(selectedSubstring as? String) == "😀",
		"AXStringForRange did not use UTF-16 ranges")

	guard let password = findElement(
		in: primary, role: kAXTextFieldRole, name: "Account password")
	else {
		try fail("could not find secure text field")
	}
	try check(
		stringAttribute(password, kAXSubroleAttribute as CFString) == kAXSecureTextFieldSubrole,
		"password field did not expose AXSecureTextField")
	try check(
		stringAttribute(password, kAXValueAttribute as CFString) == nil,
		"password field exposed its value")

	guard let lightTheme = findElement(
		in: primary, role: kAXRadioButtonRole, name: "Light Theme")
	else {
		try fail("could not find Light Theme radio button")
	}
	try observe(observer, lightTheme, kAXValueChangedNotification as CFString, recorder)
	let radioEventStart = recorder.events.count
	try perform(lightTheme, kAXPressAction as CFString)
	try check(
		numberAttribute(lightTheme, kAXValueAttribute as CFString)?.boolValue == true,
		"radio button did not select through AXPress")
	let _: Bool = try waitFor("radio value notification", timeout: arguments.timeout) {
		recorder.contains(
			kAXValueChangedNotification as CFString, target: lightTheme, since: radioEventStart)
			? true : nil
	}
	try checkSingleNotification(
		recorder, kAXValueChangedNotification as CFString, target: lightTheme,
		since: radioEventStart)

	guard let volume = findElement(in: primary, role: kAXSliderRole, name: "Volume") else {
		try fail("could not find Volume slider")
	}
	try check(
		numberAttribute(volume, kAXMinValueAttribute as CFString)?.doubleValue == 0
			&& numberAttribute(volume, kAXMaxValueAttribute as CFString)?.doubleValue == 100,
		"slider range was incorrect")
	try observe(observer, volume, kAXValueChangedNotification as CFString, recorder)
	let sliderEventStart = recorder.events.count
	try setAttribute(volume, kAXValueAttribute as CFString, 55 as CFNumber)
	try check(
		numberAttribute(volume, kAXValueAttribute as CFString)?.doubleValue == 55,
		"slider value did not update")
	let _: Bool = try waitFor("slider value notification", timeout: arguments.timeout) {
		recorder.contains(
			kAXValueChangedNotification as CFString, target: volume, since: sliderEventStart)
			? true : nil
	}
	try checkSingleNotification(
		recorder, kAXValueChangedNotification as CFString, target: volume,
		since: sliderEventStart)

	guard let retries = findElement(
		in: primary, role: kAXIncrementorRole, name: "Retry count")
	else {
		try fail("could not find Retry count incrementor")
	}
	try perform(retries, kAXIncrementAction as CFString)
	try check(
		numberAttribute(retries, kAXValueAttribute as CFString)?.doubleValue == 4,
		"incrementor did not increment")

	guard let progressBar = findElement(
		in: primary, role: kAXProgressIndicatorRole, name: "Operation progress")
	else {
		try fail("could not find progress indicator")
	}
	try check(
		numberAttribute(progressBar, kAXValueAttribute as CFString)?.doubleValue == 35,
		"progress indicator value was incorrect")

	try observe(
		observer, application, kAXFocusedUIElementChangedNotification as CFString, recorder)
	try setAttribute(save, kAXFocusedAttribute as CFString, kCFBooleanTrue)
	let _: Bool = try waitFor("application focus on Save settings", timeout: arguments.timeout) {
		var focused: CFTypeRef?
		let error = AXUIElementCopyAttributeValue(
			application, kAXFocusedUIElementAttribute as CFString, &focused)
		return error == .success && focused != nil && CFEqual(focused, save) ? true : nil
	}
	let focusEventStart = recorder.events.count
	try setAttribute(projectName, kAXFocusedAttribute as CFString, kCFBooleanTrue)
	let _: Bool = try waitFor("application focus on Project name", timeout: arguments.timeout) {
		var focused: CFTypeRef?
		let error = AXUIElementCopyAttributeValue(
			application, kAXFocusedUIElementAttribute as CFString, &focused)
		return error == .success && focused != nil && CFEqual(focused, projectName) ? true : nil
	}
	let _: Bool = try waitFor("focused element notification", timeout: arguments.timeout) {
		recorder.contains(
			kAXFocusedUIElementChangedNotification as CFString, target: projectName,
			since: focusEventStart) ? true : nil
	}
	try checkSingleNotification(
		recorder, kAXFocusedUIElementChangedNotification as CFString, target: projectName,
		since: focusEventStart)

	progress("checking geometry and hit testing")
	guard let position = pointAttribute(save, kAXPositionAttribute as CFString),
		let size = sizeAttribute(save, kAXSizeAttribute as CFString)
	else {
		try fail("save button did not expose geometry")
	}
	try check(size.width > 0 && size.height > 0, "save button geometry was empty")
	var hitElement: AXUIElement?
	let hitError = AXUIElementCopyElementAtPosition(
		application, Float(position.x + size.width / 2), Float(position.y + size.height / 2),
		&hitElement)
	try check(
		hitError == .success && hitElement != nil && CFEqual(hitElement, save),
		"AX hit testing at \(position.x + size.width / 2),\(position.y + size.height / 2) "
			+ "returned \(describe(hitElement)) instead of \(describe(save))")

	progress("checking model hierarchy and replacement")
	guard let colors = findElement(in: primary, role: kAXListRole, name: "Colors") else {
		try fail("could not find Colors list")
	}
	try check(
		elementsAttribute(colors, kAXRowsAttribute as CFString).count == 3,
		"list did not expose three rows")
	guard let projects = findElement(in: primary, role: kAXTableRole, name: "Projects") else {
		try fail("could not find Projects table")
	}
	let originalRows = elementsAttribute(projects, kAXRowsAttribute as CFString)
	try check(originalRows.count == 2, "table did not expose two rows")
	let staleRow = originalRows[0]
	try observe(observer, projects, kAXLayoutChangedNotification as CFString, recorder)
	try observe(observer, staleRow, kAXUIElementDestroyedNotification as CFString, recorder)
	guard let replaceProjects = findElement(
		in: primary, role: kAXButtonRole, name: "Replace project model")
	else {
		try fail("could not find model replacement button")
	}
	let modelEventStart = recorder.events.count
	try perform(replaceProjects, kAXPressAction as CFString)
	let replacementRows: [AXUIElement] = try waitFor(
		"replacement table model", timeout: arguments.timeout
	) {
		let rows = elementsAttribute(projects, kAXRowsAttribute as CFString)
		return rows.count == 1
			&& stringAttribute(rows[0], kAXValueAttribute as CFString) == "Gamma" ? rows : nil
	}
	try check(replacementRows.count == 1, "replacement table rows were incorrect")
	let _: Bool = try waitFor("table layout notification", timeout: arguments.timeout) {
		recorder.contains(
			kAXLayoutChangedNotification as CFString, target: projects, since: modelEventStart)
			? true : nil
	}
	let _: Bool = try waitFor("stale row destruction notification", timeout: arguments.timeout) {
		recorder.contains(
			kAXUIElementDestroyedNotification as CFString, target: staleRow, since: modelEventStart)
			? true : nil
	}
	try checkSingleNotification(
		recorder, kAXLayoutChangedNotification as CFString, target: projects,
		since: modelEventStart)
	try checkSingleNotification(
		recorder, kAXUIElementDestroyedNotification as CFString, target: staleRow,
		since: modelEventStart)
	var staleRowRole: CFTypeRef?
	try check(
		AXUIElementCopyAttributeValue(
			staleRow, kAXRoleAttribute as CFString, &staleRowRole) != .success,
		"stale model row remained queryable")

	progress("checking dynamic window lifecycle")
	guard let openWindow = findElement(
		in: primary, role: kAXButtonRole, name: "Open accessibility window")
	else {
		try fail("could not find dynamic-window button")
	}
	try observe(observer, application, kAXWindowCreatedNotification as CFString, recorder)
	let windowEventStart = recorder.events.count
	try perform(openWindow, kAXPressAction as CFString)
	let dynamicWindow: AXUIElement = try waitFor(
		"dynamic third window", timeout: arguments.timeout
	) {
		elementsAttribute(application, kAXWindowsAttribute as CFString).first {
			stringAttribute($0, kAXTitleAttribute as CFString) == "eepp - Accessibility Dynamic"
		}
	}
	let _: Bool = try waitFor("window creation notification", timeout: arguments.timeout) {
		recorder.contains(
			kAXWindowCreatedNotification as CFString, target: dynamicWindow, since: windowEventStart)
			? true : nil
	}
	try checkSingleNotification(
		recorder, kAXWindowCreatedNotification as CFString, target: dynamicWindow,
		since: windowEventStart)
	let closeDynamic: AXUIElement = try waitFor(
		"dynamic-window accessibility hierarchy", timeout: arguments.timeout
	) {
		findElement(in: dynamicWindow, role: kAXButtonRole, name: "Close dynamic window")
	}
	try stressQueriesDuringAction(
		staleElement: dynamicWindow, actionElement: closeDynamic, timeout: arguments.timeout)
	let _: Bool = try waitFor("dynamic window removal", timeout: arguments.timeout) {
		elementsAttribute(application, kAXWindowsAttribute as CFString).count == 2 ? true : nil
	}

	progress("checking primary window removal")
	guard let secondary = elementsAttribute(application, kAXWindowsAttribute as CFString).first(
		where: {
			stringAttribute($0, kAXTitleAttribute as CFString)
				== "eepp - Accessibility Secondary"
		})
	else {
		try fail("could not find the secondary window by title")
	}
	guard let closePrimary = findElement(
		in: secondary, role: kAXButtonRole, name: "Close primary window")
	else {
		try fail("could not find primary-window close button")
	}
	try stressQueriesDuringAction(
		staleElement: save, actionElement: closePrimary, timeout: arguments.timeout)
	let survivingWindow: AXUIElement = try waitFor(
		"primary window removal", timeout: arguments.timeout
	) {
		let found = elementsAttribute(application, kAXWindowsAttribute as CFString)
		return found.count == 1 ? found[0] : nil
	}
	try check(
		stringAttribute(survivingWindow, kAXTitleAttribute as CFString)
			== "eepp - Accessibility Secondary",
		"secondary window did not survive primary removal")
	var staleRole: CFTypeRef?
	let staleError = AXUIElementCopyAttributeValue(
		save, kAXRoleAttribute as CFString, &staleRole)
	try check(staleError != .success, "stale primary element remained queryable")

	progress("closing surviving secondary window")
	guard let closeSecondary = findElement(
		in: survivingWindow, role: kAXButtonRole, name: "Close secondary window")
	else {
		try fail("surviving secondary window was not queryable")
	}
	try perform(closeSecondary, kAXPressAction as CFString)
	process.waitUntilExit()
	try check(process.terminationStatus == 0, "example exited with a failure status")
	print("macOS accessibility semantic and lifecycle harness passed")
}

do {
	try run()
} catch {
	FileHandle.standardError.write(Data("macOS accessibility harness failed: \(error)\n".utf8))
	exit(EXIT_FAILURE)
}
