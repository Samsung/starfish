if(NOT TARGET starfish::ffmpeg)
    get_filename_component(_ffmpeg_prefix "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
    add_library(starfish::ffmpeg INTERFACE IMPORTED)
    foreach(_component avcodec avformat avutil swscale swresample)
        add_library(starfish::${_component} SHARED IMPORTED)
        file(GLOB _dll "${_ffmpeg_prefix}/bin/${_component}-*.dll")
        set_target_properties(starfish::${_component} PROPERTIES
            IMPORTED_LOCATION "${_dll}"
            IMPORTED_IMPLIB "${_ffmpeg_prefix}/lib/${_component}.lib")
        target_link_libraries(starfish::ffmpeg INTERFACE starfish::${_component})
    endforeach()
    set_target_properties(starfish::ffmpeg PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${_ffmpeg_prefix}/include")
endif()
