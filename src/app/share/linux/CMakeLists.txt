# SPDX-FileCopyrightText: Team OpenVPI
# SPDX-License-Identifier: Apache-2.0

set(_desktop_id "org.diffscope.${APPLICATION_NAME}")
set(_dspx_icon_name "${_desktop_id}-dspx")
set(_application_executable "$<TARGET_FILE_NAME:${APPLICATION_TARGET}>")

set(_display_name_xml "${APPLICATION_DISPLAY_NAME}")
string(REPLACE "&" "&amp;" _display_name_xml "${_display_name_xml}")
string(REPLACE "<" "&lt;" _display_name_xml "${_display_name_xml}")
string(REPLACE ">" "&gt;" _display_name_xml "${_display_name_xml}")

configure_file(application.desktop.in "${CMAKE_CURRENT_BINARY_DIR}/${_desktop_id}.desktop.in" @ONLY)
file(GENERATE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${_desktop_id}.desktop"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/${_desktop_id}.desktop.in"
)
configure_file(dspx.xml.in "${CMAKE_CURRENT_BINARY_DIR}/${APPLICATION_NAME}-dspx.xml" @ONLY)

if(APPLICATION_INSTALL)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${_desktop_id}.desktop"
        DESTINATION "${CK_INSTALL_SHARE_DIR}/applications"
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${APPLICATION_NAME}-dspx.xml"
        DESTINATION "${CK_INSTALL_SHARE_DIR}/mime/packages"
    )

    foreach(_icon_type IN ITEMS app dspx)
        if(_icon_type STREQUAL "app")
            set(_icon_context apps)
            set(_icon_name "${_desktop_id}")
        else()
            set(_icon_context mimetypes)
            set(_icon_name "${_dspx_icon_name}")
        endif()

        file(GLOB _icon_files "${CMAKE_CURRENT_SOURCE_DIR}/../../icons/${_icon_type}/*.png")
        foreach(_icon_file IN LISTS _icon_files)
            get_filename_component(_icon_size "${_icon_file}" NAME_WE)
            install(FILES "${_icon_file}"
                DESTINATION "${CK_INSTALL_SHARE_DIR}/icons/hicolor/${_icon_size}/${_icon_context}"
                RENAME "${_icon_name}.png"
            )
        endforeach()
    endforeach()
endif()
