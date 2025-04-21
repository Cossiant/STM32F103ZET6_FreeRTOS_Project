#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "gpio.h"

#include "mytask.h"

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
    SYS_USE_DATA *SYS            = (SYS_USE_DATA *)argument;
    SYS->led_control_num.Led_num = LED_AUTO;
    for (;;) {
        if (SYS->led_control_num.Led_num != LED_Artificial) {
            switch (SYS->led_control_num.Led_num) {
                case LED_AUTO:
                    HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
                    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
                    break;
                case LED_ON:
                    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
                    SYS->led_control_num.Led_num = LED_Artificial;
                    break;
                case LED_OFF:
                    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
                    SYS->led_control_num.Led_num = LED_Artificial;
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
