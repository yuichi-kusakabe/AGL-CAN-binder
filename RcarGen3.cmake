set(CROSS_PREFIX aarch64-agl-linux-)
set(CROSS_TOOLCHAIN_PATH /home/Devel/AGL/RCarGen3-agl/build/tmp/sysroots/m3ulcb)

# specify the cross-toolchain (compiler, header and library directories)
#set(CMAKE_CXX_FLAGS "--sysroot=${CROSS_TOOL_CHAIN_PATH}")
#set(CMAKE_CXX_LINK_FLAGS "--sysroot=${CROSS_TOOL_CHAIN_PATH}")
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER ${CROSS_PREFIX}gcc --sysroot=${CROSS_TOOLCHAIN_PATH})
set(CMAKE_CXX_COMPILER ${CROSS_PREFIX}g++)
set(CMAKE_INCLUDE_PATH ${CROSS_TOOLCHAIN_PATH}/usr/include)
set(CMAKE_LIBRARY_PATH ${CROSS_TOOLCHAIN_PATH}/usr/lib)
set(CMAKE_INSTALL_PREFIX /home/Devel/AGL/AFB_CANIVI/af-canivi-binding/git/AGL-CAN-binder/build)
#set(CMAKE_FIND_ROOT_PATH ${ARMADEUS_ROOT_CTC} ${CMAKE_INCLUDE_PATH})

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH ${CROSS_TOOLCHAIN_PATH})

# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
