# Hyprlanes

Hyprlanes is a Hyprland layout plugin that arranges windows in horizontal lanes. Each lane scrolls independently from side to side, and the workspace scrolls up and down between lanes.

![Demo](./hyprlanes-demo.mp4)

Hyprlanes **0.1.1** is tested with **Hyprland 0.56.2**. The setup examples below use Hyprland’s Lua configuration.

## Installation

### 1. Install the plugin

Use **hyprpm** for the simplest installation and updates. On Arch Linux and Omarchy, install the build tools and the utilities used below:

```sh
sudo pacman -S --needed hyprpm base-devel git cmake cpio meson curl jq
```

On other distributions, install `curl` and `jq`, then follow [Hyprland’s hyprpm instructions](https://wiki.hypr.land/hyprland-plugins/using-plugins/#hyprpm).

Build and enable Hyprlanes:

```sh
hyprpm update
hyprpm add https://github.com/mwinckler/hyprlanes
hyprpm enable hyprlanes
hyprpm reload
```

<details>
<summary>You can build and install from source instead.</summary>

For a manual build, install development headers matching your running Hyprland, a C++23 compiler, CMake 3.25 or newer, pkg-config, and Make. Download the source release, build it, and install it for your user:

```sh
curl -fL -o hyprlanes-0.1.1.tar.gz https://github.com/mwinckler/hyprlanes/archive/refs/tags/v0.1.1.tar.gz
tar -xzf hyprlanes-0.1.1.tar.gz
cd hyprlanes-0.1.1
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 2
cmake --install build --prefix "$HOME/.local" --strip
hyprctl plugin load "$HOME/.local/lib/hyprland/hyprlanes.so"
```

This also installs the shortcuts and Omarchy integration, so continue with [Try it](#try-it). For permanent setup, use the [manual plugin-loading call](#manual-installations).

</details>

### 2. Install the shortcuts

Install the supplied keyboard shortcuts and Omarchy layout-cycle integration:

```sh
curl -fL -o hyprlanes-0.1.1.tar.gz https://github.com/mwinckler/hyprlanes/archive/refs/tags/v0.1.1.tar.gz
tar -xzf hyprlanes-0.1.1.tar.gz
cd hyprlanes-0.1.1
install -Dm644 examples/bindings.lua "$HOME/.local/share/hyprlanes/bindings.lua"
install -Dm644 examples/omarchy.lua "$HOME/.local/share/hyprlanes/omarchy.lua"
install -Dm755 scripts/hyprlanes-layout-cycle "$HOME/.local/bin/hyprlanes-layout-cycle"
```

For Omarchy layout cycling, keep `~/.local/bin` on your session’s `PATH`. The helper uses Bash, jq, and Omarchy’s workspace commands.

## Try it

Try Hyprlanes on workspace 7 before changing your configuration:

```sh
hyprctl eval 'hl.workspace_rule({ workspace = "7", layout = "hyprlanes" })'
hyprctl eval 'dofile(os.getenv("HOME") .. "/.local/share/hyprlanes/bindings.lua")'
```

Switch to workspace 7 and open a few windows. Press **Super+Shift+Down** to move the focused window into a new lane below. Open more windows to fill that lane, then use **Super+arrow keys** to move between windows and lanes. You can also scroll with three-finger touchpad swipes.

To end the trial, switch workspace 7 to Hyprland’s built-in scrolling layout and restore your regular shortcuts:

```sh
hyprctl eval 'hl.workspace_rule({ workspace = "7", layout = "scrolling" })'
hyprctl reload
```

To load Hyprlanes automatically and keep the shortcuts, follow the setup below.

## Setup

Choose the instructions for **Omarchy** or **plain Hyprland**. Both examples use hyprpm; source builds use the replacement shown under [Manual installations](#manual-installations).

### Omarchy

Edit `~/.config/hypr/hyprland.lua` in two places, keeping your existing bootstrap and personal configuration:

1. Add this block immediately after `require("default.hypr.omarchy")`, before your personal imports. Move the existing `require("default.hypr.toggles")` line into this block so it appears once, and replace any older Hyprlanes loading lines.

```lua
-- Allow hyprpm to load plugins.
hl.permission("/usr/(bin|local/bin)/hyprpm", "plugin", "allow")

-- Load plugins at login, then apply their settings and saved layouts.
hl.on("hyprland.start", function()
    hl.exec_cmd("hyprpm reload && hyprctl reload")
end)

-- Load lane shortcuts before personal overrides.
pcall(dofile, os.getenv("HOME") .. "/.local/share/hyprlanes/bindings.lua")

-- Restore Omarchy’s saved layouts and other dynamic settings.
require("default.hypr.toggles")
```

2. Add this line at the end of the file, after all personal imports and settings, to enable Hyprlanes in Omarchy’s layout cycle.

```lua
pcall(dofile, os.getenv("HOME") .. "/.local/share/hyprlanes/omarchy.lua")
```

Use **Super+Shift+Ctrl+L** to cycle forward or **Super+Alt+L** to cycle backward through **dwindle → scrolling → hyprlanes → floating**. Omarchy saves the selection for each workspace and restores it when you log in.

### Plain Hyprland

Add this to `~/.config/hypr/hyprland.lua` after your regular keybindings. Change `7` to your preferred workspace:

```lua
hl.permission("/usr/(bin|local/bin)/hyprpm", "plugin", "allow")
hl.on("hyprland.start", function()
    hl.exec_cmd("hyprpm reload && hyprctl reload")
end)

pcall(dofile, os.getenv("HOME") .. "/.local/share/hyprlanes/bindings.lua")
pcall(hl.workspace_rule, { workspace = "7", layout = "hyprlanes" })
```

### Manual installations

In either setup above, replace the `hl.permission(...)` declaration and `hl.on(...)` startup hook with this line, before the shortcuts and saved layouts:

```lua
pcall(hl.plugin.load, os.getenv("HOME") .. "/.local/lib/hyprland/hyprlanes.so")
```

### Apply and check your configuration

Reload your configuration, check for errors, and confirm that `hyprlanes` appears in the plugin list:

```sh
hyprctl reload
hyprctl configerrors
hyprctl plugin list
```

## Keyboard shortcuts and gestures

The supplied focus, window-movement, and resize shortcuts adapt to the active layout. On other layouts and floating windows, these shortcuts use standard Hyprland actions.

| Keys | Action |
| --- | --- |
| `Super+Left/Right/Up/Down` | Move focus within a lane or to the last-focused window in an adjacent lane. |
| `Super+Shift+Left/Right` | Move the focused window left or right within its lane. |
| `Super+Shift+Up/Down` / `Super+Shift+K/J` | Move the focused window to the lane above or below, creating one at the top or bottom edge. |
| `Super+Ctrl+Up/Down` | Move the focused window into a new lane above or below. |
| `Super+-` / `Super+=` | Decrease or increase the focused window’s width. |
| `Super+Shift+-` / `Super+Shift+=` | Decrease or increase the focused lane’s height. |
| `Super+Alt+C` | Center the focused window within the available scroll range. |
| `Super+F` | Toggle fullscreen. |

Three-finger touchpad swipes scroll horizontally in the lane under the pointer and vertically through the workspace. Gestures follow `input.touchpad.natural_scroll`.

Keyboard navigation brings the focused window into view and centers its lane vertically. Each lane remembers its last-focused window. Moving a window out of a lane and back restores its place and horizontal position when possible.

## Settings

The defaults work out of the box. To customize them, add this block after plugin loading and before saved layouts or workspace rules:

```lua
pcall(hl.config, {
    plugin = {
        hyprlanes = {
            column_width = 0.5,
            row_height = 0.95,
            gesture_fingers = 3,
            gesture_sensitivity = 1.0,
        },
    },
})
```

| Setting | Default | Meaning |
| --- | --- | --- |
| `column_width` | `0.5` | Set new window widths as a fraction of the usable monitor width, from `0.1` to `1.0`. |
| `row_height` | `0.95` | Set the initial lane height as a fraction of the usable monitor height, from `0.1` to `1.0`. |
| `gesture_fingers` | `3` | Set the swipe finger count from `0` to `5`; use `0` to disable gestures. |
| `gesture_sensitivity` | `1.0` | Set the gesture scroll speed from `0.1` to `10.0`. |

The initial lane height applies when switching to Hyprlanes or opening the first window on an empty workspace. New lanes created by moving a window inherit the source lane’s current height.

To keep a workspace on Hyprlanes, add a rule after plugin settings and saved-layout restoration:

```lua
pcall(hl.workspace_rule, { workspace = "7", layout = "hyprlanes" })
```

A fixed rule takes precedence on every reload. For workspaces managed by Omarchy’s layout cycle, remove fixed layout rules so the saved selection takes effect.

## Updating

To update Hyprlanes, run the commands below. After a Hyprland upgrade, log into the updated session first.

```sh
hyprpm update
hyprpm reload -f
hyprctl reload
hyprctl configerrors
```

To update the shortcuts and Omarchy integration, repeat [installation step 2](#2-install-the-shortcuts) with your desired release. Keep personal changes in your own configuration or a personal copy of the Lua files.

For manual builds, rebuild against matching Hyprland headers and rerun the installation command. Log out and back in, or unload and reload the plugin, to activate the new binary.

## Troubleshooting

- If the plugin fails to load after an upgrade, start the updated Hyprland session and run the [update commands](#updating).
- If shortcuts are missing, run the `dofile(...)` command under [Try it](#try-it) to see any file-loading errors.
- If Omarchy’s layout cycle skips Hyprlanes, check `hyprctl plugin list` and `command -v hyprlanes-layout-cycle`. The helper must be on your session’s `PATH`.
- If a workspace keeps reverting to one layout, check for a fixed `hl.workspace_rule(...)` overriding Omarchy’s saved selection.

To disable a hyprpm installation, run `hyprpm disable hyprlanes`. Remove the Hyprlanes Lua-file loading lines and any fixed Hyprlanes workspace rules from your configuration, then run `hyprctl reload`.

## Custom shortcuts

Copy [examples/bindings.lua](examples/bindings.lua), edit the keys, and update the `pcall(dofile, ...)` path in your configuration to load your copy. This preserves lane-aware navigation and the standard actions for other layouts. Use `hl.unbind(...)` before replacing an existing shortcut.

For different Omarchy layout-cycle shortcuts, customize a copy of [examples/omarchy.lua](examples/omarchy.lua) and update its loading path. Use `o.rebind(...)` when replacing these shortcuts.

After changing shortcuts, run `hyprctl reload` and `hyprctl configerrors`.

## Layout commands

For custom bindings, pass these commands to `hl.dsp.layout(...)`. You can also run them from a terminal:

```sh
hyprctl dispatch 'hl.dsp.layout("newrow down")'
hyprctl dispatch 'hl.dsp.layout("focus up")'
hyprctl dispatch 'hl.dsp.layout("scroll 0.25 0")'
```

| Command | Behavior |
| --- | --- |
| `focus left/right/up/down` | Focus the adjacent window or the last-focused window in an adjacent lane. |
| `move left/right/up/down` | Reorder a window or move it to an adjacent lane, creating one at the top or bottom edge. |
| `newrow up/down` | Move the focused window into a new adjacent lane. |
| `width 0.5` / `width +0.1` | Set or adjust the focused window’s width. |
| `height 0.5` / `height +0.1` | Set or adjust the focused lane’s height. |
| `scroll dx dy` | Scroll horizontally and vertically by fractions of the visible workspace. |
| `center` | Center the focused window within the available scroll range. |

The directional commands also accept `l`, `r`, `u`, and `d`. Standard Hyprland resizing, floating, grouping, workspace transfers, and fullscreen handling remain available.

For release instructions, see [RELEASING.md](RELEASING.md).
