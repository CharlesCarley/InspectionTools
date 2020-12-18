# -----------------------------------------------------------------------------
#   This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
#   Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.
# ------------------------------------------------------------------------------
include(StaticRuntime)
include(CopyTarget)
set_static_runtime()


set_property(GLOBAL PROPERTY USE_FOLDERS ON)


set(INSTALL_PATH  CACHE STRING "")
set(COPY_ON_BUILD CACHE BOOL OFF)
set(BUILD_SDL     CACHE BOOL OFF)

set(InspectionTools_INSTALL_PATH   ${INSTALL_PATH})
set(InspectionTools_COPY_ON_BUILD   ${COPY_ON_BUILD})
set(InspectionTools_BUILD_SDL       ${BUILD_SDL})


set(Utils_INCLUDE ${InspectionTools_SOURCE_DIR}/Extern)
set(Math_INCLUDE  ${InspectionTools_SOURCE_DIR}/Extern)
set(SDL_INCLUDE   ${InspectionTools_SOURCE_DIR}/Extern/SDL)
set(SDL_LIBS      SDL2-static)




# -----------------------------------------------------------------------------
#                            Show config
# -----------------------------------------------------------------------------
macro(log_config)
    message(STATUS "-----------------------------------------------------------")
    message(STATUS "")
    message(STATUS "---------- Directories -----")
    message(STATUS "SDL                         : ${SDL_INCLUDE}")
    message(STATUS "Math                        : ${Math_INCLUDE}")
    message(STATUS "Utils                       : ${Utils_INCLUDE}")
    message(STATUS "---------- Extras ----------")
    message(STATUS "Install path                : ${InspectionTools_INSTALL_PATH}")
    message(STATUS "Copy on build               : ${InspectionTools_COPY_ON_BUILD}")
    message(STATUS "Building SDL                : ${InspectionTools_BUILD_SDL}")
    message(STATUS "----------------------------")
    message(STATUS "-----------------------------------------------------------")

endmacro(log_config)
log_config()
