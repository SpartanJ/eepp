# Runtime UI inspector

The eepp inspector is a small TCP interface for inspecting and interacting with a running eepp UI. It is useful for debugging layout, CSS, focus, input delivery, multiple windows, and HTML inside `UIWebView`. Its compact JSON replies and server side batches are designed to reduce tool round trips for automated agents.

## Security and activation

The capability can be compiled into every eepp application. **It is disabled by default:** a normal run has no inspector listener, token, network thread, or reachable parser. Enabling it starts an explicit developer debugging interface. In control mode, authenticated clients can click and type into the application.

The listener binds to `127.0.0.1` by default. Every connection must present a bearer token. This is a safeguard against accidental or unintended attachment, including attachment to the wrong process; it is **not a sandbox boundary against malicious software running as the same OS user**. The interface resembles accessibility or OS automation in its ability to inspect and act, but additionally exposes eepp widget classes, CSS values, geometry, nested scenes, and watches. Availability of accessibility support does not enable the inspector.

Raw TCP is plaintext, including the token, UI text, and actions. Binding to a non-loopback address prints a warning and should be used only on a trusted network. For remote use, leave the inspector on loopback and forward it with `ssh -L 9876:127.0.0.1:<remote-port> host`. Password input text is returned as `[redacted]` in summaries, inspection, trees, and watches. Screenshots capture visible pixels, which can include sensitive information even when semantic text is redacted. Screenshot files remain in the OS temporary directory until the client or OS removes them. A generated token is printed once at startup; anyone who can read that process output can authenticate. A caller supplied token is not printed.

Set the following environment variables before launching any normal eepp UI application:

| Variable | Default | Meaning |
| --- | --- | --- |
| `EEPP_INSPECTOR` | unset | Set to `1` to start after the first top-level UI scene is registered. Unset, empty, or `0` disables it. |
| `EEPP_INSPECTOR_ADDRESS` | `127.0.0.1` | TCP bind address. |
| `EEPP_INSPECTOR_PORT` | `0` | TCP port; `0` asks the OS for an available port. |
| `EEPP_INSPECTOR_READ_ONLY` | `0` | Set to `1` to reject every `input.*` command. |
| `EEPP_INSPECTOR_TOKEN` | generated | Optional caller supplied bearer token. |

```sh
EEPP_INSPECTOR=1 ./bin/ecode
EEPP_INSPECTOR=1 EEPP_INSPECTOR_PORT=9876 EEPP_INSPECTOR_READ_ONLY=1 ./bin/ecode
EEPP_INSPECTOR=1 EEPP_INSPECTOR_TOKEN="$secret" ./bin/ecode
EEPP_INSPECTOR=1 EEPP_INSPECTOR_ADDRESS=0.0.0.0 ./bin/ecode # trusted network only
```

Successful startup writes one compact line to stderr:

```text
EEPP_INSPECTOR={"protocolVersion":1,"host":"127.0.0.1","port":43187,"token":"<generated-token>","pid":19382}
```

With `EEPP_INSPECTOR_TOKEN` supplied, the `token` member is omitted. The prefix `EEPP_INSPECTOR=` is stable. The server accepts up to 16 simultaneous TCP clients, and a request line is limited to 1 MiB. Each client's pending output is limited to 4 MiB. A single query returns at most 1000 nodes; a tree returns at most 1000 nodes; a batch accepts at most 100 commands.

Copy the `port` and generated `token` from that startup line into the client environment below.
The default port is selected by the OS at launch, so the client cannot infer it in advance.
If you set `EEPP_INSPECTOR_TOKEN` yourself, use that value for the client instead.

## Python client

`projects/scripts/eepp-inspect.py` requires Python 3 and only the standard library. It reads `EEPP_INSPECTOR_HOST`, `EEPP_INSPECTOR_PORT`, and `EEPP_INSPECTOR_TOKEN`, or the corresponding `--host`, `--port`, and `--token` options. Use `--pretty` for indented JSON. Default stdout is compact JSON only; diagnostics go to stderr. Ordinary commands connect, authenticate, print one response, and disconnect.

```sh
export EEPP_INSPECTOR_PORT=43187 EEPP_INSPECTOR_TOKEN='<token>'
python3 projects/scripts/eepp-inspect.py contexts
python3 projects/scripts/eepp-inspect.py query '#preview'
python3 projects/scripts/eepp-inspect.py query --scene scene:2 'table > tr > td.problem' --properties geometry.size css.font-size
python3 projects/scripts/eepp-inspect.py tree --scene scene:2 --depth 4
python3 projects/scripts/eepp-inspect.py inspect w:42 geometry.size geometry.position css.font-size
python3 projects/scripts/eepp-inspect.py focus --scene scene:2
python3 projects/scripts/eepp-inspect.py keybindings --limit 100
python3 projects/scripts/eepp-inspect.py keybindings '#url_bar'
python3 projects/scripts/eepp-inspect.py screenshot --scene scene:2 --format png
python3 projects/scripts/eepp-inspect.py screenshot --window win:2 --rect 10 20 400 300 --format webp
python3 projects/scripts/eepp-inspect.py click --scene scene:2 '#submit'
python3 projects/scripts/eepp-inspect.py click --button right --scene scene:2 '#content'
python3 projects/scripts/eepp-inspect.py key --scene scene:2 Enter
python3 projects/scripts/eepp-inspect.py type --scene scene:2 'hello world'
python3 projects/scripts/eepp-inspect.py type --target '#search' 'hello world'
python3 projects/scripts/eepp-inspect.py watch --scene scene:2 '#problem' geometry.size css.font-size --count 3 --timeout 10
python3 projects/scripts/eepp-inspect.py batch workflow.json
python3 projects/scripts/eepp-inspect.py batch - < workflow.json
python3 projects/scripts/eepp-inspect.py raw '{"method":"ui.query","params":{"selector":"#foo"}}'
python3 projects/scripts/eepp-inspect.py session
```

`type --target` sends `input.click` followed by `input.text`, using the clicked widget's scene. `watch` streams one event per line; `--count` counts `watch.changed` events and `--timeout` bounds elapsed seconds. `batch` reads a JSON object containing `commands` from a file or stdin. `session` authenticates once, then sends one JSON request per stdin line and writes every response or event to stdout. It keeps the connection open while stdin is open. `raw` supplies a normal request object, assigning an ID if absent.

## Transport and envelopes

Protocol version 1 uses TCP, UTF-8, and newline delimited JSON: one object followed by `\n`. `\r\n` is accepted. TCP read boundaries have no meaning. Messages are compact, and a client may keep a connection open, pipeline requests, and receive unsolicited events between responses. Correlate responses by their non-negative integer `id`; response order is not guaranteed. A connection's first request must be `session.connect`.

```json
{"id":1,"method":"session.connect","params":{"protocolVersion":1,"token":"<token>","client":{"name":"my-tool","version":"1"}}}
```

Success includes `protocolVersion`, `application`, `pid`, `readOnly`, `defaultWindow`, `defaultScene`, and `capabilities`. An invalid token returns `authentication-failed` and closes the connection; a non-v1 version returns `unsupported-protocol` and closes it. Requests use `{"id":17,"method":"ui.query","params":{...}}`; `params` defaults to `{}`. Success is `{"id":17,"result":{...}}`. Failure is `{"id":17,"error":{"code":"...","message":"...","data":{...}}}`; `data` is optional. Events have no ID: `{"method":"watch.changed","params":{...}}`.

Every command may have a post-command `delay` such as `"100ms"` or `"1s"`. The command runs first; its response or the next batch command waits asynchronously. The normal UI update loop continues. A single delay is limited to 60 seconds. The accepted syntax is a non-negative decimal number followed by `ms` or `s`.

## Windows, scenes, and handles

Handles are opaque: `win:*` identifies a native window, `scene:*` a `UISceneNode`, `w:*` a widget, and `sub:*` a watch subscription. Window, scene, and widget IDs are monotonic for the server's lifetime and valid across client reconnections while the objects live. Destroyed objects invalidate their handles. Subscription IDs belong to one connection and are released on disconnect. Handles never contain pointers and do not survive server restarts.

Each selector executes in **exactly one scene**. An omitted scene means the stable `defaultScene` from the handshake, never the currently active scene. If that scene is destroyed, omission returns `default-scene-unavailable`. A scene determines its native window; a widget handle determines its scene and window. `ui.contexts` reports all discovered top-level and nested scenes. A `UIWebView` document is a separate scene and is classified `webview`, with its owner widget and parent scene. Other embedded scenes are classified `nested`.

For example, discover the WebView in its parent scene, then query its document scene:

```json
{"id":10,"method":"ui.query","params":{"selector":"#preview"}}
```

The returned WebView summary includes `"documentScene":"scene:2"`. Then:

```json
{"id":11,"method":"ui.query","params":{"scene":"scene:2","selector":"table > tr > td.problem"}}
{"id":12,"method":"ui.inspect","params":{"handle":"w:95","properties":["geometry.position","geometry.size","css.display","css.font-size"]}}
```

`#preview p` in the parent scene never enters the HTML document. `ui.contexts` also reveals secondary native windows and their scenes; pass the secondary `scene:*` to query or act there.

## Method reference

All examples below omit `id` where the surrounding explanation is enough; real requests always require it.

| Method | Parameters and defaults | Result | Notable errors |
| --- | --- | --- | --- |
| `session.connect` | `protocolVersion:1`, `token` required; optional `client` | Protocol version, application, PID, mode, default handles, capabilities | `authentication-failed`, `unsupported-protocol` |
| `session.batch` | `commands` array, at most 100 | `results` object keyed by command `name` or zero-based index string | `batch-command-failed`, `invalid-params` |
| `session.nextFrame` | `count` default 1, maximum 10 | `{ "frames": count }`; mainly used inside a batch | `invalid-params` |
| `ui.contexts` | none | `revision`, default handles, `windows`, `scenes` | `internal-error` |
| `ui.query` | `selector` required; `scene` default; `offset:0`, `limit:50` (maximum 1000); optional `properties`, `maxStringLength:4096` (maximum 65536) | Scene, selector, total, pagination, truncation, compact `nodes` | `invalid-scene`, `selector-invalid` |
| `ui.tree` | `scene` default or `root` handle; `depth:3` (maximum 32), `maxNodes:200` (maximum 1000), `includeText:false` | Flat nodes with `parent`, `children`, and `truncated` | `invalid-widget`, `invalid-scene` |
| `ui.inspect` | Exactly one of `handle` or `selector` plus optional `scene`; optional `properties`, `maxStringLength:4096` (maximum 65536) | Handle, scene, typed `properties`, `unavailable` | `invalid-widget`, `target-not-found`, `target-ambiguous` |
| `ui.keybindings` | Optional `handle` or `selector`; `scene` default; `offset:0`, `limit:50` (maximum 1000) | Scene, optional widget handle, `supported`, pagination, `bindings` | `invalid-scene`, `invalid-widget`, `selector-invalid`, `target-not-found`, `target-ambiguous` |
| `ui.focus` | `scene` default | Scene and focused widget summary or `null` | `invalid-scene` |
| `ui.screenshot` | Optional `scene` or `window` (mutually exclusive); optional `rect:[x,y,width,height]`; `format:"png"` | Temporary file path, format, window/scene handles, captured rectangle and size | `invalid-window`, `invalid-scene`, `invalid-params`, `screenshot-failed` |
| `input.click` | Exactly one target; optional `button:"left"`, `"middle"`, or `"right"`, `count:1` (maximum 2) | Target handle, scene, click point | `target-not-interactable`, `permission-denied` |
| `input.key` | `scene` default, `key` required, `action:"press"`, `modifiers:[]` | Key and action | `invalid-params`, `permission-denied` |
| `input.text` | `scene` default, `text` required (maximum 64 KiB UTF-8) | Character count | `invalid-params`, `permission-denied` |
| `watch.subscribe` | One handle or selector; explicit `properties` array; `initial:true` | Subscription and bound target values | `target-not-found`, `property-unknown` |
| `watch.unsubscribe` | `subscription` required | `{ "unsubscribed": true }` | `invalid-params` |

`ui.contexts` window entries include handle, native ID, title, pixel size and position, and focus. Scene entries include handle, window, kind, parent scene, owner widget, and root widget; WebView scenes include URI. The `revision` increases on observed topology changes. Create/destroy events are checked periodically while clients are connected; `ui.contexts` is the authoritative current snapshot.

These compact request/response pairs show each method's envelope. Handle numbers and geometry are illustrative; methods also accept the optional fields in the table above. Errors use the shared error envelope.

```json
{"id":1,"method":"session.connect","params":{"protocolVersion":1,"token":"secret"}}
{"id":1,"result":{"protocolVersion":1,"application":"ecode","pid":19382,"readOnly":false,"defaultWindow":"win:1","defaultScene":"scene:1","capabilities":["ui.contexts","ui.query","ui.tree","ui.inspect","ui.focus","ui.screenshot","ui.keybindings","session.batch","session.nextFrame","watch.properties","events.context-lifecycle","input.click","input.key","input.text"]}}
{"id":2,"method":"ui.contexts"}
{"id":2,"result":{"revision":0,"defaultWindow":"win:1","defaultScene":"scene:1","windows":[{"handle":"win:1","id":1,"title":"ecode","sizePx":[800,600],"positionPx":[0,0],"focused":true}],"scenes":[{"handle":"scene:1","window":"win:1","kind":"top-level","parentScene":null,"owner":null,"root":"w:1"}]}}
{"id":3,"method":"ui.query","params":{"selector":"#panel","properties":["geometry.size"],"limit":1}}
{"id":3,"result":{"scene":"scene:1","selector":"#panel","total":1,"offset":0,"returned":1,"truncated":false,"nodes":[{"handle":"w:2","scene":"scene:1","tag":"widget","id":"panel","classes":[],"pseudoClasses":[],"boundsPx":[10,10,100,30],"visible":true,"enabled":true,"focused":false,"properties":{"geometry.size":[100,30]}}]}}
{"id":4,"method":"ui.tree","params":{"depth":1,"maxNodes":50}}
{"id":4,"result":{"scene":"scene:1","root":"w:1","truncated":false,"nodes":[{"handle":"w:1","scene":"scene:1","tag":"root","id":"","classes":[],"pseudoClasses":[],"parent":null,"children":["w:2"]},{"handle":"w:2","scene":"scene:1","tag":"widget","id":"panel","classes":[],"pseudoClasses":[],"parent":"w:1","children":[]}]}}
{"id":5,"method":"ui.inspect","params":{"handle":"w:2","properties":["geometry.size","css.font-size"]}}
{"id":5,"result":{"handle":"w:2","scene":"scene:1","properties":{"geometry.size":[100,30],"css.font-size":"14dp"},"unavailable":[]}}
{"id":6,"method":"ui.focus"}
{"id":6,"result":{"scene":"scene:1","widget":null}}
{"id":14,"method":"ui.screenshot","params":{"scene":"scene:2","format":"png"}}
{"id":14,"result":{"path":"/tmp/eepp-inspector-19382-<random>.png","format":"png","window":"win:1","scene":"scene:2","rectPx":[100,80,480,320],"sizePx":[480,320]}}
{"id":7,"method":"input.click","params":{"handle":"w:2"}}
{"id":7,"result":{"target":"w:2","scene":"scene:1","pointPx":[60,25]}}
{"id":8,"method":"input.key","params":{"key":"Enter","action":"press","modifiers":[]}}
{"id":8,"result":{"key":"Enter","action":"press"}}
{"id":9,"method":"input.text","params":{"text":"hello"}}
{"id":9,"result":{"characters":5}}
{"id":10,"method":"watch.subscribe","params":{"handle":"w:2","properties":["geometry.size"],"initial":true}}
{"id":10,"result":{"subscription":"sub:1","targets":[{"handle":"w:2","values":{"geometry.size":[100,30]}}]}}
{"id":11,"method":"watch.unsubscribe","params":{"subscription":"sub:1"}}
{"id":11,"result":{"unsubscribed":true}}
{"id":12,"method":"session.nextFrame","params":{"count":1}}
{"id":12,"result":{"frames":1}}
{"id":13,"method":"session.batch","params":{"commands":[{"name":"panel","method":"ui.query","params":{"selector":"#panel","limit":1},"return":false},{"name":"size","method":"ui.inspect","params":{"handle":{"$ref":"panel#/nodes/0/handle"},"properties":["geometry.size"]}}]}}
{"id":13,"result":{"results":{"size":{"handle":"w:2","scene":"scene:1","properties":{"geometry.size":[100,30]},"unavailable":[]}}}}
```

`ui.query` uses eepp's CSS selector matching and returns an empty `nodes` array for no matches. Summary fields include handle, scene, tag, ID, classes, active pseudo-classes, pixel bounds, visible, enabled, focused, and optional text. Text is exposed for text controls, `UITextNode`, `UITextSpan`, and `UIRichText`; rich-text containers concatenate descendant source text without forcing layout. Summary text is limited to 256 Unicode characters. When `properties` is provided, each returned node also has the requested inspection values, avoiding one `ui.inspect` call per match. `ui.tree` does not cross a nested scene boundary; a WebView entry instead includes `documentScene`.

`ui.keybindings` lists the bindings registered directly on a scene, or on one widget selected by `handle` or `selector`. Omit the widget target to inspect the scene's bindings. It uses the same scene and target validation as `ui.inspect` and is available in read-only mode. Each entry contains `shortcut` (the platform's key name and modifiers), `command`, and numeric `keycode` and `mod` fields from `KeyBindings::Shortcut`. Entries are ordered by packed shortcut value, and every shortcut alias is retained. Pagination uses `offset` and `limit`, with `total`, `returned`, and `truncated` in the response.

Supported widgets are `UITextInput` (including password inputs), `UICodeEditor`, `UIWindow`, `UIConsole`, and widgets using `WidgetCommandExecuter`, such as diff, merge, and find/replace views. Other widgets return `supported:false` and an empty list; scenes and supported widgets with no bindings return `supported:true`. The query reports registered bindings only: it does not merge ancestor or nested-scene bindings, infer hardcoded key handlers, or assert that a command callback exists or will receive an event. For focus-related debugging, inspect the focused widget and its enclosing scene separately.

```json
{"id":20,"method":"ui.keybindings","params":{"limit":100}}
{"id":21,"method":"ui.keybindings","params":{"selector":"#url_bar"}}
```

`ui.screenshot` captures the native window containing the selected scene. With no target it captures the default scene's full native window. An explicit `window` captures that whole window; an explicit `scene` crops to its visible world bounds, including a WebView document viewport. `rect` overrides that crop and uses top-left native-window pixel coordinates. It must fit entirely inside the window and have positive width and height. Supported formats are `png`, `jpg`, `bmp`, `tga`, `qoi`, and `webp`; eepp's case-insensitive image extension parser also accepts `jpeg` and `jfif` as aliases for `jpg`. The response uses the canonical extension. The server renders the selected window after the current UI update, saves the image in the OS temporary directory, and returns its absolute `path`; it does not send image bytes over NDJSON. The caller is responsible for deleting the file when finished. The path is useful to a local agent; remote clients must separately retrieve the file, for example over SSH. Screenshot observation is available in read-only mode and can be used inside `session.batch`.

Single-widget methods never pick the first of multiple selector matches: zero matches return `target-not-found`, and multiple matches return `target-ambiguous`. `input.click` uses the window input route and scene hit testing; clipped, covered, invisible, disabled, or off-window targets can return `target-not-interactable`. It does not scroll a target into view. The default button is `left`; set `button` to `middle` for a middle click or `right` to open a context menu. Other button values return `invalid-params`. `input.key` accepts names understood by eepp's `Input::getKeyFromName()` plus `Enter` as an alias for `Return`, and modifiers `Ctrl`, `Shift`, `Alt`, `Meta`; action may be `press`, `down`, or `up`. `input.text` sends Unicode text input to the current focus path and does not focus a widget first. Read-only mode omits input capabilities and returns `permission-denied` for these methods.

### Checking context menus

Right-click a visible target, then query the popup and its items. For a WebView, the HTML target belongs to its document scene, while the popup usually belongs to the parent application scene:

```sh
python3 projects/scripts/eepp-inspect.py contexts
python3 projects/scripts/eepp-inspect.py click --button right --scene scene:2 '#context-target'
python3 projects/scripts/eepp-inspect.py query --scene scene:1 'popupmenu'
python3 projects/scripts/eepp-inspect.py tree --scene scene:1 --depth 5
```

`input.click` aims at the target's center. Choose a visible child if a large HTML element extends beyond the window; a body whose center is off-screen will return `target-not-interactable`. The command reports the chosen point in window pixels. The same button option works in a raw or batched `input.click` request: `{"method":"input.click","params":{"scene":"scene:2","selector":"#context-target","button":"right"}}`.

## Properties

`ui.inspect` defaults to `identity.*`, `geometry.*`, and `state.*`. Explicit names and the wildcards `identity.*`, `geometry.*`, `state.*`, `content.*`, `css.*`, and `*` are accepted. `css.*` expands to CSS properties implemented by that widget. Unknown or unavailable properties appear in `unavailable` rather than failing the whole inspection. Long text properties are bounded to 4096 Unicode characters by default. `maxStringLength` can raise that limit to 65536; `truncatedProperties` lists fields cut short. Watches use the 4096 character default.

| Group | Names and values |
| --- | --- |
| `identity.*` | `tag`, `id`, `classes`, `pseudoClasses` |
| `geometry.*` | `position` and `size` as local pixel `[x,y]`; `worldPosition` as window/world pixel `[x,y]`; `bounds` as `[x,y,width,height]` in the window/world hit-test space |
| `state.*` | `visible`, `enabled`, `focused` booleans |
| `content.*` | `text` from text controls and HTML text widgets (`UITextNode`, `UITextSpan`, `UIRichText`); rich-text source traversal is capped at 2048 nodes and does not enter nested scenes or text controls. Subject to truncation and secret redaction. |
| `css.*` | `css.<name>` as eepp's effective `getPropertyString()` serialization, for example `css.font-size` |

## Batching and chaining

`session.batch` executes commands sequentially on the UI thread, without rollback. Each command has `method`, optional `params`, optional unique `name`, optional `return` (default `true`), and optional post-command `delay`. `session.connect` and nested batches are forbidden. A failed step stops later steps and returns `batch-command-failed` with its index, name, and underlying `commandError`; earlier actions remain in effect.

A later command may recursively replace any parameter value with `{"$ref":"name#/json/pointer"}`. The name must identify an earlier command. The pointer is an RFC 6901 JSON Pointer into that command's **result**. A missing name, forward reference, or bad pointer produces `batch-reference-error` inside the batch failure. A result with `"return":false` is retained for references but omitted from the final wire reply. This makes dependent workflows a single agent/tool interaction.

The WebView discovery, document query, and inspection flow can be one request:

```json
{"commands":[
  {"name":"webview","method":"ui.query","return":false,"params":{"selector":"#preview","limit":1}},
  {"name":"problem","method":"ui.query","return":false,"params":{"scene":{"$ref":"webview#/nodes/0/documentScene"},"selector":"td.problem","limit":1}},
  {"name":"inspection","method":"ui.inspect","params":{"handle":{"$ref":"problem#/nodes/0/handle"},"properties":["geometry.size","css.font-size"]}}
]}
```

Only `results.inspection` is returned. To request a larger single response instead of many inspect calls, use `ui.query` with `"properties":["geometry.size","css.font-size"]` and an appropriate limit.

For action and observation in one call:

```json
{"commands":[
  {"method":"input.click","delay":"100ms","return":false,"params":{"selector":"#toggle"}},
  {"name":"state","method":"ui.inspect","params":{"selector":"#panel","properties":["geometry.size","state.visible","css.display"]}}
]}
```

The click happens first; eepp's normal timeout machinery resumes the batch at least 100 ms later while frames keep running. For a frame barrier, insert `{"method":"session.nextFrame","return":false,"params":{"count":1}}` between the click and inspection. This resumes after the next completed normal scene update. Delayed batches do not block other clients or requests. Closing a client cancels its pending batches. Batching reduces agent/tool round trips; inline properties let the client deliberately choose one larger response when that costs less than several small calls.

## Watches and events

`watch.subscribe` resolves a selector once and binds to its current matches; it is not a live selector. Watches require explicit property names, without wildcards. The initial values are in the subscribe response by default. The inspector compares only watched targets and properties once per UI frame, coalescing changes to one `watch.changed` event per target per frame. `sequence` increases per subscription. A target's destruction emits `watch.targetRemoved` with reason `target-destroyed`; once none remain, `watch.ended` has reason `no-targets`. Explicit unsubscribe returns directly without an ended event. Disconnect releases the connection's subscriptions.

```json
{"method":"watch.changed","params":{"subscription":"sub:1","sequence":7,"target":"w:42","changes":{"geometry.size":{"old":[800,600],"new":[760,600]}}}}
```

Other events are `ui.windowCreated` and `ui.windowDestroyed` with `revision` and a window descriptor or handle; `ui.sceneCreated` and `ui.sceneDestroyed` with `revision` and a scene descriptor or handle. Events may appear between request responses. Keep a watch client connected while another client sends actions.

| Event | Example `params` |
| --- | --- |
| `ui.windowCreated` | `{"revision":1,"window":{"handle":"win:2","id":2,"title":"Preview","sizePx":[800,600]}}` |
| `ui.windowDestroyed` | `{"revision":2,"window":"win:2"}` |
| `ui.sceneCreated` | `{"revision":3,"scene":{"handle":"scene:2","window":"win:1","kind":"webview","parentScene":"scene:1","owner":"w:4","root":"w:5"}}` |
| `ui.sceneDestroyed` | `{"revision":4,"scene":"scene:2"}` |
| `watch.changed` | `{"subscription":"sub:1","sequence":1,"target":"w:2","changes":{"geometry.size":{"old":[100,30],"new":[120,30]}}}` |
| `watch.targetRemoved` | `{"subscription":"sub:1","target":"w:2","reason":"target-destroyed"}` |
| `watch.ended` | `{"subscription":"sub:1","reason":"no-targets"}` |

## Errors

Stable v1 error codes are:

| Code | Meaning |
| --- | --- |
| `parse-error` | Malformed JSON. |
| `invalid-request` | Missing/invalid request envelope, ID, or params. |
| `unauthenticated` | Inspector method before connection authentication. |
| `authentication-failed` | Token rejected. |
| `unsupported-protocol` | Client requested a version other than 1. |
| `method-not-found` | Unknown method. |
| `invalid-params` | A method argument is missing, invalid, or beyond a limit. |
| `permission-denied` | An action was requested in read-only mode. |
| `invalid-window`, `invalid-scene`, `invalid-widget` | A handle does not identify a live object of that kind. |
| `default-scene-unavailable` | The original default scene was destroyed. |
| `selector-invalid` | Selector is empty or rejected by selector validation. |
| `target-not-found`, `target-ambiguous` | Single-target selector matched zero or multiple widgets. |
| `target-not-interactable` | Click point cannot reach the target. |
| `property-unknown` | Watch property cannot be read. |
| `input-unavailable` | The scene/window has no usable input context. |
| `screenshot-failed` | No drawable window/scene area, or image capture/save failed. |
| `batch-reference-error` | Invalid command reference or JSON Pointer. |
| `batch-command-failed` | A batch step failed; `data.commandError` contains its cause. |
| `request-too-large` | A request line exceeded 1 MiB; the server closes the connection. |
| `internal-error` | An unexpected command error was caught. |

## C++ API

Applications normally need no code. Tests and embedding code may use:

```cpp
#include <eepp/ui/tools/uiinspectorserver.hpp>

UIInspectorServerSettings settings;
settings.address = "127.0.0.1";
settings.port = 0;
settings.readOnly = false;
settings.token = "caller-chosen-secret"; // leave empty to generate one
UIInspectorServer::start( defaultScene, settings );
auto* server = UIInspectorServer::instance();
auto port = server->getPort();
UIInspectorServer::stop();
```

`instance()` is null when disabled. `isRunning()`, `getAddress()`, `getPort()`, and `getToken()` are available while running. Call the API on the UI thread. The framework automatically pumps it during `SceneManager::update()` and stops it during scene-manager teardown.

The headless `UIInspector` owns the handle registry and scene/widget reads. Protocol dispatch and TCP session code currently share one implementation file; the network thread only frames messages and manages connections, while the scene update pump runs every UI operation. This file layout does not affect the wire protocol.

## Troubleshooting

- **Connection refused:** confirm `EEPP_INSPECTOR=1`, read the emitted port, and use the correct host or SSH forward.
- **Authentication failed:** use the token from the same application run; a supplied token is intentionally absent from the descriptor.
- **Query returns nothing:** inspect `ui.contexts`; HTML inside a WebView is in its `documentScene`.
- **Ambiguous selector:** use a more specific selector or a widget handle.
- **Click rejected:** inspect pixel bounds and visibility; the center may be clipped, covered, or outside the window. Scrolling is manual in v1.
- **Key/text has no effect:** call `ui.focus`, then click the intended input before typing.
- **Remote debugging:** keep the listener on loopback and use SSH forwarding; direct raw TCP has no encryption.
