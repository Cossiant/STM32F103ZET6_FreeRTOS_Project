#ifndef __MY_SYS_DATA_H
#define __MY_SYS_DATA_H

#include "led.h"
#include "myprintf.h"
#include "beep.h"
#include "remote.h"
#include "robot.h"

#define Set_Time_hours  0
#define Set_Time_minute 0
#define Set_Time_second 0
// 设置当前时间结构体，实现LCD动态显示时间
typedef struct
{
    unsigned char hours;
    unsigned char minute;
    unsigned char second;
} TIME_USE_DATA;

/**
 * @brief  系统核心数据聚合结构体
 * @warning 多任务共享数据需进行操作保护和上锁
 */
typedef struct {
    ROBOT_USE_TYPE Robot_use_data;   // 机器人机械臂使用数据结构
    TIME_USE_DATA Time_use_data;     // 时间管理系统（RTC模拟层）
    USART_USE_DATA usart_use_data;   // 通信数据管理系统
    LED_USE_DATA led_control_num;    // led控制参数
    BEEP_USE_DATA Beep_control;      // 蜂鸣器控制数据结构
    REMOTE_USE_DATA Remote_use_data; // 红外遥控器数据结构
} SYS_USE_DATA;

#endif
