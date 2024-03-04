CMAKE_MINIMUM_REQUIRED (VERSION 2.8)


#######################################################
# TOOL BUILD 
#######################################################

ADD_EXECUTABLE (imgdiff EXCLUDE_FROM_ALL
    ${TOOL_ROOT}/imgdiff/imgdiff.cpp
)

SET_TARGET_PROPERTIES (imgdiff PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TOOL_ROOT}/imgdiff"
)

TARGET_LINK_LIBRARIES (imgdiff png)
TARGET_COMPILE_OPTIONS (imgdiff PUBLIC -O3 -g3 --std=c++11)

ADD_CUSTOM_TARGET (install_pixel_test_dep
    DEPENDS imgdiff
    COMMAND ${CMAKE_COMMAND} -E make_directory ~/.fonts
    COMMAND ${CMAKE_COMMAND} -E copy ${TOOL_ROOT}/fonts/StarfishAhem.ttf ~/.fonts
    COMMAND ${CMAKE_COMMAND} -E copy ${TOOL_ROOT}/fonts/SamsungOne-300C_v1.0.ttf ~/.fonts
    COMMAND ${CMAKE_COMMAND} -E copy ${TOOL_ROOT}/fonts/SamsungOne-600C_v1.0.ttf ~/.fonts
    COMMAND fc-cache -fv
    COMMAND fc-match SamsungOne
)
