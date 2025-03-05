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
 * ====================================================================
 * @endverbatim
 ************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "stdio.h"
#include "stdarg.h"
#include "string.h"
#include "gpio.h"
#include "lcd.h"
#include "delay.h"

#define UART1_DMA_RX_LEN 50

// 定义串口号
extern UART_HandleTypeDef huart1;
// 定义DMA接口
extern DMA_HandleTypeDef hdma_usart1_rx;
// 信号量
extern osSemaphoreId_t uart1_printf_gsemHandle;
extern osSemaphoreId_t uart1_rxok_gsemHandle;
extern osSemaphoreId_t LCD_refresh_gsemHandle;

// 要发送的数据
char gbuf_printf[UART1_DMA_RX_LEN];

// 接收到的数据
// 注意，本参数每次接收到新的数据都将被清空！
char Read_data[UART1_DMA_RX_LEN];

// 上一次接收到的数据
// 本参数接收到数据之后将被LED使用，使用之后将会被清空！
char LED_Read_data[UART1_DMA_RX_LEN];

// LED控制标志位
// 拥有参数LED_AUTO,LED_ON,LED_OFF
enum {
    LED_AUTO,
    LED_ON,
    LED_OFF,
    LED_Artificial
} LED_Conctrl_MOD;

unsigned char LED_Conctrl_num = LED_AUTO;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief   定制化串口格式化输出函数
 * @param   format: 格式化字符串（支持%d, %x, %s等标准格式）
 * @param   ...: 可变参数列表
 * @retval  None
 * @note    使用内部缓冲gbuf_printf（大小：UART1_DMA_RX_LEN）
 *          依赖HAL_UART_Transmit_DMA实现非阻塞发送
 *          线程安全设计（通过uart1_printf_gsemHandle互斥锁）
 * @warning 禁止在中断上下文中调用
 * @example myprintf("ADC Value: %d", adc_val);
 */
void myprintf(char *format, ...)
{
    // 创建可变参数列表类型变量ap
    va_list ap;
    // 获取信号量，如果信号量不可用就一直等待
    osSemaphoreAcquire(uart1_printf_gsemHandle, osWaitForever);
    // 初始化可变参数列表ap，让其指向format第一个参数
    va_start(ap, format);
    // 检查串口是否正在发送数据，如果是直接退出
    if (huart1.gState == HAL_UART_STATE_BUSY_TX) return;
    // 将格式化后的字符串存到gbuf_printf数组中
    vsprintf(gbuf_printf, format, ap);
    // 串口中断方式发送gbuf_printf中的数据
    HAL_UART_Transmit_IT(&huart1, (uint8_t *)gbuf_printf, strlen(gbuf_printf));
    // 释放ap的资源
    va_end(ap);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief   串口发送完成中断回调
 * @param   huart: 串口句柄指针
 * @retval  None
 * @note    释放串口发送互斥量
 *          当发送完成时触发LED闪烁指示
 * @warning 此函数在中断上下文中执行（保持简短）
 *
 * 关联全局变量:
 * - uart1_printf_gsemHandle : 串口发送互斥信号量
 * - tx_complete_flag : 发送完成标志位（bit0）
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        // 串口发送完成中断时，释放信号量
        osSemaphoreRelease(uart1_printf_gsemHandle);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief   串口数据接收处理引擎
 * @param   None
 * @retval  None
 * @note    当产生空闲中断就释放串口接收任务信号量
 *          随后进入串口接收任务（DMA模式）
 * @warning 这个函数需要将其放入中断文件(stm32f1xx_it.C)中，调用这个中断响应函数
 */
void uart1_data_in()
{
    if ((__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET)) {
        // 产生空闲中断就清除中断
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
        // 释放信号量
        osSemaphoreRelease(uart1_rxok_gsemHandle);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief  串口接收任务（DMA模式）
 * @param  argument: FreeRTOS任务参数指针
 * @retval None
 * @note   使用HAL_UART_Receive_DMA配合空闲中断实现
 *         环形缓冲区大小：UART1_DMA_RX_LEN(50)
 *         信号量：
 *           - uart1_rxok_gsemHandle : 数据接收完成信号
 *           - LCD_refresh_gsemHandle: LCD刷新触发信号
 * @warning 禁止在中断中调用本函数
 */
void StartUART1_recv_TaskFunction(void *argument)
{
    // now_dma_ip当前数据缓冲区数据长度
    // rd_dma_ip读到的数据的长度
    unsigned short now_dam_ip, rd_dma_ip = 0, Read_data_len;
    // 接受缓冲区rxBuffer，指令缓冲区Read_data
    char rxBuffer[UART1_DMA_RX_LEN];
    // 初始化DMA缓冲区
    memset(rxBuffer, 0, UART1_DMA_RX_LEN);
    // 打开串口空闲中断
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
    // 使能DMA接收功能
    HAL_UART_Receive_DMA(&huart1, (uint8_t *)rxBuffer, UART1_DMA_RX_LEN);

    for (;;) {
        // 接收到信号量才继续进行
        osSemaphoreAcquire(uart1_rxok_gsemHandle, osWaitForever);
        // 计算当前数据队列中已有的数据长度
        now_dam_ip = UART1_DMA_RX_LEN - (unsigned short int)__HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
        // 将读取到的数据的指针重置为0
        Read_data_len = 0;
        // 清空指令缓冲区
        memset(Read_data, 0, UART1_DMA_RX_LEN);
        // 如果读指针不等于当前数据指针
        while (rd_dma_ip != now_dam_ip) {
            // 将数据读取到Read_data当中
            Read_data[Read_data_len++] = rxBuffer[rd_dma_ip++];
            if (rd_dma_ip >= UART1_DMA_RX_LEN) rd_dma_ip = 0;
        }
        // 保存上次读取到的数据
        strcpy(LED_Read_data, Read_data);
        // 释放LCD刷新信号量
        osSemaphoreRelease(LCD_refresh_gsemHandle);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief   LED控制命令处理任务
 * @param   argument: FreeRTOS任务参数（未使用）
 * @retval  None
 * @note    工作模式:每隔100ms就进行一次LED_Read_data内容检测
 *          检测到LED_OFF时，串口发送Now LED OFF!并并将LED_Conctrl_num值改为LED_OFF
 *          检测到LED_ON时，串口发送Now LED ON!将LED_Conctrl_num值改为LED_ON
 *          检测到LED_AUTO时，串口发送Now LED AUTO!并将LED_Conctrl_num值改为LED_AUTO
 */
void StartLEDProcessedTaskFunction(void *argument)
{
    for (;;) {
        // 读取LED_Read_data进行检测后给LED_Conctrl_num赋值
        if (strcmp(LED_Read_data, "LED_AUTO") == 0) {
            myprintf("Now LED AUTO!");
            LED_Conctrl_num = LED_AUTO;
        }
        if (strcmp(LED_Read_data, "LED_OFF") == 0) {
            myprintf("Now LED OFF!");
            LED_Conctrl_num = LED_OFF;
        }
        if (strcmp(LED_Read_data, "LED_ON") == 0) {
            myprintf("Now LED ON!");
            LED_Conctrl_num = LED_ON;
        }
        // 清空指令缓冲区
        memset(LED_Read_data, 0, UART1_DMA_RX_LEN);
        osDelay(100);
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
/*--------------------------------------------------------------------
 * 函数名称: LCD_RefreshHandler
 * 功能描述: LCD显示刷新状态机
 * 参数:
 *   mode - 刷新模式 (0:全刷,1:局部刷新)
 * 返回值: 实际刷新耗时（单位：ms）
 * 注意事项:
 *   全刷模式耗时约18ms，建议在任务低负载时调用
 *------------------------------------------------------------------*/
void StartLCDDisplayTaskFunction(void *argument)
{
    // 定义背景刷新
    uint8_t x = 0;
    // 顶所以Lcd的id
    uint8_t lcd_id[12];
    delay_init(72); /* 延时初始化 */
    lcd_init();     /* 初始化LCD */
    g_point_color = RED;
    sprintf((char *)lcd_id, "LCD ID:%04X", lcddev.id); /* 将LCD ID打印到lcd_id数组 */

    for (;;) {
        osSemaphoreAcquire(LCD_refresh_gsemHandle, osWaitForever);
        switch (x) {
            case 0:
                lcd_clear(WHITE);
                break;

            case 1:
                lcd_clear(BLACK);
                break;

            case 2:
                lcd_clear(BLUE);
                break;

            case 3:
                lcd_clear(RED);
                break;

            case 4:
                lcd_clear(MAGENTA);
                break;

            case 5:
                lcd_clear(GREEN);
                break;

            case 6:
                lcd_clear(CYAN);
                break;

            case 7:
                lcd_clear(YELLOW);
                break;

            case 8:
                lcd_clear(BRRED);
                break;

            case 9:
                lcd_clear(GRAY);
                break;

            case 10:
                lcd_clear(LGRAY);
                break;

            case 11:
                lcd_clear(BROWN);
                break;
        }
        lcd_show_string(10, 40, 240, 32, 32, "STM32F103ZET6", RED);
        lcd_show_string(10, 80, 240, 24, 24, "TFTLCD TEST", RED);
        lcd_show_string(10, 110, 240, 16, 16, "User:Cossiant", RED);
        lcd_show_string(10, 130, 240, 16, 16, (char *)lcd_id, RED); /* 显示LCD ID */
        lcd_show_string(10, 150, 240, 16, 16, "UART read data is :", BLACK);
        lcd_show_string(10, 170, 240, 16, 16, (char *)Read_data, BLACK);
        myprintf("LCD refresh data is :%s", Read_data);
        x++;
        if (x == 12) x = 0;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
