cmake_minimum_required(VERSION 3.5)

project(Gui LANGUAGES CXX C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



set(PROJECT_SOURCES

    epaper/e-Paper/EPD_IT8951.c
    epaper/e-Paper/EPD_IT8951.h
    epaper/GUI/GUI_BMPfile.c
    epaper/GUI/GUI_BMPfile.h
    epaper/GUI/GUI_Paint.c
    epaper/GUI/GUI_Paint.h
    epaper/Fonts/font8.c
    epaper/Fonts/font12.c
    epaper/Fonts/font12CN.c
    epaper/Fonts/font16.c
    epaper/Fonts/font20.c
    epaper/Fonts/font24.c
    epaper/Fonts/font24CN.c
    epaper/Fonts/fonts.h
    epaper/Config/Debug.h
    epaper/Config/DEV_Config.c
    epaper/Config/DEV_Config.h
    epaper/bcm2835/bcm2835.c
    epaper/bcm2835/bcm2835.h

    main.cpp

)

add_executable(Gui
    ${PROJECT_SOURCES}
)

target_link_libraries(Gui PRIVATE
    # biblioteki epapieru
    pthread
    )

