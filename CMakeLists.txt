cmake_minimum_required(VERSION 3.5)

project(Gui)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



set(PROJECT_SOURCES

    lib/e-Paper/EPD_IT8951.c
    lib/e-Paper/EPD_IT8951.h
    lib/GUI/GUI_BMPfile.c
    lib/GUI/GUI_BMPfile.h
    lib/GUI/GUI_Paint.c
    lib/GUI/GUI_Paint.h
    lib/Fonts/font8.c
    lib/Fonts/font12.c
    lib/Fonts/font12CN.c
    lib/Fonts/font16.c
    lib/Fonts/font20.c
    lib/Fonts/font24.c
    lib/Fonts/font24CN.c
    lib/Fonts/fonts.h
    lib/Config/Debug.h
    lib/Config/DEV_Config.c
    lib/Config/DEV_Config.h
    lib/bcm2835/bcm2835.c
    lib/bcm2835/bcm2835.h

    main.cpp
    gui.cpp
    gui.hpp

)

add_executable(Gui
    ${PROJECT_SOURCES}
)

ADD_LIBRARY(LibsModule 
    lib/bcm2835/bcm2835.c
    lib/bcm2835/bcm2835.h
)

target_link_libraries(LibsModule -lpthread)
