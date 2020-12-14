set(TargetName hptest)

set(TargetSRC 
    hexprint.cpp
)


include_directories(${Utils_INCLUDE})
add_executable(${TargetName} ${TargetSRC})
target_link_libraries(${TargetName} Utils)

copy_install_target(${TargetName})