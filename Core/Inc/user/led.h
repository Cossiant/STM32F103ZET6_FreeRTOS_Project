#ifndef __LED_H
#define __LED_H

typedef struct
{
    unsigned char Led_num;
} LED_USE_DATA;

// LED控制标志位
// 拥有参数LED_AUTO,LED_ON,LED_OFF
typedef enum {
    LED_AUTO,
    LED_ON,
    LED_OFF,
    LED_Artificial
} LED_Conctrl_MOD;

void StartLEDWorkTaskFunction(void *argument);
#endif
