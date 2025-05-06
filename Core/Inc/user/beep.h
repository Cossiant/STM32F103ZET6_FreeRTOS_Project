#ifndef __BEEP_H
#define __BEEP_H

typedef struct
{
  unsigned int Beep_delay_num;
  unsigned char Beep_control_num;
}BEEP_USE_DATA;


typedef enum
{
  BEEP_OFF,
  BEEP_AUTO,
  BEEP_Artificial
}BEEP_Control_MOD;


void StartBeepWorkTaskFunction(void *argument);

#endif

// 1、完成小车底盘设计、安装
// 2、完成3D打印机械臂设计、制造、安装
