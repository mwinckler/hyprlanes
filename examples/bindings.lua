-- Load this file after your regular keybindings.
local function bind(keys, message, fallback)
    hl.unbind(keys)
    hl.bind(keys, function()
        local workspace = hl.get_active_special_workspace() or hl.get_active_workspace()
        local window = hl.get_active_window()
        local in_lane = workspace and workspace.tiled_layout == "hyprlanes" and window and not window.floating
        hl.dispatch(in_lane and hl.dsp.layout(message) or fallback or hl.dsp.no_op())
    end, { description = "Run Hyprlanes command '" .. message .. "'." })
end

-- Super moves focus, and Shift moves the window.
bind("SUPER + LEFT", "focus left", hl.dsp.focus({ direction = "l" }))
bind("SUPER + RIGHT", "focus right", hl.dsp.focus({ direction = "r" }))
bind("SUPER + UP", "focus up", hl.dsp.focus({ direction = "u" }))
bind("SUPER + DOWN", "focus down", hl.dsp.focus({ direction = "d" }))

bind("SUPER + SHIFT + LEFT", "move left", hl.dsp.window.swap({ direction = "l" }))
bind("SUPER + SHIFT + RIGHT", "move right", hl.dsp.window.swap({ direction = "r" }))
bind("SUPER + SHIFT + UP", "move up", hl.dsp.window.swap({ direction = "u" }))
bind("SUPER + SHIFT + DOWN", "move down", hl.dsp.window.swap({ direction = "d" }))

-- Ctrl inserts a new lane above or below the current one.
bind("SUPER + CTRL + UP", "newrow up")
bind("SUPER + CTRL + DOWN", "newrow down")

-- Remove Omarchy's physical-key resize bindings before binding the key symbols.
hl.unbind("SUPER + code:20")
hl.unbind("SUPER + code:21")
hl.unbind("SUPER + SHIFT + code:20")
hl.unbind("SUPER + SHIFT + code:21")
bind("SUPER + MINUS", "width -0.1", hl.dsp.window.resize({ x = -100, y = 0, relative = true }))
bind("SUPER + EQUAL", "width +0.1", hl.dsp.window.resize({ x = 100, y = 0, relative = true }))
bind("SUPER + SHIFT + MINUS", "height -0.1", hl.dsp.window.resize({ x = 0, y = -100, relative = true }))
bind("SUPER + SHIFT + EQUAL", "height +0.1", hl.dsp.window.resize({ x = 0, y = 100, relative = true }))

bind("SUPER + ALT + C", "center", hl.dsp.window.center())
hl.unbind("SUPER + F")
hl.bind("SUPER + F", hl.dsp.window.fullscreen(), { description = "Toggle fullscreen." })
