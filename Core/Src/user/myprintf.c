#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "stdio.h"
#include "stdarg.h"
#include "string.h"
#include "gpio.h"
#include "mytask.h"
#include "myprintf.h"

// 定义串口号
extern UART_HandleTypeDef huart1;
// 定义DMA接口
extern DMA_HandleTypeDef hdma_usart1_rx;
// 信号量
extern osSemaphoreId_t uart1_printf_gsemHandle;
extern osSemaphoreId_t uart1_rxok_gsemHandle;
extern osSemaphoreId_t LCD_refresh_gsemHandle;
// 完成数据读取，通知数据处理函数可以工作了
extern osSemaphoreId_t uart1_data_handle_gsemHandle;

// 要发送的数据
char gbuf_printf[UART1_DMA_RX_LEN];

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
    SYS_USE_DATA *SYS = (SYS_USE_DATA *)argument;
    // now_dma_ip当前数据缓冲区数据长度
    // rd_dma_ip读到的数据的长度
    unsigned short now_dam_ip, rd_dma_ip = 0, Read_data_len;
    // 接受缓冲区rxBuffer，指令缓冲区SYS->usart_use_data.Read_data
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
        memset(SYS->usart_use_data.Read_data, 0, UART1_DMA_RX_LEN);
        // 如果读指针不等于当前数据指针
        while (rd_dma_ip != now_dam_ip) {
            // 将数据读取到SYS->usart_use_data.Read_data当中
            SYS->usart_use_data.Read_data[Read_data_len++] = rxBuffer[rd_dma_ip++];
            if (rd_dma_ip >= UART1_DMA_RX_LEN) rd_dma_ip = 0;
        }
        // 保存读取到的数据，因为Read_data会清零！虽然在这个位置他是完整的数据
        strcpy(SYS->usart_use_data.Response_Read_data, SYS->usart_use_data.Read_data);
        // 释放data处理刷新信号量(Response_Read_data使用完成之后自动清空)
        osSemaphoreRelease(uart1_data_handle_gsemHandle);
        // 释放LCD刷新信号量(在LCD显示的同时将Read_data送给Last_Read_data)
        osSemaphoreRelease(LCD_refresh_gsemHandle);
    }
}
