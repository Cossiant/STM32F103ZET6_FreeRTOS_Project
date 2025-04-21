#ifndef __MY_SYS_DATA_H
#define __MY_SYS_DATA_H

#include "led.h"
#include "myprintf.h"
#include "beep.h"

#define Set_Time_hours   0
#define Set_Time_minute  0
#define Set_Time_second  0
// 设置当前时间结构体，实现LCD动态显示时间
typedef struct
{
    unsigned char hours;
    unsigned char minute;
    unsigned char second;
} TIME_USE_DATA;

/**
 * @brief  系统核心数据聚合结构体
 * @warning 多任务共享数据需原子操作保护
 */
typedef struct {
    TIME_USE_DATA Time_use_data;   //!< 时间管理系统（RTC模拟层）
    USART_USE_DATA usart_use_data; //!< 通信数据管理系统
    LED_USE_DATA led_control_num;
    BEEP_USE_DATA Beep_control;
} SYS_USE_DATA;

#endif
