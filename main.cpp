extern "C"{
    #include "epaper/GUI/GUI_Paint.h"
    // Paint_DrawPoint(UWORD, UWORD, UWORD, DOT_PIXEL, DOT_STYLE);
}

int main(){
    Paint_DrawPoint(100, 100, BLACK, DOT_PIXEL_8X8, DOT_FILL_AROUND);
 }