# Desktop notifications

`EE::System::Notification` is a static desktop-notification API. It uses Apple's UserNotifications
framework on macOS and eepp's existing portable-file-dialogs backend on Windows and Linux/BSD.
Applications include only the eepp header. It submits non-interactive OS notifications, with no
dialog, confirmation buttons, or keyboard focus change.

```cpp
#include <eepp/system/notification.hpp>
#include <eepp/system/log.hpp>

using namespace EE::System;

// On a background thread.
std::string error;
if ( !Notification::show( "My application: build finished", "All targets built successfully.", &error ) )
    Log::warning( "%s", error.c_str() );
```

There is no instance to retain or shut down. The platform backend owns persistent native resources, including
its Windows notification-area icon. Notification calls are serialized. Calls are synchronous and
can wait for helper discovery or submission, so applications should run them on a worker.
Applications decide when to notify and should rate-limit repeated changes themselves.

`show()` takes UTF-8 title/body strings as text, with native delivery on macOS and platform quoting delegated to PFD elsewhere. Embedded
NUL bytes are rejected; desktop-helper markup is escaped. The optional error output describes
validation, backend availability, helper exit failures, or submission timeout failures. Helper
submission is limited to five seconds after backend discovery. macOS authorization can wait up to 60 seconds for
the user to answer the first permission prompt. A true result means the backend
submitted the request without reporting an error; it does not confirm that the OS displayed a
banner. Permissions and Do Not Disturb can still hide a notification, and system notification
sounds follow OS preferences.

`isSupported()` checks backend availability, without requesting permissions or notifying:

- Windows uses the native notification area. The OS may truncate title/body text.
- macOS 10.14+ uses `UNUserNotificationCenter` inside the signed app bundle. The first request
  asks permission for that app, which appears under its own identity in System Settings →
  Notifications. Denied permission is reported as an error; enable it in System Settings to retry.
  Standalone executables are rejected with an explanation rather than using an unrelated identity.
  Foreground banners are supported by a process-wide delegate, unless the embedding app already
  supplies its own delegate. Notification content is silent; configurable app audio stays separate.
- Linux/BSD use Zenity, MateDialog, Qarma or KDialog and require a working desktop notification
  service. PFD handles helper selection.
- Other platforms return false.
