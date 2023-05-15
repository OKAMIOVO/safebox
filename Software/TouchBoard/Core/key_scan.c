#include "device.h"
#include "CMS32L051.h"
#include "userdefine.h"
#include "gpio.h"
#include "intp.h"
#include "iica.h"
#include <string.h>
#include "key_filter.h"
#include "log.h"
#include "cal.h"
#include "voice.h"
#include "led.h"
#include "safe_box.h"

typedef struct
{
    enum {
        TouchPowerDown = 0,
        TouchPowerOn = 1,
        LowSensitivity,
        HighSensitivity,
        NoSensing
    } Status;
} TouchPowerMgr_t;
TouchPowerMgr_t TouchPowerMgr;
void I2C_WriteSequential(uint8_t DeviceAddr, uint8_t StartAddr, uint8_t* BUFF, uint8_t Num);
void I2C_ReadSequential(uint8_t DeviceAddr, uint8_t StartAddr, uint8_t* BUFF, uint8_t Num);
void SET_CSK14_NO_SENSING(void);
void SET_CSK14_SCAN_RESUME(void);
void SET_CSK14_SCAN_PAUSE(void);
void SET_CSK14_AWAKE_SENSITIVITY(void);
void SET_CSK14_SLEEP_SENSITIVITY(void);
void I2C_ReadRandom(uint8_t DeviceAddr, uint8_t RegisterAddr, uint8_t* Point);
void I2C_WriteRandom(uint8_t DeviceAddr, uint8_t RegisterAddr, uint8_t Value);
void CSK14_Init(void);
void CSK_AWAKE(void);
void KeyBoardSleep(void);
void KeyScan(MultiTimer* timer, void* userData);
void SleepTimerCallBack(MultiTimer* timer, void* userData);
void ReportKeyEvent(int keyValue, enum KeyEvent event);
void KeyEventCallback(int keyValue, enum KeyEvent event);
extern void SetLedState(uint8_t Num, uint8_t state);
static MultiTimer keyTimer;
struct Device keyBoard = { NULL, CSK14_Init, KeyBoardSleep };
#define KEY_CNT 12
const uint8_t keyChnList[] = { 3, 6, 7, 8, 9, 10, 11, 0, 1, 2, 4, 5 };
enum KeyState keyStateList[KEY_CNT];
#define IfPush(keyValue) (scanResult & (1 << keyChnList[keyValue]))
#define SCAN_PERIOD 40
void delayMS(uint32_t n);
#define Hardware_DelayMs(sec) delayMS(sec)
#define SET_CSK14_RST_L PORT_ClrBit(PORT2, PIN2);
#define SET_CSK14_RST_H PORT_SetBit(PORT2, PIN2);
#define I2CADDR_CSK14S 0x2a
#define CSK14_SENS_CH0 200 //
#define CSK14_SENS_CH1 200 // 0x58 // key 7
#define CSK14_SENS_CH2 200 // 0x48 // key 1
#define CSK14_SENS_CH3 200 // 0x50 // key 4
#define CSK14_SENS_CH4 150 // 0x48 // key *
#define CSK14_SENS_CH5 200 // 0x40 // key 2
#define CSK14_SENS_CH6 200 // 0x40 // key 3
#define CSK14_SENS_CH7 200 // 0x40 // key 6
#define CSK14_SENS_CH8 200 // 0x40 // key 9
#define CSK14_SENS_CH9 200 // 0x50 // key 8
#define CSK14_SENS_CH10 100 // 0x48 // key 0
#define CSK14_SENS_CH11 200 // 0x40 // key #

#define CSK14_SENS_CH0_SLEEP 0x60
#define CSK14_SENS_CH1_SLEEP 0x60
#define CSK14_SENS_CH2_SLEEP 0x60
#define CSK14_SENS_CH3_SLEEP 0x60
#define CSK14_SENS_CH4_SLEEP 0x60
#define CSK14_SENS_CH5_SLEEP 0x60
#define CSK14_SENS_CH6_SLEEP 0x60
#define CSK14_SENS_CH7_SLEEP 0x60
#define CSK14_SENS_CH8_SLEEP 0x60
#define CSK14_SENS_CH9_SLEEP 0x60
#define CSK14_SENS_CH10_SLEEP 0x60
#define CSK14_SENS_CH11_SLEEP 0x60
#define CSK14_SENS_SLEEP 0x48

void KeyBoardSleep()
{
    SET_CSK14_SLEEP_SENSITIVITY();
    INTP_Init(1 << 0, INTP_FALLING);
    INTP_Start(1<<0);
    g_intp0Taken = 0;
}
void CSK14_Init()
{

	
    int i=0;
    PRINT("CSK14_Init\n");
    IICA0_Init();
    g_iica0_tx_end = 1;
    g_iica0_rx_end = 1;
	
    PORT_Init(PORT2, PIN2, OUTPUT);
    

    SET_CSK14_RST_L;
    Hardware_DelayMs(10);
    SET_CSK14_RST_H;
    Hardware_DelayMs(200);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xAF, 0x3A); // Enable write
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0x1F, 0x00); // Stop scan before change sensitivity
    Hardware_DelayMs(1);

    I2C_WriteRandom(I2CADDR_CSK14S, 0x12, 0x18); // SCAN CONFIG
    //	I2C_WriteRandom(I2CADDR_CSK14S,0x12,0x07);		//SCAN CONFIG
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0x13, 0x03); // Debounce
    Hardware_DelayMs(1);

    I2C_WriteRandom(I2CADDR_CSK14S, 0x14, 0xFF); // Enable channel 0~7
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0x15, 0x0F); // Enable channel 8~12
    Hardware_DelayMs(1);

    I2C_WriteRandom(I2CADDR_CSK14S, 0x1E, 0xff); // Set Global sensitivity
    Hardware_DelayMs(1);

    I2C_WriteRandom(I2CADDR_CSK14S, 0x2B, 0x00); // Low power scan period 1:250ms,0:500ms
    Hardware_DelayMs(1);

    I2C_WriteRandom(I2CADDR_CSK14S, 0x2C, 0x01); // key debounce,1~3
    Hardware_DelayMs(1);

    I2C_WriteRandom(I2CADDR_CSK14S, 0x30, 0x06); // Low power delay,must more than 4s.
    Hardware_DelayMs(1);

    I2C_WriteRandom(I2CADDR_CSK14S, 0x31, 0x01); // Scan Period, 16ms
    Hardware_DelayMs(1);

    I2C_WriteRandom(I2CADDR_CSK14S, 0x32, 0xff); // Awake throuhold,from 1~80, 0xff: software scan awake
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xB0, 0xff); // Enable channel0~channel7 lp scan
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xB1, 0x0f); // Enable channel8~channel11 lp scan
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE0, CSK14_SENS_CH0);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE1, CSK14_SENS_CH1);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE2, CSK14_SENS_CH2);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE3, CSK14_SENS_CH3);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE4, CSK14_SENS_CH4);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE5, CSK14_SENS_CH5);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE6, CSK14_SENS_CH6);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE7, CSK14_SENS_CH7);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE8, CSK14_SENS_CH8);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE9, CSK14_SENS_CH9);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xEA, CSK14_SENS_CH10);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xEB, CSK14_SENS_CH11);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0x1F, 0x01);

    // uint8_t temp;
    // int i = 0;
    // for (i = 0xe0; i < 0xEC; ++i) {
    //     temp = 0;
    //     I2C_ReadRandom(I2CADDR_CSK14S, i, &temp);
    //     PRINT("buf addr:%02x,value:%02X\n", i, temp);
    //     Hardware_DelayMs(1);
    // }
		
    Hardware_DelayMs(1);
    TouchPowerMgr.Status = HighSensitivity;
    for (i = 0; i < KEY_CNT; ++i) {
        keyStateList[i] = KEY_CHECK;
    }
    //PRINT("key scan\n");
    // KeyScan(&keyTimer,NULL);
		PRINT("CSK INIT FINISH\n");
    MultiTimerStart(&keyTimer, 600, KeyScan, NULL);
}
void CSK_AWAKE()
{
    CSK14_Init();
    // Hardware_DelayMs(3);

    // if ((SystemPowerMgr.AwakeSource == FingerTouch)
    //     || (SystemPowerMgr.AwakeSource == CasingOpen)) {
    //     CSK14_Init(); // Touch Recalibration
    // } else {
    //     CSK14_Init(); // Touch Recalibration
    //     // SET_CSK14_AWAKE_SENSITIVITY();
    // }
}
//void KeyEventDisp(int keyValue, uint8_t event)
//{
//#ifdef FUNC_LOG
//    const char* keyStr[] = {
//        "*",
//        "#",
//        "SET",
//        "MODE"
//    };
//    if (event == 0) {
//        PRINT("PRESS:");
//    } else if (event == 1) {
//        PRINT("release:");
//    } else if (event == 2) {
//        PRINT("LONG PRESS:")
//    }
//    if (keyValue < 10) {
//        PRINT("%d\n", keyValue);
//    } else {
//        PRINT("%s\n", keyStr[keyValue - 10]);
//    }
//#endif
//}
void KeyScan(MultiTimer* timer, void* userData)
{
    int i = 0;
    uint8_t temp[5] = { 0 };
    uint16_t scanResult = 0;
    // PRINT("key scan\n");
    I2C_ReadSequential(I2CADDR_CSK14S, 0x21, temp, 5);
    scanResult = temp[2] << 8 | temp[1];
    //PRINT("scan:%04x\n", scanResult);
    uint8_t checkSum = temp[0] + temp[1] + temp[2] + temp[4] + 0xC5;
    if (checkSum) {
        //PrintfBuf(temp, 5);
        //PRINT("checkSum=%02x", checkSum);
    } else {
        for (i = 0; i < KEY_CNT; ++i) {
            if (IfPush(i)){
                // PRINT("KEY:%d\n", i);
                
            }
            KeyFilter(IfPush(i), &keyStateList[i], i, KeyEventCallback);
        }
    }
    // PRINT("Key Scan!\n");
    MultiTimerStart(&keyTimer, SCAN_PERIOD, KeyScan, NULL);
}

void KeyEventCallback(int keyValue, enum KeyEvent event)
{
//    if()
    // SetLedState(6, event);
    // LedInit();
    // if(keyValue == 9){
    //     SetLedState(0,1);
    // }else{
    //     CLOSE_ALL_LED();
    // }
	//keyEventHandler(keyValue,event);
	// if(event == 0){
	// 	PlayWaterDrop();
	// }
    /* static uint8_t num = 0;
    if(keyValue!=10&&keyValue!=11)
        num+= keyValue;
    PRINT("num = %d\n",num);
    if(num >32){
        PRINT("OPEN TOUCH LED!\n");
        SetLedState(LED_TOUCH_BOARD_NUM, OPEN);
        num = 0;
    }
    PRINT("num = %d\n",num); */
    if(keyEventHandler != NULL)
        keyEventHandler(keyValue,event);
    
    //PlayPleaseRenewInput();
    PRINT("KeyValue = %d,event = %d\n",keyValue,event);
	//keyEventHandler(keyValue,event);
 //   ReportKeyEvent(keyValue, event);
 //   KeyEventDisp(keyValue, event);
 //   MultiTimerStart(&deviceMgr.timer, SLEEP_TIME, SleepTimerCallBack, NULL);
}

void I2C_ReadRandom(uint8_t DeviceAddr, uint8_t RegisterAddr, uint8_t* Point)
{
    IICA0_MasterSend(DeviceAddr, &RegisterAddr, 1, 20);
    while (g_iica0_tx_end == 0) {
    }
    delayMS(1);
    IICA0_MasterReceive(DeviceAddr, Point, 1, 20);
    while (g_iica0_rx_end == 0) {
    }
}
void I2C_WriteRandom(uint8_t DeviceAddr, uint8_t RegisterAddr, uint8_t Value)
{
    uint8_t tx_buf[2];
    tx_buf[0] = RegisterAddr;
    tx_buf[1] = Value;
    IICA0_MasterSend(DeviceAddr, tx_buf, 2, 20);
    while (g_iica0_tx_end == 0) {
    }
}
void SET_CSK14_SLEEP_SENSITIVITY(void)
{

    I2C_WriteRandom(I2CADDR_CSK14S, 0xAF, 0x3A); // Enable write
    I2C_WriteRandom(I2CADDR_CSK14S, 0x1F, 0x00); // Stop scan before change sensitivity
    // I2C_WriteRandom(I2CADDR_CSK14S,0xEE,CSK14_SENS_SLEEP);
    I2C_WriteRandom(I2CADDR_CSK14S, 0x1F, 0x41); // Set scan mode to Auto LP mode
    TouchPowerMgr.Status = LowSensitivity;

#if 0
	if ( (PINMACRO_CASINGOPEN_STATUS == 0x00 ) //slide is opened
		   ||(SystemPowerMgr.SlideHallIsWorking == bFALSE) //Slide Hall is Not working
		)
	   {
			I2C_WriteRandom(I2CADDR_CSK14S,0xAF,0x3A);		//Enable write
			Hardware_DelayMs(1);
			I2C_WriteRandom(I2CADDR_CSK14S,0x1F,0x00);		//Stop scan before change sensitivity
			Hardware_DelayMs(1);

/********** below code for reduce working sensitivity, ****************/
/********** ignore unexpect awake system befor Touch into LP mode ******************/
			I2C_WriteRandom(I2CADDR_CSK14S,0xE0,CSK14_SENS_CH0_SLEEP);		
			Hardware_DelayMs(1);
			I2C_WriteRandom(I2CADDR_CSK14S,0xE1,CSK14_SENS_CH1_SLEEP);	
			Hardware_DelayMs(1);
			I2C_WriteRandom(I2CADDR_CSK14S,0xE2,CSK14_SENS_CH2_SLEEP);		
			Hardware_DelayMs(1);
			I2C_WriteRandom(I2CADDR_CSK14S,0xE3,CSK14_SENS_CH3_SLEEP);	
			Hardware_DelayMs(1);
			I2C_WriteRandom(I2CADDR_CSK14S,0xE4,CSK14_SENS_CH4_SLEEP);		
			Hardware_DelayMs(1);
			I2C_WriteRandom(I2CADDR_CSK14S,0xE5,CSK14_SENS_CH5_SLEEP);	
			Hardware_DelayMs(1);
			I2C_WriteRandom(I2CADDR_CSK14S,0xE6,CSK14_SENS_CH6_SLEEP);		
			Hardware_DelayMs(1);
			I2C_WriteRandom(I2CADDR_CSK14S,0xE7,CSK14_SENS_CH7_SLEEP);	
			Hardware_DelayMs(1);
			I2C_WriteRandom(I2CADDR_CSK14S,0xE8,CSK14_SENS_CH8_SLEEP);		
			Hardware_DelayMs(1);
			I2C_WriteRandom(I2CADDR_CSK14S,0xE9,CSK14_SENS_CH9_SLEEP);	
			Hardware_DelayMs(1);
			I2C_WriteRandom(I2CADDR_CSK14S,0xEA,CSK14_SENS_CH10_SLEEP); 	
			Hardware_DelayMs(1);
			I2C_WriteRandom(I2CADDR_CSK14S,0xEB,CSK14_SENS_CH11_SLEEP); 
			Hardware_DelayMs(1);
	

			I2C_WriteRandom(I2CADDR_CSK14S,0xEE,CSK14_SENS_SLEEP); //LP wake senstivity
			Hardware_DelayMs(1);
			
			I2C_WriteRandom(I2CADDR_CSK14S,0x1F,0x41);		//Set scan mode to Auto LP mode
			Hardware_DelayMs(1);
		
			TouchPowerMgr.Status = LowSensitivity;

		}
	  else
		{
		  SET_CSK14_NO_SENSING();
		}
#endif
}
void SET_CSK14_AWAKE_SENSITIVITY(void)
{

    I2C_WriteRandom(I2CADDR_CSK14S, 0xAF, 0x3A); // Enable write
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0x1F, 0x00); // Stop scan before change sensitivity
    Hardware_DelayMs(1);

    I2C_WriteRandom(I2CADDR_CSK14S, 0xE0, CSK14_SENS_CH0);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE1, CSK14_SENS_CH1);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE2, CSK14_SENS_CH2);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE3, CSK14_SENS_CH3);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE4, CSK14_SENS_CH4);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE5, CSK14_SENS_CH5);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE6, CSK14_SENS_CH6);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE7, CSK14_SENS_CH7);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE8, CSK14_SENS_CH8);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xE9, CSK14_SENS_CH9);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xEA, CSK14_SENS_CH10);
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0xEB, CSK14_SENS_CH11);
    Hardware_DelayMs(1);
    //	I2C_WriteRandom(I2CADDR_CSK14S,0xEC,CSK14_SENS_CH12);
    //	Hardware_DelayMs(1);
    // I2C_WriteRandom(I2CADDR_CSK14S,0xED,CSK14_SENS_CH13);
    // Hardware_DelayMs(1);

    I2C_WriteRandom(I2CADDR_CSK14S, 0x1F, 0x01); // Set scan mode
    // I2C_WriteRandom(I2CADDR_CSK14S,0x1F,0x0D);		//Set scan mode to auto adjust mode

    TouchPowerMgr.Status = HighSensitivity;
}
void SET_CSK14_SCAN_PAUSE(void)
{
    I2C_WriteRandom(I2CADDR_CSK14S, 0xAF, 0x3A); // Enable write
    //	Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0x1F, 0x00); // Stop scan before change sensitivity
    //	Hardware_DelayMs(1);

    //	TouchPowerMgr.Status = NoSensing;
}
void SET_CSK14_SCAN_RESUME(void)
{
    I2C_WriteRandom(I2CADDR_CSK14S, 0xAF, 0x3A); // Enable write
    //	Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0x1F, 0x01); // Set scan mode to normal mode
    //	Hardware_DelayMs(1);
}
void SET_CSK14_NO_SENSING(void)
{
    I2C_WriteRandom(I2CADDR_CSK14S, 0xAF, 0x3A); // Enable write
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0x1F, 0x00); // Stop scan before change sensitivity
    Hardware_DelayMs(1);
    I2C_WriteRandom(I2CADDR_CSK14S, 0x1F, 0x80); // power down
    Hardware_DelayMs(1);

    TouchPowerMgr.Status = NoSensing;
}
void I2C_ReadSequential(uint8_t DeviceAddr, uint8_t StartAddr, uint8_t* BUFF, uint8_t Num)
{
    IICA0_MasterSend(DeviceAddr, (uint8_t*)&StartAddr, 1, 20);
    while (g_iica0_tx_end == 0)
        ;
    IICA0_MasterReceive(DeviceAddr, BUFF, Num, 20);
    while (g_iica0_rx_end == 0)
        ;
}
void I2C_WriteSequential(uint8_t DeviceAddr, uint8_t StartAddr, uint8_t* BUFF, uint8_t Num)
{
    uint8_t tx_buf[Num + 1];
    tx_buf[0] = StartAddr;
    memcpy(tx_buf + 1, BUFF, Num);
    IICA0_MasterSend(DeviceAddr, tx_buf, Num + 1, 20);
    while (g_iica0_tx_end == 0) {
    }
}
