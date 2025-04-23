#ifndef _MYPRINTF_H
#define _MYPRINTF_H

// 注意这里定义的数据发送和接受长度一定要足够！例如LED_AUTO就需要8*8=64！
#define UART1_DMA_RX_LEN 70

/**
 * @brief  UART通信数据管理结构体
 * @note   DMA接收数据生命周期管理：
 *         [接收] → Read_data → [处理] → Last_Read_data/LED_Read_data
 */
typedef struct {
    char Read_data[UART1_DMA_RX_LEN];          //!< 当前接收缓冲区（只有当新数据来的时候才会自动清零）
    char Last_Read_data[UART1_DMA_RX_LEN];     //!< 历史数据缓存（用于对比变更，这里的数据可以理解为长久数据，知道下一次新数据覆盖）
    char Response_Read_data[UART1_DMA_RX_LEN]; //!< 送给处理函数使用的变量（每次处理函数使用完成将会自动清空！）
} USART_USE_DATA;

void myprintf(char *format, ...);
void StartUART1_recv_TaskFunction(void *argument);

#endif
