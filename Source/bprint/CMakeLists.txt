set(TargetName bprint)

set(TargetSRC 
    bprint.cpp
)

include_directories(${Utils_INCLUDE})
add_executable(${TargetName} ${TargetSRC})
target_link_libraries(${TargetName} Utils)


copy_install_target(${TargetName})



if (InspectionTools_COPY_ON_BUILD)
    copy_target(${TargetName} ${InspectionTools_INSTALL_PATH})
endif()