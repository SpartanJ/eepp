# Bundled UI color schemes

ecode lists these palettes in **Settings → Appearance → Theme & Language → UI Theme**,
alongside user themes. Select the matching syntax scheme under **Editor → Appearance**.
Choose the terminal scheme under **Terminal → Appearance**. UI, syntax, and terminal
schemes can be selected independently.

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

Catppuccin Latte, Frappé, Macchiato, and Mocha are adapted from
[Catppuccin for VS Code](https://github.com/catppuccin/vscode) and the
[official palette](https://github.com/catppuccin/palette), using the default mauve accent.
Matching editor and terminal schemes are included for all four flavors. The terminal schemes
use the official 16 ANSI colors, including distinct normal and bright colors.

The UI palettes map workbench colors onto Breeze's shared variables, retaining its widgets
and layout. Shared control backgrounds use the upstream dropdown surface. Hint text is
adjusted only where necessary to retain 4.5:1 contrast against the main and sidebar backgrounds;
Latte's selected text uses its base color to keep contrast against mauve. Syntax colors use
the closest shared token categories; language-specific TextMate and semantic distinctions
cannot all be represented by those categories.

See [Catppuccin's MIT license](../../colorschemes/catppuccin.LICENSE.txt). The source revisions
are recorded in the CSS and color scheme files.
