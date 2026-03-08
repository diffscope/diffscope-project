# Icon generation for DiffScope
# Automatically generates platform-specific icon files from PNG sources at configure time

# Get all subdirectories in the current source directory
file(GLOB ICON_SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
list(FILTER ICON_SUBDIRS INCLUDE REGEX "^[^.].*")  # Exclude hidden directories

foreach(SUBDIR ${ICON_SUBDIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR})
        set(ICON_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR})
        
        # Check if required PNG files exist
        set(REQUIRED_SIZES 16x16 24x24 32x32 48x48 64x64 96x96 128x128 256x256)
        set(ALL_PNGS_EXIST TRUE)
        
        foreach(SIZE ${REQUIRED_SIZES})
            if(NOT EXISTS ${ICON_SOURCE_DIR}/${SIZE}.png)
                set(ALL_PNGS_EXIST FALSE)
                break()
            endif()
        endforeach()
        
        if(ALL_PNGS_EXIST)
            if(WIN32)
                # Windows: Generate ICO file using ImageMagick at configure time
                find_program(MAGICK_EXECUTABLE NAMES magick)
                
                if(MAGICK_EXECUTABLE)
                    set(OUTPUT_ICO ${CMAKE_CURRENT_BINARY_DIR}/${SUBDIR}.ico)
                    
                    # Build the list of input PNG files
                    set(INPUT_PNGS)
                    foreach(SIZE ${REQUIRED_SIZES})
                        list(APPEND INPUT_PNGS ${ICON_SOURCE_DIR}/${SIZE}.png)
                    endforeach()
                    
                    # Generate ICO file immediately during configure
                    message(STATUS "Generating ${SUBDIR}.ico")
                    execute_process(
                        COMMAND ${MAGICK_EXECUTABLE} convert ${INPUT_PNGS} ${OUTPUT_ICO}
                        RESULT_VARIABLE RESULT
                        ERROR_VARIABLE ERROR_OUTPUT
                    )
                    
                    if(RESULT EQUAL 0)
                        message(STATUS "Successfully generated ${OUTPUT_ICO}")
                    else()
                        message(WARNING "Failed to generate ${SUBDIR}.ico: ${ERROR_OUTPUT}")
                    endif()
                else()
                    message(STATUS "ImageMagick not found, skipping ${SUBDIR}.ico generation")
                endif()
                
            elseif(APPLE)
                # macOS: Generate ICNS file using iconutil at configure time
                find_program(ICONUTIL_EXECUTABLE iconutil)
                
                if(ICONUTIL_EXECUTABLE)
                    set(OUTPUT_ICNS ${CMAKE_CURRENT_BINARY_DIR}/${SUBDIR}.icns)
                    set(ICONSET_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SUBDIR}.iconset)
                    
                    message(STATUS "Generating ${SUBDIR}.icns")
                    
                    # Create iconset directory
                    file(MAKE_DIRECTORY ${ICONSET_DIR})
                    
                    # Copy required files with iconset naming convention
                    file(COPY ${ICON_SOURCE_DIR}/16x16.png DESTINATION ${ICONSET_DIR})
                    file(RENAME ${ICONSET_DIR}/16x16.png ${ICONSET_DIR}/icon_16x16.png)
                    
                    file(COPY ${ICON_SOURCE_DIR}/32x32.png DESTINATION ${ICONSET_DIR})
                    file(RENAME ${ICONSET_DIR}/32x32.png ${ICONSET_DIR}/icon_16x16@2x.png)
                    
                    file(COPY ${ICON_SOURCE_DIR}/32x32.png DESTINATION ${ICONSET_DIR})
                    file(RENAME ${ICONSET_DIR}/32x32.png ${ICONSET_DIR}/icon_32x32.png)
                    
                    file(COPY ${ICON_SOURCE_DIR}/64x64.png DESTINATION ${ICONSET_DIR})
                    file(RENAME ${ICONSET_DIR}/64x64.png ${ICONSET_DIR}/icon_32x32@2x.png)
                    
                    file(COPY ${ICON_SOURCE_DIR}/128x128.png DESTINATION ${ICONSET_DIR})
                    file(RENAME ${ICONSET_DIR}/128x128.png ${ICONSET_DIR}/icon_128x128.png)
                    
                    file(COPY ${ICON_SOURCE_DIR}/256x256.png DESTINATION ${ICONSET_DIR})
                    file(RENAME ${ICONSET_DIR}/256x256.png ${ICONSET_DIR}/icon_128x128@2x.png)
                    
                    file(COPY ${ICON_SOURCE_DIR}/256x256.png DESTINATION ${ICONSET_DIR})
                    file(RENAME ${ICONSET_DIR}/256x256.png ${ICONSET_DIR}/icon_256x256.png)
                    
                    # Copy optional files if they exist
                    if(EXISTS ${ICON_SOURCE_DIR}/512x512.png)
                        file(COPY ${ICON_SOURCE_DIR}/512x512.png DESTINATION ${ICONSET_DIR})
                        file(RENAME ${ICONSET_DIR}/512x512.png ${ICONSET_DIR}/icon_256x256@2x.png)
                        
                        file(COPY ${ICON_SOURCE_DIR}/512x512.png DESTINATION ${ICONSET_DIR})
                        file(RENAME ${ICONSET_DIR}/512x512.png ${ICONSET_DIR}/icon_512x512.png)
                    endif()
                    
                    if(EXISTS ${ICON_SOURCE_DIR}/1024x1024.png)
                        file(COPY ${ICON_SOURCE_DIR}/1024x1024.png DESTINATION ${ICONSET_DIR})
                        file(RENAME ${ICONSET_DIR}/1024x1024.png ${ICONSET_DIR}/icon_512x512@2x.png)
                    endif()
                    
                    # Generate ICNS file
                    execute_process(
                        COMMAND ${ICONUTIL_EXECUTABLE} -c icns -o ${OUTPUT_ICNS} ${ICONSET_DIR}
                        RESULT_VARIABLE RESULT
                        ERROR_VARIABLE ERROR_OUTPUT
                    )
                    
                    # Clean up iconset directory
                    file(REMOVE_RECURSE ${ICONSET_DIR})
                    
                    if(RESULT EQUAL 0)
                        message(STATUS "Successfully generated ${OUTPUT_ICNS}")
                    else()
                        message(WARNING "Failed to generate ${SUBDIR}.icns: ${ERROR_OUTPUT}")
                    endif()
                else()
                    message(STATUS "iconutil not found, skipping ${SUBDIR}.icns generation")
                endif()
            endif()
        else()
            message(STATUS "Required PNG files not found for ${SUBDIR}, skipping icon generation")
        endif()
    endif()
endforeach()
