#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "gpio.h"
#include "remote.h"
#include "lcd.h"

#include "my_sys_data.h"

void StartBeepWorkTaskFunction(void *argument)
{
    SYS_USE_DATA *SYS                  = (SYS_USE_DATA *)argument;
    SYS->Beep_control.Beep_delay_num   = 0;
    SYS->Beep_control.Beep_control_num = BEEP_OFF;
    SYS->Remote_use_data.str           = 0;
    SYS->Remote_use_data.key           = 0;
    remote_init(); /* 红外接收初始化 */
    for (;;) {
        if (SYS->Beep_control.Beep_control_num != BEEP_OFF) {
            HAL_GPIO_WritePin(Beep1_GPIO_Port, Beep1_Pin, GPIO_PIN_SET);
            osDelay(SYS->Beep_control.Beep_delay_num);
            HAL_GPIO_WritePin(Beep1_GPIO_Port, Beep1_Pin, GPIO_PIN_RESET);
            osDelay(1000 - SYS->Beep_control.Beep_delay_num);
        } else {
            Read_remote_data(&(SYS->Remote_use_data));
            osDelay(100);
        }
    }
}
