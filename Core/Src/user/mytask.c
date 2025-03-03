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

//定义串口号
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
//注意，本参数每次接收到新的数据都将被清空！
char Read_data[UART1_DMA_RX_LEN];

//上一次接收到的数据
//本参数接收到数据之后将被LED使用，使用之后将会被清空！
char LED_Read_data[UART1_DMA_RX_LEN];

// LED控制标志位
unsigned char LED_Conctrl = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 串口发送函数
// 可变长参数函数
// 与操作系统printf函数完全一致
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

// 中断响应函数
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        // 串口发送完成中断时，释放信号量
        osSemaphoreRelease(uart1_printf_gsemHandle);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 串口接收函数
// 这个函数需要将其放入中断文件中，调用这个中断响应函数
void uart1_data_in()
{
    if ((__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET)) {
        // 产生空闲中断就清除中断
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
        // 释放信号量
        osSemaphoreRelease(uart1_rxok_gsemHandle);
    }
}

// 串口接受任务
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
        // 添加字符串结束符
        // myprintf("Read data is :%s", Read_data);
        // Read_data[Read_data_len] = '\0';
        //保存上次读取到的数据
        strcpy(LED_Read_data,Read_data);
        // 释放LCD刷新信号量
        osSemaphoreRelease(LCD_refresh_gsemHandle);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// 任务1
void StartLED1TaskFunction(void *argument)
{
    for (;;) {
        if (strcmp(LED_Read_data, "LED_OFF") == 0) {
            myprintf("Now LED OFF!");
            LED_Conctrl = 1;
            HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
        }
        if (strcmp(LED_Read_data, "LED_ON") == 0) {
            myprintf("Now LED ON!");
            LED_Conctrl = 1;
            HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        }
        if (strcmp(LED_Read_data, "LED_AUTO") == 0) {
            myprintf("Now LED AUTO!");
            LED_Conctrl = 0;
        }
        // 清空指令缓冲区
        memset(LED_Read_data, 0, UART1_DMA_RX_LEN);
        osDelay(100);
    }
}

// 任务2
void StartLED2TaskFunction(void *argument)
{
    for (;;) {
        if (LED_Conctrl == 0) {
            HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
            HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
        }
        osDelay(1000);
    }
}
// LCD显示及其初始化任务
void StartLCDDisplayTaskFunction(void *argument)
{
    //定义背景刷新
    uint8_t x = 0;
    //顶所以Lcd的id
    uint8_t lcd_id[12];
    delay_init(72); /* 延时初始化 */
    lcd_init();     /* 初始化LCD */
    g_point_color = RED;
    sprintf((char *)lcd_id, "LCD ID:%04X", lcddev.id); /* 将LCD ID打印到lcd_id数组 */

    for (;;) {
        osSemaphoreAcquire(LCD_refresh_gsemHandle,osWaitForever);
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
