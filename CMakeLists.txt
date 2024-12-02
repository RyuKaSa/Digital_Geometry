cmake_minimum_required(VERSION 3.11)
project(Main)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find DGtal
find_package(DGtal REQUIRED)
include_directories(${DGTAL_INCLUDE_DIRS})
link_directories(${DGTAL_LIBRARY_DIRS})

# Find Qt6
find_package(Qt6 COMPONENTS Widgets REQUIRED)
include_directories(${Qt6Widgets_INCLUDE_DIRS})
add_definitions(${Qt6Widgets_DEFINITIONS})
link_directories(${Qt6Widgets_LIBRARY_DIRS})

# Find QGLViewer
include_directories("/opt/homebrew/opt/libqglviewer/include")
link_directories("/opt/homebrew/opt/libqglviewer/lib")

# Add TP3 executable if TP3.cpp exists
if(EXISTS "${CMAKE_SOURCE_DIR}/TP3.cpp")
    add_executable(TP3 TP3.cpp)
    target_link_libraries(TP3 ${DGTAL_LIBRARIES} Qt6::Widgets QGLViewer)
    message(STATUS "TP3 target added.")
else()
    message(WARNING "TP3.cpp not found. Skipping TP3 target.")
endif()

# Add TP1-2 executable if TP1-2.cpp exists
if(EXISTS "${CMAKE_SOURCE_DIR}/TP1-2.cpp")
    add_executable(TP1-2 TP1-2.cpp)
    target_link_libraries(TP1-2 ${DGTAL_LIBRARIES})
    message(STATUS "TP1-2 target added.")
else()
    message(WARNING "TP1-2.cpp not found. Skipping TP1-2 target.")
endif()

# Add TEST executable if TEST.cpp exists
if(EXISTS "${CMAKE_SOURCE_DIR}/TEST.cpp")
    add_executable(TEST TEST.cpp)
    target_link_libraries(TEST ${DGTAL_LIBRARIES})
    message(STATUS "TEST target added.")
else()
    message(WARNING "TEST.cpp not found. Skipping TEST target.")
endif()