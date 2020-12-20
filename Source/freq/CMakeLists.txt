set(TargetName freq)

set(Target_SRC  freq.cpp)

if (InspectionTools_BUILD_SDL)
    set(Target_SRC_EX
        freqApp.cpp
        freqApp.h
        freqFont.cpp
        freqFont.h
    )
    set(Target_INC ${Utils_INCLUDE} 
                   ${Math_INCLUDE} 
                   ${SDL_INCLUDE})
    set(Target_LNK  Utils 
                    Math 
                    ${SDL_LIBS})
    add_definitions(-DUSING_SDL)
else()
    set(Target_INC ${Utils_INCLUDE})
    set(Target_LNK  Utils)
endif()

add_executable(${TargetName} 
               ${Target_SRC}
               ${Target_SRC_EX})

include_directories(${Target_INC})
target_link_libraries(${TargetName} ${Target_LNK})

copy_install_target(${TargetName})
