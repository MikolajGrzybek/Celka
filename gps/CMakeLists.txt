cmake_minimum_required(VERSION 3.14)

project(gps LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core SerialPort  REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core SerialPort  REQUIRED)

## core
add_library(${PROJECT_NAME}
  gps.cpp
  gps.hpp

  msgs/func.cpp
)

target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::SerialPort)
target_link_libraries(${PROJECT_NAME} pthread)

##target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(${PROJECT_NAME} PUBLIC  msgs)


# extra t
add_executable(gps_exe
  main.cpp
)
target_link_libraries(gps_exe
    Qt${QT_VERSION_MAJOR}::Core
    ${PROJECT_NAME}
)

##target_include_directories(gps_exe PRIVATE include)
