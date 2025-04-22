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
    char *str                          = 0;
    unsigned char key;
    remote_init(); /* 红外接收初始化 */
    for (;;) {
        if (SYS->Beep_control.Beep_control_num != BEEP_OFF) {
            HAL_GPIO_WritePin(Beep1_GPIO_Port, Beep1_Pin, GPIO_PIN_SET);
            osDelay(SYS->Beep_control.Beep_delay_num);
            HAL_GPIO_WritePin(Beep1_GPIO_Port, Beep1_Pin, GPIO_PIN_RESET);
            osDelay(1000 - SYS->Beep_control.Beep_delay_num);
        }

        key = remote_scan();
        if (key) {
            lcd_show_num(86, 230, key, 3, 16, BLUE);          /* 显示键值 */
            lcd_show_num(86, 250, g_remote_cnt, 3, 16, BLUE); /* 显示按键次数 */
            switch (key) {
                case 0:
                    str = "ERROR";
                    break;
                case 69:
                    str = "POWER";
                    break;
                case 70:
                    str = "UP";
                    break;
                case 64:
                    str = "PLAY";
                    break;
                case 71:
                    str = "ALIENTEK";
                    break;
                case 67:
                    str = "RIGHT";
                    break;
                case 68:
                    str = "LEFT";
                    break;
                case 7:
                    str = "VOL-";
                    break;
                case 21:
                    str = "DOWN";
                    break;
                case 9:
                    str = "VOL+";
                    break;
                case 22:
                    str = "1";
                    break;
                case 25:
                    str = "2";
                    break;
                case 13:
                    str = "3";
                    break;
                case 12:
                    str = "4";
                    break;
                case 24:
                    str = "5";
                    break;
                case 94:
                    str = "6";
                    break;
                case 8:
                    str = "7";
                    break;
                case 28:
                    str = "8";
                    break;
                case 90:
                    str = "9";
                    break;
                case 66:
                    str = "0";
                    break;
                case 74:
                    str = "DELETE";
                    break;
            }
            lcd_fill(86, 270, 116 + 8 * 8, 170 + 16, WHITE);  /* 清楚之前的显示 */
            lcd_show_string(86, 270, 200, 16, 16, str, BLUE); /* 显示SYMBOL */
        } else {
            osDelay(100);
        }
    }
}
