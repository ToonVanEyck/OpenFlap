/*****************************************************************************
* | File      	:   EPD_1in54.C
* | Author      :   Waveshare team
* | Function    :   1.54inch e-paper
* | Info        :
*----------------
* |	This version:   V3.0
* | Date        :   2019-06-12
* | Info        :
* -----------------------------------------------------------------------------
* V3.0(2019-06-12):
* 1.Change:
*    lut_full_update[] => EPD_1IN54_lut_full_update[]
*    lut_partial_update[] => EPD_1IN54_lut_partial_update[] 
*    EPD_Reset() => EPD_1IN54_Reset()
*    EPD_SendCommand() => EPD_1IN54_SendCommand()
*    EPD_SendData() => EPD_1IN54_SendData()
*    EPD_WaitUntilIdle() => EPD_1IN54_ReadBusy()
*    EPD_SetLut() => EPD_1IN54_SetLut()
*    EPD_SetWindow() => EPD_1IN54_SetWindow()
*    EPD_SetCursor() => EPD_1IN54_SetCursor()
*    EPD_TurnOnDisplay() => EPD_1IN54_TurnOnDisplay()
*    EPD_Init() => EPD_1IN54_Init()
*    EPD_Clear() => EPD_1IN54_Clear()
*    EPD_Display() => EPD_1IN54_Display()
*    EPD_Sleep() => EPD_1IN54_Sleep()
* 2.remove commands define:
*   #define PANEL_SETTING                               0x00
*   #define POWER_SETTING                               0x01
*   #define POWER_OFF                                   0x02
*   #define POWER_OFF_SEQUENCE_SETTING                  0x03
*   #define POWER_ON                                    0x04
*   #define POWER_ON_MEASURE                            0x05
*   #define BOOSTER_SOFT_START                          0x06
*   #define DEEP_SLEEP                                  0x07
*   #define DATA_START_TRANSMISSION_1                   0x10
*   #define DATA_STOP                                   0x11
*   #define DISPLAY_REFRESH                             0x12
*   #define DATA_START_TRANSMISSION_2                   0x13
*   #define PLL_CONTROL                                 0x30
*   #define TEMPERATURE_SENSOR_COMMAND                  0x40
*   #define TEMPERATURE_SENSOR_CALIBRATION              0x41
*   #define TEMPERATURE_SENSOR_WRITE                    0x42
*   #define TEMPERATURE_SENSOR_READ                     0x43
*   #define VCOM_AND_DATA_INTERVAL_SETTING              0x50
*   #define LOW_POWER_DETECTION                         0x51
*   #define TCON_SETTING                                0x60
*   #define TCON_RESOLUTION                             0x61
*   #define SOURCE_AND_GATE_START_SETTING               0x62
*   #define GET_STATUS                                  0x71
*   #define AUTO_MEASURE_VCOM                           0x80
*   #define VCOM_VALUE                                  0x81
*   #define VCM_DC_SETTING_REGISTER                     0x82
*   #define PROGRAM_MODE                                0xA0
*   #define ACTIVE_PROGRAM                              0xA1
*   #define READ_OTP_DATA                               0xA2
* -----------------------------------------------------------------------------
* V2.0(2018-10-30):
* 1.Remove:ImageBuff[EPD_1IN54_HEIGHT * EPD_1IN54_WIDTH / 8]
* 2.Change:EPD_Display(UBYTE *Image)
*   Need to pass parameters: pointer to cached data
* 3.Change:
*   EPD_RST -> EPD_RST_PIN
*   EPD_DC -> EPD_DC_PIN
*   EPD_CS -> EPD_CS_PIN
*   EPD_BUSY -> EPD_BUSY_PIN
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
#include "EPD_1in54.h"

#ifdef EPD_1IN54

static const unsigned char EPD_1IN54_lut_full_update[] = {
    0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22,
    0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88,
    0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51,
    0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};

static const unsigned char EPD_1IN54_lut_partial_update[] = {
    0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
static void EPD_1IN54_Reset(void)
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
static void EPD_1IN54_SendCommand(UBYTE Reg)
{
    EPD_Digital_Write(EPD_DC_PIN, 0);
    EPD_SPI_WriteByte(Reg);
    EPD_Delay_ms(1);
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
static void EPD_1IN54_SendData(UBYTE Data)
{
    EPD_Digital_Write(EPD_DC_PIN, 1);
    EPD_SPI_WriteByte(Data);
    __NOP();
}

static void EPD_1IN54_SendDataArray(const UBYTE *Data, UWORD len)
{
    EPD_Digital_Write(EPD_DC_PIN, 1);
    while (len--)
    {
        EPD_SPI_WriteByte(*Data++);
        __NOP();
    }
}

static void EPD_1IN54_SendDataBurst(const UBYTE Data, UWORD len)
{
    EPD_Digital_Write(EPD_DC_PIN, 1);
    while (len--)
    {
        EPD_SPI_WriteByte(Data);
        __NOP();
    }
}

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
void EPD_1IN54_ReadBusy(void)
{
    EPD_Printf("e-Paper check... ");
    while(EPD_Digital_Read(EPD_BUSY_PIN) == 1) {      //LOW: idle, HIGH: busy
        EPD_Printf("busy ");
        EPD_Delay_ms(100);
    }
    EPD_Printf("idle\r\n");
}

/******************************************************************************
function :	Setting the display window
parameter:
******************************************************************************/
static void EPD_1IN54_SetWindow(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend)
{
    EPD_1IN54_SendCommand(0x44); // SET_RAM_X_ADDRESS_START_END_POSITION
    EPD_1IN54_SendData((Xstart >> 3) & 0xFF);
    EPD_1IN54_SendData((Xend >> 3) & 0xFF);

    EPD_1IN54_SendCommand(0x45); // SET_RAM_Y_ADDRESS_START_END_POSITION
    EPD_1IN54_SendData(Ystart & 0xFF);
    EPD_1IN54_SendData((Ystart >> 8) & 0xFF);
    EPD_1IN54_SendData(Yend & 0xFF);
    EPD_1IN54_SendData((Yend >> 8) & 0xFF);
}

/******************************************************************************
function :	Set Cursor
parameter:
******************************************************************************/
static void EPD_1IN54_SetCursor(UWORD Xstart, UWORD Ystart)
{
    EPD_1IN54_SendCommand(0x4E); // SET_RAM_X_ADDRESS_COUNTER
    EPD_1IN54_SendData((Xstart >> 3) & 0xFF);

    EPD_1IN54_SendCommand(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
    EPD_1IN54_SendData(Ystart & 0xFF);
    EPD_1IN54_SendData((Ystart >> 8) & 0xFF);
}

/******************************************************************************
function :	Turn On Display
parameter:
******************************************************************************/
static void EPD_1IN54_TurnOnDisplay(void)
{
    EPD_1IN54_SendCommand(0x22); // DISPLAY_UPDATE_CONTROL_2
    EPD_1IN54_SendData(0xC4);
    EPD_1IN54_SendCommand(0x20); // MASTER_ACTIVATION
    EPD_1IN54_SendCommand(0xFF); // TERMINATE_FRAME_READ_WRITE

    EPD_1IN54_ReadBusy();
}

/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
void EPD_1IN54_Init(UBYTE Mode)
{
    EPD_1IN54_Reset();

    EPD_Digital_Write(EPD_CS_PIN, 0);
    EPD_1IN54_SendCommand(0x01); // DRIVER_OUTPUT_CONTROL
    EPD_1IN54_SendData((EPD_1IN54_HEIGHT - 1) & 0xFF);
    EPD_1IN54_SendData(((EPD_1IN54_HEIGHT - 1) >> 8) & 0xFF);
    EPD_1IN54_SendData(0x00); // GD = 0; SM = 0; TB = 0;

    EPD_1IN54_SendCommand(0x0C); // BOOSTER_SOFT_START_CONTROL
    EPD_1IN54_SendData(0xD7);
    EPD_1IN54_SendData(0xD6);
    EPD_1IN54_SendData(0x9D);

    EPD_1IN54_SendCommand(0x2C); // WRITE_VCOM_REGISTER
    EPD_1IN54_SendData(0xA8); // VCOM 7C

    EPD_1IN54_SendCommand(0x3A); // SET_DUMMY_LINE_PERIOD
    EPD_1IN54_SendData(0x1A); // 4 dummy lines per gate

    EPD_1IN54_SendCommand(0x3B); // SET_GATE_TIME
    EPD_1IN54_SendData(0x08); // 2us per line

    EPD_1IN54_SendCommand(0x11);
    EPD_1IN54_SendData(0x03);

    //set the look-up table register
    EPD_1IN54_SendCommand(0x32);
    if(Mode == EPD_1IN54_FULL){
        EPD_1IN54_SendDataArray(EPD_1IN54_lut_full_update, 30);
    }else if(Mode == EPD_1IN54_PART){
        EPD_1IN54_SendDataArray(EPD_1IN54_lut_partial_update, 30);
    }else{
        EPD_Printf("error, the Mode is EPD_1IN54_FULL or EPD_1IN54_PART");
    }
    EPD_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void EPD_1IN54_Clear(void)
{
    UWORD Width, Height;
    Width = (EPD_1IN54_WIDTH % 8 == 0)? (EPD_1IN54_WIDTH / 8 ): (EPD_1IN54_WIDTH / 8 + 1);
    Height = EPD_1IN54_HEIGHT;
    EPD_Digital_Write(EPD_CS_PIN, 0);
    EPD_1IN54_SetWindow(0, 0, EPD_1IN54_WIDTH, EPD_1IN54_HEIGHT);
    for (UWORD j = 0; j < Height; j++) {
        EPD_1IN54_SetCursor(0, j);
        EPD_1IN54_SendCommand(0x24);
        EPD_1IN54_SendDataBurst(0xFF, Width);
    }
    EPD_1IN54_TurnOnDisplay();
    EPD_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_1IN54_Display(UBYTE *Image)
{
    UWORD Width, Height;
    Width = (EPD_1IN54_WIDTH % 8 == 0)? (EPD_1IN54_WIDTH / 8 ): (EPD_1IN54_WIDTH / 8 + 1);
    Height = EPD_1IN54_HEIGHT;

    EPD_Digital_Write(EPD_CS_PIN, 0);
    // UDOUBLE Offset = ImageName;
    EPD_1IN54_SetWindow(0, 0, EPD_1IN54_WIDTH, EPD_1IN54_HEIGHT);
    for (UWORD j = 0; j < Height; j++) {
        EPD_1IN54_SetCursor(0, j);
        EPD_1IN54_SendCommand(0x24);
        EPD_1IN54_SendDataArray(Image + (j * Width), Width);
        // for (UWORD i = 0; i < Width; i++) {
        //     Addr = i + j * Width;
        //     EPD_1IN54_SendData(Image[Addr]);
        // }
    }
    EPD_1IN54_TurnOnDisplay();
    EPD_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	Enter sleep mode
parameter:
******************************************************************************/
void EPD_1IN54_Sleep(void)
{
    EPD_Digital_Write(EPD_CS_PIN, 0);
    EPD_1IN54_SendCommand(0x10);
    EPD_1IN54_SendData(0x01);
    EPD_Digital_Write(EPD_CS_PIN, 1);
}

#endif
