-- Load this file after your regular Omarchy keybindings.
local function cycle(direction)
    local command = o.cmd_present("hyprlanes-layout-cycle")
        and "hyprlanes-layout-cycle" or "omarchy-hyprland-workspace-layout-toggle"
    hl.exec_cmd(command .. " " .. direction)
end

o.rebind("SUPER + SHIFT + CTRL + L", "Cycle workspace layout forward.", function() cycle("next") end)
o.rebind("SUPER + ALT + L", "Cycle workspace layout backward.", function() cycle("back") end)
