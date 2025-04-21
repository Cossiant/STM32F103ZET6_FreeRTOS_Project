#ifndef _MYPRINTF_H
#define _MYPRINTF_H

// 注意这里定义的数据发送和接受长度一定要足够！例如LED_AUTO就需要8*8=64！
#define UART1_DMA_RX_LEN 70
#define Set_Time_hours   23
#define Set_Time_minute  59
#define Set_Time_second  50

/**
 * @brief  UART通信数据管理结构体
 * @note   DMA接收数据生命周期管理：
 *         [接收] → Read_data → [处理] → Last_Read_data/LED_Read_data
 */
typedef struct {
  char Read_data[UART1_DMA_RX_LEN];      //!< 当前接收缓冲区（自动清零）
  char Last_Read_data[UART1_DMA_RX_LEN]; //!< 历史数据缓存（用于对比变更）
  char Response_Read_data[UART1_DMA_RX_LEN];
} USART_USE_DATA;

void myprintf(char *format, ...);
void StartUART1_recv_TaskFunction(void *argument);

#endif
