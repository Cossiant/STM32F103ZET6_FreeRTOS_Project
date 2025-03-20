#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

void myprintf(char *format, ...);
void StartLED1TaskFunction(void *argument);
void StartLED2TaskFunction(void *argument);
void StartLED2TaskFunction(void *argument);
void StartLCDDisplayTaskFunction(void *argument);

//设置当前时间结构体，实现LCD动态显示时间
typedef struct
{
    unsigned char hours;
    unsigned char minute;
    unsigned char second;
}TIME_STRUCT;
