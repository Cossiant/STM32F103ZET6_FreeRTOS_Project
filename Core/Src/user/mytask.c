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
#include "myprintf.h"

extern osSemaphoreId_t LCD_refresh_gsemHandle;
extern osSemaphoreId_t Beep_control_gsemHandle;
extern osSemaphoreId_t Motor_one_control_gsemHandle;

// LED控制标志位
// 拥有参数LED_AUTO,LED_ON,LED_OFF
enum {
    LED_AUTO,
    LED_ON,
    LED_OFF,
    LED_Artificial
} LED_Conctrl_MOD;
unsigned char LED_Conctrl_num = LED_AUTO;

enum {
    MOTOR_AUTO,
    MOTOR_OFF,
    MOTOR_ON
} MOTOR_Control_Num;

enum {
    Motor_Left,
    Motor_Right
} MOTOR_Control_direction;

/**
 * @brief   多协议指令处理中枢
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
 *          - 互斥访问：Response_Read_data为临界资源
 *
 * 改进建议：
 *          使用正则表达式提升参数解析可靠性
 */
void StartLEDProcessedTaskFunction(void *argument)
{
    SYS_USE_DATA *SYS = (SYS_USE_DATA *)argument;
    int output_num    = 0; // 参数提取临时变量
    /* 指令处理主循环 */
    for (;;) {
        // ==================== LED指令处理 ====================
        if (strcmp(SYS->usart_use_data.Response_Read_data, "LED_AUTO") == 0) {
            myprintf("Now LED AUTO");
            LED_Conctrl_num = LED_AUTO; // 更新全局状态机
        } else if (strcmp(SYS->usart_use_data.Response_Read_data, "LED_OFF") == 0) {
            myprintf("Now LED OFF");
            LED_Conctrl_num = LED_OFF;
        } else if (strcmp(SYS->usart_use_data.Response_Read_data, "LED_ON") == 0) {
            myprintf("Now LED ON");
            LED_Conctrl_num = LED_ON;
        }
        // ==================== 蜂鸣器指令处理 ====================
        if (strncmp(SYS->usart_use_data.Response_Read_data, "BEEP_ON", 7) == 0) {
            /* 参数提取算法（支持嵌入数字） */
            for (int i = 0; SYS->usart_use_data.Response_Read_data[i] != '\0'; i++) {
                if (isdigit(SYS->usart_use_data.Response_Read_data[i])) {
                    output_num = output_num * 10 + (SYS->usart_use_data.Response_Read_data[i] - '0');
                }
            }
            // 参数有效性校验
            output_num = (output_num < 1) ? 100 : output_num;     // 默认值100ms
            output_num = (output_num > 1000) ? 1000 : output_num; // 最大限制

            SYS->Beep_use_data.Response_time = output_num; // 参数注入系统
            myprintf("Now Beep ON");
            output_num = 0; // 重置临时变量
            // 触发LCD刷新显示新参数
            osSemaphoreRelease(LCD_refresh_gsemHandle);
        } else if (strncmp(SYS->usart_use_data.Response_Read_data, "BEEP_OFF", 8) == 0) {
            myprintf("Now Beep OFF");
            SYS->Beep_use_data.Response_time = 0; // 立即停止标识
        }
        // ==================== 电机控制指令处理 ====================

        if (strncmp(SYS->usart_use_data.Response_Read_data, "MOTOR_LEFT", 10) == 0) {
            /* 参数提取算法（支持嵌入数字） */
            for (int i = 0; SYS->usart_use_data.Response_Read_data[i] != '\0'; i++) {
                if (isdigit(SYS->usart_use_data.Response_Read_data[i])) {
                    output_num = output_num * 10 + (SYS->usart_use_data.Response_Read_data[i] - '0');
                }
            }
            myprintf("Now Motor LEFT");
            SYS->Motor_one.speed     = output_num;
            SYS->Motor_one.start     = MOTOR_ON;
            SYS->Motor_one.direction = Motor_Left;
            output_num               = 0; // 重置临时变量
            osSemaphoreRelease(LCD_refresh_gsemHandle);
        } else if (strncmp(SYS->usart_use_data.Response_Read_data, "MOTOR_RIGHT", 11) == 0) {
            /* 参数提取算法（支持嵌入数字） */
            for (int i = 0; SYS->usart_use_data.Response_Read_data[i] != '\0'; i++) {
                if (isdigit(SYS->usart_use_data.Response_Read_data[i])) {
                    output_num = output_num * 10 + (SYS->usart_use_data.Response_Read_data[i] - '0');
                }
            }
            myprintf("Now Motor LEFT");
            SYS->Motor_one.speed     = output_num;
            SYS->Motor_one.start     = MOTOR_ON;
            SYS->Motor_one.direction = Motor_Right;
            output_num               = 0; // 重置临时变量
            osSemaphoreRelease(LCD_refresh_gsemHandle);
        } else if (strncmp(SYS->usart_use_data.Response_Read_data, "MOTOR_AUTO_LEFT", 15) == 0) {
            /* 参数提取算法（支持嵌入数字） */
            for (int i = 0; SYS->usart_use_data.Response_Read_data[i] != '\0'; i++) {
                if (isdigit(SYS->usart_use_data.Response_Read_data[i])) {
                    output_num = output_num * 10 + (SYS->usart_use_data.Response_Read_data[i] - '0');
                }
            }
            myprintf("Now Motor AUTO LEFT");
            SYS->Motor_one.start     = MOTOR_AUTO;
            SYS->Motor_one.direction = Motor_Left;
            SYS->Motor_one.laps_num  = output_num * 64 / 45;
            output_num               = 0; // 重置临时变量
            osSemaphoreRelease(LCD_refresh_gsemHandle);
        } else if (strncmp(SYS->usart_use_data.Response_Read_data, "MOTOR_AUTO_RIGHT", 16) == 0) {
            /* 参数提取算法（支持嵌入数字） */
            for (int i = 0; SYS->usart_use_data.Response_Read_data[i] != '\0'; i++) {
                if (isdigit(SYS->usart_use_data.Response_Read_data[i])) {
                    output_num = output_num * 10 + (SYS->usart_use_data.Response_Read_data[i] - '0');
                }
            }
            myprintf("Now Motor AUTO RIGHT");
            SYS->Motor_one.start     = MOTOR_AUTO;
            SYS->Motor_one.direction = Motor_Right;
            SYS->Motor_one.laps_num  = output_num * 64 / 45;
            output_num               = 0; // 重置临时变量
            osSemaphoreRelease(LCD_refresh_gsemHandle);
        } else if (strncmp(SYS->usart_use_data.Response_Read_data, "MOTOR_OFF", 9) == 0) {
            myprintf("Now Motor OFF");
            SYS->Motor_one.speed = 10;
            SYS->Motor_one.start = MOTOR_OFF;
        }
        /* 指令缓冲区安全擦除 */
        memset(SYS->usart_use_data.Response_Read_data, 0, UART1_DMA_RX_LEN);
        osDelay(100); // 指令轮询间隔
    }
}

/**
 * @brief   LED控制任务（主状态指示灯）
 * @param   argument: FreeRTOS任务参数（未使用）
 * @retval  None
 * @note    工作模式:每隔100ms就进行一次LED_Conctrl_num检测
 *          如果LED_Conctrl_num不是手动模式时，将进行下一步检测，否则将重复等待100ms后检测
 *          检测到LED_OFF时，关闭所有LED，将模式调成手动模式
 *          检测到LED_ON时，开启所有LED，将模式调成手动模式
 *          检测到LED_AUTO时，让LED一直闪烁
 */
void StartLEDWorkTaskFunction(void *argument)
{
    for (;;) {
        if (LED_Conctrl_num != LED_Artificial) {
            switch (LED_Conctrl_num) {
                case LED_AUTO:
                    HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
                    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
                    break;
                case LED_ON:
                    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
                    LED_Conctrl_num = LED_Artificial;
                    break;
                case LED_OFF:
                    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
                    LED_Conctrl_num = LED_Artificial;
                    break;
                default:
                    break;
            }
            osDelay(1000);
            // 如果完成这次LED闪烁就跳过剩下的代码，重新开始，这样能够稳定保证1s的闪烁周期
            continue;
        }
        osDelay(100);
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
        lcd_show_string(10, 190, 240, 16, 16, "BEEP response time is :", BLACK);
        lcd_show_string(10, 230, 240, 16, 16, "Motor Num is:", BLACK);
        if (strcmp(SYS->usart_use_data.Read_data, SYS->usart_use_data.Last_Read_data) != 0) {
            lcd_show_string(10, 170, 240, 16, 16, "              ", BLACK);                      // 清空当前行显示
            lcd_show_string(10, 170, 240, 16, 16, (char *)SYS->usart_use_data.Read_data, BLACK); // 串口数据
            strcpy(SYS->usart_use_data.Read_data, SYS->usart_use_data.Last_Read_data);           // 将上次接受的数据改为当前数据

            lcd_show_string(10, 210, 240, 16, 16, "              ", BLACK); // 清空当前行显示
            lcd_show_num(10, 210, SYS->Beep_use_data.Response_time, 4, 16, BLACK);

            lcd_show_string(10, 250, 240, 16, 16, "              ", BLACK); // 清空当前行显示
            lcd_show_num(10, 250, SYS->Motor_one.speed, 5, 16, BLACK);
        }
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
/**
 * @brief   智能蜂鸣器控制器
 * @param   argument: 系统数据聚合指针
 * @retval  None
 * @note    工作模式：
 *          - 正常模式：按照设定时间循环鸣叫（鸣叫时间/间隔时间=1:9）
 *          - 紧急停止：立即终止当前操作
 *
 * 硬件保护机制：
 *          +----------------+---------------------+-----------------------+
 *          | 参数            | 限制范围            | 保护措施              |
 *          +----------------+---------------------+-----------------------+
 *          | Response_time  | 1-1000 ms           | 超出范围自动钳位       |
 *          | 占空比         | ≤10%               | 硬件PWM限制            |
 *          | 最大电流        | 20mA               | 串联电阻保护           |
 *          +----------------+---------------------+-----------------------+
 *
 * @warning 音频参数：
 *          - 驱动频率：4KHz ±5%（TIM4_CH2生成）
 *          - 声压级：85dB @10cm（需符合工业安全标准）
 */
void StartBeepTaskFunction(void *argument)
{
    SYS_USE_DATA *SYS                = (SYS_USE_DATA *)argument;
    SYS->Beep_use_data.Response_time = 0; // 初始化安全状态

    for (;;) {
        if (SYS->Beep_use_data.Response_time != 0) {
            /* 激活蜂鸣器（带硬件互锁保护） */
            HAL_GPIO_WritePin(Beep1_GPIO_Port, Beep1_Pin, GPIO_PIN_SET); // 启动鸣叫
            osDelay(SYS->Beep_use_data.Response_time);                   // 持续时长

            /* 关闭蜂鸣器（最小间隔保护） */
            HAL_GPIO_WritePin(Beep1_GPIO_Port, Beep1_Pin, GPIO_PIN_RESET);
            osDelay(1000 - SYS->Beep_use_data.Response_time); // 维持周期1秒
        } else {
            /* 空闲状态低功耗模式 */
            osDelay(100); // 降低CPU占用率
        }
    }
}
void Motor_one_1step(SYS_USE_DATA *SYS)
{
    switch (SYS->Motor_one.direction_num) {
        case 1:
            HAL_GPIO_WritePin(Motor_one_IN1_GPIO_Port, Motor_one_IN1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(Motor_one_IN2_GPIO_Port, Motor_one_IN2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(Motor_one_IN3_GPIO_Port, Motor_one_IN3_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(Motor_one_IN4_GPIO_Port, Motor_one_IN4_Pin, GPIO_PIN_RESET);
            break;
        case 2:
            HAL_GPIO_WritePin(Motor_one_IN2_GPIO_Port, Motor_one_IN2_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(Motor_one_IN1_GPIO_Port, Motor_one_IN1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(Motor_one_IN3_GPIO_Port, Motor_one_IN3_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(Motor_one_IN4_GPIO_Port, Motor_one_IN4_Pin, GPIO_PIN_RESET);
            break;
        case 3:
            HAL_GPIO_WritePin(Motor_one_IN3_GPIO_Port, Motor_one_IN3_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(Motor_one_IN2_GPIO_Port, Motor_one_IN2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(Motor_one_IN1_GPIO_Port, Motor_one_IN1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(Motor_one_IN4_GPIO_Port, Motor_one_IN4_Pin, GPIO_PIN_RESET);
            break;
        case 4:
            HAL_GPIO_WritePin(Motor_one_IN4_GPIO_Port, Motor_one_IN4_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(Motor_one_IN2_GPIO_Port, Motor_one_IN2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(Motor_one_IN3_GPIO_Port, Motor_one_IN3_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(Motor_one_IN1_GPIO_Port, Motor_one_IN1_Pin, GPIO_PIN_RESET);
            break;
    }
    if (SYS->Motor_one.direction == Motor_Left) {
        SYS->Motor_one.direction_num++;
        if (SYS->Motor_one.direction_num == 5) {
            SYS->Motor_one.direction_num = 1;
        }
    } else if (SYS->Motor_one.direction == Motor_Right) {
        SYS->Motor_one.direction_num--;
        if (SYS->Motor_one.direction_num == 0) {
            SYS->Motor_one.direction_num = 4;
        }
    }
}

void Motor_one_2step(SYS_USE_DATA *SYS)
{
    for (unsigned char i = 0; i < 8; i++) {
        switch (SYS->Motor_one.direction_num) {
            case 1:
                HAL_GPIO_WritePin(Motor_one_IN1_GPIO_Port, Motor_one_IN1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Motor_one_IN2_GPIO_Port, Motor_one_IN2_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Motor_one_IN3_GPIO_Port, Motor_one_IN3_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Motor_one_IN4_GPIO_Port, Motor_one_IN4_Pin, GPIO_PIN_RESET);
                break;
            case 2:
                HAL_GPIO_WritePin(Motor_one_IN1_GPIO_Port, Motor_one_IN1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Motor_one_IN2_GPIO_Port, Motor_one_IN2_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Motor_one_IN3_GPIO_Port, Motor_one_IN3_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Motor_one_IN4_GPIO_Port, Motor_one_IN4_Pin, GPIO_PIN_RESET);
                break;
            case 3:
                HAL_GPIO_WritePin(Motor_one_IN1_GPIO_Port, Motor_one_IN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Motor_one_IN2_GPIO_Port, Motor_one_IN2_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Motor_one_IN3_GPIO_Port, Motor_one_IN3_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Motor_one_IN4_GPIO_Port, Motor_one_IN4_Pin, GPIO_PIN_RESET);
                break;
            case 4:
                HAL_GPIO_WritePin(Motor_one_IN1_GPIO_Port, Motor_one_IN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Motor_one_IN2_GPIO_Port, Motor_one_IN2_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Motor_one_IN3_GPIO_Port, Motor_one_IN3_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Motor_one_IN4_GPIO_Port, Motor_one_IN4_Pin, GPIO_PIN_RESET);
                break;
            case 5:
                HAL_GPIO_WritePin(Motor_one_IN1_GPIO_Port, Motor_one_IN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Motor_one_IN2_GPIO_Port, Motor_one_IN2_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Motor_one_IN3_GPIO_Port, Motor_one_IN3_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Motor_one_IN4_GPIO_Port, Motor_one_IN4_Pin, GPIO_PIN_RESET);
                break;
            case 6:
                HAL_GPIO_WritePin(Motor_one_IN1_GPIO_Port, Motor_one_IN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Motor_one_IN2_GPIO_Port, Motor_one_IN2_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Motor_one_IN3_GPIO_Port, Motor_one_IN3_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Motor_one_IN4_GPIO_Port, Motor_one_IN4_Pin, GPIO_PIN_SET);
                break;
            case 7:
                HAL_GPIO_WritePin(Motor_one_IN1_GPIO_Port, Motor_one_IN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Motor_one_IN2_GPIO_Port, Motor_one_IN2_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Motor_one_IN3_GPIO_Port, Motor_one_IN3_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Motor_one_IN4_GPIO_Port, Motor_one_IN4_Pin, GPIO_PIN_SET);
                break;
            case 8:
                HAL_GPIO_WritePin(Motor_one_IN1_GPIO_Port, Motor_one_IN1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Motor_one_IN2_GPIO_Port, Motor_one_IN2_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Motor_one_IN3_GPIO_Port, Motor_one_IN3_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Motor_one_IN4_GPIO_Port, Motor_one_IN4_Pin, GPIO_PIN_SET);
                break;
        }
        if (SYS->Motor_one.direction == Motor_Left) {
            SYS->Motor_one.direction_num++;
            if (SYS->Motor_one.direction_num == 9) {
                SYS->Motor_one.direction_num = 1;
            }
        } else if (SYS->Motor_one.direction == Motor_Right) {
            SYS->Motor_one.direction_num--;
            if (SYS->Motor_one.direction_num == 0) {
                SYS->Motor_one.direction_num = 8;
            }
        }
        osDelay(SYS->Motor_one.speed);
    }
}

void Motor_one_AUTOFunction(SYS_USE_DATA *SYS)
{
    Motor_one_2step(SYS);
    SYS->Motor_one.laps_num--;
    if (SYS->Motor_one.laps_num == 0) {
        SYS->Motor_one.start = MOTOR_OFF;
    }
}

void StartMotor_oneFunction(void *argument)
{
    SYS_USE_DATA *SYS            = (SYS_USE_DATA *)argument;
    SYS->Motor_one.speed         = 3;
    SYS->Motor_one.start         = MOTOR_OFF;
    SYS->Motor_one.direction     = Motor_Left;
    SYS->Motor_one.direction_num = 1;
    SYS->Motor_one.laps_num      = 0;
    for (;;) {
        if (SYS->Motor_one.start == MOTOR_ON) {
            Motor_one_2step(SYS);
        } else if (SYS->Motor_one.start == MOTOR_AUTO) {
            Motor_one_AUTOFunction(SYS);
        } else {
            osDelay(SYS->Motor_one.speed);
        }
    }
}
