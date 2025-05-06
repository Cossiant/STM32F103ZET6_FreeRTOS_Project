/**
 ****************************************************************************************************
 * @file        remote.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-25
 * @brief       红外遥控解码 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 STM32F103开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20200425
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include "remote.h"

extern osSemaphoreId_t LCD_refresh_gsemHandle;

TIM_HandleTypeDef g_tim4_handle; /* 定时器4句柄 */

/**
 * @brief       红外遥控初始化
 *   @note      设置IO以及定时器的输入捕获
 * @param       无
 * @retval      无
 */
void remote_init(void)
{
    TIM_IC_InitTypeDef tim_ic_init_handle;

    g_tim4_handle.Instance           = REMOTE_IN_TIMX;     /* 通用定时器4 */
    g_tim4_handle.Init.Prescaler     = (72 - 1);           /* 预分频器,1M的计数频率,1us加1 */
    g_tim4_handle.Init.CounterMode   = TIM_COUNTERMODE_UP; /* 向上计数器 */
    g_tim4_handle.Init.Period        = 10000;              /* 自动装载值 */
    g_tim4_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_IC_Init(&g_tim4_handle);

    /* 初始化TIM4输入捕获参数 */
    tim_ic_init_handle.ICPolarity  = TIM_ICPOLARITY_RISING;                            /* 上升沿捕获 */
    tim_ic_init_handle.ICSelection = TIM_ICSELECTION_DIRECTTI;                         /* 映射到TI4上 */
    tim_ic_init_handle.ICPrescaler = TIM_ICPSC_DIV1;                                   /* 配置输入分频，不分频 */
    tim_ic_init_handle.ICFilter    = 0x03;                                             /* IC1F=0003 8个定时器时钟周期滤波 */
    HAL_TIM_IC_ConfigChannel(&g_tim4_handle, &tim_ic_init_handle, REMOTE_IN_TIMX_CHY); /* 配置TIM4通道4 */
    HAL_TIM_IC_Start_IT(&g_tim4_handle, REMOTE_IN_TIMX_CHY);                           /* 开始捕获TIM的通道值 */
    __HAL_TIM_ENABLE_IT(&g_tim4_handle, TIM_IT_UPDATE);                                /* 使能更新中断 */
}

/**
 * @brief       定时器4底层驱动，时钟使能，引脚配置
 * @param       htim:定时器句柄
 * @note        此函数会被HAL_TIM_IC_Init()调用
 * @retval      无
 */
void HAL_TIM_IC_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == REMOTE_IN_TIMX) {
        GPIO_InitTypeDef gpio_init_struct;

        REMOTE_IN_GPIO_CLK_ENABLE();     /* 红外接入引脚GPIO时钟使能 */
        REMOTE_IN_TIMX_CHY_CLK_ENABLE(); /* 定时器时钟使能 */
        __HAL_AFIO_REMAP_TIM4_DISABLE(); /* 这里用的是PB9/TIM4_CH4，参考AFIO_MAPR寄存器的设置 */

        gpio_init_struct.Pin   = REMOTE_IN_GPIO_PIN;
        gpio_init_struct.Mode  = GPIO_MODE_AF_INPUT;           /* 复用输入 */
        gpio_init_struct.Pull  = GPIO_PULLUP;                  /* 上拉 */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;         /* 高速 */
        HAL_GPIO_Init(REMOTE_IN_GPIO_PORT, &gpio_init_struct); /* 初始化定时器通道引脚 */

        HAL_NVIC_SetPriority(REMOTE_IN_TIMX_IRQn, 1, 3); /* 设置中断优先级，抢占优先级1，子优先级3 */
        HAL_NVIC_EnableIRQ(REMOTE_IN_TIMX_IRQn);         /* 开启ITM4中断 */
    }
}

/* 遥控器接收状态
 * [7]  : 收到了引导码标志
 * [6]  : 得到了一个按键的所有信息
 * [5]  : 保留
 * [4]  : 标记上升沿是否已经被捕获
 * [3:0]: 溢出计时器
 */
uint8_t g_remote_sta   = 0;
uint32_t g_remote_data = 0; /* 红外接收到的数据 */
uint8_t g_remote_cnt   = 0; /* 按键按下的次数 */

/**
 * @brief       定时器中断服务函数
 * @param       无
 * @retval      无
 */
void REMOTE_IN_TIMX_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_tim4_handle); /* 定时器共用处理函数 */
}

/**
 * @brief       定时器更新中断回调函数
 * @param       htim:定时器句柄
 * @retval      无
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) {
        HAL_IncTick();
    }
    if (htim->Instance == REMOTE_IN_TIMX) {
        if (g_remote_sta & 0x80) /* 上次有数据被接收到了 */
        {
            g_remote_sta &= ~0X10; /* 取消上升沿已经被捕获标记 */

            if ((g_remote_sta & 0X0F) == 0X00) {
                g_remote_sta |= 1 << 6; /* 标记已经完成一次按键的键值信息采集 */
            }

            if ((g_remote_sta & 0X0F) < 14) {
                g_remote_sta++;
            } else {
                g_remote_sta &= ~(1 << 7); /* 清空引导标识 */
                g_remote_sta &= 0XF0;      /* 清空计数器 */
            }
        }
    }
}

/**
 * @brief       定时器输入捕获中断回调函数
 * @param       htim:定时器句柄
 * @retval      无
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == REMOTE_IN_TIMX) {
        uint16_t dval; /* 下降沿时计数器的值 */

        if (RDATA) /* 上升沿捕获 */
        {
            __HAL_TIM_SET_CAPTUREPOLARITY(&g_tim4_handle, REMOTE_IN_TIMX_CHY, TIM_INPUTCHANNELPOLARITY_FALLING); // CC4P=1 设置为下降沿捕获
            __HAL_TIM_SET_COUNTER(&g_tim4_handle, 0);                                                            /* 清空定时器值 */
            g_remote_sta |= 0X10;                                                                                /* 标记上升沿已经被捕获 */
        } else                                                                                                   /* 下降沿捕获 */
        {
            dval = HAL_TIM_ReadCapturedValue(&g_tim4_handle, REMOTE_IN_TIMX_CHY);                               /* 读取CCR4也可以清CC4IF标志位 */
            __HAL_TIM_SET_CAPTUREPOLARITY(&g_tim4_handle, REMOTE_IN_TIMX_CHY, TIM_INPUTCHANNELPOLARITY_RISING); /* 配置TIM4通道4上升沿捕获 */

            if (g_remote_sta & 0X10) /* 完成一次高电平捕获 */
            {
                if (g_remote_sta & 0X80) /* 接收到了引导码 */
                {

                    if (dval > 300 && dval < 800) /* 560为标准值,560us */
                    {
                        g_remote_data >>= 1;               /* 右移一位 */
                        g_remote_data &= ~(0x80000000);    /* 接收到0 */
                    } else if (dval > 1400 && dval < 1800) /* 1680为标准值,1680us */
                    {
                        g_remote_data >>= 1;               /* 右移一位 */
                        g_remote_data |= 0x80000000;       /* 接收到1 */
                    } else if (dval > 2000 && dval < 3000) /* 得到按键键值增加的信息 2500为标准值2.5ms */
                    {
                        g_remote_cnt++;       /* 按键次数增加1次 */
                        g_remote_sta &= 0XF0; /* 清空计时器 */
                    }
                } else if (dval > 4200 && dval < 4700) /* 4500为标准值4.5ms */
                {
                    g_remote_sta |= 1 << 7; /* 标记成功接收到了引导码 */
                    g_remote_cnt = 0;       /* 清除按键次数计数器 */
                }
            }

            g_remote_sta &= ~(1 << 4);
        }
    }
}

/**
 * @brief       处理红外按键(类似按键扫描)
 * @param       无
 * @retval      0   , 没有任何按键按下
 *              其他, 按下的按键键值
 */
uint8_t remote_scan(void)
{
    uint8_t sta = 0;
    uint8_t t1, t2;

    if (g_remote_sta & (1 << 6)) /* 得到一个按键的所有信息了 */
    {
        t1 = g_remote_data;               /* 得到地址码 */
        t2 = (g_remote_data >> 8) & 0xff; /* 得到地址反码 */

        if ((t1 == (uint8_t)~t2) && t1 == REMOTE_ID) /* 检验遥控识别码(ID)及地址 */
        {
            t1 = (g_remote_data >> 16) & 0xff;
            t2 = (g_remote_data >> 24) & 0xff;

            if (t1 == (uint8_t)~t2) {
                sta = t1; /* 键值正确 */
            }
        }

        if ((sta == 0) || ((g_remote_sta & 0X80) == 0)) /* 按键数据错误/遥控已经没有按下了 */
        {
            g_remote_sta &= ~(1 << 6); /* 清除接收到有效按键标识 */
            g_remote_cnt = 0;          /* 清除按键次数计数器 */
        }
    }

    return sta;
}
// 读取并处理红外遥控器收到的信号
void Read_remote_data(REMOTE_USE_DATA *data)
{
    data->key = remote_scan();
    if (data->key) {
        // lcd_show_num(86, 230, key, 3, 16, BLUE);          /* 显示键值 */
        // lcd_show_num(86, 250, g_remote_cnt, 3, 16, BLUE); /* 显示按键次数 */
        data->g_remote_cnt = g_remote_cnt;
        switch (data->key) {
            case 0:
                data->str = "ERROR";
                break;
            case 69:
            //临时注释，以方便按钮控制
                //data->str = "POWER";
                break;
            case 70:
                data->str = "UP";
                break;
            case 64:
                data->str = "PLAY";
                break;
            case 71:
                data->str = "ALIENTEK";
                break;
            case 67:
                data->str = "RIGHT";
                break;
            case 68:
                data->str = "LEFT";
                break;
            case 7:
                data->str = "VOL-";
                break;
            case 21:
                data->str = "DOWN";
                break;
            case 9:
                data->str = "VOL+";
                break;
            case 22:
                data->str = "1";
                break;
            case 25:
                data->str = "2";
                break;
            case 13:
                data->str = "3";
                break;
            case 12:
                data->str = "4";
                break;
            case 24:
                data->str = "5";
                break;
            case 94:
                data->str = "6";
                break;
            case 8:
                data->str = "7";
                break;
            case 28:
                data->str = "8";
                break;
            case 90:
                data->str = "9";
                break;
            case 66:
                data->str = "0";
                break;
            case 74:
                data->str = "DELETE";
                break;
        }
        // lcd_fill(86, 270, 116 + 8 * 8, 170 + 16, WHITE);  /* 清楚之前的显示 */
        // lcd_show_string(86, 270, 200, 16, 16, str, BLUE); /* 显示SYMBOL */
    } else
        osDelay(100);
    // 如果上一次统计的按下的时间和这一次统计的时间不一致，就刷新LCD
    if (data->g_remote_cnt != data->old_remote_cnt) {
        // 释放LCD刷新信号量
        osSemaphoreRelease(LCD_refresh_gsemHandle);
        data->old_remote_cnt = data->g_remote_cnt;
    }
}
