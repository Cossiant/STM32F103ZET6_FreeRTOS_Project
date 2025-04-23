/**
 * @file    mytask.c
 * @author  Cossiant
 * @version V1.3.1
 * @date    03-March-2025
 * @brief   FreeRTOS任务核心实现
 *
 * @verbatim
 * ====================================================================
 * 系统功能:
 *  - 串口DMA通信任务 (优先级: osPriorityHigh)
 *  - LCD显示刷新任务 (优先级: osPriorityNormal)
 *  - LED控制状态机 (优先级: osPriorityLow)
 *
 * 硬件依赖:
 *  - USART1 (PA9/PA10)
 *  - FSMC Bank1 NE4 (PG12)
 *  - LED0 (PB05), LED1 (PE04)
 *  - A49881 步进电机驱动（未来将会得到支持和应用）
 *
 * 修改记录:
 * 2025-02-28 V1.0.0  完成仓库初始化及FreeRTOS初始配置
 * 2025-03-01 V1.1.0  完成串口发送任务学习，使其能够合理使用FreeRTOS通过串口发送数据
 * 2025-03-02 V1.2.0  完成DMA串口接收及其点亮LED任务
 * 2025-03-03 V1.3.1  开源至github，并完成LCD点亮工作，并使其能够正常显示通过串口发送过来的数据
 * 2025-03-05 V1.3.2  重构LED控制代码，实现命令处理和LED控制分离
 * 2025-03-20 V1.4.0  重构数据结构，使数据能够被多个函数调用和修改，并且还可以跨C文件调用。并且新增加时间显示功能
 * 2025-03-21 V1.4.1  继续完成重构，将串口打印功能独立到myprintf.c当中
 * 2025-03-21 v1.5.0  新增加蜂鸣器响应，现在可以通过串口发送命令使蜂鸣器响相对应时间
 * 2025-04-25 v2.0.0  新增红外遥控控制，并且完全重构数据结构和部分实现代码，现在整个项目可以准备接入电机输出PWM控制了
 * 已经开启PWM-TIM8
 * ====================================================================
 * @endverbatim
 ************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "gpio.h"

#include "stdio.h"
#include "stdarg.h"
#include "string.h"
#include "ctype.h"

#include "lcd.h"
#include "delay.h"
#include "mytask.h"

extern osSemaphoreId_t LCD_refresh_gsemHandle;
extern osSemaphoreId_t uart1_data_handle_gsemHandle;

/**
 * @brief   多协议指令处理中枢（100ms轮询式）
 * @param   argument: 系统数据聚合指针
 * @retval  None
 * @note    支持指令类型：
 *          +----------------+----------------------+------------------------+
 *          | 指令格式        | 功能                 | 参数要求               |
 *          +----------------+----------------------+------------------------+
 *          | LED_AUTO       | 启用LED自动模式       | 无参数                 |
 *          | LED_ON/OFF     | 强制LED开关          | 无参数                 |
 *          | BEEP_ON[time]  | 蜂鸣器定时鸣叫       | 时间参数(单位：ms)     |
 *          | BEEP_OFF       | 立即关闭蜂鸣器       | 无参数                 |
 *          +----------------+----------------------+------------------------+
 *
 * @warning 安全机制：
 *          - 指令缓冲区强制清零：防止残留指令误触发
 *          - 参数范围校验：蜂鸣器时间限制在1-1000ms
 *          - 互斥访问：Response_Read_data为临界资源，处理完成自动清零
 *
 */
void StartLEDProcessedTaskFunction(void *argument)
{
    SYS_USE_DATA *SYS = (SYS_USE_DATA *)argument;
    /* 指令处理主循环 */
    for (;;) {
        osSemaphoreAcquire(uart1_data_handle_gsemHandle, osWaitForever);
        // ==================== LED指令处理 ====================
        if (strcmp(SYS->usart_use_data.Response_Read_data, "LED_AUTO") == 0) {
            myprintf("Now LED AUTO");
            SYS->led_control_num.Led_num = LED_AUTO; // 更新全局状态机
        } else if (strcmp(SYS->usart_use_data.Response_Read_data, "LED_OFF") == 0) {
            myprintf("Now LED OFF");
            SYS->led_control_num.Led_num = LED_OFF;
        } else if (strcmp(SYS->usart_use_data.Response_Read_data, "LED_ON") == 0) {
            myprintf("Now LED ON");
            SYS->led_control_num.Led_num = LED_ON;
        }
        // ==================== 蜂鸣器指令处理 ====================
        if (strncmp(SYS->usart_use_data.Response_Read_data, "BEEP_ON", 7) == 0) {
            myprintf("Now BEEP ON");
            // 定义一个临时接收变量
            char param_str[UART1_DMA_RX_LEN - 7];
            // 将剩余字符读出来到这个接受变量
            strcpy(param_str, SYS->usart_use_data.Response_Read_data + 7);
            // 定义读出来的数字变量
            unsigned int read_data_num = 0;
            // 将读出来的字符数字送给这个数字变量
            sscanf(param_str, "%u", &read_data_num);
            // 将其送给系统变量，以供调用
            SYS->Beep_control.Beep_control_num = BEEP_AUTO;
            SYS->Beep_control.Beep_delay_num   = read_data_num;
        } else if (strncmp(SYS->usart_use_data.Response_Read_data, "BEEP_OFF", 8) == 0) {
            myprintf("Now BEEP OFF");
            SYS->Beep_control.Beep_control_num = BEEP_OFF;
            SYS->Beep_control.Beep_delay_num   = 0;
        }
        /* 指令缓冲区安全擦除 */
        memset(SYS->usart_use_data.Response_Read_data, 0, UART1_DMA_RX_LEN);
        osDelay(100); // 指令轮询间隔
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief   LCD显示刷新任务（核心GUI引擎）
 * @param   argument: 时间数据结构体指针
 * @retval  None
 * @note    多层级显示架构：
 *          --------------------------------
 *          | 层级 | 内容          | 刷新频率 |
 *          |------|---------------|---------|
 *          | L0   | 时间显示       | 1Hz     |
 *          | L1   | 设备信息       | 静态     |
 *          | L2   | 串口数据       | 事件触发 |
 *
 * @warning 注意以下内存风险：
 *          - lcd_id缓冲区仅12字节，确保sprintf不溢出
 *          - SYS->usart_use_data.Read_data需外部保证非NULL且'\0'结尾
 *
 * 硬件依赖:
 * - FSMC接口: Bank1 NE1 (PD7)
 * - 背光控制: PB15 (PWM调光支持)
 *
 * 性能参数:
 * - 单次全刷新耗时: ~25ms (320x240@16bit)
 * - 局部刷新耗时: ~5ms (时间区域)
 */
void StartLCDDisplayTaskFunction(void *argument)
{
    SYS_USE_DATA *SYS = (SYS_USE_DATA *)argument;
    uint8_t lcd_id[12]; // LCD ID显示缓冲（注意大小限制！）

    /* 硬件初始化链 */
    delay_init(72);   // 精准延时（基于SysTick）
    lcd_init();       // ILI9341驱动初始化
    lcd_clear(WHITE); // 清屏操作（防止残影）

    // 获取LCD硬件ID（关键诊断信息）
    sprintf((char *)lcd_id, "LCD ID:%04X", lcddev.id);

    /* 主刷新循环 */
    for (;;) {
        // 等待刷新信号量（最大等待时间可配置）
        osSemaphoreAcquire(LCD_refresh_gsemHandle, osWaitForever);

        /* ----- 时间显示区域（L0层）----- */
        lcd_show_xnum(10, 10, SYS->Time_use_data.hours, 2, 24, 0x80, BLACK);  // 小时
        lcd_show_string(34, 10, 240, 32, 24, ":", BLACK);                     // 冒号
        lcd_show_xnum(46, 10, SYS->Time_use_data.minute, 2, 24, 0x80, BLACK); // 分钟
        lcd_show_string(70, 10, 240, 32, 24, ":", BLACK);
        lcd_show_xnum(82, 10, SYS->Time_use_data.second, 2, 24, 0x80, BLACK); // 秒

        /* ----- 设备信息区域（L1层）----- */
        lcd_show_string(10, 40, 240, 32, 32, "STM32F103ZET6", RED);  // 主控型号
        lcd_show_string(10, 80, 240, 24, 24, "TFTLCD TEST", RED);    // 测试标识
        lcd_show_string(10, 110, 240, 16, 16, "User:Cossiant", RED); // 用户信息
        lcd_show_string(10, 130, 240, 16, 16, (char *)lcd_id, RED);  // LCD ID

        /* ----- 动态数据区域（L2层）----- */
        lcd_show_string(10, 150, 240, 16, 16, "UART read data is :", BLACK);
        lcd_show_string(10, 190, 240, 16, 16, "Beep read data is :", BLACK);

        if (strcmp(SYS->usart_use_data.Read_data, SYS->usart_use_data.Last_Read_data) != 0) {
            // 将上次接受USART的数据改为当前数据
            // 这样可以阻止当数据相同时的再次刷新，节约CPU资源
            strcpy(SYS->usart_use_data.Last_Read_data, SYS->usart_use_data.Read_data);

            lcd_show_string(10, 170, 240, 16, 16, "              ", BLACK);                      // 清空当前行显示
            lcd_show_string(10, 170, 240, 16, 16, (char *)SYS->usart_use_data.Read_data, BLACK); // 串口数据

            lcd_show_string(10, 210, 240, 16, 16, "              ", BLACK);               // 清空当前行显示
            lcd_show_xnum(10, 210, SYS->Beep_control.Beep_delay_num, 4, 16, 0X80, BLACK); // 读取出来的Beep延时数据（单位ms）
        }
        
        // 这里建议也改成检查当前按键按下时长和上次按下时长检测
        // 以此降低CPU占用率
        lcd_show_num(10, 230, SYS->Remote_use_data.key, 3, 16, BLUE);          /* 显示键值 */
        lcd_show_num(10, 250, SYS->Remote_use_data.g_remote_cnt, 3, 16, BLUE); /* 显示按键次数 */
        lcd_fill(10, 270, 116 + 8 * 8, 170 + 16, WHITE);                       /* 清楚之前的显示 */
        lcd_show_string(10, 270, 200, 16, 16, SYS->Remote_use_data.str, BLUE); /* 显示SYMBOL */

        // 调试输出（建议使用条件编译控制）
        // myprintf("LCD refresh data is :%s", SYS->usart_use_data.Read_data);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief   系统时间维护任务（RTC模拟层）
 * @param   argument: 时间数据结构体指针
 * @retval  None
 * @note    实现以下功能：
 *          - 提供软件级RTC功能（精度依赖系统节拍）
 *          - 时间递增逻辑（23:59:50 → 00:00:00）
 *          - 触发LCD刷新信号
 *
 * 关键逻辑:
 * 1. 初始化时间为23:59:50（便于测试跨日场景）
 * 2. 每1000个系统节拍（1秒）触发时间递增
 * 3. 通过信号量同步LCD刷新
 *
 * @warning 注意：
 * - 需确保与LCD任务的优先级关系（建议本任务优先级更高）
 * - 长时间运行可能存在累积误差（建议定期同步）
 *
 * 改进建议:
 * 可添加RTC硬件同步接口（备份寄存器存取）
 */
void StartTimeSetTaskFunction(void *argument)
{
    SYS_USE_DATA *SYS = (SYS_USE_DATA *)argument;

    /* 初始化模拟RTC */
    // 修改初始参数请前往mytask.h
    SYS->Time_use_data.hours  = Set_Time_hours;  // 初始小时（测试跨日场景）
    SYS->Time_use_data.minute = Set_Time_minute; // 初始分钟
    SYS->Time_use_data.second = Set_Time_second; // 初始秒数

    /* 时间维护主循环 */
    for (;;) {
        // 秒递增（原子操作建议）
        SYS->Time_use_data.second++;

        /* 时间进位逻辑链 */
        if (SYS->Time_use_data.second >= 60) { // 分钟进位
            SYS->Time_use_data.second = 0;
            SYS->Time_use_data.minute++;

            if (SYS->Time_use_data.minute >= 60) { // 小时进位
                SYS->Time_use_data.minute = 0;
                SYS->Time_use_data.hours++;

                if (SYS->Time_use_data.hours >= 24) { // 日进位
                    SYS->Time_use_data.hours = 0;
                }
            }
        }

        /* 触发LCD刷新 */
        osSemaphoreRelease(LCD_refresh_gsemHandle); // 释放信号量

        // 精确延时（误差<1个系统节拍）
        osDelay(1000); // 1000 ticks = 1秒（假设configTICK_RATE_HZ=1000）
    }
}
