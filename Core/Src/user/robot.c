#include "my_sys_data.h"
#include "FreeRTOS.h"

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
void Start_Robot_PWM_Function(SYS_USE_DATA *SYS)
{
    // 这里需要将控制参数传递给这边自定义变量，否则无法直接调用SYS当中的参数
    Write_PWM_Num_to_local(1, SYS->Robot_use_data.Motor1.PWM_execution_count);
    Write_PWM_Num_to_local(2, SYS->Robot_use_data.Motor2.PWM_execution_count);
    Write_PWM_Num_to_local(3, SYS->Robot_use_data.Motor3.PWM_execution_count);
    Robot_Motor1_Ready = Robot_Motor_Busy;
    // 启动通道1，随后会自动完成通道2和通道3的启动
    HAL_TIM_PWM_Start_IT(&htim8, TIM_CHANNEL_1);
}
/******************************************************************************************************************************************/
// 中断处理函数必须是先进行判断再进行自减，否则执行次数将会少1
// 中断处理函数
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    // 确保计数不会超过限定，导致数据溢出
    // 这个地方有问题！因为3个通道每个都可以进这个中断，导致应该输出20个波形但是其只输出7个！
    // 所以我在这里引入了标志位，这个定时器同一时刻只能控制1个通道，当通道标志位为忙的时候才可以进行计数
    if ((Robot_Motor1_PWM_execution_num > 0) && (Robot_Motor1_Ready == Robot_Motor_Busy)) Robot_Motor1_PWM_execution_num = Robot_Motor1_PWM_execution_num - 1;
    if ((Robot_Motor2_PWM_execution_num > 0) && (Robot_Motor2_Ready == Robot_Motor_Busy)) Robot_Motor2_PWM_execution_num = Robot_Motor2_PWM_execution_num - 1;
    if ((Robot_Motor3_PWM_execution_num > 0) && (Robot_Motor3_Ready == Robot_Motor_Busy)) Robot_Motor3_PWM_execution_num = Robot_Motor3_PWM_execution_num - 1;
    // 从这里开始是根据计数做对应PWM操作
    // 判断是否完成指定数量PWM波，并且确定当前状态是忙，这样就可以只执行一次而不会下次中断再进入这个判断
    if ((Robot_Motor1_PWM_execution_num == 0) && (Robot_Motor1_Ready == Robot_Motor_Busy)) {
        Robot_Motor1_Ready = Robot_Motor_Ready;
        Robot_Motor2_Ready = Robot_Motor_Busy;
        HAL_TIM_PWM_Stop_IT(&htim8, TIM_CHANNEL_1);
        HAL_TIM_PWM_Start_IT(&htim8, TIM_CHANNEL_2);
        // 这里添加return;的原因是这次启动通道2之后不需要再让执行次数-1了
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
}
/******************************************************************************************************************************************/
// 任务执行函数
// 任务函数
void StartRobotmainControlTask(void *argument)
{
    SYS_USE_DATA *SYS = (SYS_USE_DATA *)argument;
    // 这里初始化参数为20的原因是需要一个通道执行20周期PWM波
    // 准确的说是产生20个上升沿和20个下降沿，对于步进电机来说就是20个脉冲，最小转子转动20次
    // 过一段时间需要将对应角度做运算转换成对应PWM脉冲波，需要写计算函数！
    SYS->Robot_use_data.Motor1.PWM_execution_count = 20;
    SYS->Robot_use_data.Motor2.PWM_execution_count = 40;
    SYS->Robot_use_data.Motor3.PWM_execution_count = 20;
    // 你需要去考虑怎么才可以结合遥控器完成操作！
    //__HAL_TIM_SET_COMPARE
    for (;;) {
        osDelay(2000);
        // 启动PWM
        Start_Robot_PWM_Function(SYS);
    }
}

