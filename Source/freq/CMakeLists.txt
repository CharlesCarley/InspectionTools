set(TargetName freq)

set(TargetSRC 
    freq.cpp
    freqApp.cpp
    freqApp.h
)

add_executable(${TargetName} ${TargetSRC})

if (InspectionTools_BUILD_SDL)
    include_directories(${Utils_INCLUDE} ${Math_INCLUDE} ${SDL_INCLUDE})
    target_link_libraries(${TargetName} Utils Math ${SDL_LIBS})
else()
    include_directories(${Utils_INCLUDE})
    target_link_libraries(${TargetName} Utils)
endif()

copy_install_target(${TargetName})