#include "gui.hpp"

#include <time.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>

extern "C"{
    #include "lib/e-Paper/EPD_IT8951.h"
    #include "lib/GUI/GUI_Paint.h"
    #include "lib/GUI/GUI_BMPfile.h"
    #include "lib/Config/Debug.h" 
}

UBYTE *Refresh_Frame_Buf = NULL;

UBYTE *Panel_Frame_Buf = NULL;
UBYTE *Panel_Area_Frame_Buf = NULL;

bool Four_Byte_Align = false;

extern int epd_mode;
extern UWORD VCOM;
extern UBYTE isColor;
/******************************************************************************
function: Change direction of display, Called after Paint_NewImage()
parameter:
    mode: display mode
******************************************************************************/
static void Epd_Mode(int mode)
{
	if(mode == 3) {
		Paint_SetRotate(ROTATE_0);
		Paint_SetMirroring(MIRROR_NONE);
		isColor = 1;
	}else if(mode == 2) {
		Paint_SetRotate(ROTATE_0);
		Paint_SetMirroring(MIRROR_HORIZONTAL);
	}else if(mode == 1) {
		Paint_SetRotate(ROTATE_0);
		Paint_SetMirroring(MIRROR_HORIZONTAL);
	}else {
		Paint_SetRotate(ROTATE_0);
		Paint_SetMirroring(MIRROR_NONE);
	}
}


/******************************************************************************
function: Display_ColorPalette_Example
parameter:
    Panel_Width: Width of the panel
    Panel_Height: Height of the panel
    Init_Target_Memory_Addr: Memory address of IT8951 target memory address
******************************************************************************/
UBYTE Display_ColorPalette_Example(UWORD Panel_Width, UWORD Panel_Height, UDOUBLE Init_Target_Memory_Addr){
    UWORD In_4bp_Refresh_Area_Width;
    if(Four_Byte_Align == true){
        In_4bp_Refresh_Area_Width = Panel_Width - (Panel_Width % 32);
    }else{
        In_4bp_Refresh_Area_Width = Panel_Width;
    }
    UWORD In_4bp_Refresh_Area_Height = Panel_Height/16;

    UDOUBLE Imagesize;

    clock_t In_4bp_Refresh_Start, In_4bp_Refresh_Finish;
    double In_4bp_Refresh_Duration;

    Imagesize  = ((In_4bp_Refresh_Area_Width*4 % 8 == 0)? (In_4bp_Refresh_Area_Width*4 / 8 ): (In_4bp_Refresh_Area_Width*4 / 8 + 1)) * In_4bp_Refresh_Area_Height;

    if((Refresh_Frame_Buf = (UBYTE *)malloc(Imagesize)) == NULL) {
        Debug("Failed to apply for black memory...\r\n");
        return -1;
    }

    Debug("Start to demostrate 4bpp palette example\r\n");
    In_4bp_Refresh_Start = clock();

    UBYTE SixteenColorPattern[16] = {0xFF,0xEE,0xDD,0xCC,0xBB,0xAA,0x99,0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11,0x00};

    for(int i=0; i < 16; i++){
        memset(Refresh_Frame_Buf, SixteenColorPattern[i], Imagesize);
        EPD_IT8951_4bp_Refresh(Refresh_Frame_Buf, 0, i * In_4bp_Refresh_Area_Height, In_4bp_Refresh_Area_Width, In_4bp_Refresh_Area_Height, false, Init_Target_Memory_Addr, false);
    }

    In_4bp_Refresh_Finish = clock();
    In_4bp_Refresh_Duration = (double)(In_4bp_Refresh_Finish - In_4bp_Refresh_Start) / CLOCKS_PER_SEC;
    Debug( "Write and Show 4bp occupy %f second\n", In_4bp_Refresh_Duration );

    if(Refresh_Frame_Buf != NULL){
        free(Refresh_Frame_Buf);
        Refresh_Frame_Buf = NULL;
    }
    return 0;
}


/******************************************************************************
function: Display_CharacterPattern_Example
parameter:
    Panel_Width: Width of the panel
    Panel_Height: Height of the panel
    Init_Target_Memory_Addr: Memory address of IT8951 target memory address
    BitsPerPixel: Bits Per Pixel, 2^BitsPerPixel = grayscale
******************************************************************************/
UBYTE Display_CharacterPattern_Example(UWORD Panel_Width, UWORD Panel_Height, UDOUBLE Init_Target_Memory_Addr, UBYTE BitsPerPixel){
    UWORD Display_Area_Width;
    if(Four_Byte_Align == true){
        Display_Area_Width = Panel_Width - (Panel_Width % 32);
    }else{
        Display_Area_Width = Panel_Width;
    }
    UWORD Display_Area_Height = Panel_Height;

    UWORD Display_Area_Sub_Width = Display_Area_Width / 5;
    UWORD Display_Area_Sub_Height = Display_Area_Height / 5;

    UDOUBLE Imagesize;

    Imagesize = ((Display_Area_Width * BitsPerPixel % 8 == 0)? (Display_Area_Width * BitsPerPixel / 8 ): (Display_Area_Width * BitsPerPixel / 8 + 1)) * Display_Area_Height;
    if((Refresh_Frame_Buf = (UBYTE *)malloc(Imagesize)) == NULL) {
        Debug("Failed to apply for image memory...\r\n");
        return -1;
    }

    Paint_NewImage(Refresh_Frame_Buf, Display_Area_Width, Display_Area_Height, 0, BLACK);
    Paint_SelectImage(Refresh_Frame_Buf);
	Epd_Mode(epd_mode);
    Paint_SetBitsPerPixel(BitsPerPixel);
    Paint_Clear(WHITE);
    
	//For color definition of all BitsPerPixel, you can refer to GUI_Paint.
    Paint_DrawPoint(Display_Area_Sub_Width*3/8, Display_Area_Sub_Height*3/8, 0x10, DOT_PIXEL_7X7, DOT_STYLE_DFT);
    Paint_DrawPoint(Display_Area_Sub_Width*5/8, Display_Area_Sub_Height*3/8, 0x30, DOT_PIXEL_7X7, DOT_STYLE_DFT);
    Paint_DrawLine(Display_Area_Sub_Width*3/8, Display_Area_Sub_Height*5/8, Display_Area_Sub_Width*5/8, Display_Area_Sub_Height*5/8, 0x50, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
    Paint_DrawRectangle(200, 100, Display_Area_Sub_Width, Display_Area_Sub_Height, 0x00, DOT_PIXEL_3X3, DRAW_FILL_EMPTY);
  //  Paint_DrawCircle(x + Display_Area_Sub_Width/2, y + Display_Area_Sub_Height/2, Display_Area_Sub_Height/2, 0x50, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawNum(Display_Area_Sub_Width*3/10, Display_Area_Sub_Height*1/4, 2137, &Font16, 0x20, 0xE0);
    Paint_DrawString_EN(Display_Area_Sub_Width*3/10, Display_Area_Sub_Height*3/4, "hello solarku", &Font16, 0x30, 0xD0);


    switch(BitsPerPixel){
        case BitsPerPixel_8:{
            EPD_IT8951_8bp_Refresh(Refresh_Frame_Buf, 0, 0, Display_Area_Width,  Display_Area_Height, false, Init_Target_Memory_Addr);
            break;
        }
        case BitsPerPixel_4:{
            EPD_IT8951_4bp_Refresh(Refresh_Frame_Buf, 0, 0, Display_Area_Width,  Display_Area_Height, false, Init_Target_Memory_Addr,false);
            break;
        }
        case BitsPerPixel_2:{
            EPD_IT8951_2bp_Refresh(Refresh_Frame_Buf, 0, 0, Display_Area_Width,  Display_Area_Height, false, Init_Target_Memory_Addr,false);
            break;
        }
        case BitsPerPixel_1:{
            EPD_IT8951_1bp_Refresh(Refresh_Frame_Buf, 0, 0, Display_Area_Width,  Display_Area_Height, A2_Mode, Init_Target_Memory_Addr,false);
            break;
        }
    }

    if(Refresh_Frame_Buf != NULL){
        free(Refresh_Frame_Buf);
        Refresh_Frame_Buf = NULL;
    }

    return 0;
}




/******************************************************************************
function: Dynamic_Refresh_Example
parameter:
    Dev_Info: Information structure read from IT8951
    Init_Target_Memory_Addr: Memory address of IT8951 target memory address
******************************************************************************/
UBYTE Dynamic_Refresh_Example(IT8951_Dev_Info Dev_Info, UDOUBLE Init_Target_Memory_Addr){
    UWORD Panel_Width = Dev_Info.Panel_W;
    UWORD Panel_Height = Dev_Info.Panel_H;

    UWORD Dynamic_Area_Width = 96;
    UWORD Dynamic_Area_Height = 48;

    UDOUBLE Imagesize;

    UWORD Start_X = 0,Start_Y = 0;

    UWORD Dynamic_Area_Count = 0;

    UWORD Repeat_Area_Times = 0;

    //malloc enough memory for 1bp picture first
    Imagesize = ((Panel_Width * 1 % 8 == 0)? (Panel_Width * 1 / 8 ): (Panel_Width * 1 / 8 + 1)) * Panel_Height;
    if((Refresh_Frame_Buf = (UBYTE *)malloc(Imagesize)) == NULL){
        Debug("Failed to apply for picture memory...\r\n");
        return -1;
    }

    clock_t Dynamic_Area_Start, Dynamic_Area_Finish;
    double Dynamic_Area_Duration;  

    while(1)
    {
        Dynamic_Area_Width = 128;
        Dynamic_Area_Height = 96;

        Start_X = 0;
        Start_Y = 0;

        Dynamic_Area_Count = 0;

        Dynamic_Area_Start = clock();
        Debug("Start to dynamic display...\r\n");

        for(Dynamic_Area_Width = 96, Dynamic_Area_Height = 64; (Dynamic_Area_Width < Panel_Width - 32) && (Dynamic_Area_Height < Panel_Height - 24); Dynamic_Area_Width += 32, Dynamic_Area_Height += 24)
        {

            Imagesize = ((Dynamic_Area_Width % 8 == 0)? (Dynamic_Area_Width / 8 ): (Dynamic_Area_Width / 8 + 1)) * Dynamic_Area_Height;
            Paint_NewImage(Refresh_Frame_Buf, Dynamic_Area_Width, Dynamic_Area_Height, 0, BLACK);
            Paint_SelectImage(Refresh_Frame_Buf);
			Epd_Mode(epd_mode);
            Paint_SetBitsPerPixel(1);

           for(int y=Start_Y; y< Panel_Height - Dynamic_Area_Height; y += Dynamic_Area_Height)
            {
                for(int x=Start_X; x< Panel_Width - Dynamic_Area_Width; x += Dynamic_Area_Width)
                {
                    Paint_Clear(WHITE);

                    //For color definition of all BitsPerPixel, you can refer to GUI_Paint.h
                    Paint_DrawRectangle(0, 0, Dynamic_Area_Width-1, Dynamic_Area_Height, 0x00, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

                    Paint_DrawCircle(Dynamic_Area_Width*3/4, Dynamic_Area_Height*3/4, 5, 0x00, DOT_PIXEL_1X1, DRAW_FILL_FULL);

                    Paint_DrawNum(Dynamic_Area_Width/4, Dynamic_Area_Height/4, ++Dynamic_Area_Count, &Font20, 0x00, 0xF0);

                    switch(epd_mode){
                        case 1:
						EPD_IT8951_1bp_Refresh(Refresh_Frame_Buf, Panel_Width-Dynamic_Area_Width-x-16, y, Dynamic_Area_Width,  Dynamic_Area_Height, A2_Mode, Init_Target_Memory_Addr, true);
                            break;
                        case 2:
   						EPD_IT8951_1bp_Refresh(Refresh_Frame_Buf, 1280-Dynamic_Area_Width-x, y, Dynamic_Area_Width,  Dynamic_Area_Height, A2_Mode, Init_Target_Memory_Addr, true);
                            break;
                        default:
						EPD_IT8951_1bp_Refresh(Refresh_Frame_Buf, x, y, Dynamic_Area_Width,  Dynamic_Area_Height, A2_Mode, Init_Target_Memory_Addr, true);
                            break;
                    }
                }
            }
            Start_X += 32;
            Start_Y += 24;
        }

        Dynamic_Area_Finish = clock();
        Dynamic_Area_Duration = (double)(Dynamic_Area_Finish - Dynamic_Area_Start) / CLOCKS_PER_SEC;
        Debug( "Write and Show occupy %f second\n", Dynamic_Area_Duration );

        Repeat_Area_Times ++;
        if(Repeat_Area_Times > 0){
            break;
        }
    }
    if(Refresh_Frame_Buf != NULL){
        free(Refresh_Frame_Buf);
        Refresh_Frame_Buf = NULL;
    }

    return 0;
}




void Color_Test(IT8951_Dev_Info Dev_Info, UDOUBLE Init_Target_Memory_Addr)
{
	PAINT_TIME Time = {2020, 9, 30, 18, 10, 34};
	
	while(1) 
	{
		UWORD Panel_Width = Dev_Info.Panel_W;
		UWORD Panel_Height = Dev_Info.Panel_H;

		UDOUBLE Imagesize;

		//malloc enough memory for 1bp picture first
		Imagesize = ((Panel_Width * 1 % 8 == 0)? (Panel_Width * 1 / 8 ): (Panel_Width * 1 / 8 + 1)) * Panel_Height;
		if((Refresh_Frame_Buf = (UBYTE *)malloc(Imagesize*4)) == NULL) {
			Debug("Failed to apply for picture memory...\r\n");
		}

		Paint_NewImage(Refresh_Frame_Buf, Panel_Width, Panel_Height, 0, BLACK);
		Paint_SelectImage(Refresh_Frame_Buf);
		Epd_Mode(epd_mode);
		Paint_SetBitsPerPixel(4);
		Paint_Clear(WHITE);

		if(0) {
			Paint_DrawRectangle(100, 100, 300, 300, 0x0f00, DOT_PIXEL_1X1, DRAW_FILL_FULL);	//Red
			Paint_DrawRectangle(100, 400, 300, 600, 0x00f0, DOT_PIXEL_1X1, DRAW_FILL_FULL);	//Green
			Paint_DrawRectangle(100, 700, 300, 900, 0x000f, DOT_PIXEL_1X1, DRAW_FILL_FULL);	//Bule
			
			Paint_DrawCircle(500, 200, 100, 0x00ff, DOT_PIXEL_1X1, DRAW_FILL_FULL);
			Paint_DrawCircle(500, 500, 100, 0x0f0f, DOT_PIXEL_1X1, DRAW_FILL_FULL);
			Paint_DrawCircle(500, 800, 100, 0x0ff0, DOT_PIXEL_1X1, DRAW_FILL_FULL);
			
			Paint_DrawLine(1000, 200, 1100, 200, 0x055a, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
			Paint_DrawLine(1000, 300, 1100, 300, 0x05a5, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
			Paint_DrawLine(1000, 400, 1100, 400, 0x0a55, DOT_PIXEL_3X3, LINE_STYLE_SOLID);

			Paint_DrawString_EN(1000, 500, "Hello, World!", &Font24, 0x0aa5, 0x0fff);
			Paint_DrawString_EN(1000, 600, "Hello, World!", &Font24, 0x0a5a, 0x0fff);
			Paint_DrawString_EN(1000, 700, "Hello, World!", &Font24, 0x05aa, 0x0fff);

			Paint_DrawString_CN(700, 400, "ÄãºÃ Î¢Ñ©µç×Ó", &Font24CN, 0x00fa, 0x0000);
			Paint_DrawNum(700, 500, 123456789, &Font24, 0x0a0f, 0x0fff);
			Paint_DrawTime(700, 600, &Time, &Font24, 0x0fa0, 0x0fff);
		}else {
			for(UWORD j=0; j<14; j++) {
				for(UWORD i=0; i<19; i++) {
					Paint_DrawRectangle(i*72, j*72+1, (i+1)*72-1, (j+1)*72, (i+j*19)*15, DOT_PIXEL_1X1, DRAW_FILL_FULL);
				}
			}
		}

		EPD_IT8951_4bp_Refresh(Refresh_Frame_Buf, 0, 0, Panel_Width,  Panel_Height, false, Init_Target_Memory_Addr, false);
		
		if(Refresh_Frame_Buf != NULL) {
			free(Refresh_Frame_Buf);
			Refresh_Frame_Buf = NULL;
		}

		DEV_Delay_ms(5000);
		break;
	}
}
