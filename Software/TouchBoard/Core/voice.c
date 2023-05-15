#include "voice.h"
#include <stdint.h>

#include "CMS32L051.h"
#include "userdefine.h"
#include "gpio.h"
#include "tim4.h"
#include "queue.h"
#include "MultiTimer.h"
#include <string.h>

#include "com.h"
#include "device.h"

typedef enum
{
    bFALSE,
    bTRUE = ~bFALSE
} bool_t;
struct
{
    enum
    {
        VoiceIdle = 0,
        SendingDataStart = 1,
        SendingData,
        SendingDataEnd,
        ContinueSendData
    } Status;
    uint8_t BitPoint;
    uint8_t PulseWidthCnt;
    uint8_t Data;
    uint8_t DataBuff[100];
    uint8_t SendDataNum;
    uint8_t TotalDataNum;
    uint16_t VoicePlayEndCnt;
    bool_t VoicePlayEnd;
} VoiceDataTransferMgr;
#define SET_VOICEDATA_H PORT_SetBit(PORT1, PIN2);
#define SET_VOICEDATA_L PORT_ClrBit(PORT1, PIN2);
#define STATUS_PINMACRO_VOICEBUSY PORT_GetBit(PORT2, PIN3)
#define STATUS_PINMACRO_VOICEDATA PORT_GetBit(PORT1, PIN2)
#define DEF_VoiceSegmentEndFlag 0xFA
#define DEBUG_MARK
#define DEF_ButtonPress_Voice PLAY_BUTTON_VOICE(VOICE_WaterDrop)
#define DEF_Fail_Beep PLAY_VOICE_ONESEGMENT_FIXED(VOICE_FailVoice)

extern void VOICE_Init(void);
extern void PLAY_BUTTON_VOICE(uint8_t segment);
extern void PLAY_VOICE_ONESEGMENT(uint8_t segment);
extern void PLAY_VOICE_TWOSEGMENT(uint8_t *segment);
extern void PLAY_VOICE_THREESEGMENT(uint8_t *segment);
extern void PLAY_VOICE_MULTISEGMENTS(uint8_t BUFF[]);
extern void PLAY_VOICE_ONESEGMENT_FIXED(uint8_t segment);
extern void PLAY_VOICE_TWOSEGMENT_FIXED(uint8_t segment1, uint8_t segment2);
extern void PLAY_VOICE_THREESEGMENT_FIXED(uint8_t segment1, uint8_t segment2, uint8_t segment3);
extern void SET_VOLUME(uint8_t volume);
extern void STOP_VOICEPLAY(void);
extern void PLAY_VOICE_DOORBELL(void);
extern void VoicePlayerPowerDown(void);

extern void PLAY_VOICE2_ONESEGMENT(uint8_t segment);
extern void PLAY_VOICE2_TWOSEGMENT(uint8_t segment1, uint8_t segment2);
extern void PLAY_VOICE2_DOORBELL(void);
extern void PLAY_VOICE2_CONTINUE(uint8_t segment);
extern void STOP_VOICEPLAY2(void);
uint8_t SystemLanguage = 0;
#define TranslateNumberToVoice(num) (VOICE_Zero + 2 * (num))
void VoiceInit(void);
void VoiceSleep(void);
void VoiceTask(MultiTimer *timer, void *userData);
void PlayPleaseInputFpm();

struct Device voicePlayer = {NULL, VoiceInit, VoiceSleep};
typedef struct
{
    enum
    {
        StartVoiceEnableSetting = 0,
        WaitForVoiceEnableSettingUserConfirm = 1,
        VoiceEnableSettingSuccess,
        VoiceEnableSettingEXIT
    } Status;
    bool_t Enable;
    uint8_t volume;
    uint8_t TimeCnt;
} VoiceMgr_t;
VoiceMgr_t VoiceMgr;

MultiTimer voiceTimer;

void VoiceInit()
{
    PORT_Init(PORT1, PIN2, OUTPUT);       // VOICE DATA
    PORT_Init(PORT2, PIN3, PULLUP_INPUT); // VOICE BUSY
    TM40_IntervalTimer(TM4_CHANNEL_3, 225); // 需要测试保证准确300us
    VoiceDataTransferMgr.PulseWidthCnt = 0;
    VoiceDataTransferMgr.Status = VoiceIdle;
    // MultiTimerStart(&voiceTimer, 1000, VoiceTask, NULL);
}

// void VoiceTask(MultiTimer *timer, void *userData)
// {
    
//     MultiTimerStart(&voiceTimer, 1, VoiceTask, NULL);
// }

void VoiceSleep()
{
    TM40_Channel_Stop(TM4_CHANNEL_3);
    PRINT("voice player sleep\n");
}

void PLAY_VOICE_ONESEGMENT_FIXED(uint8_t segment)
{
    VoiceDataTransferMgr.DataBuff[0] = segment;
    VoiceDataTransferMgr.BitPoint = 0x00;
    VoiceDataTransferMgr.SendDataNum = 0;
    VoiceDataTransferMgr.TotalDataNum = 1;
    VoiceDataTransferMgr.Status = SendingDataStart;
    VoiceDataTransferMgr.VoicePlayEndCnt = 0;
    VoiceDataTransferMgr.VoicePlayEnd = bFALSE;
}

void PLAY_VOICE_TWOSEGMENT_FIXED(uint8_t segment1, uint8_t segment2)
{

    //   uint8_t Buff[4];

    //    Buff[0] = segment1;
    //    Buff[1] = segment2;
    //    Buff[2] = DEF_VoiceSegmentEndFlag;
    //  PLAY_VOICE_MULTISEGMENTS_FIXED(Buff);
}

void PLAY_VOICE_THREESEGMENT_FIXED(uint8_t segment1, uint8_t segment2, uint8_t segment3)
{
    //    uint8_t Buff[4];

    //    Buff[0] = segment1;
    //    Buff[1] = segment2;
    //    Buff[2] = segment3;
    //    Buff[3] = DEF_VoiceSegmentEndFlag;
    //  PLAY_VOICE_MULTISEGMENTS_FIXED(Buff);
}

void PLAY_VOICE_MULTISEGMENTS_FIXED(const uint8_t BUFF[], int n)
{
    uint8_t SegmentCnt;
    for (SegmentCnt = 0; SegmentCnt < 49 && SegmentCnt < n; SegmentCnt++)
    {
        if (BUFF[SegmentCnt] == DEF_VoiceSegmentEndFlag)
        {
            break;
        }
        VoiceDataTransferMgr.DataBuff[2 * SegmentCnt] = 0xF3;                 // Continue play
        VoiceDataTransferMgr.DataBuff[2 * SegmentCnt + 1] = BUFF[SegmentCnt]; // Continue play
    }
    VoiceDataTransferMgr.BitPoint = 0x00;
    VoiceDataTransferMgr.SendDataNum = 0;
    VoiceDataTransferMgr.TotalDataNum = 2 * SegmentCnt;
    VoiceDataTransferMgr.Status = SendingDataStart;
    VoiceDataTransferMgr.VoicePlayEndCnt = 0;
    VoiceDataTransferMgr.VoicePlayEnd = bFALSE;
}

void PLAY_VOICE_ONESEGMENT(uint8_t segment)
{
    uint8_t Buff[2];

    Buff[0] = segment;
    Buff[1] = DEF_VoiceSegmentEndFlag;
    PLAY_VOICE_MULTISEGMENTS(Buff);
}

void PLAY_VOICE_TWOSEGMENT(uint8_t *segment)
{
    uint8_t Buff[3];

    /*  Buff[0] = segment1;
     Buff[1] = segment2; */
    Buff[0] = segment[0];
    Buff[1] = segment[1];
    Buff[2] = DEF_VoiceSegmentEndFlag;
    PLAY_VOICE_MULTISEGMENTS(Buff);
}

void PLAY_VOICE_THREESEGMENT(uint8_t *segment)
{
    uint8_t Buff[4];
    // if ( VoiceMgr.Enable == bTRUE)
    {
        /* Buff[0] = segment1;
        Buff[1] = segment2;
        Buff[2] = segment3; */
        Buff[0] = segment[0];
        Buff[1] = segment[1];
        Buff[2] = segment[2];
        Buff[3] = DEF_VoiceSegmentEndFlag;
        PLAY_VOICE_MULTISEGMENTS(Buff);
    }
}

void PLAY_VOICE_SEGMENT(const uint8_t *segment, uint8_t len)
{
    uint8_t Buff[len + 1];
    for (int i = 0; i < len; i++)
    {
        Buff[i] = segment[i];
    }
    Buff[len] = DEF_VoiceSegmentEndFlag;
    PLAY_VOICE_MULTISEGMENTS(Buff);
}

void PLAY_VOICE_MULTISEGMENTS(uint8_t BUFF[])
{
    uint8_t SegmentCnt;

    for (SegmentCnt = 0; SegmentCnt < 49; SegmentCnt++)
    {
        if (BUFF[SegmentCnt] == DEF_VoiceSegmentEndFlag)
        {
            break;
        }
        VoiceDataTransferMgr.DataBuff[2 * SegmentCnt] = 0xF3;                                  // Continue play
        VoiceDataTransferMgr.DataBuff[2 * SegmentCnt + 1] = BUFF[SegmentCnt] + SystemLanguage; // Continue play
    }
    VoiceDataTransferMgr.BitPoint = 0x00;
    VoiceDataTransferMgr.SendDataNum = 0;
    VoiceDataTransferMgr.TotalDataNum = 2 * SegmentCnt;
    VoiceDataTransferMgr.Status = SendingDataStart;
    VoiceDataTransferMgr.VoicePlayEndCnt = 0;
    VoiceDataTransferMgr.VoicePlayEnd = bFALSE;
}

void STOP_PLAY_VOICE_MULTISEGMENTS(void)
{
    uint8_t SegmentCnt = 0;

    VoiceDataTransferMgr.DataBuff[2 * SegmentCnt] = 0xFE;                                  // Continue play

    VoiceDataTransferMgr.BitPoint = 0x00;
    VoiceDataTransferMgr.SendDataNum = 0;
    VoiceDataTransferMgr.TotalDataNum = 2 * SegmentCnt;
    VoiceDataTransferMgr.Status = SendingDataStart;
    VoiceDataTransferMgr.VoicePlayEndCnt = 0;
    VoiceDataTransferMgr.VoicePlayEnd = bFALSE;
}

void PLAY_BUTTON_VOICE(uint8_t segment)
{
    PLAY_VOICE_ONESEGMENT_FIXED(segment);
}

void STOP_VOICEPLAY(void)
{
    PLAY_VOICE_ONESEGMENT_FIXED(0xFE);
}

void SET_VOLUME(uint8_t volume)
{
    PLAY_VOICE_ONESEGMENT_FIXED(0xE0 + volume); //,VOICE_VolumeAdjust);
}

void VoicePlayerPowerDown(void)
{
    // HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8|GPIO_PIN_9);
}

// void PlayVoice(const uint8_t *buf, int n);

void PlayWaterDrop()
{
    uint8_t temp = VOICE_WATER_DROP;
    PLAY_VOICE_ONESEGMENT(temp);
    // PlayVoice(&temp, 1);
}
void PlayPleaseInputAain()
{
    uint8_t temp[] = {VOICE_PLEASE, VOICE_AGAIN, VOICE_INPUT};
    PLAY_VOICE_SEGMENT(temp, sizeof(temp));
    // PLAY_VOICE_THREESEGMENT(temp);
    // PlayVoice(temp, sizeof(temp));
}
void PlayPleaseRenewInput()
{
    uint8_t temp[] = {VOICE_PLEASE, VOICE_RENEW, VOICE_INPUT};
    PLAY_VOICE_SEGMENT(temp, sizeof(temp));
    // PLAY_VOICE_THREESEGMENT(temp);
    //    PlayVoice(temp, sizeof(temp));
}
void PlayPleaseInputFpm()
{
    uint8_t temp[] = {VOICE_PASSWORD, VOICE_REGISTER, VOICE_SUCCESS, VOICE_PLEASE, VOICE_INPUT, VOICE_FINGERPRINT};
    PLAY_VOICE_SEGMENT(temp, sizeof(temp));
    //    PlayVoice(temp, sizeof(temp));
}

void PlayInputFpm(){
    uint8_t temp[] = {VOICE_PLEASE, VOICE_INPUT, VOICE_FINGERPRINT};
    PLAY_VOICE_SEGMENT(temp, sizeof(temp));
}

void PlayRegisterFail()
{
    uint8_t temp[] = {VOICE_REGISTER, VOICE_FAIL};
    PLAY_VOICE_SEGMENT(temp, sizeof(temp));
    // PLAY_VOICE_TWOSEGMENT(temp);
    //    PlayVoice(temp, sizeof(temp));
}
void PlayAlarmOpened()
{
    uint8_t temp[] = {VOICE_Alarm, VOICE_OPEN};
    PLAY_VOICE_SEGMENT(temp, sizeof(temp));
    // PLAY_VOICE_TWOSEGMENT(temp);
    //    PlayVoice(temp, sizeof(temp));
}
void PlayPleaseInputPassword()
{
    uint8_t temp[] = {VOICE_PLEASE, VOICE_INPUT, VOICE_PASSWORD};
    PLAY_VOICE_SEGMENT(temp, sizeof(temp));
    // PLAY_VOICE_THREESEGMENT(temp);
    //    PlayVoice(temp, sizeof(temp));
}

void TIM40CH3_Interrupt() // 200us interval
{
    DEBUG_MARK;

    if (VoiceDataTransferMgr.PulseWidthCnt > 0)
    {
        VoiceDataTransferMgr.PulseWidthCnt--;
    }

    if (VoiceDataTransferMgr.PulseWidthCnt == 0)
    {
        switch (VoiceDataTransferMgr.Status)
        {
        default:

        case VoiceIdle:
            SET_VOICEDATA_H;
            if (STATUS_PINMACRO_VOICEBUSY != 0)
            {
                if (VoiceDataTransferMgr.VoicePlayEndCnt < 1500)
                {
                    VoiceDataTransferMgr.VoicePlayEndCnt++;
                }
                else
                {
                    VoiceDataTransferMgr.VoicePlayEnd = bTRUE;
                }
                DEBUG_MARK;
            }
            else
            {
                VoiceDataTransferMgr.VoicePlayEndCnt = 0;
                VoiceDataTransferMgr.VoicePlayEnd = bFALSE;
                DEBUG_MARK;
            }
            break;

        case SendingDataStart:

            if (STATUS_PINMACRO_VOICEDATA != 0)
            {
                SET_VOICEDATA_L;
                VoiceDataTransferMgr.PulseWidthCnt = 17; // 5.1ms
            }
            else
            {
                VoiceDataTransferMgr.Status = SendingData; // SendingData;
                VoiceDataTransferMgr.Data = VoiceDataTransferMgr.DataBuff[VoiceDataTransferMgr.SendDataNum];
            }

            break;

        case SendingData:

            if (STATUS_PINMACRO_VOICEDATA == 0) // send LOW
            {
                SET_VOICEDATA_H;
                if ((VoiceDataTransferMgr.Data & 0x01) != 0x00)
                {
                    VoiceDataTransferMgr.PulseWidthCnt = 3; // 900US
                }
                else
                {
                    VoiceDataTransferMgr.PulseWidthCnt = 1; // 300US
                }
            }
            else // send low level
            {
                SET_VOICEDATA_L;
                if ((VoiceDataTransferMgr.Data & 0x01) != 0x00)
                {
                    VoiceDataTransferMgr.PulseWidthCnt = 1; // 300US
                }
                else
                {
                    VoiceDataTransferMgr.PulseWidthCnt = 3; // 900US
                }

                VoiceDataTransferMgr.Data >>= 1;

                if (++VoiceDataTransferMgr.BitPoint >= 8)
                {
                    VoiceDataTransferMgr.Status = SendingDataEnd;
                    DEBUG_MARK;
                }
            }
            break;

        case SendingDataEnd:

            SET_VOICEDATA_H;

            VoiceDataTransferMgr.SendDataNum++;

            if (VoiceDataTransferMgr.SendDataNum < VoiceDataTransferMgr.TotalDataNum)
            {
                VoiceDataTransferMgr.Status = ContinueSendData;
            }
            else
            {
                VoiceDataTransferMgr.Status = VoiceIdle;
            }

            VoiceDataTransferMgr.PulseWidthCnt = 1; // 300US
            DEBUG_MARK;
            break;

        case ContinueSendData:
        {
            VoiceDataTransferMgr.Status = SendingDataStart;
            VoiceDataTransferMgr.BitPoint = 0x00;
        }
            DEBUG_MARK;
            break;
        }
    }
}
