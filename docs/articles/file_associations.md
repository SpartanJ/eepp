# File and Folder Associations

`EE::System::FileAssociation` registers per-user file handlers. Use extension registration for
filename suffixes, and direct MIME registration for types such as folders that have no extension.
`EE::UI::Tools::FileAssociationsSettings` supplies settings controls for both forms.

## Direct MIME registration

Direct MIME registration is available on XDG desktops (Linux and BSD). Check
`FileAssociation::supportsMimeTypes()` before offering it. Other platforms return a failure and an
explanation through `getLastError()`.

```cpp
#include <eepp/system/fileassociation.hpp>

using namespace EE::System;

FileAssociation association( { "my-files", "My Files", executablePath, iconPath } );
if ( FileAssociation::supportsMimeTypes() ) {
    // Run registration on a worker thread, since it writes files and updates desktop databases.
    bool success = association.setRegisteredMimeTypes(
        { "inode/directory" }, { "inode/directory" }, true );
}
```

The first list selects the registered subset of the supported types in the second list. Types are
case-insensitive and must be exact MIME names; wildcards and malformed names fail without changing
the registration. Selecting `inode/directory` makes the application the default folder handler.
The generated desktop entry advertises the selected types, and the per-user MIME application
preferences retain other handlers after the application in their priority lists.

Pass an empty selection to remove the application's direct MIME registrations. Other handlers
remain available, including the previous default. Extension registrations are independent and are
preserved when changing MIME registrations; changing extensions also preserves direct MIME types.

The optional third argument controls whether the application launcher appears in desktop menus.
When it is omitted, visibility is preserved. Hiding a launcher retains its desktop entry while any
association needs it. Clearing the last association while the launcher is hidden removes the entry.

`getRegisteredMimeTypes(supportedTypes)` returns this application's directly registered subset,
sorted and lower-case. It reports the application's registration, which may differ from the current
default if another application has subsequently changed the user's preference.

## Settings controls

Add a category to an existing `UISettingsPanel`, then use `addMimeTypesTo()`:

```cpp
using namespace EE::UI::Tools;

FileAssociationsSettings::addLauncherTo( panel, "my-files" );
FileAssociationsSettings::addMimeTypesTo(
    panel,
    { "folderAssociations", "general.desktop_integration", "Use My Files as the Default File Manager",
      "Open folders from other applications in My Files." },
    FileAssociationsSettings::applicationForCurrentProcess( "my-files" ),
    { "inode/directory" }, threadPool );
```

The checkbox applies changes immediately on the supplied thread pool. Checking it registers the
supplied MIME types together, and unchecking it removes those associations. Launcher visibility is
preserved in both cases. The control is disabled while registration runs; failures restore its
previous value. The optional completion callback runs on the UI thread while the panel is alive.

Existing `addTo()` calls continue to manage extension associations, and `addLauncherTo()` provides
launcher-only controls.

In eefm, open **Configure eefm → General → Desktop Integration** and check
**Use eefm as the Default File Manager**. The launcher install option is on the same page.
