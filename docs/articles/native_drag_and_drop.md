# Native file drag and drop

Use `EE::Window::DragAndDrop` to let users drag local files from an eepp window into another
application, such as a file manager or a browser upload input. File reception uses the existing
`InputEvent::FileDropped` event.

Include `<eepp/window/draganddrop.hpp>` or the umbrella `<eepp/window.hpp>` header. All drag API
calls belong on the window/UI thread.

## Start a file drag

Create a facade for the source window and pass a `DragOffer` containing absolute UTF-8 filesystem
paths. This function should be called from a drag gesture, after the pointer has moved beyond your
application's drag threshold and while the left mouse button is still held:

```cpp
#include <eepp/window/draganddrop.hpp>
#include <eepp/window/window.hpp>
#include <iostream>

using namespace EE::Window;

void beginFileDrag( Window& window, const std::string& absoluteUtf8Path ) {
    auto drag = DragAndDrop::forWindow( window );
    if ( !drag.getCapabilities().fileSource ) {
        std::cerr << "Native file dragging is unavailable on this display backend\n";
        return;
    }

    DragOffer offer;
    offer.files.push_back( { absoluteUtf8Path } );

    auto started = drag.startDrag( offer, []( DragResult result ) {
        switch ( result.reason ) {
            case DragEndReason::Dropped:
                std::cout << "The destination accepted the files\n";
                break;
            case DragEndReason::Cancelled:
                std::cout << "Drag cancelled\n";
                break;
            default:
                std::cerr << "Drag failed: " << result.error << '\n';
                break;
        }
    } );

    if ( started.status != DragStartStatus::Started )
        std::cerr << "Could not start drag: " << started.error << '\n';
}
```

Add more entries to `offer.files` to drag multiple files. Pass filesystem paths, rather than
`file://` URLs. Relative paths, invalid UTF-8, embedded NULs and empty offers are rejected. Windows
accepts absolute drive paths and UNC paths with a server and share; device paths are unsupported.
Paths must refer to local files available to the receiving application. Remote URLs and virtual
files are unsupported.

`DragOffer::actions` defaults to Copy, which is currently the only supported action. Move, Link,
Ask and combinations of actions are rejected. A successful native drop means the destination
accepted the transfer; it does not mean a browser finished uploading or a file manager finished
copying. Keep the source files available and do not delete them based on `Dropped`.

## Connect a pointer gesture

For a file card or row, handle the gesture as follows:

1. On left-button down inside the draggable item, remember the pointer position and arm the drag.
2. On pointer motion, wait until the distance from the press passes a threshold, such as 8 pixels.
3. Disarm the gesture and call `startDrag()` once, while the left button is still down.
4. On button release, disarm any gesture that has not started.

`startDrag()` does not detect the source widget or apply a movement threshold for you. Use
`isDragging()` to query the source window's active session. Keep updating input through your normal
application loop: it advances the drag and delivers completion. On Windows, the next input update
enters the native modal drag loop until the gesture ends. On macOS, AppKit runs its drag loop inside
event processing in the same way. In both cases the application does not render frames until the
drop or cancellation, so present any visual change, such as dimming the source item, before then.

Native dragging is independent of eepp's internal UI dragging. When handing an internal drag to the
native API, end the internal drag without emitting its drop action, clear the dispatcher's dragged
node and any application preview state, and release internal capture before starting the native
drag. Native gesture cleanup produces a release trigger without a click; an internal drag left
active could treat that release as an internal drop.

## Handle start results and completion

`startDrag()` returns immediately with a `DragStartResult`:

| Status | Meaning |
| --- | --- |
| `Started` | The offer was accepted; completion will be delivered later. |
| `Busy` | Another process-wide native drag is active or awaiting completion. |
| `InvalidOffer` | The paths, actions or preview are invalid; read `error` for details. |
| `InvalidGesture` | The required initiating left-button gesture is unavailable. |
| `Unsupported` | The active backend cannot provide native file dragging. |
| `Failed` | Native setup failed; read `error` for details. |

A rejected start never invokes the completion callback. For an accepted start, completion runs once
when input is updated, outside native event hooks and after native session resources have been
released. The callback may start another drag. `DragResult::reason` reports `Dropped`, `Cancelled`
or a failure; `error` provides diagnostic text. `performedAction` is Copy for a successful drop and
None otherwise.

Call `drag.cancelDrag()` to cancel the source window's session. Cancellation requests do not invoke
the callback inline; keep updating input to receive completion. Escape and closing or destroying
the source window also cancel the drag. Window destruction remains synchronous: the drag does not
keep a destroyed window alive. AppKit cannot abort a running drag: on macOS, `cancelDrag()` delivers
a `Cancelled` completion immediately and the image stays under the pointer until release, but every
target then refuses the drop.

The facade borrows its window and must not outlive it. The offer and preview image only need to
remain alive during `startDrag()`; a started session owns its native file list, copied preview and
completion callback. Completion can arrive after the source window has been destroyed, so avoid
capturing raw window/widget pointers or references to shorter-lived state. Capture independently
owned state, or use weak ownership and check it before accessing UI objects. Engine teardown
releases pending sessions without invoking their completion callbacks. Destroy the engine after
returning to the application loop.

## Customize the drag preview

A drag shows a file icon and file count by default. To show your own image, set
`offer.preview.image` before starting:

```cpp
#include <eepp/graphics/image.hpp>

EE::Graphics::Image preview( "assets/file-card.png", 4 );
DragOffer offer;
offer.files.push_back( { absoluteUtf8Path } );
offer.preview.image = &preview;
offer.preview.hotspot = { 8, 8 };
auto started = DragAndDrop::forWindow( window ).startDrag( offer );
```

The hotspot is the pointer position relative to the image's top-left corner, in desktop pixels.
With `{8, 8}`, the pointer sits eight pixels into the image. The default `{-12, -16}` places the
image just beyond the pointer so the target remains visible.

Images must have RGB or RGBA pixels, be at most 2048 pixels per side, and contain no more than
1,048,576 pixels in total. Each hotspot coordinate must be between -2048 and 2048. On Windows and
X11, alpha is composited onto the preview background; Wayland and macOS keep it transparent. The
image is copied during `startDrag()` and may be released or reused as soon as the call returns.

On macOS, the default preview is each file's Finder icon, and AppKit draws the item count and the
copy cursor. Preview pixels map to display pixels, so on a Retina display the image appears at half
its pixel size in points, matching a widget snapshot taken from the window's framebuffer.

To display a widget, capture it into an `Image` once when the gesture starts and pass that image.
Any `UIWidget` can supply the snapshot; the drag API does not retain or render the widget. See
[`snapshotWidget()` in the example](../../src/examples/native_drag_and_drop/native_drag_and_drop.cpp)
for a framebuffer readback implementation.

## Receive files

Check `getCapabilities().fileTarget` before offering a native receiving area. Register an input
callback on the receiving window:

```cpp
#include <eepp/window/input.hpp>
#include <iostream>

const auto callbackId = window.getInput()->pushCallback( []( InputEvent* event ) {
    if ( event->Type == InputEvent::FileDropped ) {
        // Consume the path here, or copy it if it must survive this callback.
        std::cout << "Received: " << event->file.file << '\n';
    }
} );

// When the listener is no longer needed, while the window is still alive:
// window.getInput()->popCallback( callbackId );
```

The file path is borrowed and valid only during the event callback. A multiple-file drop produces
individual file events. This receiving API does not expose negotiated actions, MIME selection or a
grouped drop session. Text drops continue to use `InputEvent::TextDropped`.

## Check platform support

Query capabilities for the actual window rather than assuming support from the operating system:

| Capability | What it tells you |
| --- | --- |
| `fileSource` | Local files can be dragged out of this window. |
| `dragImage` | The native source supports a preview image. |
| `fileTarget` | The window supports existing file-drop events. |
| `dataSource` | Arbitrary native data sources; currently false. |
| `negotiatedTarget` | Native target action/MIME negotiation; currently false. |

Outbound local-file dragging is implemented for Windows, macOS, X11 and Wayland, with SDL2 or SDL3.
Haiku currently reports no file-source support. On Linux/BSD, X11 and Wayland client headers enable
their respective transports when building eepp. Wayland requires client headers and a client library
at version 1.20 or newer. Both client libraries load dynamically; no additional consumer link
dependency or build option is needed.
Without the corresponding headers, eepp still builds with that transport disabled; check
`getCapabilities().fileSource` before offering native file dragging.

On Wayland, start the drag from a real left-button press on the source window while that button is
still held. Injecting only eepp/SDL input events cannot authorize a native drag: the compositor must
have issued the press serial. A second button held during the gesture is unsupported. The compositor
moves the drag image, including transparent pixels and its hotspot. A small badge shows Copy,
rejected, or pending target negotiation without changing the supplied image. This requires the core
subcompositor protocol. File reception continues through SDL's ordinary drop events, including drops
into another window of the same application. Compositors must provide version 3 of the core
data-device protocol for Copy negotiation and completion.

On Windows, SDL2 receives files through its legacy file-drop path, which cannot receive the outgoing
OLE gesture used by this source. Use SDL3 for eepp-window-to-eepp-window native drops. Both backends
can drag files out to native OLE targets. On X11, the source window itself is excluded as a native
target; keep same-window operations in your internal UI drag handling. On macOS and Wayland, a drag
that returns to the source window can deliver ordinary `FileDropped` events there; keep same-window
operations internal on those platforms as well.

Applications can start the native drag as soon as their own drag passes its threshold and keep the
internal drag running beside it, as file managers do. Files can then be dropped on any window,
including one the user switches to with Alt+Tab mid-drag. Internal drag feedback continues while
the source window still receives the pointer. Ignore the internal drag's release while the native
session is active and resolve it from the completion instead. On X11, a release over the source
window ends the session `Cancelled` with `DragResult::releasedOverSource` set, so the application
completes it as its internal drop. Elsewhere, such a drop arrives as `FileDropped` events.

Applications that install their own `SDL_SetX11EventHook` must coordinate its use: SDL3 has a single
hook slot, which eepp uses during an active native drag and clears afterwards.

## Run the example

The complete [example](../../src/examples/native_drag_and_drop/native_drag_and_drop.cpp) demonstrates
a draggable widget snapshot, status feedback and a receiving window:

```sh
bin/eepp-native-drag-and-drop /absolute/file.txt /absolute/another-file.txt
```

Run `bin/eepp-native-dnd-tests` without arguments for a manual playground with temporary sample
files, or supply paths with `--manual`. You control the pointer; drag the card into the receiving
window, a file manager or a browser upload input. Escape cancels the gesture.

For automated Wayland integration checks, build `eepp-native-dnd-tests` and run:

```sh
python3 projects/scripts/test-wayland-dnd.py
```

This requires Weston with its X11 backend, Xvfb, xdotool and ImageMagick. It creates an isolated
Weston session and injects input only there; it does not move your desktop pointer. Protocol logs
are saved in the temporary directory printed by the runner. Normal manual runs never inject input.
