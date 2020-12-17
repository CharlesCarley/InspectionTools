macro(copy_target TARNAME DESTDIR)

    if (NOT ${DESTDIR} STREQUAL "")

        add_custom_command(TARGET ${TARNAME} 
                           POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARNAME}> 
                           "${DESTDIR}/$<TARGET_FILE_NAME:${TARNAME}>"
                           COMMENT "    Copying target ${TARNAME} to ${DESTDIR}"
                           )

        if (MSVC)
            set_target_properties(
                ${TARNAME} 
                PROPERTIES 
                VS_DEBUGGER_COMMAND  
               "${DESTDIR}/$<TARGET_FILE_NAME:${TARNAME}>"
            )
        endif()
    endif()

endmacro(copy_target)



macro(copy_install_target TARNAME)

    if (NOT ${InspectionTools_INSTALL_PATH} STREQUAL "")
        if (NOT ${ARGN} STREQUAL "")
            install(TARGETS ${TARNAME}
                    DESTINATION "${InspectionTools_INSTALL_PATH}/${ARGN}/"
	        )
        else()
            install(TARGETS ${TARNAME}
                    DESTINATION "${InspectionTools_INSTALL_PATH}"
	        )
        endif()
    endif()

    if (InspectionTools_COPY_ON_BUILD)
        copy_target(${TARNAME} ${InspectionTools_INSTALL_PATH})
    endif()

endmacro(copy_install_target)
