# Hyprlanes

Hyprlanes is a C++ Hyprland layout plugin with independently scrolling horizontal lanes (rows) on a vertically scrolling workspace.

## Installation

Install [hyprpm and its build dependencies](https://wiki.hypr.land/hyprland-plugins/using-plugins/#hyprpm). On Arch Linux and Omarchy, use:

```sh
sudo pacman -S --needed hyprpm base-devel git cmake cpio meson
```

Then build and enable Hyprlanes through hyprpm:

```sh
hyprpm update
hyprpm add https://github.com/mwinckler/hyprlanes
hyprpm enable hyprlanes
hyprpm reload
```

Hyprlanes **0.1.0** is tested with **Hyprland 0.56.2**. The `hyprpm.toml` manifest on `master` maps supported Hyprland releases to compatible source commits. Hyprpm builds that source against matching Hyprland headers. Additional Hyprland releases require source-compatibility verification as well as rebuilding.

### Install the optional integration files

The supplied Lua files provide lane-aware shortcuts and Omarchy layout cycling. Download the release and install these files for your user:

```sh
curl -fL -o hyprlanes-0.1.0.tar.gz https://github.com/mwinckler/hyprlanes/archive/refs/tags/v0.1.0.tar.gz
tar -xzf hyprlanes-0.1.0.tar.gz
cd hyprlanes-0.1.0
install -Dm644 examples/bindings.lua "$HOME/.local/share/hyprlanes/bindings.lua"
install -Dm644 examples/omarchy.lua "$HOME/.local/share/hyprlanes/omarchy.lua"
install -Dm755 scripts/hyprlanes-layout-cycle "$HOME/.local/bin/hyprlanes-layout-cycle"
```

Keep `~/.local/bin` on your `PATH` so Omarchy can find the cycle command. It uses Bash, jq, and Omarchy's workspace commands. Load the files using the [persistent configuration](#persistent-configuration) below; personal shortcuts can override the supplied defaults.

## Build from source

For a manual build, use matching Hyprland development headers (0.56.2 or newer), a C++23 compiler, CMake 3.25 or newer, pkg-config, and Make. Run these commands from the source directory:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 2
```

The output is `build/hyprlanes.so`. The plugin checks the running compositor's build signature, including its commit and supporting-library versions, when loading. Rebuild after upgrading Hyprland or changing those dependencies.

For a temporary trial, load the compiled plugin:

```sh
hyprctl plugin load "$PWD/build/hyprlanes.so"
```

For persistent use, install the plugin and integration files:

```sh
cmake --install build --prefix "$HOME/.local" --strip
```

This installs `~/.local/lib/hyprland/hyprlanes.so` along with the same integration files described above. The [persistent configuration](#persistent-configuration) section includes the loading call for manual builds.

## Try it

Once Hyprlanes is loaded, enable it on workspace 7 and load the installed lane shortcuts:

```sh
hyprctl eval 'hl.workspace_rule({ workspace = "7", layout = "hyprlanes" })'
hyprctl eval 'dofile(os.getenv("HOME") .. "/.local/share/hyprlanes/bindings.lua")'
```

Switch to workspace 7, open a few windows, and press **Super+Shift+Down** to move the focused window to the lane below, creating one at the bottom edge. Open more windows in that lane to grow its horizontal tape. All shortcuts work directly in your regular keybinding mode.

| Keys | Action |
| --- | --- |
| `Super+Left/Right/Up/Down` | Move focus within a lane or to the remembered window in an adjacent lane. |
| `Super+Shift+Left/Right` | Move the focused window left/right within its lane. |
| `Super+Shift+Up/Down` | Move the focused window to the lane above/below, creating a lane at a vertical edge. |
| `Super+Ctrl+Up/Down` | Move the focused window into a new lane above/below. |
| `Super+-` / `Super+=` | Decrease/increase the focused window’s width. |
| `Super+Shift+-` / `Super+Shift+=` | Decrease/increase the focused lane’s height. |
| `Super+Alt+C` | Center the focused window within the available scroll range. |
| `Super+F` | Toggle fullscreen. |

The example replaces the usual Super+arrow focus bindings, Super+Shift+arrow window-swap bindings, and Super+minus/equal resize bindings with layout-aware actions. These use lane commands for tiled windows on Hyprlanes workspaces and standard Hyprland actions on other layouts or floating windows. Super+Ctrl+Up/Down applies to tiled Hyprlanes windows. Super+Alt+C also centers floating windows.

Three-finger touchpad swipes scroll horizontally in the row under the pointer and vertically through the workspace. Gestures follow `input.touchpad.natural_scroll`. Keyboard navigation centers the focused lane vertically, brings its focused window into view horizontally, and remembers the last-focused window in each row. The first and last lanes can also be centered, including a workspace with a single lane.

Moving a window out of a lane and back restores its place beside its former neighbors and its horizontal screen position when possible.

To end this temporary trial, return workspace 7 to Hyprland’s built-in scrolling layout and reload your regular configuration:

```sh
hyprctl eval 'hl.workspace_rule({ workspace = "7", layout = "scrolling" })'
hyprctl reload
```

To disable a hyprpm installation, run `hyprpm disable hyprlanes`. For a manually loaded trial, run `hyprctl plugin unload "$PWD/build/hyprlanes.so"` from the source directory.

## Persistent configuration

For Omarchy, place this block in `~/.config/hypr/hyprland.lua` immediately after `require("default.hypr.omarchy")`, before your personal configuration imports. Replace any existing Hyprlanes loading lines and the existing `require("default.hypr.toggles")` line with this block:

```lua
-- Allow hyprpm to load plugins when permission management is enabled.
hl.permission("/usr/(bin|local/bin)/hyprpm", "plugin", "allow")

-- Load managed plugins, then apply their configuration and saved layouts.
hl.on("hyprland.start", function()
    hl.exec_cmd("hyprpm reload && hyprctl reload")
end)

-- Load lane shortcuts before personal overrides.
pcall(dofile, os.getenv("HOME") .. "/.local/share/hyprlanes/bindings.lua")

-- Restore saved workspace layouts and other dynamic settings.
require("default.hypr.toggles")
```

Hyprpm loads enabled plugins after Hyprland starts. The subsequent `hyprctl reload` reapplies plugin settings and restores saved Hyprlanes layouts once the plugin is registered. The `hyprland.start` event runs once per session, so this configuration reload completes the startup sequence.

`bindings.lua` installs layout-aware lane shortcuts. Your personal configuration imports follow this block so your own shortcuts and settings take precedence.

After all personal configuration imports, load the extended layout cycle so it replaces any earlier bindings for its shortcuts:

```lua
pcall(dofile, os.getenv("HOME") .. "/.local/share/hyprlanes/omarchy.lua")
```

`omarchy.lua` uses `o.rebind(...)` to give each cycling shortcut one action. Use `o.rebind(...)` when replacing a cycle shortcut in your own configuration as well.

For plain Hyprland, use the permission declaration, startup hook, and `bindings.lua` loading call in your Lua configuration. Select Hyprlanes with a [workspace layout rule](#optional-fixed-workspace-layout). Omarchy uses the additional `omarchy.lua` and `default.hypr.toggles` calls to manage its cycling and saved layouts.

`pcall(fn, ...)` runs a protected Lua call and returns its success flag and result or error. This lets configuration loading continue when the plugin or integration files are unavailable. These examples discard those return values; use a direct call or inspect the returned error when diagnosing a load failure.

For a manual installation, replace the permission declaration and startup hook with a synchronous load before the integration files and saved layouts:

```lua
pcall(hl.plugin.load, os.getenv("HOME") .. "/.local/lib/hyprland/hyprlanes.so")
```

For development, point that call at your source-tree build:

```lua
pcall(hl.plugin.load, "/absolute/path/to/hyprlanes/build/hyprlanes.so")
```

The installed Lua files work with either loading method. Use the startup hook for hyprpm or a direct loading call for manual builds. After rebuilding manually, rerun the installation command to update installed files; unload and reload the plugin to activate an updated binary.

Apply the configuration and check that Hyprlanes appears in the plugin list:

```sh
hyprctl reload
hyprctl configerrors
hyprctl plugin list
```

Choose Hyprlanes with **Super+Shift+Ctrl+L** or **Super+Alt+L**. Omarchy saves and restores the selected layout. Remove any existing fixed `hl.workspace_rule(...)` layout rules for workspaces managed by the cycle so their saved selections control the layout.

### Optional settings

To override the defaults, add this settings block after the startup hook or direct plugin-loading call, before saved workspace layouts are restored. The values shown are the defaults:

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

`column_width` sets the width of new windows, and `row_height` sets the initial row height when switching to Hyprlanes or opening the first window in an empty workspace. A configured `row_height` overrides the `0.95` default. New rows created by moving a window inherit the source row’s current height. Both settings accept fractions from `0.1` to `1.0` of the usable monitor area. `gesture_fingers` accepts `0` through `5`, with `0` disabling gesture handling. `gesture_sensitivity` accepts `0.1` through `10.0`.

### Optional fixed workspace layout

To pin a workspace’s tiled layout to Hyprlanes, add an explicit rule after your plugin settings and any saved-layout restoration, using your preferred workspace number:

```lua
pcall(hl.workspace_rule, { workspace = "7", layout = "hyprlanes" })
```

This fixed rule takes precedence over saved layout selections on each reload. Use the cycle to manage workspaces whose layout should change interactively.

## Updating

After upgrading Hyprland and starting the updated session, rebuild the managed plugins and activate the new binaries:

```sh
hyprpm update
hyprpm reload -f
hyprctl reload
hyprctl configerrors
```

Use the same commands to receive compatible Hyprlanes updates. The forced plugin reload activates rebuilt binaries even when an older copy is already loaded. Download the desired release and repeat the integration-file installation commands to update the companion files. Keep custom bindings in your personal configuration or a personal copy of the Lua files.

Release maintainers can follow [RELEASING.md](RELEASING.md) to validate a new version, tag its source, and update compatibility pins.

## Custom shortcuts

Every lane command is user-bindable through `hl.dsp.layout(...)`. Edit a personal copy of `bindings.lua` and update its loading path to customize the keys while keeping its layout-aware routing, or replace its `pcall(dofile, ...)` line with your own bindings:

```lua
local function lane_focus(direction)
    return function()
        local workspace = hl.get_active_special_workspace() or hl.get_active_workspace()
        local window = hl.get_active_window()
        local in_lane = workspace and workspace.tiled_layout == "hyprlanes" and window and not window.floating
        hl.dispatch(in_lane and hl.dsp.layout("focus " .. direction) or hl.dsp.focus({ direction = direction }))
    end
end

hl.unbind("SUPER + H")
hl.bind("SUPER + H", lane_focus("l"))
hl.unbind("SUPER + J")
hl.bind("SUPER + J", lane_focus("d"))
hl.unbind("SUPER + K")
hl.bind("SUPER + K", lane_focus("u"))
hl.unbind("SUPER + L")
hl.bind("SUPER + L", lane_focus("r"))
```

Super+H and Super+L move focus within the current lane and stop at its left or right edge. Super+J and Super+K focus the remembered window in the adjacent lane regardless of its horizontal position. These bindings use standard directional focus on other layouts and floating windows. `hl.dsp.focus(...)` uses Hyprland’s spatial navigation; custom lane-focus shortcuts should route all four directions through Hyprlanes’ `focus left/right/up/down` layout commands, as shown above.

Other commands, such as `hl.dsp.layout("move down")` and `hl.dsp.layout("newrow down")`, are directly bindable and act on the active workspace’s layout. Use `hl.unbind(...)` before replacing an existing shortcut so each keypress runs one action.

## Omarchy layout cycling

`hyprlanes-layout-cycle [next|back]` extends Omarchy’s workspace cycle to **dwindle → scrolling → hyprlanes → floating** while Hyprlanes is loaded. It delegates the built-in transitions to `omarchy-hyprland-workspace-layout-toggle`, including Omarchy’s floating-window handling, notifications, and workspace state. The `back` argument traverses the same cycle in reverse.

The [persistent configuration](#persistent-configuration) above enables this integration. `omarchy.lua` replaces Super+Shift+Ctrl+L’s forward layout cycle and Super+Alt+L’s backward layout cycle with the extended cycle. To choose other shortcuts, edit a personal copy of `omarchy.lua` and update its loading path. Run `hyprctl reload` and `hyprctl configerrors` after updating your configuration.

Returning to the global default clears the saved workspace override, so the workspace follows that default across reloads.

While Hyprlanes is unavailable, the integration uses Omarchy’s original cycling command and restores saved Hyprlanes selections as scrolling. Removing `hyprlanes-layout-cycle` also restores the original command through the binding’s availability check; removing the integration file lets your regular bindings take effect on the next reload.

## Layout commands

Commands work with `hl.dsp.layout(...)` in keybindings or through the CLI:

```sh
hyprctl dispatch 'hl.dsp.layout("newrow down")'
hyprctl dispatch 'hl.dsp.layout("focus up")'
hyprctl dispatch 'hl.dsp.layout("scroll 0.25 0")'
```

| Command | Behavior |
| --- | --- |
| `focus left/right/up/down` | Focus the adjacent window or the remembered window in an adjacent row. |
| `move left/right/up/down` | Reorder a window or transfer it to an adjacent row, creating a row at a vertical edge. |
| `newrow up/down` | Extract the focused window into a new adjacent row. |
| `width 0.5` / `width +0.1` | Set or adjust the focused window’s width. |
| `height 0.5` / `height +0.1` | Set or adjust the focused row’s height. |
| `scroll dx dy` | Move the active row’s horizontal camera and workspace’s vertical camera by viewport fractions. |
| `center` | Center the focused window within the scrollable extent. |

The directional commands also accept `l`, `r`, `u`, and `d`. Standard Hyprland resizing, floating, grouping, workspace transfers, and fullscreen handling remain available.

## Implementation

`src/TapeLayout.cpp` owns the rows, sizes, camera offsets, and navigation in normalized coordinates. `src/Hyprlanes.cpp` connects that model to Hyprland’s tiling algorithm and input events. Hyprland handles window animation, decorations, monitor clipping, and fullscreen presentation.
