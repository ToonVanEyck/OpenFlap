/*****************************************************************************
* | File      	:   EPD_7in5bc_HD_test.c
* | Author      :   Waveshare team
* | Function    :   5.83inch B&C e-paper test demo
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2019-06-13
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "EPD_Test.h"
#include "EPD_7in5b_HD.h"
#include "EPD_TestImage.h"

#ifdef EPD_7IN5B_HD
int EPD_test(void)
{
    printf("EPD_7IN5B_HD_test Demo\r\n");
    if(EPD_Module_Init()!=0){
        return -1;
    }

    printf("e-Paper Init and Clear...\r\n");
    EPD_7IN5B_HD_Init();
    EPD_7IN5B_HD_Clear();
    EPD_Delay_ms(500);
//		EPD_7IN5B_HD_ClearRed();
//		EPD_Delay_ms(3000);
//		EPD_7IN5B_HD_ClearBlack();
//		EPD_Delay_ms(3000);
    //Create a new image cache named IMAGE_BW and fill it with white
    UBYTE *BlackImage, *RYImage;
    UWORD Imagesize = ((EPD_7IN5B_HD_WIDTH % 8 == 0)? (EPD_7IN5B_HD_WIDTH / 8 ): (EPD_7IN5B_HD_WIDTH / 8 + 1)) * EPD_7IN5B_HD_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize/2)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        return -1;
    }
    printf("NewImage:BlackImage \r\n");
    Paint_NewImage(BlackImage, EPD_7IN5B_HD_WIDTH, EPD_7IN5B_HD_HEIGHT/2 , 0, WHITE);

    //Select Image
   // Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
#if 1
    EPD_7IN5B_HD_DisplayImage(gImage_7in5_V2_b,gImage_7in5_V2_ry,16,10,800,480);
		EPD_Delay_ms(5000);
#endif
		
#if 1  // Drawing on the image
    /*Horizontal screen*/
    //1.Draw black image
		printf("Drawing on the image \r\n");
    Paint_Clear(WHITE);
    Paint_DrawPoint(10, 80, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
    Paint_DrawPoint(10, 90, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
    Paint_DrawPoint(10, 100, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    Paint_DrawPoint(10, 110, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    Paint_DrawLine(20, 70, 70, 120, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(70, 70, 20, 120, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);      
    Paint_DrawRectangle(20, 70, 70, 120, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(80, 70, 130, 120, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(10, 0, "waveshare", &Font16, BLACK, WHITE);    
    Paint_DrawString_CN(130, 20, "微雪电子", &Font24CN, WHITE, BLACK);
    Paint_DrawNum(10, 50, 987654321, &Font16, WHITE, BLACK);
    EPD_7IN5B_HD_DisplayPicture(BlackImage,0);
    Paint_Clear(WHITE);
    EPD_7IN5B_HD_DisplayPicture(BlackImage,1);
    //2.Draw red image
    Paint_Clear(WHITE);
    Paint_DrawCircle(160, 95, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(210, 95, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawLine(85, 95, 125, 95, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawLine(105, 75, 105, 115, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);  
    Paint_DrawString_CN(130, 0,"你好abc", &Font12CN, BLACK, WHITE);
    Paint_DrawString_EN(10, 20, "hello world", &Font12, WHITE, BLACK);
    Paint_DrawNum(10, 33, 123456789, &Font12, BLACK, WHITE);
    EPD_7IN5B_HD_DisplayPicture(BlackImage,2);
    Paint_Clear(WHITE);
    printf("EPD_Display\r\n");
    EPD_7IN5B_HD_DisplayPicture(BlackImage,3);
    EPD_Delay_ms(5000);
#endif

    printf("Clear...\r\n");
    EPD_7IN5B_HD_Clear();

    printf("Goto Sleep...\r\n");
    EPD_7IN5B_HD_Sleep();
    free(BlackImage);
    free(RYImage);
    BlackImage = NULL;
    RYImage = NULL;

    // close 5V
    printf("close 5V, Module enters 0 power consumption ...\r\n");
    EPD_Module_Exit();
    
    return 0;
}
#endif
