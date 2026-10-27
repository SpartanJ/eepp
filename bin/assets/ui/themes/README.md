# Bundled UI color schemes

ecode lists these palettes in **Settings → Appearance → Theme & Language → UI Theme**,
alongside user themes. Select the matching syntax scheme under **Editor → Appearance**.
UI and syntax schemes can also be selected independently.

Each CSS file changes only `:root` variables and inherits Breeze's widget styles through
`--inherit-base-theme: true`. The optional `--color-scheme: light` or `dark` selects the
appropriate UI media rules while the palette is active. Returning to a theme without this
variable restores the user's configured appearance preference.

A file with the same name in the user's `themes` directory overrides a bundled palette.

Tokyo Night, Tokyo Night Storm, and Tokyo Night Light are adapted from
[Enkia's VS Code themes](https://github.com/tokyo-night/tokyo-night-vscode-theme).
The palettes preserve their workbench colors where eepp has an equivalent shared variable.
Shared text uses the upstream editor foreground for readability, and hint text retains a
minimum 4.5:1 contrast against the main background. The upstream translucent scrollbars
are intentionally subtle.

See [the upstream MIT license](../../colorschemes/tokyo-night.LICENSE.txt).
