# 实验4：USART 轮询、中断、DMA

## 本次完成范围

已完成 CubeMX 配置、HAL 工程、三个 Keil 编译目标及真实软件仿真。未烧录开发板，未执行两台电脑之间的实物文件传输，也未测试 Windows 串口助手是否丢数据。

任务三仅完成串口字节轨迹观察和寄存器波特率核算，**尚未完成 TX 引脚逐位波形验证**。当前 Keil STM32F103 模拟器提供 S1IN/S1OUT 字节级串口，实际测试 PORTA 的 PA9 位不随发送变化，USART1_TX 不是可用逻辑分析仪信号。保存的 S1OUT 曲线表示字节数值，不是串口电平；不能用其高低变化冒充起始位、数据位或停止位。

## 软件与配置

- STM32CubeMX 6.18.1：E:\STM32CubeMX，官方安装包及 STM32CubeF1 V1.8.0 固件已校验下载摘要。
- Keil MDK 5.24.2：复用 E:\keli5，ARM Compiler 5.06 update 5，STM32F1xx_DFP 2.2.0。
- MCU：STM32F103C8T6；HSE 8 MHz，PLL ×9，SYSCLK/PCLK2 72 MHz，PCLK1 36 MHz。
- USART1：PA9 TX、PA10 RX，115200，8 数据位、1 停止位、无校验、无流控。
- DMA1 Channel4：USART1_TX，Normal，内存递增、字节宽度；DMA 和 USART1 中断均使能。
- 保留 SWD：PA13/PA14。

## 打开与编译

用 Keil 打开 MDK-ARM/Lab4_USART.uvprojx。工具栏选择目标，F7 编译：

| 目标 | 功能 | 编译结果 |
| --- | --- | --- |
| Polling | 轮询接收指令，阻塞发送 | 0 错误、0 警告 |
| Interrupt | 单字节接收中断、发送中断 | 0 错误、0 警告 |
| DMA | DMA 连续发送 | 0 错误、0 警告 |

应用代码集中在 Core/Src/app.c，主函数只调用 App_Start 和 App_Step。三个目标用 APP_MODE=1/2/3 选择实现，不需要复制三份代码。所有版本均没有 HAL_Delay。

发送文本为 `hello windows！` 加 CR/LF。全角叹号使用 UTF-8（EF BC 81），每行共 18 字节，不发送字符串末尾 NUL。旧版 Keil UART 窗口按本地字符集显示时，叹号可能乱码；这是显示编码问题，不能据此判断串口丢字节。真实串口助手应选择 UTF-8 或十六进制检查。

轮询、中断目标：输入 `#` 暂停，输入 `*` 恢复。不截断已经启动的一行。DMA 目标用于连续发送，无控制字符功能。

不要直接用 CubeMX 重新生成覆盖当前 MDK 工程：当前三目标及模拟器设置是生成后配置的，重新生成可能重置这些设置。IOC 已保留供查看配置。

## 复现软件仿真

1. 选择目标，F7 编译，Ctrl+F5 进入调试（已选 Use Simulator）。
2. 打开 View → Serial Windows → UART #1，以及 View → Command Window。
3. 在 Command 输入 `INCLUDE Simulation.ini`。若当前工作目录不在 MDK-ARM，使用完整路径 `INCLUDE "E:\Embedded\Test\4\project\Lab4_USART\MDK-ARM\Simulation.ini"`。
4. Polling/Interrupt 输入 `control_test()`；DMA 输入 `dma_test()`。然后按 F5 运行；脚本会自动停止。
5. 脚本以 S1IN 写入 0x23/0x2A 来模拟真正的接收事件，没有直接修改 sending。S1TIME=1 开启串口时序模拟。
6. 如需日志，在运行前输入 `LOG > 文件名.log`，结束后输入 `LOG OFF`。

原始结果：

| 目标 | 实际观察 |
| --- | --- |
| Polling | 12 帧 → 收到 # 后 13 帧；保持 20 ms 仍 13 帧；收到 * 后 25 帧；2 条指令，0 错误 |
| Interrupt | 12 帧 → 收到 # 后 13 帧；保持 20 ms 仍 13 帧；收到 * 后 25 帧；2 条指令，0 错误 |
| DMA | 20 ms 时 12 帧，之后 30 ms 增至 31 帧；0 错误 |

对应日志在 MDK-ARM/*_Simulation.log，编译日志在 Objects/各目标/*.build_log.htm。

逻辑分析仪 Setup → Import Signal Definitions 可导入 Serial_Signals.uvl；Load 可打开 Interrupt_Serial_Trace.uvla。S1OUT 为实际模拟器输出字节值，sending 为软件发送允许状态，PORTA 按 bit9 掩码显示用于记录模拟器引脚波形限制。轨迹来自实际运行，不是人工生成的串口波形。

## 波特率与理论时间

仿真读取 SystemCoreClock=72000000、USART1_BRR=0x271=625，USART1 使用 PCLK2=72 MHz。因此寄存器对应波特率为 72000000/625=115200 bps；理想位宽 8.68056 μs；8N1 一字节 10 位，时间 86.80556 μs；18 字节一行理想串行时间 1.5625 ms，另有软件启动间隔。

这些是寄存器核算值，不是 PA9 位波形实测值。实际晶振误差、Windows 接收和物理线路仍需后续实物验证。

文件传输理论时间 t=文件字节数×10/波特率（8N1），实际还包含软件间隔及协议开销。CH340 通常完成 USB↔UART 转换；RS-232 还需要相应电平收发器，不能将 RS-232 正负电压直接连接 STM32 的 3.3V UART 引脚。未来接线前必须核对模块输出类型。双方独立供电时一般不用互接 VCC，但非隔离 UART/RS-232 通信需要可靠参考地；未接 GND 偶尔工作可能依赖其他接地路径，不能作为可靠连接。

## 参考资料阅读记录

- https://blog.csdn.net/cleveryoga/article/details/121344003 — 已读，文件大小/波特率/时间；本工程按 8N1 的 10 位开销计算。
- https://blog.csdn.net/vic_to_ry/article/details/110451036 — 已读，STM32 USART。
- https://blog.csdn.net/qq_47281915/article/details/121024427 — 已读，中断背景。
- https://www.cnblogs.com/breezy-ye/articles/12157442.html — 已读，DMA。
- https://blog.csdn.net/as480133937/article/details/104827639/ — 已读，HAL/DMA。
- https://blog.csdn.net/qq_47281915/article/details/121053903 — 仅阅读可访问部分，存在付费限制。
- https://www.pianshen.com/article/8285571527/ — DNS 访问失败，未读正文。
- 三份 PDF 仅提供了文件名，未收到文件，未读正文。

本说明是工程交接和验证记录，不是最终实验报告。后续仍需补任务三位波形验证、实物测试和正式提交材料。
