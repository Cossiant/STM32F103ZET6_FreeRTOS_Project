#ifndef __ROBOT_H
#define __ROBOT_H

// 电机状态参数
typedef enum {
    Robot_Motor_Ready,
    Robot_Motor_Busy
} ROBOT_MOTOR_READY_MOD;

// 机械臂的控制模式enum
typedef enum {
    // 第一种模式是空模式（默认上电是这个模式，也可以通过红外按钮获得）
    Robot_Mod_NULL,
    // 随后是单独的电机控制模式
    Robot_Mod_Motor1,
    Robot_Mod_Motor2,
    Robot_Mod_Motor3,
    // 第三种模式是移动电机控制模式
    Robot_Mod_Move,
    // 第四种是自动模式（也是通过红外遥控器给予）
    Robot_Mod_AUTO
} ROBOT_KEY_MOD;

// 电机旋转方向enum
typedef enum {
    Robot_rotation_left,
    Robot_rotation_right
} MOTOR_ROTATION_DIRECTION_MOD;

// 机器人电机结构体
typedef struct
{
    // 电机旋转方向（这个需要单独写函数来完成使用）
    unsigned char Motor_rotation_direction;
    // 电机旋转度数（这个需要单独写函数来完成使用）
    unsigned char Motor_rotation_degrees;
    // 根据电机旋转度数确定的对应的电机PWM执行次数
    unsigned int PWM_execution_count;
} MOTOR_USE_DATA;

// 移动地盘结构体
typedef struct
{
    // 小车移动的方向（这个需要单独写函数来完成使用）（这个参数是用来控制IO口的）
    unsigned char Movement_direction;
    // 对应的小车电机PWM执行次数
    unsigned int PWM_execution_count;
} CAR_MOVE_USE_DATA;

// 定义机器人机械臂使用数据结构
typedef struct
{
    unsigned char Motor_Mod;     // 当前电机运行的模式
    MOTOR_USE_DATA Motor1;       // 机械臂电机1
    MOTOR_USE_DATA Motor2;       // 机械臂电机2
    MOTOR_USE_DATA Motor3;       // 机械臂电机3
    CAR_MOVE_USE_DATA Car_Motor; // 小车的4个电机都通过这一个信号量来控制
} ROBOT_USE_TYPE;

#endif
