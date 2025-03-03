# 🚀 STM32F103ZET6-FreeRTOS-Project

## 📖 项目简介

基于Keil MDK 5.32开发的工业级嵌入式系统，实现以下核心功能：

- 🖥 TFT-LCD动态显示（320x240@16bit）
- 💡 双模式LED控制（手动/自动）
- 📡 高可靠串口通信（DMA+FreeRTOS队列）
- ⚙️ 完整的Keil MDK工程架构

## 🌟 核心特性

| 模块         | 技术亮点              | 性能指标             |
| ------------ | --------------------- | -------------------- |
| **开发环境** | Keil MDK 5.32         | AC6编译器优化等级-O2 |
| **驱动架构** | HAL库+FreeRTOS 10.4.6 | CMSIS-RTOS2封装层    |
| **编译系统** | 多目录结构化工程      | 支持Batch Build      |
| **调试支持** | ST-Link V2调试        | 实时变量监控         |

## 🛠️ 硬件要求

### 必需配置

| 组件     | 型号          | 接口说明     |
| -------- | ------------- | ------------ |
| 主控板   | STM32F103ZET6 | 全功能评估板 |
| 调试器   | ST-Link V2    | SWD接口      |
| LCD屏    | ILI9341控制器 | FSMC接口     |
| 串口模块 | CH340G        | 115200bps    |

## 🖥️ 开发环境配置

### 必需软件

```text
- Keil MDK 5.32 (包含ARM Compiler 6)
- STM32F1xx_DFP 2.4.0
- STM32CubeMX 6.6.1 (用于HAL库生成)
- FreeRTOSv202112.00 (CMSIS-RTOS2适配层)
```

## 工程结构

```
`STM32F103ZET6_FreeRTOS_Project/
├─MDK-ARM/                     # Keil工程核心目录
│  ├─FreeRTOSSTM32ZET6.uvprojx # 主工程文件
│  └─RTE/_FreeRTOSSTM32ZET6    # Runtime Environment配置
├─Core/                        # 用户代码
│  ├─Inc/                      # 头文件目录
│  │  └─user/                  # 用户自定义头文件
│  └─Src/                      # 源文件目录
│     └─user/                  # 用户应用代码
├─Drivers/                     # 官方驱动
│  ├─CMSIS/                    # Cortex核心支持
│  └─STM32F1xx_HAL_Driver/     # HAL库实现
├─Middlewares/                 # RTOS中间件
│  └─FreeRTOS/                 # FreeRTOS移植文件
│     └─portable/RVDS/ARM_CM3  # Keil专用移植层
└─.eide/                       # EIDE插件配置`
```

## ⚡ 快速开始

> 打开Keil工程
>
> 双击打开 MDK-ARM/FreeRTOSSTM32ZET6.uvprojx
>
> 1. Project → Options for Target 'FreeRTOSSTM32ZET6'
> 2. Device 选项卡确认选择 STM32F103ZE
> 3. C/C++ 选项卡检查预定义宏：
>
> Ctrl+F7 或点击Build按钮 → 输出窗口应显示: *** Using Compiler 'V6.16', folder: 'C:\Keil_v5\ARM\ARMCLANG\bin' Build target 'FreeRTOSSTM32ZET6'
>
> 1. 连接ST-Link调试器
> 2. Flash → Download (快捷键F8)
> 3. 观察输出窗口提示：

## 📜 许可协议

本项目采用 **BSD 3-Clause License**，完整许可文本见 [LICENSE](LICENSE) 文件。

------

> 🔧 技术支持：[L861149396@163.com](mailto:L861149396@163.com)
> 🐛 问题跟踪：[Issues](https://github.com/Cossiant/STM32F103ZET6_FreeRTOS_Project/issues)
> 📅 最后更新：2025-03-03
> ![Keil Version](https://img.shields.io/badge/Keil_MDK-5.32-blue)
