# Compatibility package for Starfish, which uses find_package(PNG CONFIG).
# DALi libpng 1.6.37 exports the imported target as `png` under share/libpng.
include("${CMAKE_CURRENT_LIST_DIR}/../libpng/libpngConfig.cmake")
if(NOT TARGET PNG::PNG)
    add_library(PNG::PNG ALIAS png)
endif()
