set(TargetName fimg)

set(Target_SRC
    fimg.cpp
    fimgApp.cpp
    fimgApp.h
    fimgPixelMap.cpp
    fimgPixelMap.h
    ../common/freqFont.cpp
    ../common/freqFont.h
    ../common/drawUtils.cpp
    ../common/drawUtils.h
)
set(Target_INC ${Utils_INCLUDE} 
                ${Math_INCLUDE} 
                ${SDL_INCLUDE}
                ../common)

set(Target_LNK  Utils 
                Math 
                ${SDL_LIBS})
add_definitions(-DUSING_SDL)
add_executable(${TargetName} 
               ${Target_SRC}
               ${Target_SRC_EX})

include_directories(${Target_INC} ${Image_INCLUDE})
target_link_libraries(${TargetName} ${Target_LNK} )

copy_install_target(${TargetName})
