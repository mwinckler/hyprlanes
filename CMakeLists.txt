cmake_minimum_required(VERSION 3.25)
project(hyprlanes VERSION 0.1.1 LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(HYPRLAND REQUIRED IMPORTED_TARGET hyprland>=0.56.2)

add_library(hyprlanes MODULE src/Plugin.cpp src/Hyprlanes.cpp src/TapeLayout.cpp)
target_compile_features(hyprlanes PRIVATE cxx_std_23)
target_compile_options(hyprlanes PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(hyprlanes PRIVATE PkgConfig::HYPRLAND)
set_target_properties(hyprlanes PROPERTIES PREFIX "")

include(GNUInstallDirs)
install(TARGETS hyprlanes LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/hyprland)
install(PROGRAMS scripts/hyprlanes-layout-cycle DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES examples/bindings.lua examples/omarchy.lua DESTINATION ${CMAKE_INSTALL_DATADIR}/hyprlanes)
