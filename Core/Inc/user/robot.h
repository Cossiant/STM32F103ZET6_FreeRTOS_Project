#ifndef __ROBOT_H
#define __ROBOT_H

// 电机状态参数
typedef enum {
    Robot_Motor_Ready,
    Robot_Motor_Busy
} ROBOT_MOTOR_READY_MOD;

// 机器人电机结构体
typedef struct
{
    // PWM占空比
    unsigned char PWM_duty_cycle;
    // 电机旋转方向
    unsigned char Motor_rotation_direction;
    // 电机是否选中
    unsigned char Motor_choose;
    // 电机旋转度数
    unsigned char Motor_rotation_degrees;
    // 根据电机旋转度数确定的对应的电机PWM执行次数
    unsigned int PWM_execution_count;
} MOTOR_USE_DATA;

// 定义机器人机械臂使用数据结构
typedef struct
{
    MOTOR_USE_DATA Motor1; // 机械臂电机1
    MOTOR_USE_DATA Motor2; // 机械臂电机2
    MOTOR_USE_DATA Motor3; // 机械臂电机3
} ROBOT_USE_TYPE;

#endif
