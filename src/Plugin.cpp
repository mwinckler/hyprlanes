#include "Hyprlanes.hpp"

#include <hyprland/src/config/values/types/FloatValue.hpp>
#include <hyprland/src/config/values/types/IntValue.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include <stdexcept>

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    if (__hyprland_api_get_hash() != std::string(__hyprland_api_get_client_hash())) {
        throw std::runtime_error("Rebuild hyprlanes against the running Hyprland version.");
    }
    using namespace Config::Values;
    const bool configured =
        HyprlandAPI::addConfigValueV2(handle, makeShared<CFloatValue>("plugin:hyprlanes:column_width",
                                                                      "The default window width as a viewport fraction.", 0.5F,
                                                                      SFloatValueOptions{.min = 0.1F, .max = 1.0F})) &&
        HyprlandAPI::addConfigValueV2(handle, makeShared<CFloatValue>("plugin:hyprlanes:row_height",
                                                                      "The default row height as a viewport fraction.", 0.95F,
                                                                      SFloatValueOptions{.min = 0.1F, .max = 1.0F})) &&
        HyprlandAPI::addConfigValueV2(handle, makeShared<CIntValue>("plugin:hyprlanes:gesture_fingers",
                                                                    "The number of fingers used for scrolling; zero disables gestures.", 3,
                                                                    SIntValueOptions{.min = 0, .max = 5})) &&
        HyprlandAPI::addConfigValueV2(handle, makeShared<CFloatValue>("plugin:hyprlanes:gesture_sensitivity",
                                                                      "The multiplier for touchpad scrolling distance.", 1.0F,
                                                                      SFloatValueOptions{.min = 0.1F, .max = 10.0F}));
    if (!configured ||
        !HyprlandAPI::addTiledAlgo(handle, "hyprlanes", &typeid(Hyprlanes::Algorithm), [] { return makeUnique<Hyprlanes::Algorithm>(); })) {
        throw std::runtime_error("Failed to register the hyprlanes layout.");
    }
    return {"hyprlanes", "Independent horizontal scrolling rows on a vertically scrolling workspace.", "Hyprlanes contributors", "0.1.0"};
}
