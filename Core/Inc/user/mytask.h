#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

void myprintf(char *format, ...);
void StartLED1TaskFunction(void *argument);
void StartLED2TaskFunction(void *argument);
void StartLED2TaskFunction(void *argument);
void StartLCDDisplayTaskFunction(void *argument);
