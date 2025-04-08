#ifndef __MYTASK_H
#define __MYTASK_H

// 注意这里定义的数据发送和接受长度一定要足够！例如LED_AUTO就需要8*8=64！
#define UART1_DMA_RX_LEN 70
#define Set_Time_hours   23
#define Set_Time_minute  59
#define Set_Time_second  50

// 设置当前时间结构体，实现LCD动态显示时间
typedef struct
{
    unsigned char hours;
    unsigned char minute;
    unsigned char second;
} TIME_USE_DATA;

/**
 * @brief  UART通信数据管理结构体
 * @note   DMA接收数据生命周期管理：
 *         [接收] → Read_data → [处理] → Last_Read_data/LED_Read_data
 */
typedef struct {
    char Read_data[UART1_DMA_RX_LEN];      //!< 当前接收缓冲区（自动清零）
    char Last_Read_data[UART1_DMA_RX_LEN]; //!< 历史数据缓存（用于对比变更）
    char Response_Read_data[UART1_DMA_RX_LEN];
} USART_USE_DATA;
//蜂鸣器响应时间
typedef struct
{
    int Response_time;
} BEEP_USE_DATA;
// 每个电机对应的操作数据
typedef struct
{
    int speed;                   // 速度变量
    int laps_num;                // 自动模式下旋转的角度
    unsigned char start;         // 开启变量
    unsigned char direction;     // 方向变量
    unsigned char direction_num; // 方向选择中间变量
} MOTOR_USE_DATA;
// 在这里定义所有已有的电机
typedef struct
{
    MOTOR_USE_DATA MOTOR1;
    MOTOR_USE_DATA MOTOR2;
    MOTOR_USE_DATA MOTOR3;
} MOTOR_USE_TYPE;


/**
 * @brief  系统核心数据聚合结构体
 * @warning 多任务共享数据需原子操作保护
 */
typedef struct {
    TIME_USE_DATA Time_use_data;   //!< 时间管理系统（RTC模拟层）
    USART_USE_DATA usart_use_data; //!< 通信数据管理系统
    BEEP_USE_DATA Beep_use_data;
    MOTOR_USE_TYPE MOTOR_type;
} SYS_USE_DATA;


void StartLEDProcessedTaskFunction(void *argument);
void StartLEDWorkTaskFunction(void *argument);
void StartLCDDisplayTaskFunction(void *argument);
void StartTimeSetTaskFunction(void *argument);
void StartBeepTaskFunction(void *argument);
void Motor_one_1step(MOTOR_USE_DATA *motor);
void Motor_one_AUTOFunction(SYS_USE_DATA *SYS);
void StartMotor_oneFunction(void *argument);

#endif
