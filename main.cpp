extern "C"{
    #include "lib/e-Paper/EPD_IT8951.h"
    #include "lib/GUI/GUI_BMPfile.h"
    #include "lib/Config/DEV_Config.h" 
}

#include "gui.hpp"

#include <math.h>
#include <cstdlib>     //exit()
#include <signal.h>     //signal()

#define USE_Normal_Demo true

UWORD VCOM = 2510;

IT8951_Dev_Info Dev_Info;
UWORD Panel_Width;
UWORD Panel_Height;
UDOUBLE Init_Target_Memory_Addr;
int epd_mode = 0;	//0: no rotate, no mirror
					//1: no rotate, horizontal mirror, for 10.3inch
					//2: no totate, horizontal mirror, for 5.17inch
					//3: no rotate, no mirror, isColor, for 6inch color
					
void  Handler(int signo){
    Debug("\r\nHandler:exit\r\n");
    if(Refresh_Frame_Buf != NULL){
        free(Refresh_Frame_Buf);
        Debug("free Refresh_Frame_Buf\r\n");
        Refresh_Frame_Buf = NULL;
    }
    if(bmp_src_buf != NULL){
        free(bmp_src_buf);
        Debug("free bmp_src_buf\r\n");
        bmp_src_buf = NULL;
    }
    if(bmp_dst_buf != NULL){
        free(bmp_dst_buf);
        Debug("free bmp_dst_buf\r\n");
        bmp_dst_buf = NULL;
    }
    Debug("Going to sleep\r\n");
    EPD_IT8951_Sleep();
    DEV_Module_Exit();
    exit(0);
}


int main(int argc, char *argv[])
{
    //Exception handling:ctrl + c
    signal(SIGINT, Handler);

    //Init the BCM2835 Device
    if(DEV_Module_Init()!=0){
        return -1;
    }

    double vcom_val = 2.10;
    VCOM = (UWORD)(fabs(vcom_val)*1000);
    Debug("VCOM value:%d\r\n", VCOM);
    Debug("Display mode:%d\r\n", epd_mode);
    Dev_Info = EPD_IT8951_Init(VCOM);

    //get some important info from Dev_Info structure
    Panel_Width = Dev_Info.Panel_W;
    Panel_Height = Dev_Info.Panel_H;
    Init_Target_Memory_Addr = Dev_Info.Memory_Addr_L | (Dev_Info.Memory_Addr_H << 16);
    char* LUT_Version = (char*)Dev_Info.LUT_Version;
    if( strcmp(LUT_Version, "M641") == 0 ){
        //6inch e-Paper HAT(800,600), 6inch HD e-Paper HAT(1448,1072), 6inch HD touch e-Paper HAT(1448,1072)
        A2_Mode = 4;
        Four_Byte_Align = true;
    }else if( strcmp(LUT_Version, "M841_TFAB512") == 0 ){
        //Another firmware version for 6inch HD e-Paper HAT(1448,1072), 6inch HD touch e-Paper HAT(1448,1072)
        A2_Mode = 6;
        Four_Byte_Align = true;
    }else{
        //default set to 6 as A2 Mode
        A2_Mode = 6;
    }
    Debug("A2 Mode:%d\r\n", A2_Mode);

	EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, INIT_Mode);

#if(USE_Normal_Demo)
    //Show 16 grayscale
    Display_ColorPalette_Example(Panel_Width, Panel_Height, Init_Target_Memory_Addr);
	EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, GC16_Mode);

    //Show some character and pattern
    Display_CharacterPattern_Example(Panel_Width, Panel_Height, Init_Target_Memory_Addr, BitsPerPixel_4);
    EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, GC16_Mode);
    
    //Show A2 mode refresh effect
    EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, A2_Mode);
    Dynamic_Refresh_Example(Dev_Info,Init_Target_Memory_Addr);
    EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, A2_Mode);
    EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, GC16_Mode);
	
#endif

    //We recommended refresh the panel to white color before storing in the warehouse.
    EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, INIT_Mode);

    //EPD_IT8951_Standby();
    EPD_IT8951_Sleep();

    //In case RPI is transmitting image in no hold mode, which requires at most 10s
    DEV_Delay_ms(5000);

    DEV_Module_Exit();
    return 0;
}