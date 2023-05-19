#include "CMS32L051.h"
#include "userdefine.h"
#include "sci.h"
#include "device.h"
#include <string.h>
#include "intp.h"
#include "gpio.h"
void PLAY_VOICE_MULTISEGMENTS_FIXED(const uint8_t BUFF[], int);
void FpIdentifyResult(uint8_t result);
int UART1_Write_TxBUFFER(unsigned char* buff, unsigned char len);
extern void FPM_SendGetImageCmd(void); // ide
extern void FPM_SendGetEnrollImageCmd(void); // reg
extern void FPM_SendGenCharCmd(uint8_t BufferID); // both
extern void FPM_SendRegModelCmd(void); // reg
extern void FPM_SendStoreCharCmd(uint8_t BufferID, uint16_t UserID); // reg
extern void FPM_SendSearchCmd(uint8_t BufferID, uint16_t StartPage, uint16_t PageNum); // both
extern void FPM_DeleteAllCharCmd(void); // del all
extern void FPM_SendReadIndexTableCmd(void); // read user list
extern void FPM_SetBreathingLED(uint8_t mode, uint8_t startcolor, uint8_t endcolor, uint8_t looptimes); // led
extern void FPM_SendSleepCmd(void); // sleep
extern void FPMcmd_Excute(void);
extern void FPM_Mgr_Task(void);
#define CMD_TEST_CONNECTION 0x0001 // 进行与设备的通讯测试
#define CMD_SET_PARAM 0x0002 // 设 置 设 备 参 数 (Device ID, Security Level, Baudrate,Duplication Check,
                             //  AutoLearn,TimeOut)注：TimeOut只适用于
#define CMD_GET_PARAM 0x0003 // 获 取 设 备 参 数 (Device ID, Security Level, Baudrate,Duplication Check,
                             //  AutoLearn，TimeOut)注：TimeOut只适用于滑动采集器
#define CMD_GET_DEVICE_INFO 0x0004 // 获取设备信息
#define CMD_ENTER_IAP_MODE 0x0005 // 将设备设置为 IAP状态
#define CMD_GET_IMAGE 0x0020 // 从采集器采集指纹图像并保存于 ImageBuffer 中
#define CMD_FINGER_DETECT 0x0021 // 检测指纹输入状态
#define CMD_UP_IMAGE 0x0022 // 将保存于 ImageBuffer 中的指纹图像上传至 HOST
#define CMD_DOWN_IMAGE 0x0023 // HOST下载指纹图像到模块的 ImageBuffer 中
#define CMD_SLED_CTRL 0x0024 // 控制采集器背光灯的开/关（注：半导体传感器不用此功能）
#define CMD_STORE_CHAR 0x0040 // 将指定编号 Ram Buffer中的 Template，注册到指定编号的库中
#define CMD_LOAD_CHAR 0x0041 // 读取库中指定编号中的 Template到指定编号的 Ram Buffer
#define CMD_UP_CHAR 0x0042 // 将保存于指定编号的 Ram Buffer 中的 Template 上传至 HOST
#define CMD_DOWN_CHAR 0x0043 // 从 HOST下载 Template到模块指定编号的 Ram Buffer 中
#define CMD_DEL_CHAR 0x0044 // 删除指定编号范围内的 Template 。
#define CMD_GET_EMPTY_ID 0x0045 // 获取指定范围内可注册的（没有注册的）第一个模板编号。
#define CMD_GET_STATUS 0x0046 // 获取指定编号的模板注册状态。
#define CMD_GET_BROKEN_ID 0x0047 // 检查指定编号范围内的所有指纹模板是否存在坏损的情况
#define CMD_GET_ENROLL_COUNT 0x0048 // 获取指定编号范围内已注册的模板个数。
#define CMD_GENERATE 0x0060 // 将 ImageBuffer 中的指纹图像生成模板数据１４嬗谥付ū嗪诺? Ram Buffer 中。
#define CMD_MERGE 0x0061 // 将保存于 Ram Buffer 中的两或三个模板数据融合成一个模板数据

#define CMD_MATCH 0x0062 // 指定 Ram Buffer 中的两个指纹模板之间进行 1:1 比对
#define CMD_SEARCH 0x0063 // 指定 Ram Buffer 中的模板与指纹库中指定
                          // 定编号范围内的所有模板之间进行 1:N 比对
#define CMD_VERIFY 0x0064 // 指定 Ram Buffer 中的治颇０逵胫肝瓶庵兄付ū嗪诺闹肝颇０逯浣? 1:1比对
#define CMD_SET_MODULE_SN 0x0008 // 在设备中设置模块序列号信息（Module SN）
#define CMD_GET_MODULE_SN 0x0009 // 获取本设备的模块序列海 Module SN）
#define CMD_FP_CANCEL 0x0025 // 取消指纹采集操作（只适用于带 TimeOut 参数的滑动传感器）
#define CMD_GET_ENROLLED_ID_LIST 0x0049 // 获取已注册 User ID 列表
#define CMD_ENTER_STANDY_STATE 0x000C // 使模块进入休眠状獭注：有些模块不支持菝吖δ埽淙荒？橄煊Ω弥噶罘祷爻晒?

#define FPM_SID 0x01
#define FPM_DID 0x02
struct
{
    uint8_t TxBUFFER[20];
    uint8_t TxPoint;
    uint8_t TxLength;

    uint8_t RX_Buffer[100];
    uint8_t RX_DataPoint;
    enum {
        Idle = 0,
        ReceivingData = 1,
        GotNewCmd
    } Status;
} UART1_Mgr;
static MultiTimer taskTimer, comTimer;
typedef enum {
    Error_NONE = 0, //		00H ：表示指令执行完毕或
    Error_Fail = 1, //		01H ：表示数据包接收错误；
    Error_NoFinger, //		02H ：表示传感器上没有手指；
    Error_GetImage, //		03H ：表示录入指纹图像失败；
    Error_FingerTooDry, //		04H ：表示指纹图像太干、淡而生不成特征；
    Error_FingerTooDamp, //		05H ：表示指纹图像太湿、糊而生不成特征；
    Error_GenChar, //		06H ：表示指纹图像太乱而生不成特征；
    Error_GenChar1, //		07H ：表示指纹图像正常，但特征点太少（或面积小）而生不成；
    Error_NotMatch, //		08H ：表示指纹不匹配
    Error_UnRegistered, //		09H ：表示 没搜索到指纹；
    Error_Combine, //		0aH ：表示特征合并失败；
    Error_FpAddrOutRange, //		0bH ：表示访问指纹库时地址序号超出范围；
    Error_ReadTemplate, //		0cH ：表示从指纹库读模板出错或无效；
    Error_UploadChar, //		0dH ：表示上传特征失败；
    Error_GetData, //		0eH ：表示模块不能接后续数据包
    Error_UploadImage, //		0fH ：表示上传图像失败
    Error_DeleteTemplate, //		10H ：表示删除模板失败；
    Error_DeleteAll, //		11H ：表示清空指纹库失败；
    Error_GoToSleep, //		12H ：表示不能进入低功耗状态；
    Error_Passcode, //		13H ：表示口令不正确；
    Error_SystemReset, //		14H ：表示系统复位失败；
    Error_BufferData, //		15H ：表示缓冲区内没有效原始图而生不成像；
    Error_OnlineUpdate, //		16H ：表示在线升级失败；
    Error_FingerNotRelease, //		17H ：表示残留指纹或两次采集之间手没有移动过；
    Error_RWFlash, //		18H ：表示读写 表示读写FLASH出错；
    Error_GenRandomNumber, //		19H ：随机数生成失败
    Error_RegisterInvalid, //		1aH ：无效寄存器号；
    Error_ValueOfRegister, //		1bH ：寄存器设定内容错误号；
    Error_NotePage, //		1cH ：记事本页码指定错误；
    Error_PortOperation, //		1dH ：端口操作失败；
    Error_AutoEnroll, //		1eH ：自动注册（enroll）失败；
    Error_MemoryIsFull, //		1fH ：指纹库满；
    Error_DeviceAddress, //		20H ：设备地址错误
    Error_Password, //		21H ：密码有误；
    Error_TemplateIsNotEmpty, //		22H ：指纹模板非空；
    Error_TemplateIsEmpty, //		23H ：指纹模板为空；
    Error_FpMemoryIsEmpty, //		24H ：指纹库为空；
    Error_EnrollTimes, //		25H ：录入次数设置错误；
    Error_TimeOut, //		26H ：超时；
    Error_TemplateIsRegistered, //		27H ：指纹已存在
    Error_TemplateIsUnion, //		28H ：指纹模板有关联
    Error_Sensor, //		29H ：传感器初始化失败；
    Error_Reserve, //		2AH ：Reserved
    Error_SerialNumberMismatched,
    Error_DataPackageF0 = 0xF0, //		f0H ：有后续数据包的指令，正确接收用0xf0应答
    Error_DataPackageF1 = 0xF1, //		f1H ：有后续数据包的指令，命令包用0xf1应答；
    Error_FlashChecksum = 0xF2, //		f2H ：表示烧写内部FLASH时，校验和错误
    Error_FlashPackageHead = 0xF3, //		f3H ：表示烧写内部FLASH时，包标识错误
    Error_FlashPackageLength = 0xF4, //		f4H ：表示烧写内部FLASH时，包长度错误
    Error_FlashHexFileTooLong = 0xF5, //		f5H ：表示烧写内部FLASH时，代码长度太长
    Error_FlashWriteFail = 0xF6, //		f6H ：表示烧写内部FLASH时，烧写时，烧写FLASH失败 ；
    Error_BadImage = 0xF9, //		f9H ：采集到的图像不清晰，魔力FPC指纹头
    Error_CheckSum
} FPMcmdErrorType_t;

struct
{
    uint16_t Para1;
    uint16_t Para2;
    uint8_t Buff[100];
    enum {
        WaitACK = 0,
        GotACK = 1
    } Status;
    enum {
        GetImageCmd = 0,
        GenCharCmd = 1,
        RegModelCmd,
        StoreCharCmd
    } Type;
    FPMcmdErrorType_t ErrorCode;
} FpmAckMgr;
static uint8_t timeoutCnt = 0;
void FPcmd_Init(void)
{
    UART1_Mgr.TxPoint = 0;
    UART1_Mgr.TxLength = 0;
}

void FPM_ResetRX(void)
{
    UART1_Mgr.RX_DataPoint = 0x00;
    UART1_Mgr.Status = Idle;
}

void FPMcmd_Excute(void)
{
    uint16_t i;
    // uint8_t RxBuff[100];
    uint16_t CmdLenth, CKS, TempCKS;

    if (UART1_Mgr.Status != GotNewCmd) { // UART得到新命令
        return;
    }
    CmdLenth = (UART1_Mgr.RX_Buffer[7] * 256) + UART1_Mgr.RX_Buffer[8] + 9; //>>8
    if (CmdLenth >= 100) {
        UART1_Mgr.RX_DataPoint = 0x00;
        UART1_Mgr.Status = Idle; // 处理模式
                                 //  OutputDec(CmdLenth);
                                 //  CLRWDT();
                                 //  Hardware_DelayMs(500);
        return;
    }
    for (i = 0; i < CmdLenth; i++) {
        FpmAckMgr.Buff[i] = UART1_Mgr.RX_Buffer[i]; // 复制Rx
    }

    UART1_Mgr.RX_DataPoint = 0x00;
    UART1_Mgr.Status = Idle; // 处理模式

    CKS = 0x0000;
    for (i = 6; i < (CmdLenth - 2); i++) {
        CKS = CKS + FpmAckMgr.Buff[i]; // 将RX累加[6,cmdength-2]
    }

    TempCKS = (FpmAckMgr.Buff[CmdLenth - 2] * 256) + FpmAckMgr.Buff[CmdLenth - 1];

    if (CKS != TempCKS) {
        FpmAckMgr.Status = GotACK;
        FpmAckMgr.ErrorCode = Error_CheckSum;
        return; // if check sum is failed, ignore this data strin//检查命令和
    }
    if (FpmAckMgr.Buff[6] == 0x07) {
        FpmAckMgr.Status = GotACK;
        FpmAckMgr.ErrorCode = (FPMcmdErrorType_t)FpmAckMgr.Buff[9];
        FpmAckMgr.Para1 = FpmAckMgr.Buff[10] * 256 + FpmAckMgr.Buff[11];
        FpmAckMgr.Para2 = FpmAckMgr.Buff[12] * 256 + FpmAckMgr.Buff[13];
    } else {
        FpmAckMgr.Status = GotACK;
        FpmAckMgr.ErrorCode = Error_Fail; // data package fail//数据包失败
    }
    MultiTimerCallback_t temp = comTimer.callback;
    if (temp) {
        MultiTimerStop(&comTimer);
        temp(&comTimer, NULL);
    }
    // PRINT("GOT ACK!\n");
    // PrintfBuf(FpmAckMgr.Buff,CmdLenth);
}

/**
 * @brief 验证指纹时， 探测手指，探测到后录入指纹图像存于图像缓冲区。
 * 
 */
void FPM_SendGetImageCmd(void)
{

    uint8_t buff[12] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x01, 0x00, 0x05 };

    UART1_Write_TxBUFFER(&buff[0], 12);

    FPM_ResetRX();
}

/**
 * @brief 注册指纹时， 探测手指，探测到后录入指纹图像存于图像缓冲区。
 * 
 */
void FPM_SendGetEnrollImageCmd(void)
{

    uint8_t buff[12] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x29, 0x00, 0x2D };

    UART1_Write_TxBUFFER(&buff[0], 12);

    FPM_ResetRX();
}

/**
 * @brief 将图像缓冲区中的原始图像生成指纹特征文件存于模板缓冲区。
 * 
 * @param BufferID 在注册过程中， BufferID 表示此次提取的特征存放在缓冲区中的位置；其他情况中， BufferID
 * 有相应的默认值。
 */
void FPM_SendGenCharCmd(uint8_t BufferID)
{
    uint8_t i;
    uint16_t CKS;

    uint8_t buff[13] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x02, 0x00, 0x00, 0x05 };

    buff[10] = BufferID;

    CKS = 0x0000;
    for (i = 6; i < 11; i++) {
        CKS += buff[i];
    }
    buff[11] = CKS >> 8;
    buff[12] = CKS;

    UART1_Write_TxBUFFER(&buff[0], 13);

    FPM_ResetRX();
}

/**
 * @brief 将特征文件融合后生成一个模板，结果存于模板缓冲区中。
 * 
 */
void FPM_SendRegModelCmd(void)
{

    uint8_t buff[12] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x05, 0x00, 0x09 };

    UART1_Write_TxBUFFER(&buff[0], 12);

    FPM_ResetRX();
}

/**
 * @brief 将模板缓冲区中的模板文件存到 PageID 号 flash 数据库位置。
 * 
 * @param BufferID 模板缓冲区 默认为 1
 * @param UserID 指纹库位置号
 */
void FPM_SendStoreCharCmd(uint8_t BufferID, uint16_t UserID)
{
    uint8_t i;
    uint16_t CKS;

    uint8_t buff[15] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x06, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00 };

    buff[10] = BufferID;

    buff[11] = UserID >> 8;
    buff[12] = UserID;

    CKS = 0x0000;
    for (i = 6; i < 13; i++) {
        CKS += buff[i];
    }
    buff[13] = CKS >> 8;
    buff[14] = CKS;

    UART1_Write_TxBUFFER(&buff[0], 15);

    FPM_ResetRX();
}

/**
 * @brief 以模板缓冲区中的特征文件搜索整个或部分指纹库。
 * 
 * @param BufferID 模板缓冲区
 * @param StartPage 起始页
 * @param PageNum 页数
 */
void FPM_SendSearchCmd(uint8_t BufferID, uint16_t StartPage, uint16_t PageNum)
{
    uint8_t i;
    uint16_t CKS;

    uint8_t buff[17] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    buff[10] = BufferID;

    buff[11] = StartPage >> 8;
    buff[12] = StartPage;

    buff[13] = PageNum >> 8;
    buff[14] = PageNum;

    CKS = 0x0000;
    for (i = 6; i < 15; i++) {
        CKS += buff[i];
    }
    buff[15] = CKS >> 8;
    buff[16] = CKS;

    UART1_Write_TxBUFFER(&buff[0], 17);

    FPM_ResetRX();
}

/**
 * @brief 删除 flash 数据库中指定 ID 号开始的 N 个指纹模板。
 * 
 * @param StartPageID 指纹库模板号
 * @param CharNum 删除的模板个数
 */
void FPM_DeleteCharCmd(uint16_t StartPageID, uint16_t CharNum)
{
    uint8_t i;
    uint16_t CKS;

    uint8_t buff[16] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    buff[10] = StartPageID >> 8;
    buff[11] = StartPageID;

    buff[12] = CharNum >> 8;
    buff[13] = CharNum;

    CKS = 0x0000;
    for (i = 6; i < 14; i++) {
        CKS += buff[i];
    }
    buff[14] = CKS >> 8;
    buff[15] = CKS;

    UART1_Write_TxBUFFER(&buff[0], 16);

    FPM_ResetRX();
}

/**
 * @brief 清空指纹库
 * 
 */
void FPM_DeleteAllCharCmd(void)
{

    uint8_t buff[12] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x0D, 0x00, 0x11 };

    UART1_Write_TxBUFFER(&buff[0], 12);

    FPM_ResetRX();
}

void FPM_SendGetSerialNumberCmd(void)
{

    uint8_t buff[13] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x34, 0x00, 0x00, 0x39 };

    UART1_Write_TxBUFFER(&buff[0], 13);

    FPM_ResetRX();
}

/**
 * @brief 读有效模板个数
 * 
 */
void FPM_GetValidTempleteNumCmd(void)
{

    uint8_t buff[12] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x1D, 0x00, 0x21 };

    UART1_Write_TxBUFFER(&buff[0], 12);

    FPM_ResetRX();
}

/**
 * @brief 读取录入模版的索引表
 * 
 */
void FPM_SendReadIndexTableCmd(void)
{
    uint8_t i;
    uint16_t CKS;

    uint8_t buff[13] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x1F, 0x00, 0x00, 0x39 };
    CKS = 0x0000;
    for (i = 6; i < 11; i++) {
        CKS += buff[i];
    }
    buff[11] = CKS >> 8;
    buff[12] = CKS;

    UART1_Write_TxBUFFER(&buff[0], 13);

    FPM_ResetRX();
}

/**
 * @brief 一站式注册指纹，包含采集指纹、生成特征、组合模板、存储模板等功能。
 * 
 * @param UserID 高字节在前，低字节在后。 例如录入 1 号指纹，则是 0001H。
 */
void FPM_SendAutoRegisterCmd(uint16_t UserID)
{
    uint8_t i;
    uint16_t CKS;

    uint8_t buff[17] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x08, 0x31, 0x00, 0x01, 0x02, 0x00, 0xCF, 0x00, 0x39 };

    buff[10] = UserID >> 8;

    buff[11] = UserID;

    CKS = 0x0000;
    for (i = 6; i < 15; i++) {
        CKS += buff[i];
    }
    buff[15] = CKS >> 8;
    buff[16] = CKS;
    UART1_Write_TxBUFFER(&buff[0], 17);

    FPM_ResetRX();
}

/**
 * @brief 控制呼吸灯
 * 
 * @param mode 功能码：呼吸灯、闪烁灯、常开灯、常闭灯
 * @param startcolor 起始颜色
 * @param endcolor 结束颜色
 * @param looptimes 循环次数 表示呼吸或者闪烁 x 次灯
 */
void FPM_SetBreathingLED(uint8_t mode, uint8_t startcolor, uint8_t endcolor, uint8_t looptimes)
{
    uint8_t i;
    uint16_t CKS;

    uint8_t buff[16] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39 };

    buff[10] = mode;
    buff[11] = startcolor;
    buff[12] = endcolor;
    buff[13] = looptimes;

    CKS = 0x0000;
    for (i = 6; i < 14; i++) {
        CKS += buff[i];
    }
    buff[14] = CKS >> 8;
    buff[15] = CKS;
    UART1_Write_TxBUFFER(&buff[0], 16);

    FPM_ResetRX();
}

/**
 * @brief 写模组寄存器。
 * 
 * @param Level 比对阀值寄存器
 */
void FPM_SetSecurityLevel(uint8_t Level) // from 1 ~5
{
    uint8_t i;
    uint16_t CKS;

    uint8_t buff[14] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x05, 0x0E, 0x05, 0x00, 0x00, 0x39 };

    buff[11] = Level;

    CKS = 0x0000;
    for (i = 6; i < 12; i++) {
        CKS += buff[i];
    }
    buff[12] = CKS >> 8;
    buff[13] = CKS;

    UART1_Write_TxBUFFER(&buff[0], 14);

    FPM_ResetRX();
}

void FPM_TurnOffAntiFakeFp(void) // from 1 ~5
{
    uint8_t i;
    uint16_t CKS;

    uint8_t buff[14] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x05, 0x0E, 0x08, 0x00, 0x00, 0x39 };

    buff[11] = 0x05;
    // Bit0�? 0-关闭抗残留功能，1-打开抗残留功能（默认状�?�）�?
    // Bit1�? 0-关闭指纹膜认假算法，1-打开指纹膜认假算法（默认状�?�）�?
    // Bit2�? 0-关闭学习功能�?1-打开学习功能（默认状态）�?
    // Bit3~ Bit7：保留（默认�? 0）�??

    CKS = 0x0000;
    for (i = 6; i < 12; i++) {
        CKS += buff[i];
    }
    buff[12] = CKS >> 8;
    buff[13] = CKS;

    UART1_Write_TxBUFFER(&buff[0], 14);

    FPM_ResetRX();
}

/**
 * @brief 设置传感器进入休眠模式
 * 
 */
void FPM_SendSleepCmd(void)
{

    uint8_t buff[12] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x33, 0x00, 0x37 };
    UART1_Write_TxBUFFER(&buff[0], 12);

    FPM_ResetRX();
}

/**
 * @brief 获取芯片唯一序列号
 * 
 */
void FPM_SendGetChipSerialNumberCmd(void)
{

    uint8_t buff[13] = { 0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x34, 0x00, 0x00, 0x39 };

    UART1_Write_TxBUFFER(&buff[0], 13);

    FPM_ResetRX();
}

void FPM_Mgr_Task(void)
{
    FPMcmd_Excute();
}

void ComInit(void);
void FPM_ComSleep(void);
struct Device comFingerprint = { NULL, ComInit, NULL };

void FPM_ComTask(MultiTimer* timer, void* userData);
static uint8_t rxByte;
uint8_t fpUserCnt = 0;
void FpmUserReport(uint8_t cnt);
void StartIdentifyFp(void);
void StopIdentify(void);
void StartRegisterFp(uint8_t);
void ContinueRegisterFp(uint8_t);
void StoreFp(uint8_t userID);
void StopRegisterFp(void);
void DelAllUserFormFpm(void);
struct FpmComTask {
    uint8_t startIdentifyFlag : 1;
    uint8_t stopIdentifyFlag : 1;
    uint8_t startRegisterFlag : 1;
    uint8_t continueRegisterFlag : 1;
    uint8_t storeFlag : 1;
    uint8_t sleepFpmFlag : 1;
    uint8_t clrAllFpFlag : 1;
    uint8_t setColorFlag : 1;
    uint8_t setColor;
    uint8_t storeUserId;
    uint8_t continueEnrollTimes;
};
struct FpmComTask fpmTask;
#define OFF 0
#define RED 1
#define BLUE 2
#define GREEN 4
uint8_t colorSet;
void SetColor(uint8_t color);
void FPMDealInit(void);
void SetColorCallback(MultiTimer* timer, void* userData)
{
    if (FpmAckMgr.Status == GotACK) {
        fpmTask.setColorFlag = 0;
        PRINT("SET COLOR success!\n");
    } else {
        PRINT("SET COLOR TIME OUT!\n");
        timeoutCnt++;
        if (timeoutCnt >= 2) {
            timeoutCnt = 0;
            PRINT("timeoutCnt 3 RESET color\n");
            SetColor(colorSet);
        } else {
            PRINT("get image timeout\n");
            MultiTimerStart(&comTimer, 200, SetColorCallback, NULL);
        }
    }
}
void SetColor(uint8_t color)
{
    colorSet = color;
    FpmAckMgr.Status = WaitACK;
    FPM_SetBreathingLED(1, color, color, 255);
    MultiTimerStart(&comTimer, 300, SetColorCallback, NULL);
}
void ReadUserListCallback(MultiTimer* timer, void* userData)
{
    int USERID, i, j;
    fpUserCnt = 0;
    if (FpmAckMgr.Status == GotACK) {
        if (FpmAckMgr.ErrorCode == Error_NONE) {
            USERID = 0x00;
            for (i = 0; i < 32; i++) {
                for (j = 0; j < 8; j++) {
                    if ((FpmAckMgr.Buff[10 + i] & 0x01) != 0) {
                        if (USERID < 20) {
                            fpUserCnt++;
                        }
                    }
                    FpmAckMgr.Buff[10 + i] >>= 1;
                    USERID++;
                }
            }
            FpmUserReport(fpUserCnt);
            //  PLAY_VOICE_MULTISEGMENTS_FIXED(&temp,1);
        }
    }
}
void delayMS(uint32_t n);
void ComInit()
{
    SystemCoreClockUpdate();
    UART1_Init(SystemCoreClock, 57600);
    UART1_Receive(&rxByte, 1);
    PORT_Init(PORT12, PIN3, OUTPUT);
    PORT_Init(PORT13, PIN6, INPUT);
    PORT_SetBit(PORT12, PIN3);
    delayMS(100);
    FpmAckMgr.Status = WaitACK;
    FPM_SendReadIndexTableCmd();
    while (UART1_Mgr.Status != GotNewCmd)
        ;
    FPMcmd_Excute();
    int USERID, i, j;
    fpUserCnt = 0;
    if (FpmAckMgr.Status == GotACK) {
        if (FpmAckMgr.ErrorCode == Error_NONE) {
            USERID = 0x00;
            for (i = 0; i < 32; i++) {
                for (j = 0; j < 8; j++) {
                    if ((FpmAckMgr.Buff[10 + i] & 0x01) != 0) {
                        if (USERID < 20) {
                            fpUserCnt++;
                        }
                    }
                    FpmAckMgr.Buff[10 + i] >>= 1;
                    USERID++;
                }
            }
            FpmUserReport(fpUserCnt);
            PRINT("FpmUserReport excute!\n");
            //  PLAY_VOICE_MULTISEGMENTS_FIXED(&temp,1);
        }
    }
    MultiTimerStart(&taskTimer, 10, FPM_ComTask, NULL);
    fpmTask.startIdentifyFlag = 0;
    fpmTask.stopIdentifyFlag = 0;
    fpmTask.startRegisterFlag = 0;
    fpmTask.continueRegisterFlag = 0;
    fpmTask.storeFlag = 0;
    fpmTask.sleepFpmFlag = 0;
    fpmTask.clrAllFpFlag = 0;
    fpmTask.setColorFlag = 0;
}

typedef enum {
    FPMcmdStart = 0,
    SendFirstGetImageCmd,
    WaitForFirstGetImageCmdACK,
    SendFirstGenCharCmd,
    WaitForFirstGenCharCmdACK,
    SendDetectFingerFirstTimeRemoveCmd,
    WaitForDetectFingerFirstTimeRemoveCmdACK,
    SendSecondGetImageCmd,
    WaitForSecondGetImageCmdACK,
    SendSecondGenCharCmd,
    WaitForSecondGenCharCmdACK,
    SendDetectFingerSecondTimeRemoveCmd,
    WaitForDetectFingerSecondTimeRemoveCmdACK,
    SendThirdGetImageCmd,
    WaitForThirdGetImageCmdACK,
    SendThirdGenCharCmd,
    WaitForThirdGenCharCmdACK,
    SendDetectFingerThirdTimeRemoveCmd,
    WaitForDetectFingerThirdTimeRemoveCmdACK,
    SendFourthGetImageCmd,
    WaitForFourthGetImageCmdACK,
    SendFourthGenCharCmd,
    WaitForFourthGenCharCmdACK,
    SendRegModelCmd,
    WaitForRegModelCmdACK,
    SendStoreCharCmd,
    WaitForStoreCharCmdACK,
    // StartFpUserDelete,
    SendDeleteCharCmd,
    WaitForDeleteCharCmdACK,
    StartFpUserIdentify,
    SendGetImageCmd,
    WaitForGetImageCmdACK,
    SendGenCharCmd,
    WaitForGenCharCmdACK,
    SendDetectFingerRemoveCmd,
    WaitForDetectFingerRemoveCmdACK,
    SendSearchCmd,
    WaitForSearchCmdACK,
    SendGetSerialNumberCmd,
    WaitForGetSerialNumberCmdACK,
    // StartAllUserFpDelete,
    //		SendDeleteAllUserFpCmd,
    //		WaitForDeleteAllUserFpCmdACK,
    success,
    fail

} FPMcmdStatus_t;
struct
{
    uint16_t UserID;
    uint16_t TimeCnt;
    FPMcmdStatus_t Status;
    FPMcmdErrorType_t ErrorType;
} FpIdentifyMgr;
#define Def_FPMcmdTimeOutDelay 128 // 2s
#define Def_IdendtifyFailScreenTimeDelay 192 // 3s
extern uint8_t sleepFlag;
typedef enum { bFALSE = 0,
    bTRUE = ~bFALSE } bool_t;
struct
{
    uint16_t UserID;
    uint16_t TimeCnt;
    uint8_t EnrollSuccessTimes;
    uint8_t EnrollFailTimes;
    bool_t DuplicateCheck;
    FPMcmdStatus_t Status;
} FpRegisterMgr;
void delayMS(uint32_t n);
#define Hardware_DelayMs(sec) delayMS(sec)
#define FPMpowerDown 0
uint8_t FPMpowerMgrStatus;
#define PINMACRO_FPM_TOUCH_STATUS (PORT_GetBit(PORT13, PIN6))
void FPM_ComTask(MultiTimer* timer, void* userData)
{
    // PRINT("FPM_ComTask\n");
    FPM_Mgr_Task();
    if (fpmTask.sleepFpmFlag) {
        // PRINT("fpmTask.sleepFpmFlag=1\n");
        FPMpowerMgrStatus = 1;
        int i;
        int FP_SleepFailedTimes = 0;
        int FPtimeout_times = 0;
        int count = 0;
        while (1) {
            count++;
            FpmAckMgr.Status = WaitACK;
            FPM_SendSleepCmd();
            comTimer.callback = NULL;
            MultiTimerStop(&comTimer);
            for (i = 0; i < 25; i++) {
                Hardware_DelayMs(50);
                FPM_Mgr_Task();
                if (FpmAckMgr.Status == GotACK) {
                    if ((FpmAckMgr.ErrorCode == Error_NONE) && PINMACRO_FPM_TOUCH_STATUS == 0x00) {
                        FPMpowerMgrStatus = FPMpowerDown;
                        break;
                    } else {
                        FP_SleepFailedTimes++;
                        if (FP_SleepFailedTimes > 350) // FP not released more than 10s
                        {
                            FPMpowerMgrStatus = FPMpowerDown;
                        }
                        break;
                    }
                } else {
                    if (i > 23) // time out,FP failed
                    {
                        FPtimeout_times++;
                        if (FPtimeout_times > 2) {
                            FPMpowerMgrStatus = FPMpowerDown;
                            break;
                        }
                    }
                }
            }
            if (FPMpowerMgrStatus == FPMpowerDown) {
                PRINT("FPMpowerDown\n");
                UART1_Stop();
                PORT_ClrBit(PORT12, PIN3);
                INTP_Init(1 << 1, INTP_RISING);
                INTP_Start(1 << 1);
                break;
            }
        }
        // MultiTimerStart(&deviceMgr.timer, 0, SleepTimerCallBack, NULL);
        return;
    }
    if (fpmTask.stopIdentifyFlag) {
        FpmAckMgr.Status = GotACK;
    }
    if (FpmAckMgr.Status == GotACK) {
        if (fpmTask.setColorFlag) {
            PRINT("set color\n");
            SetColor(fpmTask.setColor);
        } else if (fpmTask.startIdentifyFlag) {
            fpmTask.startIdentifyFlag = 0;
            StartIdentifyFp();
            PRINT("start identify\n");
        } else if (fpmTask.stopIdentifyFlag) {
            fpmTask.stopIdentifyFlag = 0;
            StopIdentify();
            PRINT("stop identify\n");
        } else if (fpmTask.startRegisterFlag) {
            fpmTask.startRegisterFlag = 0;
            StartRegisterFp(1);
            PRINT("start reg\n")
        } else if (fpmTask.continueRegisterFlag) {
            fpmTask.continueRegisterFlag = 0;
            ContinueRegisterFp(fpmTask.continueEnrollTimes);
            PRINT("continue reg\n");
        } else if (fpmTask.clrAllFpFlag) {
            fpmTask.clrAllFpFlag = 0;
            DelAllUserFormFpm();
            PRINT("DEL ALL\n");
        } else if (fpmTask.storeFlag) {
            fpmTask.storeFlag = 0;
            StoreFp(fpmTask.storeUserId);
            PRINT("store\n");
        } else if (fpmTask.sleepFpmFlag) {
            fpmTask.sleepFpmFlag = 0;
            //    StopRegisterFp();
            PRINT("stop reg\n");
        }
    }
    MultiTimerStart(&taskTimer, 15, FPM_ComTask, NULL);
}

void FPM_ComSleep()
{
    /* FpmAckMgr.Status = WaitACK; */
    /* FPM_SendSleepCmd();
    while (UART1_Mgr.Status != GotNewCmd)
        ;
    FPMcmd_Excute();
    if (FpmAckMgr.Status == GotACK && FpmAckMgr.ErrorCode == Error_NONE && PINMACRO_FPM_TOUCH_STATUS == 0x00) {
        PORT_ClrBit(PORT12, PIN3);
        UART1_Stop();
        INTP_Init(1 << 1, INTP_RISING);
        INTP_Start(1 << 1);
    } */
}
uint8_t txBuf[50];
static uint8_t uart1SendBusyFlag = 0;
int UART1_Write_TxBUFFER(unsigned char* buff, unsigned char len)
{
    if (!uart1SendBusyFlag) {
        if(buff == NULL || len == 0){
            return 0;
        }
        // PRINT("send to fpm:");
        // PrintfBuf(buff, len);
        memcpy(txBuf, buff, len);
        UART1_Send(txBuf, len);
        return 1;
    } else {
        return 0;
    }
}
void Uart1SendEndCallback()
{
    uart1SendBusyFlag = 0;
}
void Uart1RxByteCallback()
{
    // com.rxBuf[com.rxCnt++]=rxByte;
    if (UART1_Mgr.Status == GotNewCmd) {
        // do nothing
    } else if ((UART1_Mgr.Status == Idle) && (rxByte == 0xEF)) {
        UART1_Mgr.RX_Buffer[UART1_Mgr.RX_DataPoint] = rxByte;
        UART1_Mgr.RX_DataPoint++;
        UART1_Mgr.Status = ReceivingData;
    } else if (UART1_Mgr.Status == ReceivingData) {
        UART1_Mgr.RX_Buffer[UART1_Mgr.RX_DataPoint] = rxByte;
        UART1_Mgr.RX_DataPoint++;
        if ((UART1_Mgr.RX_Buffer[0] == 0xEF) && (UART1_Mgr.RX_DataPoint == (UART1_Mgr.RX_Buffer[8] + 9)) && (UART1_Mgr.RX_DataPoint > 9)) {
            UART1_Mgr.Status = GotNewCmd;
        } else if (UART1_Mgr.RX_DataPoint > 99) {
            UART1_Mgr.RX_DataPoint = 0x00;
            UART1_Mgr.Status = Idle;
        }
    }
    UART1_Receive(&rxByte, 1);
}

/**
 * @brief 发送搜索指纹命令回调函数（BufferId = 1）
 * 
 * @param timer 
 * @param userData 
 */
void SearchCharCallback(MultiTimer* timer, void* userData)
{
    if (FpmAckMgr.Status == GotACK) {
        FpIdentifyResult(FpmAckMgr.ErrorCode);
        fpmTask.startIdentifyFlag = 0;
        if (FpmAckMgr.ErrorCode == Error_NONE) {
            PRINT("PASS!\n");
            // 验证成功上报
        } else {
            PRINT("FpmAckMgr.ErrorCode=%d\n", FpmAckMgr.ErrorCode);
        }
    } else {
        PRINT("Search timeout!\n");
        FpIdentifyResult(0xff);
        fpmTask.startIdentifyFlag = 0;
    }
}

/**
 * @brief 发送生成特征指令回调函数（BufferId = 1）
 * 
 * @param timer 
 * @param userData 
 */
void SendGenCharCallback(MultiTimer* timer, void* userData)
{
    if (FpmAckMgr.Status == GotACK) {
        if (FpmAckMgr.ErrorCode == Error_NONE) {
            FPM_SendSearchCmd(0x01, 0x0000, 20);
            FpmAckMgr.Status = WaitACK;
            MultiTimerStart(&comTimer, 300, SearchCharCallback, NULL);
        } else {
            fpmTask.startIdentifyFlag = 0;
            FpIdentifyResult(Error_GenChar);
            // 验证失败上报
            PRINT("Error_GenChar fail!\n");
        }
    } else {
        timeoutCnt++;
        if (timeoutCnt >= 2) {
            timeoutCnt = 0;
            FpIdentifyResult(0XFF);
            fpmTask.startIdentifyFlag = 0;
            PRINT("timeoutCnt 3 RESTART\n");
        } else {
            PRINT("gen char timeout\n");
            MultiTimerStart(&comTimer, 100, SendGenCharCallback, NULL);
        }
    }
}

/**
 * @brief 获取图像指令回调函数
 * 
 * @param timer 
 * @param userData 
 */
void GetImageCallback(MultiTimer* timer, void* userData)
{
    if (FpmAckMgr.Status == GotACK && FpmAckMgr.ErrorCode == Error_NONE) {
        FPM_SendGenCharCmd(0x01);
        FpmAckMgr.Status = WaitACK;
        MultiTimerStart(&comTimer, 300, SendGenCharCallback, NULL);
    } else {
        FPM_SendGetImageCmd();
        FpmAckMgr.Status = WaitACK;
        MultiTimerStart(&comTimer, 200, GetImageCallback, NULL);
    }
}
void StartIdentifyFp()
{
    FPM_SendGetImageCmd();
    FpmAckMgr.Status = WaitACK;
    MultiTimerStart(&comTimer, 200, GetImageCallback, NULL);
}

void StopIdentify(void)
{
    MultiTimerStop(&comTimer);
    timeoutCnt = 0;
}
void GenRegFpChar(void);
void FpRegGenCharResult(uint8_t result);

/**
 * @brief 生成特征指令回调函数
 * 
 * @param timer 
 * @param userData 
 */
void GenRegFpCharCb(MultiTimer* timer, void* userData)
{
    if (FpmAckMgr.Status == GotACK) {
        timeoutCnt = 0;
        fpmTask.stopIdentifyFlag = 0;
        FpRegGenCharResult(FpmAckMgr.ErrorCode);
        if (FpmAckMgr.ErrorCode == Error_NONE) {
        } else {
            PRINT("ERROR CODE =%d\n", FpmAckMgr.ErrorCode);
        }
    } else {
        timeoutCnt++;
        if (timeoutCnt >= 2) {
            timeoutCnt = 0;
            FpRegGenCharResult(0xff);
        } else {
            PRINT("gen got ack timeout\n");
            MultiTimerStart(&comTimer, 100, GenRegFpCharCb, NULL);
        }
    }
}

/**
 * @brief 生成特征指令
 * 
 */
void GenRegFpChar()
{
    FpmAckMgr.Status = WaitACK;
    FPM_SendGenCharCmd(FpRegisterMgr.EnrollSuccessTimes);
    MultiTimerStart(&comTimer, 200, GenRegFpCharCb, NULL);
}

/**
 * @brief 开始注册回调函数
 * 
 * @param timer 
 * @param userData 
 */
void StartRegisterFpCb(MultiTimer* timer, void* userData)
{

    if (FpmAckMgr.Status == GotACK) {
        timeoutCnt = 0;
        fpmTask.stopIdentifyFlag = 0;
        if (FpmAckMgr.ErrorCode == Error_NONE) {
            PRINT("gen reg fp char\n")
            GenRegFpChar();
        } else {
            if ((FpmAckMgr.ErrorCode == Error_GetImage) || (FpmAckMgr.ErrorCode == Error_NoFinger) || (FpmAckMgr.ErrorCode == Error_BadImage)) {
                StartRegisterFp(FpRegisterMgr.EnrollSuccessTimes);
                PRINT("no image RESTART\n");
            } else {
                FpRegGenCharResult(FpmAckMgr.ErrorCode);
                PRINT("ERROR CODE =%d\n", FpmAckMgr.ErrorCode);
            }
        }
    } else {
        timeoutCnt++;
        if (timeoutCnt >= 2) {
            timeoutCnt = 0;
            PRINT("timeoutCnt 3 RESTART\n");
            StartRegisterFp(FpRegisterMgr.EnrollSuccessTimes);
        } else {
            PRINT("get image timeout\n");
            MultiTimerStart(&comTimer, 100, StartRegisterFpCb, NULL);
        }
    }
}

void StartRegisterFp(uint8_t enrollTimes)
{
    FpRegisterMgr.EnrollSuccessTimes = enrollTimes;
    FpmAckMgr.Status = WaitACK;
    FPM_SendGetEnrollImageCmd();
    MultiTimerStart(&comTimer, 200, StartRegisterFpCb, NULL);
}

void ContinueRegisterCb(MultiTimer* timer, void* userData)
{
    if (FpmAckMgr.Status == GotACK) {
        timeoutCnt = 0;
        if (FpmAckMgr.ErrorCode == Error_NoFinger) {
            PRINT("finger moved start register\n")
            StartRegisterFp(FpRegisterMgr.EnrollSuccessTimes + 1);
        } else {
            PRINT("Detect finger move or not\n");
            ContinueRegisterFp(FpRegisterMgr.EnrollSuccessTimes);
        }
    } else {
        timeoutCnt++;
        if (timeoutCnt >= 2) {
            timeoutCnt = 0;
            PRINT("timeoutCnt 3 RESTART\n");
            ContinueRegisterFp(FpRegisterMgr.EnrollSuccessTimes);
        } else {
            PRINT("get image timeout\n");
            MultiTimerStart(&comTimer, 100, ContinueRegisterCb, NULL);
        }
    }
}
void ContinueRegisterFp(uint8_t enrollTimes)
{
    FpRegisterMgr.EnrollSuccessTimes = enrollTimes;
    FpmAckMgr.Status = WaitACK;
    FPM_SendGetEnrollImageCmd();
    MultiTimerStart(&comTimer, 200, ContinueRegisterCb, NULL);
}
uint8_t storeUserID;
void FpRegStroeResult(uint8_t result);

/**
 * @brief 存储模板指令回调函数
 * 
 * @param timer 
 * @param userData 
 */
void StoreCharCb(MultiTimer* timer, void* userData)
{
    if (FpmAckMgr.Status == GotACK) {
        timeoutCnt = 0;
        FpRegStroeResult(FpmAckMgr.ErrorCode);
        if (FpmAckMgr.ErrorCode == Error_NONE) {
            PRINT("stroe char success\n");
        } else {
            PRINT("stroe char FAILE\n");
            PRINT("ERROR CODE =%d\n", FpmAckMgr.ErrorCode);
        }
    } else {
        timeoutCnt++;
        if (timeoutCnt >= 2) {
            timeoutCnt = 0;
            PRINT("timeoutCnt 3 RESTART\n");
            FpRegStroeResult(0xff);
        } else {
            PRINT("StoreFpUser timeout\n");
            MultiTimerStart(&comTimer, 100, StoreCharCb, NULL);
        }
    }
}

/**
 * @brief 存储模板
 * 
 * @param userID 
 */
void StoreFpUser(uint8_t userID)
{
    storeUserID = userID;
    FpmAckMgr.Status = WaitACK;
    FPM_SendStoreCharCmd(0x01, storeUserID);    // 存储模板指令
    MultiTimerStart(&comTimer, 200, StoreCharCb, NULL);
}

/**
 * @brief 合并模板指令回调函数
 * 
 * @param timer 
 * @param userData 
 */
void SendRegModelCb(MultiTimer* timer, void* userData)
{
    if (FpmAckMgr.Status == GotACK) {
        timeoutCnt = 0;
        FpRegStroeResult(FpmAckMgr.ErrorCode);
        if (FpmAckMgr.ErrorCode == Error_NONE) {
            PRINT("reg model success\n");
            StoreFpUser(storeUserID);   // 存储模板
        } else {
            PRINT("reg model FAILE\n");
            PRINT("ERROR CODE =%d\n", FpmAckMgr.ErrorCode);
        }
    } else {
        timeoutCnt++;
        if (timeoutCnt >= 2) {
            timeoutCnt = 0;
            PRINT("timeoutCnt 3 RESTART\n");
            FpRegStroeResult(0xff);
        } else {
            PRINT("SendRegModelCb timeout\n");
            MultiTimerStart(&comTimer, 100, SendRegModelCb, NULL);
        }
    }
}

void StoreFp(uint8_t userID)
{
    storeUserID = userID;
    FpmAckMgr.Status = WaitACK;
    FPM_SendRegModelCmd();  // 合并模板指令
    MultiTimerStart(&comTimer, 200, SendRegModelCb, NULL);
}

void StopRegisterFp()
{
    MultiTimerStop(&comTimer);
}
void DelFpmCb(MultiTimer* timer, void* userData)
{
    if (FpmAckMgr.Status == GotACK) {
        PRINT("clr success!\n");
    } else {
        PRINT("clr TIME OUT!\n");
        timeoutCnt++;
        if (timeoutCnt >= 2) {
            timeoutCnt = 0;
            PRINT("timeoutCnt 3 REtry del \n");
            DelAllUserFormFpm();
        } else {
            PRINT("get image timeout\n");
            MultiTimerStart(&comTimer, 100, DelFpmCb, NULL);
        }
    }
}
void DelAllUserFormFpm()
{
    timeoutCnt = 0;
    FpmAckMgr.Status = WaitACK;
    FPM_DeleteAllCharCmd();
    MultiTimerStart(&comTimer, 200, DelFpmCb, NULL);
}
