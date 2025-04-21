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
