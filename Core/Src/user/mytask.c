#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "stdio.h"
#include "stdarg.h"
#include "string.h"

extern UART_HandleTypeDef huart1;
extern osSemaphoreId_t uart1_printf_gsemHandle;

char gbuf_printf[80];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//串口发送函数
//可变长参数函数
//与操作系统printf函数完全一致
void myprintf(char* format,...)
{
    //创建可变参数列表类型变量ap
    va_list ap;
    //获取信号量，如果信号量不可用就一直等待
    osSemaphoreAcquire(uart1_printf_gsemHandle,osWaitForever);
    //初始化可变参数列表ap，让其指向format第一个参数
    va_start(ap,format);
    //检查串口是否正在发送数据，如果是直接退出
    if (huart1.gState==HAL_UART_STATE_BUSY_TX)return;
    //将格式化后的字符串存到gbuf_printf数组中
    vsprintf(gbuf_printf,format,ap);
    //串口中断方式发送gbuf_printf中的数据
    HAL_UART_Transmit_IT(&huart1,(uint8_t*)gbuf_printf,strlen(gbuf_printf));
    //释放ap的资源
    va_end(ap);
}

//中断响应函数
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        //串口发送完成中断时，释放信号量
        osSemaphoreRelease(uart1_printf_gsemHandle);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//任务1
void StartLED1TaskFunction(void *argument)
{
    for (;;)
    {
        myprintf("<test1 test1 test1>\r\n");
        osDelay(100);
    }
    
}

//任务2
void StartLED2TaskFunction(void *argument)
{
    for (;;)
    {
        myprintf("<test2 test2 test2>\r\n");
        osDelay(200);
    }
    
}

