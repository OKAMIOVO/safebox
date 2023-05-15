#include "CMS32L051.h" // Device header
#include "userdefine.h"
#include "tim4.h"
#include "gpio.h"
#include "Multitimer.h"
#include <stdio.h>
#include "motor.h"
#include "log.h"
#define MOTOR_STOP 0
#define MOTOR_UNLOCK 1
#define MOTOR_PAUSE 2
#define MOTOR_LOCK 3
#define PERIOD (16 * 1000)
#define PERCENT (PERIOD / 100)
#define START_PWM (0)
void MotorInit(void);
void MotorSleep(void);
struct Device motor = { NULL, MotorInit, NULL };
void SetPwm(int pwm)
{
    if (pwm == 0) {
        // TM41_Channel_Stop(TM4_CHANNEL_0 | TM4_CHANNEL_1 | TM4_CHANNEL_2);
        TM41->TDR11 = 0;
        TM41->TDR12 = 0;
        // PORT_ClrBit(PORT1,PIN1);
        // 	PORT_ClrBit(PORT2,PIN2);
    } else {
        int pwmA = 0, pwmB = 0;
        if (pwm > 100) {
            pwm = 100;
        } else if (pwm < -100) {
            pwm = -100;
        }
        if (pwm >= 0) {
            pwmA = pwm * PERCENT;
        } else {
            pwmB = -pwm * PERCENT;
        }
        if (pwmA != TM41->TDR11 || pwmB != TM41->TDR12) {
            TM41_Channel_Stop(TM4_CHANNEL_0 | TM4_CHANNEL_1 | TM4_CHANNEL_2);
            TM41->TDR11 = pwmA;
            TM41->TDR12 = pwmB;
            TM41_Channel_Start(TM4_CHANNEL_0 | TM4_CHANNEL_1 | TM4_CHANNEL_2);
        }
    }
}
MultiTimer motorTimer;
uint8_t motorState = MOTOR_STOP;
int pwmMotor = 0;
void MotorTimerHandler(MultiTimer* timer, void* userData)
{
    if (motorState == MOTOR_UNLOCK) {
        if (pwmMotor < 100) {
            pwmMotor++;
            if (pwmMotor >= 100) {
                PRINT("unlock \n");
                MultiTimerStart(&motorTimer, 3000, MotorTimerHandler, &motorState);
            } else {
                MultiTimerStart(&motorTimer, 10, MotorTimerHandler, &motorState);
            }
        } else {
            PRINT("pause\n");
            motorState = MOTOR_PAUSE;
            pwmMotor = 0;
            MultiTimerStart(&motorTimer, 3000, MotorTimerHandler, &motorState);
        }
    } else if (motorState == MOTOR_PAUSE) {
        PRINT("lock soft start\n");
        motorState = MOTOR_LOCK;
        pwmMotor = 0;
        MultiTimerStart(&motorTimer, 0, MotorTimerHandler, &motorState);
    } else if (motorState == MOTOR_LOCK) {
        if (pwmMotor > -100) {
            pwmMotor--;
            if (pwmMotor <= -100) {
                PRINT("lock\n");
                MultiTimerStart(&motorTimer, 3000, MotorTimerHandler, &motorState);
            } else {
                MultiTimerStart(&motorTimer, 10, MotorTimerHandler, &motorState);
            }
        } else {
            PRINT("stop\n");
            motorState = MOTOR_STOP;
            pwmMotor = 0;
        }
    }
    SetPwm(pwmMotor);
    MultiTimerStart(&deviceMgr.timer, SLEEP_TIME, SleepTimerCallBack, NULL);
}
void MotorInit()
{
    TM41_PWM_1Period_2Duty(PERIOD, 0, 0);
}

void StartUnlock()
{
    PRINT("unlock soft start\n");
    motorState = MOTOR_UNLOCK;
    pwmMotor = START_PWM;
    MultiTimerStart(&motorTimer, 0, MotorTimerHandler, &motorState);
    MultiTimerStart(&deviceMgr.timer, SLEEP_TIME, SleepTimerCallBack, NULL);
}
