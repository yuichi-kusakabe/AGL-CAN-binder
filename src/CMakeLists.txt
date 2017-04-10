cmake_minimum_required(VERSION 3.3)

###########################################################################

link_libraries(-Wl,--as-needed -Wl,--gc-sections)

add_compile_options(-Wall -Wextra -Wconversion)
add_compile_options(-Wno-unused-parameter) # frankly not using a parameter does it care?
add_compile_options(-Werror=maybe-uninitialized)
add_compile_options(-Werror=implicit-function-declaration)
add_compile_options(-ffunction-sections -fdata-sections)
add_compile_options(-Wl,--as-needed -Wl,--gc-sections)
add_compile_options(-fPIC)

set(CMAKE_C_FLAGS_PROFILING    "-g -O0 -pg -Wp,-U_FORTIFY_SOURCE")
set(CMAKE_C_FLAGS_DEBUG        "-g -O0 -ggdb -Wp,-U_FORTIFY_SOURCE")
set(CMAKE_C_FLAGS_RELEASE      "-g -O2")
set(CMAKE_C_FLAGS_CCOV         "-g -O2 --coverage")

###########################################################################

include(FindPkgConfig)

#*<NESSARY>  pkg_check_modules(EXTRAS REQUIRED json-c libsystemd afb-daemon)
add_compile_options(${EXTRAS_CFLAGS})
include_directories(${EXTRAS_INCLUDE_DIRS})
link_libraries(${EXTRAS_LIBRARIES})

###########################################################################
# the binding for afb

message(STATUS "Creation af-canivi-binding for AFB-DAEMON")

###############################################################
#pkg_get_variable(afb_binding_install_dir afb-daemon binding_install_dir)

#*<NESSARY>
#* execute_process(
#*	COMMAND pkg-config --variable binding_install_dir afb-daemon
#*	OUTPUT_VARIABLE afb_binding_install_dir OUTPUT_STRIP_TRAILING_WHITESPACE
#* )

###############################################################
add_library(af-canivi-binding MODULE af-canivi-binding.c bind_event.c can_json.c canid_info.c canid_search.c can_signal_event.c socketcan_raw.c)
set_target_properties(af-canivi-binding PROPERTIES
	PREFIX ""
	LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/export.map"
)

#*<NESSARY>  install(TARGETS af-canivi-binding LIBRARY DESTINATION ${afb_binding_install_dir})
# <OR following packaging>
##############################################################
# WGT packaging

#message(STATUS "Creation of af-canivi.wgt package for AppFW")
##############################################################
#configure_file(config.xml.in config.xml)
#add_custom_command(
#    OUTPUT af-canivi.wgt
#    COMMAND rm -rf package
#    COMMAND mkdir -p package/libs package/htdocs
#    COMMAND cp config.xml package/
#    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_ICON} package/icon.png
#    COMMAND cp af-canivi-binding.so package
#    COMMAND wgtpkg-pack -f -o af-canivi.wgt package
#)
#add_custom_target(widget ALL DEPENDS af-canivi.wgt)

