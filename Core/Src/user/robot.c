#include "my_sys_data.h"
#include "FreeRTOS.h"
#include "string.h"

// 定义PWM要输出的数周期数量
// ROBOT.c私有变量
unsigned int Robot_Motor1_PWM_execution_num = 0;
unsigned int Robot_Motor2_PWM_execution_num = 0;
unsigned int Robot_Motor3_PWM_execution_num = 0;
// 定义PWM当前占位状态
// ROBOT.c私有变量
unsigned char Robot_Motor1_Ready = Robot_Motor_Ready;
unsigned char Robot_Motor2_Ready = Robot_Motor_Ready;
unsigned char Robot_Motor3_Ready = Robot_Motor_Ready;
// 定义当前是否是自动模式
// ROBOT.c私有变量
unsigned char Robot_Mod_TIM_PWM = Robot_Mod_NULL;
/******************************************************************************************************************************************/
// 变量传递函数，将要执行的PWM传递给这个C文件的私有变量
// ROBOT.c私有函数
void Write_PWM_Num_to_local(unsigned char which_one, unsigned int inputnum)
{
    switch (which_one) {
        case 1:
            // 控制的PWM生成数量+1的原因是因为每当PWM开始生成的时候都会-1
            Robot_Motor1_PWM_execution_num = inputnum + 1;
            break;
        case 2:
            Robot_Motor2_PWM_execution_num = inputnum + 1;
            break;
        case 3:
            Robot_Motor3_PWM_execution_num = inputnum + 1;
            break;
        default:
            break;
    }
}
/******************************************************************************************************************************************/
// 写入参数并启动机械臂PWM，目前还没有添加方向和位控制
// 公共函数
void Start_Robot_PWM_Function(SYS_USE_DATA *SYS, unsigned char channel_num)
{
    // 这里需要将控制参数传递给这边自定义变量，否则无法直接调用SYS当中的参数
    Write_PWM_Num_to_local(1, SYS->Robot_use_data.Motor1.PWM_execution_count);
    Write_PWM_Num_to_local(2, SYS->Robot_use_data.Motor2.PWM_execution_count);
    Write_PWM_Num_to_local(3, SYS->Robot_use_data.Motor3.PWM_execution_count);
    // 将当前电机的状态给到TIM用于判断是否自动开启通道转换
    Robot_Mod_TIM_PWM = SYS->Robot_use_data.Motor_Mod;
    switch (channel_num) {
        case 1:
            /* code */
            Robot_Motor1_Ready = Robot_Motor_Busy;
            // 启动通道1，随后根据当前的mod模式决定是否开启通道自动转换
            HAL_TIM_PWM_Start_IT(&htim8, TIM_CHANNEL_1);
            break;
        case 2:
            /* code */
            Robot_Motor2_Ready = Robot_Motor_Busy;
            // 启动通道2，随后会自动完成通道3的启动
            HAL_TIM_PWM_Start_IT(&htim8, TIM_CHANNEL_2);
            break;
        case 3:
            /* code */
            Robot_Motor3_Ready = Robot_Motor_Busy;
            // 启动通道3
            HAL_TIM_PWM_Start_IT(&htim8, TIM_CHANNEL_3);
            break;
        default:
            break;
    }
}
/******************************************************************************************************************************************/
// 中断处理函数必须是先进行判断再进行自减，否则执行次数将会少1
// 中断处理函数
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    // 确保计数不会超过限定，导致数据溢出
    // 所以我在这里引入了标志位，这个定时器同一时刻只能控制1个通道，当通道标志位为忙的时候才可以进行计数
    if ((Robot_Motor1_PWM_execution_num > 0) && (Robot_Motor1_Ready == Robot_Motor_Busy)) Robot_Motor1_PWM_execution_num = Robot_Motor1_PWM_execution_num - 1;
    if ((Robot_Motor2_PWM_execution_num > 0) && (Robot_Motor2_Ready == Robot_Motor_Busy)) Robot_Motor2_PWM_execution_num = Robot_Motor2_PWM_execution_num - 1;
    if ((Robot_Motor3_PWM_execution_num > 0) && (Robot_Motor3_Ready == Robot_Motor_Busy)) Robot_Motor3_PWM_execution_num = Robot_Motor3_PWM_execution_num - 1;
    // 从这里开始是根据计数做对应PWM操作
    // 判断是否完成指定数量PWM波，并且确定当前状态是忙，这样就可以只执行一次而不会下次中断再进入这个判断
    // 因此在这里需要分情况，如果是手动模式那么不需要进行PWM波通道自动切换，但是如果是自动模式就需要让其能够实现自动切换
    if (Robot_Mod_TIM_PWM == Robot_Mod_AUTO) {
        if ((Robot_Motor1_PWM_execution_num == 0) && (Robot_Motor1_Ready == Robot_Motor_Busy)) {
            Robot_Motor1_Ready = Robot_Motor_Ready;
            Robot_Motor2_Ready = Robot_Motor_Busy;
            HAL_TIM_PWM_Stop_IT(&htim8, TIM_CHANNEL_1);
            HAL_TIM_PWM_Start_IT(&htim8, TIM_CHANNEL_2);
        }
        // 同理，但是判断的是2通道
        if ((Robot_Motor2_PWM_execution_num == 0) && (Robot_Motor2_Ready == Robot_Motor_Busy)) {
            Robot_Motor2_Ready = Robot_Motor_Ready;
            Robot_Motor3_Ready = Robot_Motor_Busy;
            HAL_TIM_PWM_Stop_IT(&htim8, TIM_CHANNEL_2);
            HAL_TIM_PWM_Start_IT(&htim8, TIM_CHANNEL_3);
        }
        // 同理，但是判断的是3通道
        if ((Robot_Motor3_PWM_execution_num == 0) && (Robot_Motor3_Ready == Robot_Motor_Busy)) {
            Robot_Motor3_Ready = Robot_Motor_Ready;
            HAL_TIM_PWM_Stop_IT(&htim8, TIM_CHANNEL_3);
        }
    } else {
        // 如果不是自动模式就需要单独来控制
        if ((Robot_Motor1_PWM_execution_num == 0) && (Robot_Motor1_Ready == Robot_Motor_Busy)) {
            Robot_Motor1_Ready = Robot_Motor_Ready;
            HAL_TIM_PWM_Stop_IT(&htim8, TIM_CHANNEL_1);
        }
        if ((Robot_Motor2_PWM_execution_num == 0) && (Robot_Motor2_Ready == Robot_Motor_Busy)) {
            Robot_Motor2_Ready = Robot_Motor_Ready;
            HAL_TIM_PWM_Stop_IT(&htim8, TIM_CHANNEL_2);
        }
        if ((Robot_Motor3_PWM_execution_num == 0) && (Robot_Motor3_Ready == Robot_Motor_Busy)) {
            Robot_Motor3_Ready = Robot_Motor_Ready;
            HAL_TIM_PWM_Stop_IT(&htim8, TIM_CHANNEL_3);
        }
    }
}
/**********************************************************************************************************************************************/
// 为了实现低内聚高耦合，因此在这里设定按键检测判断函数
// Key_num是红外读取的key值
// Motor_num是当前正在处理的电机值
void Infrared_directional_button_detection(SYS_USE_DATA *SYS, unsigned char Motor_num)
{
    switch (Motor_num) {
        case 1:
            // 判断left
            if (SYS->Remote_use_data.key == 68) {
                // 首先完成当前电机旋转方向值给予
                // 也可以直接操作IO口（没写代码，需要写上）(IO低电平)
                /**************************************/
                HAL_GPIO_WritePin(Motor_GPIO_CH1_GPIO_Port, Motor_GPIO_CH1_Pin, GPIO_PIN_RESET);
                /**************************************/
                SYS->Robot_use_data.Motor1.Motor_rotation_direction = Robot_rotation_left;
                // 输出PWM，因为osdelay为2，因此每个周期应该也为2
                SYS->Robot_use_data.Motor1.PWM_execution_count = 2;
                // 这个红外遥控按钮遥控的是1号电机，因此操作一号电机启动
                Start_Robot_PWM_Function(SYS, 1);
            }
            // 判断right
            if (SYS->Remote_use_data.key == 67) {
                // 首先完成当前电机旋转方向值给予
                // 也可以直接操作IO口（没写代码，需要写上）(IO高电平)
                /**************************************/
                HAL_GPIO_WritePin(Motor_GPIO_CH1_GPIO_Port, Motor_GPIO_CH1_Pin, GPIO_PIN_SET);
                /**************************************/
                SYS->Robot_use_data.Motor1.Motor_rotation_direction = Robot_rotation_right;
                // 输出PWM，因为osdelay为2，因此每个周期应该也为2
                SYS->Robot_use_data.Motor1.PWM_execution_count = 2;
                // 这个红外遥控按钮遥控的是1号电机，因此操作一号电机启动
                Start_Robot_PWM_Function(SYS, 1);
            }
            // 自动清空PWM波参数
            if (SYS->Remote_use_data.key == 0) {
                // 将PWM执行的参数清零
                SYS->Robot_use_data.Motor1.PWM_execution_count = 0;
                // 在这里清除IO口到默认设置（没写代码，需要写上，默认高电平）
                /**************************************/
                HAL_GPIO_WritePin(Motor_GPIO_CH1_GPIO_Port, Motor_GPIO_CH1_Pin, GPIO_PIN_SET);
                /**************************************/
            }
            break;
        case 2:
            // 判断up（低电平往前）
            if (SYS->Remote_use_data.key == 70) {
                HAL_GPIO_WritePin(Motor_GPIO_CH2_GPIO_Port, Motor_GPIO_CH2_Pin, GPIO_PIN_RESET);
                SYS->Robot_use_data.Motor2.Motor_rotation_direction = Robot_rotation_left;
                SYS->Robot_use_data.Motor2.PWM_execution_count      = 2;
                Start_Robot_PWM_Function(SYS, 2);
            }
            // 判断down（高电平往后）
            if (SYS->Remote_use_data.key == 21) {
                HAL_GPIO_WritePin(Motor_GPIO_CH2_GPIO_Port, Motor_GPIO_CH2_Pin, GPIO_PIN_SET);
                SYS->Robot_use_data.Motor2.Motor_rotation_direction = Robot_rotation_right;
                SYS->Robot_use_data.Motor2.PWM_execution_count      = 2;
                Start_Robot_PWM_Function(SYS, 2);
            }
            if (SYS->Remote_use_data.key == 0) {
                SYS->Robot_use_data.Motor2.PWM_execution_count = 0;
                HAL_GPIO_WritePin(Motor_GPIO_CH2_GPIO_Port, Motor_GPIO_CH2_Pin, GPIO_PIN_SET);
            }
            break;
        case 3:
            // 判断up（低电平上高）
            if (SYS->Remote_use_data.key == 70) {
                HAL_GPIO_WritePin(Motor_GPIO_CH3_GPIO_Port, Motor_GPIO_CH3_Pin, GPIO_PIN_RESET);
                SYS->Robot_use_data.Motor3.Motor_rotation_direction = Robot_rotation_left;
                SYS->Robot_use_data.Motor3.PWM_execution_count      = 2;
                Start_Robot_PWM_Function(SYS, 3);
            }
            // 判断down（高电平下低）
            if (SYS->Remote_use_data.key == 21) {
                HAL_GPIO_WritePin(Motor_GPIO_CH3_GPIO_Port, Motor_GPIO_CH3_Pin, GPIO_PIN_SET);
                SYS->Robot_use_data.Motor3.Motor_rotation_direction = Robot_rotation_right;
                SYS->Robot_use_data.Motor3.PWM_execution_count      = 2;
                Start_Robot_PWM_Function(SYS, 3);
            }
            if (SYS->Remote_use_data.key == 0) {
                SYS->Robot_use_data.Motor3.PWM_execution_count = 0;
                HAL_GPIO_WritePin(Motor_GPIO_CH3_GPIO_Port, Motor_GPIO_CH3_Pin, GPIO_PIN_SET);
            }
            break;
            // 注意！从这里开始就是对小车底盘的控制了！
        case 4:
            // 按下left键，小车进行左转运动
            if (SYS->Remote_use_data.key == 68) {
                // 对于小车左转，需要分别对电机1、电机2正转，电机3和电机4反转
                // 并且需要将PWM信号进行正常输出，在这里表现为osDelay(2)即可，如果频率太高可以进行调整
                // 电机1反转
                HAL_GPIO_WritePin(Car_Motor_1IN1_GPIO_Port, Car_Motor_1IN1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_1IN2_GPIO_Port, Car_Motor_1IN2_Pin, GPIO_PIN_RESET);
                // 电机2正转
                HAL_GPIO_WritePin(Car_Motor_2IN1_GPIO_Port, Car_Motor_2IN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_2IN2_GPIO_Port, Car_Motor_2IN2_Pin, GPIO_PIN_SET);
                // 电机3反转
                HAL_GPIO_WritePin(Car_Motor_3IN1_GPIO_Port, Car_Motor_3IN1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_3IN2_GPIO_Port, Car_Motor_3IN2_Pin, GPIO_PIN_RESET);
                // 电机4正转
                HAL_GPIO_WritePin(Car_Motor_4IN1_GPIO_Port, Car_Motor_4IN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_4IN2_GPIO_Port, Car_Motor_4IN2_Pin, GPIO_PIN_SET);
                // 对对应通道输出PWM信号
                HAL_GPIO_WritePin(Car_Motor_1P1_GPIO_Port, Car_Motor_1P1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_2P1_GPIO_Port, Car_Motor_2P1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_3P1_GPIO_Port, Car_Motor_3P1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_4P1_GPIO_Port, Car_Motor_4P1_Pin, GPIO_PIN_RESET);
                osDelay(10);
                HAL_GPIO_WritePin(Car_Motor_1P1_GPIO_Port, Car_Motor_1P1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_2P1_GPIO_Port, Car_Motor_2P1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_3P1_GPIO_Port, Car_Motor_3P1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_4P1_GPIO_Port, Car_Motor_4P1_Pin, GPIO_PIN_SET);
                osDelay(10);
            }
            // 按下right键，小车进行右转运动
            if (SYS->Remote_use_data.key == 67) {
                // 电机1反转
                HAL_GPIO_WritePin(Car_Motor_1IN1_GPIO_Port, Car_Motor_1IN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_1IN2_GPIO_Port, Car_Motor_1IN2_Pin, GPIO_PIN_SET);
                // 电机2正转
                HAL_GPIO_WritePin(Car_Motor_2IN1_GPIO_Port, Car_Motor_2IN1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_2IN2_GPIO_Port, Car_Motor_2IN2_Pin, GPIO_PIN_RESET);
                // 电机3反转
                HAL_GPIO_WritePin(Car_Motor_3IN1_GPIO_Port, Car_Motor_3IN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_3IN2_GPIO_Port, Car_Motor_3IN2_Pin, GPIO_PIN_SET);
                // 电机4正转
                HAL_GPIO_WritePin(Car_Motor_4IN1_GPIO_Port, Car_Motor_4IN1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_4IN2_GPIO_Port, Car_Motor_4IN2_Pin, GPIO_PIN_RESET);
                // 对对应通道输出PWM信号
                HAL_GPIO_WritePin(Car_Motor_1P1_GPIO_Port, Car_Motor_1P1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_2P1_GPIO_Port, Car_Motor_2P1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_3P1_GPIO_Port, Car_Motor_3P1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_4P1_GPIO_Port, Car_Motor_4P1_Pin, GPIO_PIN_RESET);
                osDelay(10);
                HAL_GPIO_WritePin(Car_Motor_1P1_GPIO_Port, Car_Motor_1P1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_2P1_GPIO_Port, Car_Motor_2P1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_3P1_GPIO_Port, Car_Motor_3P1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_4P1_GPIO_Port, Car_Motor_4P1_Pin, GPIO_PIN_SET);
                osDelay(10);
            }
            // 按下up键，小车前进
            if (SYS->Remote_use_data.key == 70) {
                // 电机1正转
                HAL_GPIO_WritePin(Car_Motor_1IN1_GPIO_Port, Car_Motor_1IN1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_1IN2_GPIO_Port, Car_Motor_1IN2_Pin, GPIO_PIN_RESET);
                // 电机2正转
                HAL_GPIO_WritePin(Car_Motor_2IN1_GPIO_Port, Car_Motor_2IN1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_2IN2_GPIO_Port, Car_Motor_2IN2_Pin, GPIO_PIN_RESET);
                // 电机3反转
                HAL_GPIO_WritePin(Car_Motor_3IN1_GPIO_Port, Car_Motor_3IN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_3IN2_GPIO_Port, Car_Motor_3IN2_Pin, GPIO_PIN_SET);
                // 电机4反转
                HAL_GPIO_WritePin(Car_Motor_4IN1_GPIO_Port, Car_Motor_4IN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_4IN2_GPIO_Port, Car_Motor_4IN2_Pin, GPIO_PIN_SET);
                // 对对应通道输出PWM信号
                HAL_GPIO_WritePin(Car_Motor_1P1_GPIO_Port, Car_Motor_1P1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_2P1_GPIO_Port, Car_Motor_2P1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_3P1_GPIO_Port, Car_Motor_3P1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_4P1_GPIO_Port, Car_Motor_4P1_Pin, GPIO_PIN_RESET);
                osDelay(10);
                HAL_GPIO_WritePin(Car_Motor_1P1_GPIO_Port, Car_Motor_1P1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_2P1_GPIO_Port, Car_Motor_2P1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_3P1_GPIO_Port, Car_Motor_3P1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_4P1_GPIO_Port, Car_Motor_4P1_Pin, GPIO_PIN_SET);
                osDelay(10);
            }
            // 按下down键，小车后退
            if (SYS->Remote_use_data.key == 21) {

                // 电机1正转
                HAL_GPIO_WritePin(Car_Motor_1IN1_GPIO_Port, Car_Motor_1IN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_1IN2_GPIO_Port, Car_Motor_1IN2_Pin, GPIO_PIN_SET);
                // 电机2正转
                HAL_GPIO_WritePin(Car_Motor_2IN1_GPIO_Port, Car_Motor_2IN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_2IN2_GPIO_Port, Car_Motor_2IN2_Pin, GPIO_PIN_SET);
                // 电机3反转
                HAL_GPIO_WritePin(Car_Motor_3IN1_GPIO_Port, Car_Motor_3IN1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_3IN2_GPIO_Port, Car_Motor_3IN2_Pin, GPIO_PIN_RESET);
                // 电机4反转
                HAL_GPIO_WritePin(Car_Motor_4IN1_GPIO_Port, Car_Motor_4IN1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_4IN2_GPIO_Port, Car_Motor_4IN2_Pin, GPIO_PIN_RESET);
                // 对对应通道输出PWM信号
                HAL_GPIO_WritePin(Car_Motor_1P1_GPIO_Port, Car_Motor_1P1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_2P1_GPIO_Port, Car_Motor_2P1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_3P1_GPIO_Port, Car_Motor_3P1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_4P1_GPIO_Port, Car_Motor_4P1_Pin, GPIO_PIN_RESET);
                osDelay(10);
                HAL_GPIO_WritePin(Car_Motor_1P1_GPIO_Port, Car_Motor_1P1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_2P1_GPIO_Port, Car_Motor_2P1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_3P1_GPIO_Port, Car_Motor_3P1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(Car_Motor_4P1_GPIO_Port, Car_Motor_4P1_Pin, GPIO_PIN_SET);
                osDelay(10);
            }
            // 没有按下任何按键的时候，需要将所有IO全部放置于0
            if (SYS->Remote_use_data.key == 0) {
                HAL_GPIO_WritePin(Car_Motor_1IN1_GPIO_Port, Car_Motor_1IN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_1IN2_GPIO_Port, Car_Motor_1IN2_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_2IN1_GPIO_Port, Car_Motor_2IN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_2IN2_GPIO_Port, Car_Motor_2IN2_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_3IN1_GPIO_Port, Car_Motor_3IN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_3IN2_GPIO_Port, Car_Motor_3IN2_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_4IN1_GPIO_Port, Car_Motor_4IN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Car_Motor_4IN2_GPIO_Port, Car_Motor_4IN2_Pin, GPIO_PIN_RESET);
                osDelay(5);
            }

            break;
        default:
            break;
    }
}
/***********************************************************************************************************************************************/
/*
 * 我需要在这里完成通过读取红外参数来控制机械臂移动的函数
 */
void remote_control_robot(SYS_USE_DATA *SYS)
{
    // 首先需要完成的是读取红外当前时刻的数据
    // 我在这里设定的是当红外按钮上一个按键是POWER时启动空模式
    // 当红外按钮为ALIENTEK时设定为手动模式
    // 自动模式可以后期加上
    // 注意这里可千万别用switch，因为switch没有办法判断字符串！
    if (strcmp(SYS->Remote_use_data.str, "POWER") == 0) {
        // 启动空模式
        SYS->Robot_use_data.Motor_Mod = Robot_Mod_NULL;
    }
    if (strcmp(SYS->Remote_use_data.str, "PLAY") == 0) {
        // 启动小车移动控制模式（这个需要4个通道控制）
        SYS->Robot_use_data.Motor_Mod = Robot_Mod_Move;
    }
    if (strcmp(SYS->Remote_use_data.str, "1") == 0) {
        // 启动电机1单独控制模式
        SYS->Robot_use_data.Motor_Mod = Robot_Mod_Motor1;
    }
    if (strcmp(SYS->Remote_use_data.str, "2") == 0) {
        // 启动电机2单独控制模式
        SYS->Robot_use_data.Motor_Mod = Robot_Mod_Motor2;
    }
    if (strcmp(SYS->Remote_use_data.str, "3") == 0) {
        // 启动电机3单独控制模式
        SYS->Robot_use_data.Motor_Mod = Robot_Mod_Motor3;
    }
    // 这里暂时没有添加自动模式！

    // 读取完成后就可以进行判断当前模式
    // 这里目前是有问题的，因为up和down需要两个电机合作，而左右只需要控制电机1即可因此这里写的是控制电机1的代码
    switch (SYS->Robot_use_data.Motor_Mod) {
        // 这里是手动模式
        // 1电机只判断左右
        // 2电机只判断上下
        // 3电机只判断上下
        // 还有后续的小车移动电机这个直接单独用一种模式（四个电机用一个通道就够了，判断的任务交给IO控制）
        case Robot_Mod_Motor1:
            Infrared_directional_button_detection(SYS, 1);
            break;
        case Robot_Mod_Motor2:
            Infrared_directional_button_detection(SYS, 2);
            break;
        case Robot_Mod_Motor3:
            Infrared_directional_button_detection(SYS, 3);
            break;
        case Robot_Mod_Move:
            // 现在是进行小车底盘控制
            Infrared_directional_button_detection(SYS, 4);
            break;
        case Robot_Mod_NULL:
            // 空模式这里目前什么都不执行
            osDelay(2);
            break;
        // 如果是自动模式或者其他就退出函数
        default:
            break;
    }
}

/******************************************************************************************************************************************/
// 任务执行函数
// 任务函数
void StartRobotmainControlTask(void *argument)
{
    SYS_USE_DATA *SYS = (SYS_USE_DATA *)argument;
    osDelay(500);
    // 首先需要将mod模式设定为NULL
    SYS->Robot_use_data.Motor_Mod = Robot_Mod_NULL;
    for (;;) {
        remote_control_robot(SYS);
        osDelay(2);
    }
}
