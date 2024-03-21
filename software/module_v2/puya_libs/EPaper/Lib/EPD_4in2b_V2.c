/*****************************************************************************
* | File      	:   EPD_4in2b_V2.c
* | Author      :   Waveshare team
* | Function    :   4.2inch e-paper b V2
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2020-11-25
* | Info        :
* -----------------------------------------------------------------------------
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
#include "EPD_4in2b_V2.h"

#ifdef EPD_4IN2B_V2

/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
static void EPD_4IN2B_V2_Reset(void)
{
    EPD_Digital_Write(EPD_RST_PIN, 1);
    EPD_Delay_ms(200);
    EPD_Digital_Write(EPD_RST_PIN, 0);
    EPD_Delay_ms(2);
    EPD_Digital_Write(EPD_RST_PIN, 1);
    EPD_Delay_ms(200);
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
static void EPD_4IN2B_V2_SendCommand(UBYTE Reg)
{
    EPD_Digital_Write(EPD_DC_PIN, 0);
    EPD_Digital_Write(EPD_CS_PIN, 0);
    EPD_SPI_WriteByte(Reg);
    EPD_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
static void EPD_4IN2B_V2_SendData(UBYTE Data)
{
    EPD_Digital_Write(EPD_DC_PIN, 1);
    EPD_Digital_Write(EPD_CS_PIN, 0);
    EPD_SPI_WriteByte(Data);
    EPD_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
void EPD_4IN2B_V2_ReadBusy(void)
{
    EPD_Printf("e-Paper busy\r\n");
    unsigned char busy;
    do{
        EPD_4IN2B_V2_SendCommand(0x71);
		busy = EPD_Digital_Read(EPD_BUSY_PIN);
		busy =!(busy & 0x01); 
    }while(busy);
    EPD_Printf("e-Paper busy release\r\n");
    EPD_Delay_ms(200);
}

/******************************************************************************
function :	Turn On Display
parameter:
******************************************************************************/
static void EPD_4IN2B_V2_TurnOnDisplay(void)
{
    EPD_4IN2B_V2_SendCommand(0x12); // DISPLAY_REFRESH
    EPD_Delay_ms(100);
    EPD_4IN2B_V2_ReadBusy();
}

/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
void EPD_4IN2B_V2_Init(void)
{
    EPD_4IN2B_V2_Reset();
    
    EPD_4IN2B_V2_SendCommand(0x04); 
    EPD_4IN2B_V2_ReadBusy();

    EPD_4IN2B_V2_SendCommand(0x00);
    EPD_4IN2B_V2_SendData(0x0f);
}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void EPD_4IN2B_V2_Clear(void)
{
    UWORD Width, Height;
    Width = (EPD_4IN2B_V2_WIDTH % 8 == 0)? (EPD_4IN2B_V2_WIDTH / 8 ): (EPD_4IN2B_V2_WIDTH / 8 + 1);
    Height = EPD_4IN2B_V2_HEIGHT;

    EPD_4IN2B_V2_SendCommand(0x10);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_4IN2B_V2_SendData(0xFF);
        }
    }

    EPD_4IN2B_V2_SendCommand(0x13);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_4IN2B_V2_SendData(0xFF);
        }
    }

    EPD_4IN2B_V2_TurnOnDisplay();
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_4IN2B_V2_Display(const UBYTE *blackimage, const UBYTE *ryimage)
{
    UWORD Width, Height;
    Width = (EPD_4IN2B_V2_WIDTH % 8 == 0)? (EPD_4IN2B_V2_WIDTH / 8 ): (EPD_4IN2B_V2_WIDTH / 8 + 1);
    Height = EPD_4IN2B_V2_HEIGHT;

    EPD_4IN2B_V2_SendCommand(0x10);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_4IN2B_V2_SendData(blackimage[i + j * Width]);
        }
    }

    EPD_4IN2B_V2_SendCommand(0x13);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_4IN2B_V2_SendData(ryimage[i + j * Width]);
        }
    }

    EPD_4IN2B_V2_TurnOnDisplay();
}

/******************************************************************************
function :	Enter sleep mode
parameter:
******************************************************************************/
void EPD_4IN2B_V2_Sleep(void)
{
    EPD_4IN2B_V2_SendCommand(0X50);
    EPD_4IN2B_V2_SendData(0xf7);		//border floating	

    EPD_4IN2B_V2_SendCommand(0X02);  	//power off
    EPD_4IN2B_V2_ReadBusy(); //waiting for the electronic paper IC to release the idle signal
    EPD_4IN2B_V2_SendCommand(0X07);  	//deep sleep
    EPD_4IN2B_V2_SendData(0xA5);
}

#endif
