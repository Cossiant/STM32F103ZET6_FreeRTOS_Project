#ifndef __MYTASK_H
#define __MYTASK_H

#include "my_sys_data.h"

void StartLEDProcessedTaskFunction(void *argument);
void StartLEDWorkTaskFunction(void *argument);
void StartLCDDisplayTaskFunction(void *argument);
void StartTimeSetTaskFunction(void *argument);

#endif
