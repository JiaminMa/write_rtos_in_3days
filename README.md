# BLOG:https://blog.csdn.net/u011280717/article/details/79337791


遥想当年刚学习操作系统的时候，很难理解教科书中关于线程/进程的描述。原因还是在于操作系统书上的内容太过抽象，对于一个没有看过内核代码的初学者来说，很难理解各种数据结构的调度。后来自己也买了一些造轮子的书，照着好几本书也造了几个玩具操作系统，有X86，有ARM的。经过实践之后回头再去看操作系统的书，才恍然大悟操作系统书中所写的知识点。<br/>
看了许多操作系统实践类的书籍后，有些书只是浅尝辄止，试图用300页将通用操作系统各个模块都讲了一遍，这一类书帮助读者理解操作系统还是有限；而有些书写的确实很不错，内容详实，然而动辄上千页，让读者望而生畏，但是读完并且照着书写完一个玩具OS的话，绝对对OS的理解有很大帮助。这里推荐郑刚老师写的《操作系统真相还原》，本人觉得这本书非常好，深入浅出。那我为何还要写这篇博客呢？我觉得操作系统内核最核心，且初学者最难理解的部分莫过于进程/线程(在RTOS中称为任务)，所以本文试图写一个只有不到1000多行代码的RTOS来帮助读者理解操作系统核心部分。一般小型RTOS中并没有虚拟内存管理，文件系统，设备管理等模块，这样减小读者的负担，更好理解操作系统的核心部分（进程/线程/任务），在这之后再去学习其他的模块必然事半功倍。所以本文仅仅作为一篇入门读物，若是能帮助各位看官进入操作系统的大门，也算是功德无量。当然在下才疏学浅，难免有错误的地方，大神发现的话请指出。<br/>
话不多说，直接进入正题。

# 预备知识
虽然本文旨在一篇入门的教程，但希望读者具有以下的预备知识，否则读起来会有诸多不顺。

- C语言，至少熟悉指针的用法
- ARM Cortex M3/M4架构（后面简称CM3）
	如果没有学习过ARM CM3的读者，推荐阅读CORTEX_M3权威指南，第一，二，三，四，五，六章。
- linux 操作，简单的shell，Makefile即可
- RTOS 简介
 [https://baike.baidu.com/item/%E5%AE%9E%E6%97%B6%E6%93%8D%E4%BD%9C%E7%B3%BB%E7%BB%9F/357530?fr=aladdin&fromid=987080&fromtitle=RTOS](https://baike.baidu.com/item/%E5%AE%9E%E6%97%B6%E6%93%8D%E4%BD%9C%E7%B3%BB%E7%BB%9F/357530?fr=aladdin&fromid=987080&fromtitle=RTOS "RTOS 百度百科")

# 源码GIT
https://github.com/JiaminMa/write_rtos_in_3days.git

# 环境搭建
本文使用qemu虚拟机来仿真arm cortex m3的芯片，QEMU可以自己编译，也可以下载，我已经编译好一份QEMU，各位看官可以直接clone该git然后使用tools里面的qemu即可。编译器使用的是GNU的arm-none-eabi-gcc，这个可以使用sudo apt-get install gcc-arm-none-eabi
下载到。哦对了，我的linux用的是ubuntu16 64位，希望各位看官可以用相同版本的ubuntu，否则可能会有一些环境的问题，概不负责。以下乃环境搭建参考步骤：

1. git clone https://github.com/JiaminMa/write_rtos_in_3days.git
2. vim ~/.bashrc
3. export PATH=$PATH:/mnt/e/write\_rtos\_in\_3days/tools， 这一步每个人的配置不一样，要把write\_rtos\_in\_3days/tools设置为读者自己的tools的目录
4. source ~/.bashrc
5. sudo apt-get install gcc-arm-none-eabi


# 1 QEMU ARM CORTEX M3入门
qemu-system-arm对于CORTEX M的芯片官方只支持了Stellaris LM3S6965EVB和Stellaris LM3S811EVB，本文使用了LM3S6965EVB作为开发平台。非官方的有STM32等其他CM3/4的芯片及开发板，但这里选用官方的支持更稳定一些。我在doc目录下放了LM3S6965的芯片手册，感兴趣的读者可以自己看，实际上本文在写嵌入式操作系统中，除了UART并没有使用到LM3S6965的外设，大部分代码都是针对ARM CM3内核的操作，所以并不需要对LM3S6965EVB很清楚。

## 打印Hello World
没错，本章就是要在qemu平台上打印最喜闻乐见的Hello world。本节的完整代码在01\_hello\_world中。
### 异常向量表
当CM3内核响应了一个发生的异常后，对应的异常服务例程(ESR)就会执行。为了决定ESR的入口地址，CM3使用了“向量表查表机制”。这里使用一张向量表。向量表其实是一个WORD（32位整数）数组，每个下标对应一种异常，该下标元素的值则是该ESR的入口地址。向量表在地址空间中的位置是可以设置的，通过NVIC中的一个重定位寄存器来指出向量表的地址。在复位后，该寄存器的值为0。因此，在地址0处必须包含一张向量表，用于初始时的异常分配。 


| 异常类型 | 表项地址偏移量 | 异常向量 |
|-|-|-|
|0|0x00|MSP初始值|
|1|0x04|复位函数入口|
|2|0x08|NMI|
|3|0x0C|Hard Fault|
|4|0x10|MemManage Fault|
|5|0x14|总线Fault|
|6|0x18|用法Fault|
|7-10|0x1c-0x28|保留|
|11|0x2c|SVC|
|12|0x30|调试监视器|
|13|0x34|保留|
|14|0x38|PendSV|
|15|0x3c|SysTick|
|16|0x40|IRQ #0|
|17|0x44|IRQ #1|
|18-255|0x48-0x3ff|IRQ#2-#239|

举个例子，如果发生了异常11（SVC），则NVIC会计算出偏移移量是11x4=0x2C，然后从那里取出服务例程的入口地址并跳入。要注意的是这里有个另类：0号类型并不是什么入口地址，而是给出了复位后MSP的初值。 ***Cortex M3权威指南P43 3.5向量表>***

本文中，int_vector.c中包含了异常向量表，源代码如下。我们将MSP(主栈)的值设为0x2000c000，程序入口为main，NMI中断和HardFault中断分别为自己处理函数，其他异常以及中断暂时全部使用IntDefaultHandler。

```c

static void NmiSR(void){
    while(1);
}

static void FaultISR(void){
    while(1);
}

static void IntDefaultHandler(void){
    while(1);
}


__attribute__ ((section(".isr_vector")))void (*g_pfnVectors[])(void) =
{
    0x2000c000,                             // StackPtr, set in RestetISR
    main,                                    // The reset handler
    NmiSR,                                  // The NMI handler
    FaultISR,                               // The hard fault handler
    IntDefaultHandler,                      // The MPU fault handler
    IntDefaultHandler,                      // The bus fault handler
    IntDefaultHandler,                      // The usage fault handler
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    IntDefaultHandler,                      // SVCall handler
    IntDefaultHandler,                      // Debug monitor handler
    0,                                      // Reserved
    IntDefaultHandler,                      // The PendSV handler
    IntDefaultHandler,                      // The SysTick handler
    IntDefaultHandler,                      // GPIO Port A
    IntDefaultHandler,                      // GPIO Port B
    IntDefaultHandler,                      // GPIO Port C
    IntDefaultHandler,                      // GPIO Port D
    IntDefaultHandler,                      // GPIO Port E
    IntDefaultHandler,                      // UART0 Rx and Tx
    IntDefaultHandler,                      // UART1 Rx and Tx
    IntDefaultHandler,                      // SSI0 Rx and Tx
    IntDefaultHandler,                      // I2C0 Master and Slave
    IntDefaultHandler,                      // PWM Fault
    IntDefaultHandler,                      // PWM Generator 0
    IntDefaultHandler,                      // PWM Generator 1
    IntDefaultHandler,                      // PWM Generator 2
    IntDefaultHandler,                      // Quadrature Encoder 0
    IntDefaultHandler,                      // ADC Sequence 0
    IntDefaultHandler,                      // ADC Sequence 1
    IntDefaultHandler,                      // ADC Sequence 2
    IntDefaultHandler,                      // ADC Sequence 3
    IntDefaultHandler,                      // Watchdog timer
    IntDefaultHandler,                      // Timer 0 subtimer A
    IntDefaultHandler,                      // Timer 0 subtimer B
    IntDefaultHandler,                      // Timer 1 subtimer A
    IntDefaultHandler,                      // Timer 1 subtimer B
    IntDefaultHandler,                      // Timer 2 subtimer A
    IntDefaultHandler,                      // Timer 2 subtimer B
    IntDefaultHandler,                      // Analog Comparator 0
    IntDefaultHandler,                      // Analog Comparator 1
    IntDefaultHandler,                      // Analog Comparator 2
    IntDefaultHandler,                      // System Control (PLL, OSC, BO)
    IntDefaultHandler,                      // FLASH Control
    IntDefaultHandler,                      // GPIO Port F
    IntDefaultHandler,                      // GPIO Port G
    IntDefaultHandler,                      // GPIO Port H
    IntDefaultHandler,                      // UART2 Rx and Tx
    IntDefaultHandler,                      // SSI1 Rx and Tx
    IntDefaultHandler,                      // Timer 3 subtimer A
    IntDefaultHandler,                      // Timer 3 subtimer B
    IntDefaultHandler,                      // I2C1 Master and Slave
    IntDefaultHandler,                      // Quadrature Encoder 1
    IntDefaultHandler,                      // CAN0
    IntDefaultHandler,                      // CAN1
    IntDefaultHandler,                      // CAN2
    IntDefaultHandler,                      // Ethernet
    IntDefaultHandler,                      // Hibernate
    IntDefaultHandler,                      // USB0
    IntDefaultHandler,                      // PWM Generator 3
    IntDefaultHandler,                      // uDMA Software Transfer
    IntDefaultHandler,                      // uDMA Error
    IntDefaultHandler,                      // ADC1 Sequence 0
    IntDefaultHandler,                      // ADC1 Sequence 1
    IntDefaultHandler,                      // ADC1 Sequence 2
    IntDefaultHandler,                      // ADC1 Sequence 3
    IntDefaultHandler,                      // I2S0
    IntDefaultHandler,                      // External Bus Interface 0
    IntDefaultHandler,                      // GPIO Port J
    IntDefaultHandler,                      // GPIO Port K
    IntDefaultHandler,                      // GPIO Port L
    IntDefaultHandler,                      // SSI2 Rx and Tx
    IntDefaultHandler,                      // SSI3 Rx and Tx
    IntDefaultHandler,                      // UART3 Rx and Tx
    IntDefaultHandler,                      // UART4 Rx and Tx
    IntDefaultHandler,                      // UART5 Rx and Tx
    IntDefaultHandler,                      // UART6 Rx and Tx
    IntDefaultHandler,                      // UART7 Rx and Tx
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    IntDefaultHandler,                      // I2C2 Master and Slave
    IntDefaultHandler,                      // I2C3 Master and Slave
    IntDefaultHandler,                      // Timer 4 subtimer A
    IntDefaultHandler,                      // Timer 4 subtimer B
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    IntDefaultHandler,                      // Timer 5 subtimer A
    IntDefaultHandler,                      // Timer 5 subtimer B
    IntDefaultHandler,                      // Wide Timer 0 subtimer A
    IntDefaultHandler,                      // Wide Timer 0 subtimer B
    IntDefaultHandler,                      // Wide Timer 1 subtimer A
    IntDefaultHandler,                      // Wide Timer 1 subtimer B
    IntDefaultHandler,                      // Wide Timer 2 subtimer A
    IntDefaultHandler,                      // Wide Timer 2 subtimer B
    IntDefaultHandler,                      // Wide Timer 3 subtimer A
    IntDefaultHandler,                      // Wide Timer 3 subtimer B
    IntDefaultHandler,                      // Wide Timer 4 subtimer A
    IntDefaultHandler,                      // Wide Timer 4 subtimer B
    IntDefaultHandler,                      // Wide Timer 5 subtimer A
    IntDefaultHandler,                      // Wide Timer 5 subtimer B
    IntDefaultHandler,                      // FPU
    IntDefaultHandler,                      // PECI 0
    IntDefaultHandler,                      // LPC 0
    IntDefaultHandler,                      // I2C4 Master and Slave
    IntDefaultHandler,                      // I2C5 Master and Slave
    IntDefaultHandler,                      // GPIO Port M
    IntDefaultHandler,                      // GPIO Port N
    IntDefaultHandler,                      // Quadrature Encoder 2
    IntDefaultHandler,                      // Fan 0
    0,                                      // Reserved
    IntDefaultHandler,                      // GPIO Port P (Summary or P0)
    IntDefaultHandler,                      // GPIO Port P1
    IntDefaultHandler,                      // GPIO Port P2
    IntDefaultHandler,                      // GPIO Port P3
    IntDefaultHandler,                      // GPIO Port P4
    IntDefaultHandler,                      // GPIO Port P5
    IntDefaultHandler,                      // GPIO Port P6
    IntDefaultHandler,                      // GPIO Port P7
    IntDefaultHandler,                      // GPIO Port Q (Summary or Q0)
    IntDefaultHandler,                      // GPIO Port Q1
    IntDefaultHandler,                      // GPIO Port Q2
    IntDefaultHandler,                      // GPIO Port Q3
    IntDefaultHandler,                      // GPIO Port Q4
    IntDefaultHandler,                      // GPIO Port Q5
    IntDefaultHandler,                      // GPIO Port Q6
    IntDefaultHandler,                      // GPIO Port Q7
    IntDefaultHandler,                      // GPIO Port R
    IntDefaultHandler,                      // GPIO Port S
    IntDefaultHandler,                      // PWM 1 Generator 0
    IntDefaultHandler,                      // PWM 1 Generator 1
    IntDefaultHandler,                      // PWM 1 Generator 2
    IntDefaultHandler,                      // PWM 1 Generator 3
    IntDefaultHandler                       // PWM 1 Fault
};

```

### main函数
CM3内核从异常向量表中取出MSP，然后设置MSP后就跳到reset向量中，在这里是main函数，其启动过程如下图所示。main函数的实现在main.c中，源代码如下，非常简单，往串口数据寄存器中写数据打印Hello World，然后就while(1)循环。由于这是QEMU虚拟机，所以并不需要对串口进行初始化等操作，直接往DR寄存器里写数据即可打印出字符，在真实的硬件这么做是不行的，必须初始化串口的时钟已经相应的寄存器来配置其工作模式。

![](https://i.imgur.com/WT9joSC.gif)

***main.c***
```c
#include <stdint.h>
volatile uint32_t * const UART0DR = (uint32_t *)0x4000C000;

void send_str(char *s)
{
    while(*s != '\0') {
        *UART0DR = *s++;
    }
}

void main()
{
    send_str("hello world\n");
    while(1);
}
```
### 存储分布
CM3的存储器映射是相对固定的，具体可以参看《CORTEX_M3 权威指南》84页的图5.1。本文中的存储分布如下表所示，0x0-0x40000为只读存储,即FLASH，0x20000000-0x20040000为SRAM区。FLASH和SRAM分别是256K。

|内存地址|存储区域|
|-|-|
|0x0-0x400|异常向量表|
|0x400-0x40000|代码段，只读数据段|
|0x20000000-0x20004000|数据段，bss段|
|0x20004000-0x20008000|进程堆栈段(PSP)|
|0x20008000-0x2000c000|主栈段(MSP)|

具体实现参看链接文件rtos.ld,链接文件在后面的文章不会改动，所以只需要记住即可。

***rtos.ld***
```c
MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 256K
    SRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 256K
}

SECTIONS
{
    .text :
    {
        _text = .;
        KEEP(*(.isr_vector))
        *(.text*)
        *(.rodata*)
        _etext = .;
    } > FLASH

    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.gnu.linkonce.armexidx.*)
    }

    .data : AT(ADDR(.text) + SIZEOF(.text))
    {
        _data = .;
        *(vtable)
        *(.data*)
        _edata = .;
    } > SRAM

    .bss :
    {
        _bss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > SRAM

    . = ALIGN(32);          
    _p_stack_bottom = .;
    . = . + 0x4000;
    _p_stack_top = 0x20008000;
    . = . + 0x4000;         
    _stack_top = 0x2000c000; 
}

```
### Makefile
Makefile 指定了编译器，编译选项以及编译命令等，在后续章节中，只需要objs := 即可，当加入一个新的源文件只需要在obj后面添加相应的.o即可。比如新建了test.c，那么改成objs := int_vector.o main.o test.o即可。这里不解释Makefile的原理，如果有不熟悉的读者请自行学习Makefile的规则，网上关于Makefile的好教程有许多。
***Makefile***
```
TOOL_CHAIN = arm-none-eabi-
CC = ${TOOL_CHAIN}gcc
AS = ${TOOL_CHAIN}as
LD = ${TOOL_CHAIN}ld
OBJCOPY = ${TOOL_CHAIN}objcopy
OBJDUMP = $(TOOL_CHAIN)objdump

CFLAGS		:= -Wall -g -fno-builtin -gdwarf-2 -gstrict-dwarf -mcpu=cortex-m3 -mthumb -nostartfiles  --specs=nosys.specs -std=c11 \
				-O0 -Iinclude
LDFLAGS		:= -g

objs := int_vector.o main.o

rtos.bin: $(objs)
	${LD} -T rtos.ld -o rtos.elf $^
	${OBJCOPY} -O binary -S rtos.elf $@
	${OBJDUMP} -D -m arm rtos.elf > rtos.dis

run: $(objs)
	${LD} -T rtos.ld -o rtos.elf $^
	${OBJCOPY} -O binary -S rtos.elf rtos.bin
	${OBJDUMP} -D -m arm rtos.elf > rtos.dis
	qemu-system-arm -M lm3s6965evb --kernel rtos.bin -nographic

debug: $(objs)
	${LD} -T rtos.ld -o rtos.elf $^
	${OBJCOPY} -O binary -S rtos.elf rtos.bin
	${OBJDUMP} -D -m arm rtos.elf > rtos.dis
	qemu-system-arm -M lm3s6965evb --kernel rtos.bin -nographic -s -S

%.o:%.c
	${CC} $(CFLAGS) -c -o $@ $<

%.o:%.s
	${CC} $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o *.elf *.bin *.dis

```
###执行/调试
好了，终于把所有的源文件，链接文件和Makefile搞定了，运行一把。可以看到以下打印，那么说明执行正确。
![](https://i.imgur.com/mlest8u.png)

如果需要调试的话,执行make debug，然后在另外一个窗口使用arm-linux-gdb调试，如下图所示
![](https://i.imgur.com/e31DHpV.png)

## CM3进阶
本节代码在02_cm3文件夹下

### 异常向量表改动
在完成了hello world后，我们可以实现CM3更多的功能了。我们要把常用的CM3的操作实现一把。首先改写int_vector.c。因为在进入c函数之前需要做一些栈的操作，所以讲reset handler从main换成reset_handler, reset_handler在cm3_s.s中实现。还有就是将会实现sys_tick的中断服务函数。这里有细心的哥们会问为什么reset_handler + 1。原因是对于CM3的thumb code指令集地址最低位必须为1，而reset_handler定义在汇编.S文件中，引入到C文件里编译器并没有自动+1，所以这里手动+1。而main是定义在c文件中，所以它已经自动将最低位+1了。
***main.c***
```c
main,                                    // The reset handler
...
IntDefaultHandler,                      // The SysTick handler
```
改为
```c
((unsigned int)reset_handler + 1),      // The reset handler
...
systick_handler,                      // The SysTick handler
```

### reset_handler
reset_handler的实现很简单，将CM3运行时的栈切换成PSP，然后设置PSP的值，我习惯除了中断处理程序使用MSP，其他代码都用PSP。切换栈寄存器的动作很简单，就是修改CONTROL寄存器的第1位，即可，CONTROL寄存器定义如下图。_p_stack_top定义在rtos.ld中，其值是0x20008000。最后就是跳转到main来执行c代码。对于PSP和MSP是什么的朋友可能需要去看看CM3权威指南了哦。
![](https://i.imgur.com/oHqfYVZ.png)
***cm3_s.s***
```
.text                                   
.code 16                                
.global main                            
.global reset_handler                   
.global _p_stack_top                    
.global get_psp                         
.global get_msp                         
.global get_control_reg                 
                                        
reset_handler:                          
                                        
    /*Set the stack as process stack*/     
	/* tmp = CONTROL
	 * tmp |= 2
	 * CONTROL = tmp 
	 * /           
    mrs r0, CONTROL                  
    mov r1, #2                          
    orr r0, r1                    
    msr CONTROL, r0                     
                                        
    ldr r0, =_p_stack_top               
    mov sp, r0                          
                                        
    ldr r0, =main                       
    blx r0                              
    b .                                 

```

### main函数改动

main函数主要完成以下两点：
- 清0 BSS段
BSS段里存放的是未初始化的全局变量以及静态变量，内存在真实的物理硬件上上电后是随机值，所以需要对BSS段中的数据清0，以免发生不测。当然在虚拟机上，未曾使用的内存应该是0，但为了规范起见，还是将bss清0。
- 使能systick
systick是CM3的内核组件，其初始化的代码在cm3.c中实现，在下个小节讲解，本小节只讲解main函数的改变。systick_handler是systick的中断服务程序，在main.c中实现，每当systick中断发生时，就会进入到systick_handler中执行相关代码，在这里只是打印一句话。

***main.c***
```c
#include "os_stdio.h"
#include <stdint.h>
#include "cm3.h"

extern uint32_t _bss;
extern uint32_t _ebss;

static inline void clear_bss(void)
{
    uint8_t *start = (uint8_t *)_bss;
    while ((uint32_t)start < _ebss) {
        *start = 0;
        start++;
    }
}

void systick_handler(void)
{
    DEBUG("systick_handler\n");
}

int main()
{

    systick_t *systick_p = (systick_t *)SYSTICK_BASE;
    clear_bss();

    DEBUG("Hello RTOS\n");
    DEBUG("psp:0x%x\n", get_psp());
    DEBUG("msp:0x%x\n", get_msp());

    init_systick();
    while(1) {
    }
    return 0;
}
```

### Systick使能
SysTick定时器被捆绑在NVIC中，用于产生SysTick异常（异常号：15）。在以前，操作系统还有所有使用了时基的系统，都必须一个硬件定时器来产生需要的“滴答”中断，作为整个系统的时基。滴答中断对操作系统尤其重要。例如，操作系统可以为多个任务许以不同数目的时间片，确保没有一个任务能霸占系统；或者把每个定时器周期的某个时间范围赐予特定的任务等，还有操作系统提供的各种定时功能，都与这个滴答定时器有关。因此，需要一个定时器来产生周期性的中断，而且最好还让用户程序不能随意访问它的寄存器，以维持操作系统“心跳”的节律。
Cortex-M3处理器内部包含了一个简单的定时器。因为所有的CM3芯片都带有这个定时器，软件在不同 CM3器件间的移植工作就得以化简。该定时器的时钟源可以是内部时钟（FCLK，CM3上的自由运行时钟），或者是外部时钟（ CM3处理器上的STCLK信号）。不过，STCLK的具体来源则由芯片设计者决定，因此不同产品之间的时钟频率可能会大不相同。因此，需要检视芯片的器件手册来决定选择什么作为时钟源。
SysTick定时器能产生中断，CM3为它专门开出一个异常类型，并且在向量表中有它的一席之地。它使操作系统和其它系统软件在CM3器件间的移植变得简单多了，因为在所有CM3产品间，SysTick的处理方式都是相同的。 ***选自CORTEX_M3权威指南 P137***
有4个寄存器控制SysTick定时器，如下表所示:

***SysTick控制及状态寄存器（地址：0xE000_E010）***

|位段|名称|类型|复位值|描述|
|-|-|-|-|
|16|COUNTFLAG|R|0|如果在上次读取本寄存器后，SysTick已经计到了0，则该位为1。如果读取该位，该位将自动清零|
|2|CLKSOURCE|R/W|0|0=外部时钟源(STCLK)<br/> 1=内核时钟(FCLK)|
|1|TICKINT|R/W|0|1=SysTick倒数计数到0时产生SysTick异常请求<br/>0=数到0时无动作|
|0|ENABLE|R/W|0|SysTick定时器的使能位|

***SysTick重装载数值寄存器（地址：0xE000_E014）***

|位段|名称|类型|复位值|描述|
|-|-|-|-|-|
|23:0|RELOAD|R/W|0|读取时返回当前倒计数的值，写它则使之清零，同时还会清除在SysTick控制及状态寄存器中的COUNTFLAG标志|

***SysTick校准数值寄存器（地址：0xE000_E01C）***

|位段|名称|类型|复位值|描述|
|-|-|-|-|-|
|23:0|CURRENT|R/Wc|0|读取时返回当前倒计数的值，写它则使之清零，同时还会清除在SysTick控制及状态寄存器中的COUNTFLAG标志|

***SysTick校准数值寄存器（地址：0xE000_E01C）***

|位段|名称|类型|复位值|描述|
|-|-|-|-|-|
|31|NOREF|R|-|1=没有外部参考时钟（STCLK不可用）<BR/>0=外部参考时钟可用|
|30|SKEW|R|-|1=校准值不是准确的10ms<BR/>0=校准值是准确的10ms|
|23:0|TENMS|R/W|0|在10ms的间隔中倒计数的格数。芯片设计者应该通过Cortex-M3的输入信号提供该数值。若该值读回零，则表示无法使用校准功能|

在本节中，使用SystemClock作为systick的时钟，设置为1s发生一次systick中断，所以将reload寄存器设置为12M，最后是将systick的中断优先级设置为最低。调用这个函数之后，就能使能systick了，systick在后面的RTOS实现中扮演着关键的角色。


***cm3.h***
```c
#ifndef CM3_H
#define CM3_H
#include <stdint.h>

#define SCS_BASE            (0xE000E000)                 /*System Control Space Base Address */
#define SYSTICK_BASE        (SCS_BASE +  0x0010)         /*SysTick Base Address*/
#define SCB_BASE            (SCS_BASE +  0x0D00)
#define HSI_CLK             12000000UL
#define SYSTICK_PRIO_REG    (0xE000ED23)

typedef struct systick_tag {
    volatile uint32_t ctrl;
    volatile uint32_t load;
    volatile uint32_t val;
    volatile uint32_t calrb;
}systick_t;

extern void init_systick(void);
#endif /*CM3_H*/
```

***cm3.c***
```
#include "cm3.h"

void init_systick()
{
    systick_t *systick_p = (systick_t *)SYSTICK_BASE;
	uint8_t *sys_prio_p = (uint8_t *)SYSTICK_PRIO_REG;
 	/*Set systick as lowest prio*/
   	*sys_prio_p = 0xf0;
	/*set systick 1s*/
    systick_p->load = (HSI_CLK & 0xffffffUL) - 1;
    systick_p->val = 0;
	/*Enable interrupt, System clock source, Enable Systick*/    
	systick_p->ctrl = 0x7;
}
```

### printk/DEBUG打印实现
有了串口打印之后，实现printf(k)/DEBUG函数就很简单了，打印函数实现在os_stdio.c中。关于如何实现printf的文章网上有很多，这里就不展开了，读者有兴趣可以去参考其他文章。本文重点还是放在RTOS的实现上。DEBUG是一个宏,只有在DEBUG_SUPPORT定义的情况下才会实现打印

***os_stdio.h***
```c
#define DEBUG_SUPPORT
#ifdef DEBUG_SUPPORT
#define DEBUG printk
#else
#define DEBUG no_printk
#endif /*DEBUG*/
```

好了，大功告成，执行make run，可以看到sys_tick一秒打印一次如下图。
![](https://i.imgur.com/NQiLokN.png)

# 2 RTOS初探:任务切换
在上述简单讲了CM3的启动以及systick组件后，终于可以上硬菜了。好了，本节主要探讨两个问题：
1. **任务是怎么切换的？**
2. **任务是什么切换的？**

本节代码位于03_rtos_basic下。

## 任务是怎么切换的？
### 任务定义及任务接口定义
task.h定义了任务的数据结构task_t, 以及任务的接口，task_init, task_sched, task_switch, task_run_first。
在当前代码下，定义了一个任务表g_task_table,该表现在只存放两个任务的指针，然后定义了g_current_task用来指向当前任务，g_next_task指向下一个准备运行的任务。
任务控制块task_t中现在只包含一个值，就是当前任务栈的指针。任务与任务之间不共享栈空间，这点在操作系统的书上都有写，其实你可以把任务当做是通用OS中的内核线程，它们共享全局数据区，但都拥有自己的栈空间。独立的栈空间对于主要用于保存任务执行的上下文以及局部变量。

***图 双任务结构***
![](https://i.imgur.com/iOjYAxi.png)
***include/task.h***
```c
#ifndef TASK_H
#define TASK_H

#include <stdint.h>

typedef uint32_t task_stack_t;
/*Task Control block*/
typedef struct task_tag {

    task_stack_t *stack;
}task_t;

extern task_t *g_current_task;
extern task_t *g_next_task;
extern task_t *g_task_table[2];

extern void task_init (task_t * task, void (*entry)(void *), void *param, uint32_t * stack);
extern void task_sched(void);
extern void task_switch(void);
extern void task_run_first(void);

#endif /*TASK_H*/
```

###任务切换过程
先来谈一谈任务间切换的过程，两个任务切换过程原理很简单，分为两部分：
- 保存当前任务的寄存器
 本文中使用CM3的PendSV来实现了任务切换的功能。CM3处理异常/中断时，硬件会把R0-R3，R12，LR，PC, XPSR自动压栈。然后由PendSV的中断服务程序(后面简称PendSV ISR)手动把R4-R11寄存器压入任务栈中，这样就完成了任务上下文的保存。
- 恢复下一个任务的寄存器(包含PC)，当恢复PC时就能跳转到任务被打断的地方继续执行。
 恢复过程正好与保存过程相反，PendSV ISR会先手动地将R4-R11恢复到CM3中，然后在PendSV ISR退出时，CM3硬件会自动将R0-R3，R12，LR,XPSR恢复到CM3的寄存器中。
如下图所示，便是任务切换的过程：***(注:图中任务恢复的<----SP慢了一拍，看官注意下就好了，不想重画动态图了，图层太多了)***
![](https://i.imgur.com/s3QbnvA.gif)

好，那我们先看一下任务切换的源代码。任务切换这一段代码必须使用汇编来写，所以将pendsv ISR放在cm3_s.s中实现。 代码很简单，首先判断PSP是否为0，如果是0的话说明是第一个任务启动，那么就不存在任务保存一说，所以第54行就跳转到恢复任务的代码，后续会看到第一个任务启动与其它任务切换稍有不同，会先设置PSP为0，当然也可以使用一个全局变量来标志是否是第一个任务启动，纯属个人喜好。

第61-64行就是将R0-R11保存到当前任务的栈空间中，然后将SP的值赋给任务控制块中的task_t.stack。这个就完成了整个任务的保存。

第69-73行是将g_next_task指向的任务赋值给g_current_task，然后从g_current_task中取出任务的栈指针。

第75-76行是将任务栈中所保存的R0-R11恢复到CM3的寄存器中。

第78行设置PSP为当前SP值，79行就直接切换到PSP去运行，需要注意的是，此时此刻的LR寄存器并不是返回地址，而是一个特殊的含义：
在出入ISR的时候，LR的值将得到重新的诠释，这种特殊的值称为“EXC_RETURN”，在异常进入时由系统计算并赋给LR，并在异常返回时使用它。EXC_RETURN的二进制值除了最低4位外全为1，而其最低4位则有另外的含义（后面讲到，见表9.3和表9.4）

|位段|含义|
|-|-|
|31:4|EXC_RETURN标识：必须全为1|
|3|0=返回后进入Handler模式<BR/>1=返回后进入线程模式|
|2|0=从主堆栈中做出栈操作，返回后使用MSP，<BR/>1=从进程堆栈中做出栈操作，返回后使用PSP|
|1|保留，必须为0|
|0|0=返回ARM状态。<BR/>1=返回Thumb状态。在CM3中必须为1|

当执行完80行bx lr之后，硬件会自动恢复栈中的值到R0-R3，R12，LR，PC, XPSR。完成任务的切换 ***摘自《Cortex M3权威指南》***

***cm3_s.s***
```c
 51 pendsv_handler:
 52     /*CM3 will push the r0-r3, r12, r14, r15, xpsr by hardware*/
 53     mrs     r0, psp
 54     cbz     r0, pendsv_handler_nosave
 55
 56     /* g_current_task->psp-- = r11;
 57      * ...
 58      * g_current_task->psp-- = r4;
 59      * g_current_task->stack = psp;
 60      */
 61     stmdb   r0!, {r4-r11}
 62     ldr     r1, =g_current_task
 63     ldr     r1, [r1]
 64     str     r0, [r1]
 65
 66 pendsv_handler_nosave:
 67
 68     /* *g_current_task = *g_next_task */
 69     ldr     r0, =g_current_task
 70     ldr     r1, =g_next_task
 71     ldr     r2, [r1]
 72     str     r2, [r0]
 73
 74     /*r0 = g_current_task->stack*/
 75     ldr     r0, [r2]
 76     ldmia   r0!, {r4-r11}
 77
 78     msr     psp, r0
 79     orr     lr, lr, #0x04   /*Swtich to PSP*/
 80     bx      lr
```

顺带就把触发任务切换(即触发PendSV)的函数讲了吧，task_run_first是在启动第一个任务的时候调用的，而task_switch是在已经有任务的情况下才会调用。所以task_run_first只会被调用一次，而后面的切换全都使用task_switch。两者唯一的区别在于task_run_first会设置PSP为0，缘由在上面已经讲过，PendSV会根据PSP是否为0判断是不是第一次启动任务。然后往NVIC_INT_CTRL这个寄存器里触发PendSV异常即可进行PendSV ISR完成任务切换

***cm3.h***
```c
 11 #define NVIC_INT_CTRL       0xE000ED04
 12 #define NVIC_PENDSVSET      0x10000000
 13 #define NVIC_SYSPRI2        0xE000ED22
 14 #define NVIC_PENDSV_PRI     0x000000FF
```

***task.c***
```c
 43 void task_switch()
 44 {
 45     MEM32(NVIC_INT_CTRL) = NVIC_PENDSVSET;
 46 }
 47
 48 void task_run_first()
 49 {
 50     DEBUG("%s\n", __func__);
 51     set_psp(0);
 52     MEM8(NVIC_SYSPRI2) = NVIC_PENDSV_PRI;
 53     MEM32(NVIC_INT_CTRL) = NVIC_PENDSVSET;
 54 }
```
**NVIC_INT_CTRL寄存器片段**
![](https://i.imgur.com/3JgJPcA.png)


### 任务初始化
在了解了任务切换的过程后，就知道去初始化任务了，首先任务需要一段自己栈空间，因此传入参数stack，然后任务有自己的函数入口地址，因此需要传入entry，entry需要param作为函数参数调用，然后每个任务对应一个task_t控制块。即使是没有运行过的任务，也需要经过任务切换(PendSV)的招待，也就是将任务栈中的上下文恢复到寄存器中。所以目前为止，任务初始化就是将相应的寄存器初始值手动PUSH到任务栈中。PC保存的是任务的入口函数，那么当下一次任务切换时，就能切换到entry函数里面执行。然后把param参数传入到entry里，因为R0是函数调用的第一个参数，所以需要把param压栈到R0的位置，最后将栈指针保存到task_t.stack中。

***task.c***
```c
void task_init (task_t * task, void (*entry)(void *), void *param, uint32_t * stack) 
{                                                                                    
    DEBUG("%s\n", __func__);                                                         
    *(--stack) = (uint32_t) (1 << 24);          //XPSR, Thumb Mode                   
    *(--stack) = (uint32_t) entry;              //PC                                 
    *(--stack) = (uint32_t) 0x14;               //LR                                 
    *(--stack) = (uint32_t) 0x12;               //R12                                
    *(--stack) = (uint32_t) 0x3;                //R3                                 
    *(--stack) = (uint32_t) 0x2;                //R2                                 
    *(--stack) = (uint32_t) 0x1;                //R1                                 
    *(--stack) = (uint32_t) param;              //R0                                 
    *(--stack) = (uint32_t) 0x11;               //R11                                
    *(--stack) = (uint32_t) 0x10;               //R10                                
    *(--stack) = (uint32_t) 0x9;                //R9                                 
    *(--stack) = (uint32_t) 0x8;                //R8                                 
    *(--stack) = (uint32_t) 0x7;                //R7                                 
    *(--stack) = (uint32_t) 0x6;                //R6                                 
    *(--stack) = (uint32_t) 0x5;                //R5                                 
    *(--stack) = (uint32_t) 0x4;                //R4                                 
                                                                                     
    task->stack = stack;                                                             
}                                                                                    
```
那我们看一下应用程序是如何初始化task的。本章的应用只有两个任务进行来回切换，代码如下，首先定义了两个任务task1和task2，然后分别定义了两个task的栈以及入口函数，在main函数中调用task_init分别对两个任务进行初始化，然后将任务表的第0个元素指向task1，第1个元素指向task2， 如***图 双任务结构***所示一样。然后将下一个任务指向g_task_table[0]，即task1，调用task_run_first,进行第一次任务切换，也就是启动第一个任务。
***main.c***
```c
23 void task1_entry(void *param)    
24 {                                
		...                           
30 }                                
31                                  
32 void task2_entry(void *param)    
33 {                                
		...                        
39 }                                
40                                  
41 task_t task1;                    
42 task_t task2;                    
43 task_stack_t task1_stk[1024];    
44 task_stack_t task2_stk[1024];   
45 
46 int main()
47 {
48
49     systick_t *systick_p = (systick_t *)SYSTICK_BASE;
50     clear_bss();
51
52     DEBUG("Hello RTOS\n");
53     DEBUG("psp:0x%x\n", get_psp());
54     DEBUG("msp:0x%x\n", get_msp());
55
56     task_init(&task1, task1_entry, (void *)0x11111111, &task1_stk[1024]);
57     task_init(&task2, task2_entry, (void *)0x22222222, &task2_stk[1024]);
58
59     g_task_table[0] = &task1;
60     g_task_table[1] = &task2;
61     g_next_task = g_task_table[0];
62
63     task_run_first();
64
65     for(;;);
66     return 0;
67 } 
```


## 任务是什么切换？ 任务的调度
上述小节回答了**任务是怎么切换的？**那么本小节和下一章将说明**任务是什么切换**。在本章中所还未引入systick中断来处理任务的调度(即什么时候进行的切换)。为了给读者更直观的印象，本小节将在任务内部进行手动切换任务。首先看一下任务调度的源码，很简单。当前任务如果是g_task_table[0]，那么下一个运行的任务就是g_task_table[1]，反之一样，在分配好g_current_task和g_next_task后，调用task_switch进行任务的切换， 即进入PendSV ISR，上一小节已经分析过了PendSV ISR的代码。

***task.c***
```c
 32 void task_sched()
 33 {
 34     if (g_current_task == g_task_table[0]) {
 35         g_next_task = g_task_table[1];
 36     } else {
 37         g_next_task = g_task_table[0];
 38     }
 39
 40     task_switch();
 41 }
```

***main.c***
```c
18 void delay(uint32_t count)  
19 {                           
20     while(--count > 0);     
21 }                           
22                             
23 void task1_entry(void *param)    
24 {                                
25     for(;;) {                    
26         printk("task1_entry\n"); 
27         delay(65536000);         
28         task_sched();            
29     }                            
30 }                                
31                                  
32 void task2_entry(void *param)    
33 {                                
34     for(;;) {                    
35         printk("task2_entry\n"); 
36         delay(65536000);         
37         task_sched();            
38     }                            
39 }     
```
看一下任务内部做了什么？其实很简单，任务1打印了一句话，然后软件延时了一段时间，调用task_sched切换到任务2执行，任务2做相同的工作。这样就实现了连个任务之间来回切换工作，我们可以运行make run，看到运行结果如下所示。
![](https://i.imgur.com/QmyL6nM.png)

# 3 任务延时
在上一章中，我们实现了任务切换以及任务的调度。当时我们在任务中用到的延时函数是使用软件延时来做的，使用这种延时方式来做是有问题的。比如说当task1在执行软件延时时，task1是独占CPU的，这个时候其他的任务是没办法使用CPU的。而我们使用操作系统的原因之一就是想让CPU的利用率足够高，所以正确的情况应该是当task1调用延迟函数之后，task1应该将CPU使用权交给其他的task。本章就是讨论如何实现这样的任务延时函数。

## 空闲任务Idle task
在正式开始任务延时的话题前，我们需要先引入空闲任务(idle task)的概念，即所有的任务都暂停的时候，CPU干点什么事呢？不可能让CPU跑飞吧，所以此时引用idle task，让CPU运行idle task。当其他task被某一种情况唤醒，需要运行的时候，idle task就会交出的CPU的控制权给其他task。
Idle task的定义，初始化等与其他应用task并无差异，直接看代码。从idle_task_entry中就可以看出空闲任务其实不停地循环，直至被RTOS任务调度函数打断。空闲与其他的区别是不加入到任务表g_task_table[2]中，它有一个独立的指针g_idle_task。

***task.c***
```c
8 static task_t g_idle_task_obj;
9 static task_t *g_idle_task;
10 static task_stack_t g_idle_task_stk[1024];

12 static void idle_task_entry(void *param)
13 {
14     for(;;);
15 }

110 void init_task_module()
111 {
112     task_init(&g_idle_task_obj, idle_task_entry, (void *)0, &g_idle_task_stk[1024]);
113     g_idle_task = &g_idle_task_obj;
114
115 }
```

## 任务延时实现
任务延时最理想的实现情况是为一个任务分配一个硬件定时器，当硬件定时器完成定时后触发相应的中断来完成任务的调度。如下图所示，假设定时之前，当前任务是空闲任务，task1拥有硬件定时器1，task2拥有硬件定时器2，分别计数，当定时器1定时时间到，RTOS将当前任务g_current_task切换到任务1执行。
![](https://i.imgur.com/aTLoHo1.gif)
但这样存在的问题是，一般的SOC并不具备太多的硬件定时器，所以当任务达到几十甚至上百个的时候，这种是无法完成的。那就需要软件的方法来完成任务延时。各位看官应该记得CM3进阶章节中的systick定时器，任务延时就使用了这个定时器，我们只使用这一硬件定时器，然后给每一个任务分配一个软件计数器，当systick发生一次中断就对task中软件计数器减1，当某一个任务的软件计数器到时时，就触发一次任务调度。如下图所示：
![](https://i.imgur.com/UGSiGeF.gif)

在理解完使用软件定时器的原理后，我们直接看代码，实现在task_t中定义个字段delay_ticks用于软件计数。然后定义任务延时接口task_delay，其参数是delay_ticks个数，各位看官应该还记得之前systick是1s触发一次中断，所以这里1个delay_tick = 1s。最后定义task_system_tick_handler接口，该接口是被定期器中断函数调用，这是由于不同的芯片的定时器中断不同，所以这里定义一个统一接口让定时器中断函数调用，可以看到systick_handler中什么也没干，就是调用task_system_tick_handler。
***task.h***
```c
  8 typedef struct task_tag {
  9
 10     task_stack_t *stack;
 11     uint32_t delay_ticks;
 12 }task_t;

 22 extern void task_delay(uint32_t ticks);
 23 extern void task_system_tick_handler(void);
```
***cm3.c***
```c
 14 void systick_handler(void)
 15 {
 16     /*DEBUG("systick_handler\n");*/
 17     task_system_tick_handler();
 18 }
```

### task_delay接口实现
这个函数非常简单，仅仅只是对任务表中的delay_ticks进行赋值，然后触发一次任务调度。因为一旦有任务调用该接口，就说明当前任务需要延时不需要再占用CPU，所以需要触发一次任务调度。
***task.c***
```c
 92 void task_delay(uint32_t ticks)
 93 {
 94     g_current_task->delay_ticks = ticks;
 95     task_sched();
 96 }
```

### task_system_tick_handler接口实现
这个函数就是遍历任务表g_task_table，对任务表中的每一个任务的delay_ticks减1，对应于上图中systick中断发生的时候，task1和task2的delay_ticks都会减1操作。前提是确保该task的delay ticks必须大于0才行，delay ticks大于0代表该任务有延时操作。在对所有任务的delay_ticks减1操作后，触发一次任务调度。
***task.c***
```c
 98 void task_system_tick_handler(void)
 99 {
100     uint32_t i;
101     for (i = 0; i < 2; i++) {
102         if (g_task_table[i]->delay_ticks > 0) {
103             g_task_table[i]->delay_ticks--;
104         }
105     }
106
107     task_sched();
108 }
```

### 任务调度函数task_sched改动
在引用空闲函数以及延时函数之后，需要对调度函数进行一些改造，代码如下，现在这个函数只是为了demo任务延时的缓兵之计，后续章节会对该函数进行大改。但在这里还是理解一下这个函数干了什么事。

44-50行处理当前任务是idle task时，分别判断任务表g_task_table是否有任务已经延时时间到，如果某一个任务延时时间到，那么将g_next_task指向该任务，然后调用task_switch进行任务切换，如果在任务表中没有任务延时时间到，那么就不需要进行任务切换，idle task继续运行。

53-58行处理当前任务是task1时，如果task2的延时时间到，那么就切换到task2中执行；如果task1的delay_ticks不为0，那么切换到idle task运行，这种情况实际上就是task1调用了task_delay函数触发的任务调度引起；如果两种都不是，那就不需要进行任务调度，还是继续运行task1。

61-68行处理当前任务是task2的情况，其逻辑跟task1一样，不再重复。

```c
41 void task_sched()
 42 {
 43
 44     if (g_current_task == g_idle_task) {
 45         if (g_task_table[0]->delay_ticks == 0) {
 46             g_next_task = g_task_table[0];
 47         } else if (g_task_table[1]->delay_ticks == 0) {
 48             g_next_task = g_task_table[1];
 49         } else {
 50             goto no_need_sched;
 51         }
 52     } else {
 53         if (g_current_task == g_task_table[0]) {
 54             if (g_task_table[1]->delay_ticks == 0) {
 55                 g_next_task = g_task_table[1];
 56             } else if (g_current_task->delay_ticks != 0) {
 57                 g_next_task = g_idle_task;
 58             } else {
 59                 goto no_need_sched;
 60             }
 61         } else if (g_current_task == g_task_table[1]) {
 62             if (g_task_table[0]->delay_ticks == 0) {
 63                 g_next_task = g_task_table[0];
 64             } else if (g_current_task->delay_ticks != 0) {
 65                 g_next_task = g_idle_task;
 66             } else {
 67                 goto no_need_sched;
 68             }
 69         }
 70     }
 71
 72
 73     task_switch();
 74
 75 no_need_sched:
 76     return;
 77 }
```

## 应用代码测试
首先在main函数要调用init_task_module()来初始化空闲任务idle task。然后将task1和task2中delay(65536000)改为task_delay。task1 延时一个tick(相当于1s)，而task2延时5个tick，最后结果可以看到task1与task2交替执行，但task1打印5句时，task2才打印一句，这就证明延时函数工作了。
***main.c***
```c
 18 void task1_entry(void *param)
 19 {
 20     init_systick(1000);
 21     for(;;) {
 22         printk("%s\n", __func__);
 23         task_delay(1);
 24     }
 25 }
 26
 27 void task2_entry(void *param)
 28 {
 29     for(;;) {
 30         printk("%s\n", __func__);
 31         task_delay(5);
 32     }
 33 }

 40 int main()
 41 {
		...
 56     init_task_module();
 57
 58     task_run_first();
		...
 61     return 0;
 62 }

```

![](https://i.imgur.com/KZN3wP3.png)
虽然从打印上来看，跟之前纯软件延迟差不太多，但其背后的原理是完全不同的。纯软件在延时不释放CPU，会使其他任务得不到CPU使用权，而调用task_delay接口，当前任务就会释放CPU使用权，RTOS会进行一次任务调度将CPU使用权交给其他任务。


# DAY1总结
总结第一天的如下：
1. 环境搭建
2. QEMU CM3仿真：UART打印，systick，gdb调试
3. RTOS基础：任务切换/任务调度/任务延时简单实现(基于双任务及空闲任务)。

第二天会涉及RTOS的内核核心实现，包括任务挂起/唤醒/删除，延时队列，临界区保护，优先级抢占调度及时间片调度。

话不多说，直接进入正题，今天要实现的便是RTOS任务相关的所有功能

# 1 临界区保护

本节代码在05_critical下

为什么需要临界区保护呢，请看下图：
![](https://i.imgur.com/AMWhSzC.gif)
当task1要对共享资源进行**读-改-写**操作时，在写回之前被某一事件中断打断切换到task2，而此时task2恰巧也有修改共享资源x的代码，此时task2将共享资源修改成了11，当完成这个操作后，task2交出cpu控制权，此时RTOS又切换到了task1运行，执行**读-改-写**的写操作，将tmp值回写到共享资源x中，此时task2对共享资源x操作会被覆盖，等于没有发生。这其中的共享资源x便称为***临界资源***，当任务对临界资源进行操作时，必须要有相应的临界区保护方能逃过一劫。
对于临界区保护有多种方式：
- 关中断，此种方法最为简单粗暴，直接关闭中断，再也不会有任何事件打断，当前任务会独占CPU，等到执行完成临界区代码后再打开中断。
- 调度锁，在进入临界区后不允许OS进行任务调度，此种方法只能用于任务之间，无法用于ISR和任务之间。
- 同步机制, 信号量，互斥锁等，此种方法只能用于任务之间。

## 没有临界区保护
本小节主要实现关中断来完成临界区保护，调度锁本文没有完成，而同步机制会在后续的事件控制块中讲述。我们首先来看一下没有临界区保护的代码的行为。代码主要是修改两个应用任务,其描述与上图相符合，task1每隔1s更新一次test_sync_val，task2每隔5s**读-改-写**一次test_sync_val。

***main.c***
```c
 18 uint32_t test_sync_val = 0;
 19
 20 void task1_entry(void *param)
 21 {
 22     uint32_t status = 0;
 23     init_systick(1000);
 24     for(;;) {
 25         printk("%s\n", __func__);
 26         //status = task_enter_critical();
 27         task_delay(1);
 28         test_sync_val++;
 29         //task_exit_critical(status);
 30         printk("task1:test_sync_val:%d\n", test_sync_val);
 31     }
 32 }
 33
 34 void task2_entry(void *param)
 35 {
 36     uint32_t counter = test_sync_val;
 37     uint32_t status = 0;
 38
 39     for(;;) {
 40         printk("%s\n", __func__);
 41
 42         //status = task_enter_critical();
 43         counter = test_sync_val;
 44         task_delay(5);
 45         test_sync_val = counter +1;
 46         //task_exit_critical(status);
 47
 48         printk("task2:test_sync_val:%d\n", test_sync_val);
 49     }
 50 }
```
运行结果如下图，task1修改的结果会被task2的**读-改-写**，而这并不是我们想看到的结果。
![](https://i.imgur.com/swkYedC.png)

## 增加临界区保护
###接口定义
临界区保护需要定义两个接口。task_enter_critical在进入临界区调用，task_exit_critical在退出临界区调用。
***task.h***
```c
25 extern uint32_t task_enter_critical(void);
26 extern void task_exit_critical(uint32_t status);
```
###接口实现
task_enter_critical很简单，首先获取primask的值，然后保存下来，因为我们的OS允许进入多次临界区，所以除了关闭CM3的IRQ以外，还需要把primask的值记录下来，等到退出时恢复。这样是防止嵌套情况下退出临界区错误打开了中断。
PRIMASK是个只有单一比特的寄存器。在它被置1后，就关掉所有可屏蔽的异常，只剩下NMI和硬fault可以响应。它的缺省值是0，表示没有关中断。

task_exit_critical与task_enter_critical正好是反操作，恢复primask的值并打开irq。 primask和irq中断开关的函数必须用汇编来实现，实现在cm3_s.s中。
***task.c***
```c
118 uint32_t task_enter_critical(void)
119 {
120     uint32_t ret = get_primask();
121     disable_irq();
122     return ret;
123 }
124
125 void task_exit_critical(uint32_t status)
126 {
127     set_primask(status);
128     enable_irq();
129 }
```
***cm3_s.s***
```
 86 get_primask:
 87     mrs     r0, PRIMASK
 88     blx     lr
 89
 90 set_primask:
 91     msr     PRIMASK, r0
 92     blx     lr
 93
 94 disable_irq:
 95     cpsid   i
 96     blx     lr
 97
 98 enable_irq:
 99     cpsie   i
100     blx     lr
```

### 应用测试
应用代码和上一小节基本一样，只是将注释的临界区保护打开。不多说，直接看运行结果，可以看到两个任务能够很好同步了共享资源的读写。
***main.c***
```c
 18 uint32_t test_sync_val = 0;
 19
 20 void task1_entry(void *param)
 21 {
 22     uint32_t status = 0;
 23     init_systick(1000);
 24     for(;;) {
 25         printk("%s\n", __func__);
 26         status = task_enter_critical();
 27         task_delay(1);
 28         test_sync_val++;
 29         task_exit_critical(status);
 30         printk("task1:test_sync_val:%d\n", test_sync_val);
 31     }
 32 }
 33
 34 void task2_entry(void *param)
 35 {
 36     uint32_t counter = test_sync_val;
 37     uint32_t status = 0;
 38
 39     for(;;) {
 40         printk("%s\n", __func__);
 41
 42         status = task_enter_critical();
 43         counter = test_sync_val;
 44         task_delay(5);
 45         test_sync_val = counter +1;
 46         task_exit_critical(status);
 47
 48         printk("task2:test_sync_val:%d\n", test_sync_val);
 49     }
 50 }
```

![](https://i.imgur.com/27MESiB.png)

# 2 OS数据结构
本节代码位于06_multi_prio

在将优先级，时间片调度及延时队列等问题前，有必要把OS中的数据结构给捋一遍。其实就两，位图和双向循环链表。
## 位图
位图法就是bitmap的缩写。所谓bitmap，就是用每一位来存放某种状态，适用于大规模数据，但数据状态又不是很多的情况。通常是用来判断某个数据状态。

###接口定义
本文中的bitmap数据结构很简单，就包含了一个uint32_t的字段bitmap，原因是本文用到的位图是用于任务优先级，而本设计中OS的任务优先级最多大就31，最小为0，所以只需要一个字节的位图即可。所以这里的位图结构并不算一个通用数据结构。

***lib.h***
```c
  6 /*Bitmap*/
  7 typedef struct bitmap_tag {
  8     uint32_t bitmap;
  9 }bitmap_t;
 10
 11 extern void bitmap_init(bitmap_t *bitmap);
 12 extern uint32_t bitmap_count(void);
 13 extern void bitmap_set(bitmap_t *bitmap, uint32_t pos);
 14 extern void bitmap_clear(bitmap_t *bitmap, uint32_t pos);
 15 extern uint32_t bitmap_get_first_set(bitmap_t *bitmap);
```
###实现
***lib.c***
bitmap_init初始化bitmap,对所有的位清零。

bitmap_count计算bitmap一共有多少个位，这里返回固定的32。

bitmap_set设置bitmap某一位。

bitmap_clear清零bitmap某一位。

bitmap_get_first_set获取bitmap第一个非0位。有些读者可能会感到迷惑：获取第一个非0位不就是一个简单的循环查位吗？为什么要搞得这么复杂，确如读者所想，如果只是为了获取第一个非0位，事情不会如此复杂，一个循环就能解决问题，但因为这里用于RTOS，所以时间上它的操作必须是常量，所以这里采取了一个读者不熟悉的做法，至于这个做法的原理，读者只要模仿计算机执行这段程序即可。***节选自《嵌入式实时操作系统uC/OS-II原理及应用(第四版)》***
```
void bitmap_init(bitmap_t *bitmap)
{
    bitmap->bitmap = 0;
}

uint32_t bitmap_count()
{
    return 32;
}

void bitmap_set(bitmap_t *bitmap, uint32_t pos)
{
    bitmap->bitmap |= 1 << pos;
}

void bitmap_clear(bitmap_t *bitmap, uint32_t pos)
{
    bitmap->bitmap &= ~(1 << pos);
}

uint32_t bitmap_get_first_set(bitmap_t *bitmap)
{
    uint32_t pos = 32;
    static const uint8_t quick_table[] =
    {
        /* 00 */ 0xff, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 10 */ 4,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 20 */ 5,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 30 */ 4,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 40 */ 6,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 50 */ 4,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 60 */ 5,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 70 */ 4,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 80 */ 7,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 90 */ 4,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* A0 */ 5,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* B0 */ 4,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* C0 */ 6,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* D0 */ 4,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* E0 */ 5,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* F0 */ 4,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
    };

    if (bitmap->bitmap & 0xff) {
        pos = quick_table[bitmap->bitmap & 0xff];
    } else if (bitmap->bitmap & 0xff00) {
        pos = quick_table[(bitmap->bitmap >> 8) & 0xff] + 8;
    } else if (bitmap->bitmap & 0xff0000) {
        pos = quick_table[(bitmap->bitmap >> 16) & 0xff] + 16;
    } else if (bitmap->bitmap & 0xFF000000) {
        pos = quick_table[(bitmap->bitmap >> 24) & 0xFF] + 24;
    } else {
        pos = bitmap_count();
    }

    return pos;
}
```

## 双向循环链表

双向循环链表采用了类似linux双向循环链表的实现，相比于linux的链表只是多了个count字段记录链表中有多少个节点，原理网上有很多，就不讲了，百度搜索linux双向循环链表即可。这里仅把接口定义描述一遍，代码实现有兴趣的朋友可以结合网上linux双向循环链表来理解。

list_node_t定义了链表节点，prev指向前一个节点，next指向后一个节点
list_t定义了链表结构，其中包含一个链表头和一个计数值count，该计数值代表链表中含有多少个链表节点。
container_of从链表节点元素获取该其父结构。
list_init初始化链表。
list_count获取链表含有多少个链表节点(除头节点)。
list_head获取链表头节点。
list_tail获取链表尾节点。
node_prev获取该节点的前一个节点。
node_next获取该节点的后一个节点。
list_remove_all删除所有链表所有节点。
list_insert_head插入list_node到链表头部。
list_append_last插入list_node到链表尾部。
list_remove_first删除链表第一个元素并返回。
list_remove删除链表节点node。
***lib.h***
```c
/*Double Linked List*/
typedef struct list_node_tag {
    struct list_node_tag *prev;
    struct list_node_tag *next;
}list_node_t;

extern void list_node_init(list_node_t *node);

typedef struct list_tag {
    list_node_t head;
    uint32_t node_count;
}list_t;

#define container_of(node, parent, name) (parent *)((uint32_t)node - (uint32_t)&((parent *)0)->name)
extern void list_init(list_t *list);
extern uint32_t list_count(list_t *list);
extern list_node_t *list_head(list_t *list);
extern list_node_t *list_tail(list_t *list);
extern list_node_t *node_prev(list_node_t *list_node);
extern list_node_t *node_next(list_node_t *list_node);
extern void list_remove_all(list_t *list);
extern void list_insert_head(list_t *list, list_node_t *list_node);
extern void list_append_last(list_t *list, list_node_t *list_node);
extern list_node_t *list_remove_first(list_t *list);
extern void list_remove(list_t *list, list_node_t *node);
```
# 3 多优先级任务
RTOS与通用OS比较大的一个差异就是抢占式调度。抢占式调度一定是基于多优先级完成的，高优先级的任务能抢占低优先级任务的CPU使用权。
回想现在我们的系统中关于任务的数据结构只有一个任务表g_task_table，而且此时的任务是没有优先级之分的。本节完成一个优先级只对应于一个任务。UCOS2也是这种做法，系统简单的情况下使用一个优先级对应一个任务已然足够。在这种前提下，任务表g_task_table的索引对应于任务的优先级。只有一个任务表是不够的，我们需要另外一个数据结构来表示该优先级有没有处于可运行状态。这时上一章节所说的bitmap就派上用场了。规定RTOS只有32个优先级，bitmap的每一位对应于一个优先级，当某一位设1，表示该优先级有任务需要运行，如果该位为0，则表示该优先级没有任务需要运行。大致情况如下图：

![](https://i.imgur.com/GZsQ11I.gif)

如上所述，现在一共有两个数据结构，一个是任务表g_task_table，另外一个乃优先级位图g_task_prio_bitmap。初试状态g_task_prio_bitmap的第0位为0，表示最高优先级并没有需要运行的任务，所以非0最低位为第一位，因此此时RTOS执行任务表中task2(优先级为1)，随后第0位被某个事件置1，表示此时优先级0有任务需要运行，当下一次任务调度的时候，就会将当前任务切换到task1(优先级为0)来执行。

##实现
1. 在任务控制块结构中添加表示优先级的字段prio
***task.h***
```c
  8 /*Task Control block*/
  9 typedef struct task_tag {
 10
 11     task_stack_t *stack;
 12     uint32_t delay_ticks;
 13     uint32_t prio;
 14 }task_t;
```
2. 修改task_init接口，初始化时需要传入prio
***task.c***
```c
 21 void task_init (task_t * task, void (*entry)(void *), void *param, uint32_t prio, uint32_t * stack)
 22 {
		...
		  //初始化优先级
 43     task->prio = prio;
 44	 //任务表中一个优先级对应一个任务
 45     g_task_table[prio] = task;
		  //设置优先级位图相应的位
 46     bitmap_set(&g_task_prio_bitmap, prio);
 47 }
```
3. 增加接口task_t *task_highest_ready()。该任务返回最高优先级的任务指针。代码实现很简单，就是从bitmap中获取最低非0位，即需要运行的最高优先级，然后从任务表中返回任务指针即可。

***task.c***
```c
159 task_t *task_highest_ready()
160 {
161     uint32_t highest_prio = bitmap_get_first_set(&g_task_prio_bitmap);
162     return g_task_table[highest_prio];
163 }
```
4. 还记得之前的又长又臭的task_sched吗，这次要推倒重来了，焕然一新的调度算法！上代码
***task.c***
```c
 49 void task_sched()
 50 {
 51     uint32_t status = task_enter_critical();
 52     task_t *temp_task_p;
 53
 54     if (g_sched_lock > 0) {
 55         goto no_need_sched;
 56     }
 57
 58     temp_task_p = task_highest_ready();
 59     if (temp_task_p != g_current_task) {
 60         g_next_task = temp_task_p;
 61         task_switch();
 62     }
 63
 64 no_need_sched:
 65     task_exit_critical(status);
 66     return;
 67 }
```
这次代码的逻辑就非常简单了，获取最高优先级的任务，如果当前任务已经是最高优先级了，那什么也不做，否则就触发一次任务切换，将任务切换到更高优先级的任务去。

5. 修改延时函数 void task_delay(uint32_t ticks)。原来的延时函数只是将ticks赋值给当前任务的delay_ticks字段。多增加了一句bitmap_clear(&g_task_prio_bitmap, g_current_task->prio)，就是说在调用延时函数后，需要把该任务对应的优先级位图清0。那么下次调度的时候就不会选中这个任务了。

***task.c***
```c
 82 void task_delay(uint32_t ticks)
 83 {
		...
 86     bitmap_clear(&g_task_prio_bitmap, g_current_task->prio);
		...
 89 }
```
6.修改void task_system_tick_handler(void)。其流程为遍历任务表g_task_table，如果该某一个任务的延时时间到了，那么就将它再次加入就绪表中，即将g_task_prio_bitmap相应的prio位置1。

***task.c***
```c
 96 void task_system_tick_handler(void)
 97 {
 98     uint32_t status = task_enter_critical();
 99     uint32_t  i = 0;
100     for (i = 0; i < OS_PRIO_COUNT; i++) {
101         if (g_task_table[i] == (task_t *)NULL) {
102             continue;
103         }
104
105         if (g_task_table[i]->delay_ticks > 0) {
106             g_task_table[i]->delay_ticks--;
107         } else {
108             bitmap_set(&g_task_prio_bitmap, i);
109         }
110     }
111
112     task_exit_critical(status);
113     task_sched();
114 }
```
增加void task_delay_s(uint32_t seconds)。这个其实跟优先级无关，只是将systick每1s发生中断改成了每10ms发生一次中断，这样更符合正常RTOS的做法。这个接口用于延时几秒。

***task.c***
```c
 91 void task_delay_s(uint32_t seconds)
 92 {
 93     task_delay(seconds * 100);
 94 }
```

## 应用测试
将task1设置为最高优先级0，task2设置为优先级1，但运行的结果还是两个任务交替的打印。因为当task1延时的时候，它会把相应优先级位清0，即从优先级表中移除该任务，然后让低优先级的task2运行。如果task1去掉延时函数，那么task1会一直独占cpu而不让低优先级的task2运行。
***main.c***
```c
 19 void task1_entry(void *param)
 20 {
 21     init_systick(10);
 22     for(;;) {
 23         printk("%s\n", __func__);
 24         task_delay_s(1);
 25     }
 26 }
 27
 28 void task2_entry(void *param)
 29 {
 30
 31     for(;;) {
 32         printk("%s\n", __func__);
 33         task_delay_s(2);
 34     }
 35 }
 42 int main()
 51     task_init(&task1, task1_entry, (void *)0x11111111, 0, &task1_stk[1024]);
 52     task_init(&task2, task2_entry, (void *)0x22222222, 1, &task2_stk[1024]);
 57     return 0;
 58 }
```

![](https://i.imgur.com/Gw99KRg.png)

# 4 延时队列
本章源代码位于07_delay_queue目录下

小结一下，此时此刻，有数据结构任务表，优先级位图。本小节我们增加一个延时队列来存放处于延时状态的任务。也就是说当任务处于延时状态时,RTOS会将任务从任务表中移除添加到延时队列中。过程如下图：
![](https://i.imgur.com/U2w5lqu.gif)

##实现
理论很简单，来看下代码实现：

- 首先要在任务控制块task_t增加延时节点delay_node，并且增加一个字段state，代表任务状态。当前任务状态分为两种，就绪状态OS_TASK_STATE_RDY和延时状态OS_TASK_STATE_DELAYED。当然需要定义一个延时队列变量g_task_delay_list，在task_init中初始化delay_node

***task.h***
```c
  8 #define OS_TASK_STATE_RDY                   0
  9 #define OS_TASK_STATE_DELAYED               (1 << 1)
 10
 11 typedef uint32_t task_stack_t;
 12 /*Task Control block*/
 13 typedef struct task_tag {
 14
 15     task_stack_t *stack;
 16     uint32_t delay_ticks;
 17     uint32_t prio;
 18
 19     list_node_t delay_node;
 20     uint32_t state;
 21 }task_t;
```
***task.c***
```c
 15 static list_t g_task_delay_list;
 22 void task_init (task_t * task, void (*entry)(void *), void *param, uint32_t prio, uint32_t * stack)
 23 {
 46     task->state = OS_TASK_STATE_RDY;
 47     list_node_init(&task->delay_node);
 51 }
```

- 增加一些辅助接口
task_ready接口用于将任务加入就绪任务表中(即加入任务表，将相应就绪位图置1。
task_unready接口用于将任务从就绪任务表中移除。
task_delay_wait接口将任务加入延时队列并且将任务状态置为延时状态。
task_delay_wakeup接口将任务从延时队列移除并且清除任务延时状态。

***task.c***
```c
186 void task_ready(task_t *task)
187 {
188     g_task_table[task->prio] = task;
189     bitmap_set(&g_task_prio_bitmap, task->prio);
190 }
191
192 void task_unready(task_t *task)
193 {
194     g_task_table[task->prio] = (task_t *)NULL;
195     bitmap_clear(&g_task_prio_bitmap, task->prio);
196 }
197
198 void task_delay_wait(task_t *task, uint32_t ticks)
199 {
200     task->delay_ticks = ticks;
201     list_insert_head(&g_task_delay_list, &(task->delay_node));
202     task->state |= OS_TASK_STATE_DELAYED;
203 }
204
205 void task_delay_wakeup(task_t *task)
206 {
207     list_remove(&g_task_delay_list, &(task->delay_node));
208     task->state &= ~OS_TASK_STATE_DELAYED;
209 }
```

- 修改task_delay接口

task_delay接口实现逻辑相较以前更为清晰，就两步：1.将任务加入延时队列，2.将任务从就绪表中移除。
***task.c***
```c
 86 void task_delay(uint32_t ticks)
 87 {
 88     uint32_t status = task_enter_critical();
 89
 90     /*  1.Add task to delay list
 91      *  2.Remove the task from task list
 92      *  3.Clear the task bit from prioity bitmap
 93      * */
 94     task_delay_wait(g_current_task, ticks);
 95     task_unready(g_current_task);
 96
 97     task_exit_critical(status);
 98     task_sched();
 99 }
```

- 修改task_system_tick_handler接口
这个函数原来是遍历就绪表，对就绪表中每一个任务自减。而现在加入了延时队列后，不需要再对就绪表进行遍历，只需要遍历延时队列中任务即可。实现很简单，遍历延时队列，对每一个任务的delay_tick自减1，如果该任务的delay_ticks为0，那么将该任务从延时队列移除，加入就绪表中，最后触发一次任务调度
***task.c***
```c
106 void task_system_tick_handler(void)
107 {
108     uint32_t status = task_enter_critical();
109     list_node_t *head = &(g_task_delay_list.head);
110     list_node_t *temp_node = head->next;
111     task_t *task = (task_t *)NULL;
112     /*
113      *  For each the delay list, and do:
114      *  1. Self sub the node delay ticks
115      * */
116     while (temp_node != head) {
117         task = container_of(temp_node, task_t, delay_node);
118         temp_node = temp_node->next;
119         if (--task->delay_ticks == 0) {
120             /*
121              *  1.Remove the task from delay list
122              *  2. Add the task to task table
123              *  3.Set the prio bit to bitmap
124              * */
125             task_delay_wakeup(task);
126             task_ready(task);
127         }
128     }
129
130     task_exit_critical(status);
131     task_sched();
132 }
```

- 不要忘了初始化延时队列
在OS跑起来前，初始化一把延时队列。
***task.c***
```c
134 void init_task_module()
135 {
136     task_init(&g_idle_task_obj, idle_task_entry, (void *)0, OS_PRIO_COUNT - 1,  &g_idle_task_stk[1024]);
137     g_idle_task = &g_idle_task_obj;
138
139     g_sched_lock = 0;
140     list_init(&g_task_delay_list);
141
142     g_next_task = task_highest_ready();
143     task_run_first();
144 }
```
应用测试代码并无改动，还是两个任务交替执行打印。这里就不再截图了。各位看官运行在07_delay_queue运行make run即可。

# 5 同优先级时间片调度
本节代码位于08_prio_slice目录下

第3节中我们实现了一个优先级对应于一个任务，那么本节将带领你完成一个优先级对应于多个任务。而同优先级的任务采用何种调度方法呢？在这里，我们采用了时间片轮转来调度同优先级的任务。
![](https://i.imgur.com/oPtc5cA.gif)
如上图所示，task1和task3是同优先级的任务，此时优先级bitmap的第一个非0位是第零位。现假设一个任务的时间片就是一个systick，那么当一次systick中断发生时，task1会取出插入到list0尾，而task1后面的task3会成为新的第一个就绪任务，并且运行它。

## 代码实现
- 首先要在任务控制块task_t中添加跟时间片相关的字段slice以及就绪队列相关的链表节点prio_list_node，同时我们定义最大时间片为10个systick。

***task.h***
```c

#define OS_SLICE_MAX        10

 13 typedef struct task_tag {
 14
 15     task_stack_t *stack;
 16     uint32_t delay_ticks;
 17     uint32_t prio;
 18
 19     list_node_t delay_node;
 20     uint32_t state;
 21
 22     list_node_t prio_list_node;
 23     uint32_t slice;
 24 }task_t;
```

- 之前就绪任务表g_task_table也需要修改了，之前是一个优先级对应于一个任务，而现在一个优先级对应了多个任务，所以此时存放在就绪表中的是一个任务链表，修改如下。
***task.c***
```c
extern list_t g_task_table[OS_PRIO_COUNT];
```
- 初始化任务函数需要把slice和prio_list_node初始化，并把该任务加入到就绪表相应的链表尾部。代码如下

***task.c***
```c
 22 void task_init (task_t * task, void (*entry)(void *), void *param, uint32_t prio, uint32_t * stack)
 23 {
 49     task->slice = OS_SLICE_MAX;
 50     list_node_init(&task->prio_list_node);
 51     list_append_last(&g_task_table[prio], &(task->prio_list_node));
 52     bitmap_set(&g_task_prio_bitmap, prio);
 53 }
```

- 修改task_system_tick_handler接口
在该函数中增加代码如下,检查当前的任务时间片是否用完，如果用完，那么将CPU交给后一个同优先级的任务。从代码上反应就是将任务从相应的就绪任务链表里拿出来插入到该任务链表尾。
***task.c***
```c
132     /*
133      *  check whether the time slice of current task exhausts
134      *  if time slice is over, move the task to the prio list tail
135      */
136     if (--g_current_task->slice == 0) {
137         if (list_count(&g_task_table[g_current_task->prio]) > 0) {
138             list_remove(&g_task_table[g_current_task->prio], &(g_current_task->prio_list_node));
139             list_append_last(&g_task_table[g_current_task->prio], &(g_current_task->prio_list_node));
140             g_current_task->slice = OS_SLICE_MAX;
141         }
142     }
```

- 修改task_highest_ready接口
之前直接返回就绪表中相应优先级的任务即可，而现在需要先拿出就绪表中的链表，然后从链表里拿出第一个任务返回。
***task.c***
```c
196 task_t *task_highest_ready()
197 {
198     /*
199      *              Highest prio task
200      *                      |
201      * g_task_table[0] -> task -> task -> task;
202      * ....
203      * g_task_table[31] -> task -> task;
204      */
205     uint32_t highest_prio = bitmap_get_first_set(&g_task_prio_bitmap);
206     list_node_t *node = list_head(&(g_task_table[highest_prio]));
207     return container_of(node, task_t, prio_list_node);
208 }
```
- 修改task_ready和task_unready接口
task_ready之前是直接将该task放到就绪表中，而现在是将任务插入到相应就绪表中的优先级链表中。
task_unready稍微复杂一点，就是说只有在优先级链表已经没有了任务，那么才把优先级bitmap清0。否则该优先级的任务链表还存有一个任务，那么优先级bitmap中的位就不会清零。

***task.c***
```c
210 void task_ready(task_t *task)
211 {
212     list_append_last(&g_task_table[task->prio], &(task->prio_list_node));
213     bitmap_set(&g_task_prio_bitmap, task->prio);
214 }
215
216 void task_unready(task_t *task)
217 {
218     list_remove(&g_task_table[task->prio], &(task->prio_list_node));
219     if (list_count(&g_task_table[task->prio]) == 0) {
220         bitmap_clear(&g_task_prio_bitmap, task->prio);
221     }
222     bitmap_clear(&g_task_prio_bitmap, task->prio);
223 }
```

## 应用测试
测试代码如下，创建3个task，task1的优先级为0(最高)，task2和task3的优先级为2。此时当task1处于延时状态时，task2和task3会根据时间片进行轮询调度。可以看到task2和task3使用的是软件延时，所以如果时间片调度的话，在task1处于延时状态时，只会运行task2。但我们现在已经有了同优先级的时间片调度，所以能看到task2和task3交替运行。
***main.c***
```c
 19 void task1_entry(void *param)
 20 {
 21     init_systick(10);
 22     for(;;) {
 23         printk("%s\n", __func__);
 24         task_delay_s(2);
 25     }
 26 }
 27
 28 void delay(uint32_t delay)
 29 {
 30     while(delay--);
 31 }
 32
 33 void task2_entry(void *param)
 34 {
 35
 36     for(;;) {
 37         printk("%s\n", __func__);
 38         delay(65536000);
 39     }
 40 }
 41
 42 void task3_entry(void *param)
 43 {
 44     for(;;) {
 45         printk("%s\n", __func__);
 46         delay(65536000);
 47     }
 48 }

 69     task_init(&task1, task1_entry, (void *)0x11111111, 0, &task1_stk[1024]);
 70     task_init(&task2, task2_entry, (void *)0x22222222, 1, &task2_stk[1024]);
 71     task_init(&task3, task3_entry, (void *)0x33333333, 1, &task3_stk[1024]);
```
make run走你！
![](https://i.imgur.com/O5pgfhB.png)

# 6任务挂起/唤醒

任务的挂起/唤醒其实很简单，就是将任务从就绪表中的优先级队列中将任务移除即可，而任务的唤醒只是将任务重新放到该链表尾部即可。并不添加任何新的数据结构去管理这些被挂机的任务，当然各位看官如果想自己用一个链表把这些挂起串起来也可以。

## 实现
那我们不多说了，应该很简单，所以直接看代码实现吧。

- 虽然不需要增加数据结构，但我们需要一个状态位来标志该任务是否是挂起状态。并且我们认为任务是可以多次挂起的，所以加入一个挂起计数。当挂起计数为0的时候，也就是所以的挂起操作都对应一个唤醒操作时，才把任务加入到就绪链表中。

***task.h***
```c
 10 #define OS_TASK_STATE_SUSPEND               (1 << 2)
 13 typedef struct task_tag {
 		......
 27     /*Suspend resume*/
 28     uint32_t suspend_cnt;
 29 }task_t;
```

- 实现接口task_suspend和task_resume。
当任务处于延时状态时，任务是不能被挂起的，task_suspend首先判断了任务是不是延时状态。如果不是，那就把任务的挂起计数自增1，然后把任务的挂起位置1，然后把任务从就绪表中移除，如果挂起的任务是自己，那么就触发一次任务调度。
task_resume先判断任务是不是挂起状态。然后把任务的挂起计数自增1，将任务的挂起位清零，将任务加入到就绪表中。如果该任务挂起计数为0，就触发一次任务调度。

***task.c***
```c
241 void task_suspend(task_t *task)
242 {
243     uint32_t status = task_enter_critical();
244
245     /*Don't suspend the task in delay state*/
246     if (!(task->state & OS_TASK_STATE_DELAYED)) {
247         /*If the task is first suspend*/
248         if (task->suspend_cnt++ <= 0) {
249             task->state |= OS_TASK_STATE_SUSPEND;
250
251             task_unready(task);
252             if (task == g_current_task) {
253                 task_sched();
254             }
255         }
256
257     }
258
259     task_exit_critical(status);
260 }
261
262 extern void task_resume(task_t *task)
263 {
264     uint32_t status = task_enter_critical();
265
266     if (task->state & OS_TASK_STATE_SUSPEND){
267         if (--task->suspend_cnt == 0) {
268             task->state &= ~OS_TASK_STATE_SUSPEND;
269             task_ready(task);
270             task_sched();
271         }
272     }
273     task_exit_critical(status);
274 }
```

## 应用测试
测试代码只有两个任务，task1在打印完before suspend后会挂起自己，然后等待唤醒。此时RTOS会切换到task2来运行，task2会在打印完后延时1秒唤醒task1。可以推断出打印的顺序是
task1_entry efore_suspend
task2_entry
task1_entry after_suspend


***main.c***
```c
 25 void task1_entry(void *param)
 26 {
 27     init_systick(10);
 28     for(;;) {
 29         printk("%s:before suspend\n", __func__);
 30         task_suspend(&task1);
 31         printk("%s:after suspend\n", __func__);
 32     }
 33 }
 40 void task2_entry(void *param)
 41 {
 42
 43     for(;;) {
 44         printk("%s\n", __func__);
 45         task_delay_s(1);
 46         task_resume(&task1);
 47     }
 48 }

```
make run走你！

![](https://i.imgur.com/9GaQl4j.png)



# 7 任务删除
本节代码位于10_task_delete
任务的删除分为两种
- 强制删除
	这种任务删除的方式<BR/>优点在于任务能够及时删除<br/>缺点在于任务删除时会有一定概率导致待删除任务持有的资源无法释放。
- 设置删除标志，待删除任务自己删除
	一个任务设置一个删除标志，等待待删除任务自己调用删除任务的函数。这样做的优缺点正好与第一种强制删除相反。
删除任务很简单，只需要把任务从就绪表，延时队列中去掉即可。

##代码实现
在任务控制块中加入任务删除的字段。
- clean是任务删除时候调用的回调函数，多用于释放任务所占有的资源
- clean_param是删除回调函数所需要的参数
- request_del_flag用于非强制删除时的删除标志
- task_force_delete强制删除接口
- task_request_delete非强制删除接口
- is_task_request_delete判断该任务是否有删除请求
- task_delete_self任务删除自己
***task.h***
```c
 30     /*Task delete*/
 31     void (*clean)(void *param);
 32     void *clean_param;
 33     uint8_t request_del_flag;
 34 }task_t;

 60 extern void task_force_delete(task_t *task);
 61 extern void task_request_delete(task_t *task);
 62 extern uint8_t is_task_request_delete(void);
 63 extern void task_delete_self(void);
```

首先要在task_init函数中对新加的字段进行初始化
***task.c***
```c
 22 void task_init (task_t * task, void (*entry)(void *), void *param, uint32_t prio, uint32_t * stack)
 23 {
		...
 57     task->clean = (void (*)(void *))NULL;
 58     task->clean_param = (void *)NULL;
 59     task->request_del_flag = 0;
 60 }

```
task_force_delete接口很简单，如果任务处于延时队列中，那么就将任务从延时队列删除，否则把任务从就绪表中移除，并且调用任务的清除函数。注意，不能删除处于挂起状态的任务。如果删除的任务是当前任务，那么触发一次任务调度。
```c
298 void task_force_delete(task_t *task)
299 {
300     uint32_t status = task_enter_critical();
301
302     if (task->state & OS_TASK_STATE_DELAYED) {
303         task_remove_from_delay_list(task);
304     } else if (!(task->state & OS_TASK_STATE_SUSPEND)) {
305         task_remove_from_prio_table(task);
306     }
307
308     if (task->clean) {
309         task->clean(task->clean_param);
310     }
311
312     if (g_current_task == task) {
313         task_sched();
314     }
315
316     task_exit_critical(status);
317 }
318
```
task_request_delete设置任务的request_del_flag为1即可。
```c
319 void task_request_delete(task_t *task)
320 {
321     uint32_t status = task_enter_critical();
322
323     task->request_del_flag = 1;
324
325     task_exit_critical(status);
326 }
```

is_task_request_delete返回当前任务的request_del_flag
```c
328 uint8_t is_task_request_delete()
329 {
330     uint8_t delete;
331
332     uint32_t status = task_enter_critical();
333     delete = g_current_task->request_del_flag;
334     task_exit_critical(status);
335
336     return delete;
337 }
```

task_delete_self将自己从就绪表中移除，然后调用任务清除函数，并且触发一次任务调度。因为该接口只能由运行中的任务调用，所以不存在把任务从延时队列中移除。
```c
339 void task_delete_self(void)                                      
340 {                                                                
341     uint8_t status = task_enter_critical();                      
342                                                                  
343     task_remove_from_prio_table(g_current_task);                 
344                                                                  
345     if (g_current_task->clean) {                                 
346         g_current_task->clean(g_current_task->clean_param);      
347     }                                                            
348                                                                  
349     task_sched();                                                
350                                                                  
351     task_exit_critical(status);                                  
352 }                                                                
                                                                     
```

## 应用测试
task2强制删除task1，所以task1的for循环只会运行一次，即它的打只运行一次。
task4请求删除task3，因为task3不是强制删除，所以task3的循环会运行两次，当第二次循环的时候发现task4设置了删除标志，task3就会把自己删除。
***main.c***
```c
void task1_entry(void *param)
{
    init_systick(10);

    task_set_clean_callbk(g_current_task, task1_cleanup_func, (void *)0);
    for(;;) {
        printk("%s:before delay\n", __func__);
        task_delay_s(1);
        printk("%s:after delay\n", __func__);
    }
}

void delay(uint32_t delay)
{
    while(delay--);
}

void task2_entry(void *param)
{
    uint32_t task_del = 0;
    for(;;) {
        printk("%s:before delay\n", __func__);
        task_delay_s(1);
        printk("%s:after delay\n", __func__);

        if (!task_del) {
            task_force_delete(&task1);
            task_del = 1;
        }
    }
}

void task3_entry(void *param)
{
    for(;;) {
        printk("%s:before delay\n", __func__);
        task_delay_s(1);
        printk("%s:after delay\n", __func__);
        if (is_task_request_delete()) {
            task_delete_self();
        }

        task_delay_s(1);
    }
}

void task4_entry(void *param)
{
    uint32_t task3_del = 0;
    for(;;) {
        printk("%s:before delay\n", __func__);
        task_delay_s(1);
        printk("%s:after delay\n", __func__);
        if (!task3_del) {
            task_request_delete(&task3);
            task3_del = 1;
        }
        task_delay_s(1);
    }
}
```

![](https://i.imgur.com/WasSKU3.png)
# 8 任务查询
任务查询就是查询任务在某一时刻的一些关键信息。只有extern void task_get_info(task_t *task, task_info_t *info)一个接口。因为这一节代码非常简单，直接上代码，也不测试了。
***task.h***
```c
typedef struct task_info_tag {

    uint32_t delay_ticks;
    uint32_t prio;
    uint32_t state;
    uint32_t slice;
    uint32_t suspend_cnt;
}task_info_t;
```

***task.c***
```c
void task_get_info(task_t *task, task_info_t *info)
{
    uint32_t status = task_enter_critical();

    info->delay_ticks = task->delay_ticks;
    info->prio = task->prio;
    info->state = task->state;
    info->slice = task->slice;
    info->suspend_cnt = task->suspend_cnt;

    task_exit_critical(status);
}
```


第三天将实现事件控制块和存储管理这一块相关代码。


今天实现事件控制块，存储管理以及定时器。

# 1 事件控制块
本节代码位于12_event中

什么是事件控制块呢？

可以这样理解，前面学习我们已经知道，创建一个任务需要给这个任务分配一个任务控制块，这个任务控制块存储着关于这个任务的重要信息。那么，事件控制块就好比任务里的任务控制块。它存储着这个事件的重要信息，我们说创建一个事件（信号，邮箱，消息队列），其本质的过程就是初始化这个事件控制块。

一个任务或者中断服务子程序可以通过事件控制块ECB（Event Control Blocks）来向另外的任务发信号。这里，所有的信号都被看成是事件（Event）。一个任务还可以等待另一个任务或中断服务子程序给它发送信号。这里要注意的是，只有任务可以等待事件发生，中断服务子程序是不能这样做的。

对于处于等待状态的任务，还可以给它指定一个最长等待时间，以此来防止因为等待的事件没有发生而无限期地等下去。

多个任务可以同时等待同一个事件的发生。在这种情况下，当该事件发生后，所有等待该事件的任务中，优先级最高的任务得到了该事件并进入就绪状态，准备执行。上面讲到的事件，可以是信号量、邮箱或者消息队列等。  ***引用自http://blog.csdn.net/h32dong809/article/details/7082490***

如下图是一个典型是事件过程。
最开始任务是task1，task1请求事件event0，此时event0没有事件发生，因此将task1加入event0的等待队列。然后运行的任务是task2，task2页请求event0，此时event0还是没有事件发生，因此task2加入到event0的等待队列。过了一段时间，task0发生event0事件，此时event0控制块有事件发生，将处于等待队列的task1唤醒，触发一次任务调度，当前任务转换为task1，此时task2还是依然处于事件控制块等待队列。
![](https://i.imgur.com/0vIqYrF.gif)

##代码实现

###接口定义

首先看一下事件控制块的定义，以及操作接口。当前没有一种具体的时间控制块，所以event_type_e只有一种UNKNOWN类型的事件控制块。
- event_t就是事件控制块结构定义，只包含两个字段，一个是事件控制块的类型event_type_e，另一个就是事件控制块的任务等待队列wait_list。
- event_init初始化事件控制块。
- event_wait将任务task等待事件event，state为任务等待某一种事件时的状态，这里暂时用不到，填0即可，timeout为任务task等待事件event的超时时间。 msg也是特定事件控制块所用的，这里也暂时用不上，填0即可。
- event_wakeup触发一次event事件，将它等待队列中的任务按一定规则唤醒。
- event_remove_task移除任务task中所有等待的事件。
- event_remove_all移除event中所有等待的事件。
- event_wait_count返回该事件中等待任务的个数。

***event.h***
```c
  1 #ifndef EVENT_H
  2 #define EVENT_H
  3
  4 #include <stdint.h>
  5 #include "os_stdio.h"
  6 #include "lib.h"
  7 #include "task.h"
  8
  9 typedef enum event_type_tag {
 10     EVENT_TYPE_UNKNOWN = 0,
 11 }event_type_e;
 12
 13 typedef struct event_tag {
 14     event_type_e type;
 15     list_t wait_list;
 16 }event_t;
 17
 18 extern void event_init(event_t *event, event_type_e type);
 19 extern void event_wait(event_t *event, task_t *task, void *msg, uint32_t state, uint32_t timeout);
 20 extern void event_wakeup(event_t *event, void *msg, uint32_t result);
 21 extern void event_remove_task(task_t *task, void *msg, uint32_t result);
 22 extern uint32_t event_remove_all(event_t *event, void *msg, uint32_t result);
 23 extern uint32_t event_wait_count(event_t *event);
 24
 25 #endif /*EVENT_H*/
```

### 实现
- 修改task.h, 之前任务状态只有就绪，延时和挂起，现在新增一种等待状态OS_TASK_WAIT_MASK。在task_t中增加任务控制块相关的字段，wait_event是该任务等待的事件的指针，一般来说，一个任务同一时间点只能等待一个事件。event_msg是任务像事件获取消息时的缓冲区，wait_event_result记录了任务等待事件的结果，是正常等待呢还是超时等待。

***task.h***
```c
 13 #define OS_TASK_WAIT_MASK                   (0xFF << 16)
		...
 38     /*Event control block*/
 39     struct event_tag *wait_event;
 40     void *event_msg;
 41     uint32_t wait_event_result;
 42
 43 }task_t;
```

- 在task.c中对上述几个字段进行初始化

***task.c***
```c
 63     /*Event control block*/
 64     task->wait_event = (event_t *)NULL;
 65     task->event_msg = (void *)0;
 66     task->wait_event_result = NO_ERROR;
 67
 68 }
```

- 实现event_init接口，代码很简单，初始化event的类型和等待链表即可。
***event.c***
```c
  4 void event_init(event_t *event, event_type_e type)
  5 {
  6     event->type = type;
  7     list_init(&event->wait_list);
  8 }

```

- 实现event_wait接口。1.首先将task的任务控制块相关的字段根据传入的参数相应的赋值。2.将任务task从就绪表移除，把它加入event的等待队列尾部。3.如果设置了超时，那么将该任务还要加入到延时队列中，这样当延时时间到时，能将任务从延时队列中唤醒，并且加入到就绪表。

***event.c***
```c
 10 void event_wait(event_t *event, task_t *task, void *msg, uint32_t state, uint32_t timeout)
 11 {
 12     uint32_t status = task_enter_critical();
 13
 14     task->state |= state;
 15     task->wait_event = event;
 16     task->wait_event_result = NO_ERROR;
 17
 18     task_unready(task);
 19     list_append_last(&event->wait_list, &task->prio_list_node);
 20     if (timeout != 0) {
 21         task_delay_wait(task, timeout);
 22     }
 23
 24     task_exit_critical(status);
 25 }
```

所以在task_system_tick_handler要做一些改动，加入对任务控制块的处理。如下代码, 当任务延时时间到，并且有在等待某一个事件的话，不仅要把任务从延时队列中拿到，还需要把它从相应任务控制块中的等待队列拿掉，加入到就绪表中。
***task.c***
```c
123 void task_system_tick_handler(void)
124 {
		....
133     while (temp_node != head) {
134         task = container_of(temp_node, task_t, delay_node);
135         temp_node = temp_node->next;
136         if (--task->delay_ticks == 0) {
137
138             if (task->wait_event != (event_t *)NULL) {
139                 event_remove_task(task, (void *)0, ERROR_TIMEOUT);
140             }
		...
149         }
150     }
151
```

- 实现 event_wakeup接口。该函数逻辑很简单，就是将任务控制块event等待队列中的第一个任务拿出来，然后把相应的msg和result填到任务task相应字段, 清除任务的等待状态标志。如果任务设置了等待超时，那么这个时候还需要把任务从等待队列中移除，加入到就绪表中，触发一次任务调度。
***event.c***
```c
 27 task_t *event_wakeup(event_t *event, void *msg, uint32_t result)
 28 {
 29     list_node_t *node = (list_node_t *)NULL;
 30     task_t *task = (task_t *)NULL;
 31
 32     uint32_t status = task_enter_critical();
 33
 34     if ((node = list_remove_first(&event->wait_list)) != (list_node_t *)NULL) {
 35         task = (task_t *)container_of(node, task_t, prio_list_node);
 36         task->wait_event = (event_t *)NULL;
 37         task->event_msg = msg;
 38         task->state &= ~OS_TASK_WAIT_MASK;
 39
 40         if (task->delay_ticks != 0) {
 41             task_delay_wakeup(task);
 42         }
 43         task_ready(task);
 44     }
 45
 46     task_exit_critical(status);
 47		return task; 
 48}
```

- 实现event_remove_task接口。 该接口就是将任务从自己等待的事件的等待队列中移除而已。

***event.c***
```c
 49 void event_remove_task(task_t *task, void *msg, uint32_t result)
 50 {
 51     uint32_t status = task_enter_critical();
 52
 53     list_remove(&task->wait_event->wait_list, &task->prio_list_node);
 54     task->wait_event = (event_t *)NULL;
 55     task->event_msg = msg;
 56     task->wait_event_result = result;
 57     task->state &= ~OS_TASK_WAIT_MASK;
 58
 59     task_exit_critical(status);
 60 }
```
- 实现event_remove_all接口。 这个接口跟event_wakeup实现很类似，只是event_wakeup只唤醒一个任务，而这个接口会把所有处于等待队列中的任务都唤醒。代码上实现就是遍历event的等待队列，将其移除并加入到就绪表中。
***event.h***
```c
 62 uint32_t event_remove_all(event_t *event, void *msg, uint32_t result)
 63 {
 64     list_node_t *node = (list_node_t *)NULL;
 65     uint32_t count = 0;
 66     uint32_t status = task_enter_critical();
 67
 68     DEBUG("%s:\n", __func__);
 69     count = list_count(&event->wait_list);
 70     while ((node = list_remove_first(&event->wait_list)) != (list_node_t *)NULL) {
 71         DEBUG("########\n");
 72         task_t *task = (task_t *)container_of(node, task_t, prio_list_node);
 73         task->wait_event = (event_t *)NULL;
 74         task->event_msg = msg;
 75         task->wait_event_result = result;
 76         task->state &= ~OS_TASK_WAIT_MASK;
 77         if (task->delay_ticks != 0) {
 78             task_delay_wakeup(task);
 79         }
 80         task_ready(task);
 81     }
 82
 83     task_exit_critical(status);
 84     return count;
 85 }
```

## 测试

task3和task2分别等待event_wait_normal事件，task1每次都会移除所有等待的事件。所以task2和task3总是在task1调用完event_remove_all才得到运行。
***main.c***
```c
event_t event_wait_timeout;
event_t event_wait_normal;

void task1_entry(void *param)
{
    init_systick(10);
    event_init(&event_wait_timeout, EVENT_TYPE_UNKNOWN);

    for(;;) {
        printk("%s\n", __func__);
        uint32_t wakeup_count = event_remove_all(&event_wait_normal, (void *)0, 0);
        if (wakeup_count > 0) {
            task_sched();
            printk("wakeup_count:%d\n", wakeup_count);
            printk("count:%d\n", event_wait_count(&event_wait_normal));
        }
        task_sched();
        task_delay_s(1);
    }
}

void delay(uint32_t delay)
{
    while(delay--);
}

void task2_entry(void *param)
{
    for(;;) {

        event_wait(&event_wait_normal, g_current_task, (void *)0, 0, 0);
        task_sched();
        printk("%s\n", __func__);
        task_delay_s(1);
    }
}

void task3_entry(void *param)
{
    event_init(&event_wait_normal, EVENT_TYPE_UNKNOWN);
    for(;;) {
        event_wait(&event_wait_normal, g_current_task, (void *)0, 0, 0);
        task_sched();
        printk("%s\n", __func__);
        task_delay_s(1);
    }
}


```

![](https://i.imgur.com/XLxp9aL.png)

#2 信号量

本节代码位于13_semaphore中

相信本文的读者一定是对信号量的概念以及操作的是非常了解的，本文也不是写书，就不详细解释信号量的概念和操作了。如果实在是小白入门，可以上网查信号量，资料非常多，我想重新写一遍也未必解释的比之前的好。

##信号量概述
信号量(Semaphore)，有时被称为信号灯，是在多线程环境下使用的一种设施，是可以用来保证两个或多个关键代码段不被并发调用。在进入一个关键代码段之前，线程必须获取一个信号量；一旦该关键代码段完成了，那么该线程必须释放信号量。其它想进入该关键代码段的线程必须等待直到第一个线程释放信号量。为了完成这个过程，需要创建一个信号量VI，然后将Acquire Semaphore VI以及Release Semaphore VI分别放置在每个关键代码段的首末端。确认这些信号量VI引用的是初始创建的信号量。

### 特性
抽象的来讲，信号量的特性如下：信号量是一个非负整数（车位数），所有通过它的线程/进程（车辆）都会将该整数减一（通过它当然是为了使用资源），当该整数值为零时，所有试图通过它的线程都将处于等待状态。在信号量上我们定义两种操作： Wait（等待） 和 Release（释放）。当一个线程调用Wait操作时，它要么得到资源然后将信号量减一，要么一直等下去（指放入阻塞队列），直到信号量大于等于一时。Release（释放）实际上是在信号量上执行加操作，对应于车辆离开停车场，该操作之所以叫做“释放”是因为释放了由信号量守护的资源

### 操作方式
对信号量有4种操作(#include<semaphore.h>)：
1. 初始化（initialize），也叫做建立（create） int sem_init(sem_t *sem, int pshared, unsigned int value);
2. 等信号（wait），也可叫做挂起（suspend）int sem_wait(sem_t *sem);
3. 给信号（signal）或发信号（post） int sem_post（sem_t *sem）;
4. 清理（destroy） int sem_destory(sem_t *sem);[1] 

***上述文字引用自https://baike.baidu.com/item/%E4%BF%A1%E5%8F%B7%E9%87%8F/9807501?fr=aladdin***

![](https://i.imgur.com/CbcJfvY.gif)

知乎上一个帖子讲信号量的：
https://www.zhihu.com/question/47411729/answer/126924789

## 代码实现

###接口定义
信号量无非就是PV操作，P对信号量减一，V对信号量加1。上文已经信号量操作简单解释了一下，我们来看看我们的系统中信号量的相关定义。
- sem_t 定义了信号量的结构，信号量是一种特殊的事件控制块，信号量可以理解为一种事件存在，所以它的结构中包含了一个event_t成员，如果对读者熟悉OOP，可以把信号量sem_t继承事件控制块event_t。其他的字段都是这种特殊的事件控制块独有的字段，count是信号量当前的计数值，而max_count是信号量可以操作最大的计数值。
- sem_info_t 定义了信号量的查询的结构，类似于任务查询之类的东西，并不是关键的数据结构。
- sem_init 初始化信号量sem， count是信号量初始值，max_count为信号量最大值。
- sem_acquire就是信号量的P操作，其作用就是将信号量减1，如果信号量小于0了，那么获取该信号量的任务就要进入信号量的任务等待队列，即进入到事件控制块的等待队列。wait_ticks设置等待信号量超时时间。
- sem_acquire_no_wait也是信号量的P操作，它和sem_acquire的区别是sem_acquire_no_wait也是信号量的V操作并不会进行任务的等待。如果信号量已经没有了资源，即信号量小于1，那么任务也不会等待该信号量。
- sem_release是信号量的P操作，即对信号量+1，当信号量的等待队列中存在任务时，唤醒等待队列中的第一个任务，否则就是对信号量+1，当然信号量计数值不能超过信号量的最大计数值。
- sem_get_info获取信号量sem在某一时刻的关键信息并保存到info里。
- sem_destory 销毁信号量sem。
***sem.h***
```c
  1 #ifndef SEM_H
  2 #define SEM_H
  3
  4 #include "event.h"
  5 #include "config.h"
  6
  7 typedef struct sem_tag {
  8
  9     event_t event;
 10     uint32_t count;
 11     uint32_t max_count;
 12 }sem_t;
 13
 14 typedef struct sem_info_tag {
 15     uint32_t count;
 16     uint32_t max_count;
 17     uint32_t task_count;
 18 }sem_info_t;
 19
 20 extern void sem_init(sem_t *sem, uint32_t count, uint32_t max_count);
 21 extern uint32_t sem_acquire(sem_t *sem, uint32_t wait_ticks);
 22 extern uint32_t sem_acquire_no_wait(sem_t *sem);
 23 extern void sem_release(sem_t *sem);
 24 extern void sem_get_info(sem_t *sem, sem_info_t *info);
 25 extern uint32_t sem_destory(sem_t *sem);
 26
 27 #endif /*SEM_H*/
```
### 实现
- 首先需要修改一下event.h，在event_type_e中加入信号量类型EVENT_TYPE_SEM
***event.h***
```c
  9 typedef enum event_type_tag {
 10     EVENT_TYPE_UNKNOWN  = 0,
 11     EVENT_TYPE_SEM      = 1,
 12 }event_type_e;
 13
```
- sem_init函数其实现与接口描述一样，首先初始化信号量sem中的事件控制块event成员，将其type设置为EVENT_TYPE_SEM。接着就是设置sem_init的count和max_count值。
***sem.c***
```c
  5 void sem_init(sem_t *sem, uint32_t count, uint32_t max_count)
  6 {
  7     event_init(&sem->event, EVENT_TYPE_SEM);
  8
  9     if (max_count == 0) {
 10         sem->count = count;
 11     } else {
 12         sem->count = count > max_count ? max_count : count;
 13     }
 14 }
```

- sem_acquire函数实现。从代码中看到如果sem的count大于0，那么就是对信号量的计数值count-1即可。如果信号量的计数值已经小于0了，那么将任务加入到信号量的等待队列中。这里的操作是调用了event_wait函数，而等待的事件控制块就是sem内部的event成员，将任务加入到了事件控制块的等待队列后，需要触发一次任务调度。 
从这个函数中我们就可以看到信号量其实就是对事件控制块的一种特殊的封装，其等待及唤醒的操作都是通过调用信号量sem内部事件控制块的函数来完成的。而信号量值负责对计数值的操作。
***sem.c***
```c
 16 uint32_t sem_acquire(sem_t *sem, uint32_t wait_ticks)
 17 {
 18     uint32_t status = task_enter_critical();
 19
 20     if (sem->count > 0) {
 21         --sem->count;
 22         task_exit_critical(status);
 23         return NO_ERROR;
 24     } else {
 25         event_wait(&sem->event, g_current_task, (void *)0, EVENT_TYPE_SEM, wait_ticks);
 26         task_exit_critical(status);
 27         task_sched();
 28         return g_current_task->wait_event_result;
 29     }
 30
 31 }
```

- sem_acquire_no_wait实现与sem_acquire类似，只是sem_acquire_no_wait在信号量小于1的时候直接返回即可，并不加入到信号量的等待队列中。
***sem.c***
```c
 33 uint32_t sem_acquire_no_wait(sem_t *sem)
 34 {
 35     uint32_t status = task_enter_critical();
 36
 37     if (sem->count > 0) {
 38         --sem->count;
 39         task_exit_critical(status);
 40         return NO_ERROR;
 41     } else {
 42         task_exit_critical(status);
 43         return g_current_task->wait_event_result;
 44     }
 45 }
```

- sem_release正好与sem_acquire实现相反的操作。首先判断信号量等待队列中是否有任务在等待，如果有任务等待，那么将等待队列的第一个任务唤醒即可，并且相应地触发一次任务调度。如果等待队列中不存在任务，那么就把信号量+1，然后返回即可。
***sem.c***
```c
 47 void sem_release(sem_t *sem)
 48 {
 49     uint32_t status = task_enter_critical();
 50     if (event_wait_count(&sem->event) > 0) {
 51         task_t *task = event_wakeup(&sem->event, (void *)0, NO_ERROR);
 52         if (task->prio < g_current_task->prio) {
 53             task_sched();
 54         }
 55     } else {
 56         sem->count++;
 57         if ((sem->max_count != 0) && (sem->count > sem->max_count)) {
 58             sem->count = sem->max_count;
 59         }
 60     }
 61     task_exit_critical(status);
 62 }
```

- sem_get_info函数实现，很简单，无须解释。
***sem.c***
```c
 64 void sem_get_info(sem_t *sem, sem_info_t *info)
 65 {
 66     uint32_t status = task_enter_critical();
 67     info->count = sem->count;
 68     info->max_count = sem->max_count;
 69     info->task_count = event_wait_count(&sem->event);
 70     task_exit_critical(status);
 71 }
```

- sem_destory将信号销毁。其操作就是将信号量等待等待队列中所有的任务移除，并且将信号量置为0。如果信号量等待队列中存在任务的话，在移除之后就触发一次任务调度。
***sem.c***
```c
 73 uint32_t sem_destory(sem_t *sem)
 74 {
 75     uint32_t status = task_enter_critical();
 76
 77     uint32_t count = event_remove_all(&sem->event, (void *)0, ERROR_DEL);
 78     sem->count = 0;
 79     task_exit_critical(status);
 80
 81     if (count > 0) {
 82         task_sched();
 83     }
 84     return count;
 85 }
```

## 测试
测试代码中定义了两个信号量sem1和sem2。task1初始化信号量为0，所以task1一旦调用sem_acquire它就会等待sem1发生，而task2中调用sem_release来释放信号量，此时会将等待中task1唤醒。task1的打印在sem_acquire后面。所以即使task1的打印会在task2的打印之后打印。
task3初始化信号量sem2为0，此时调用sem_acquire会进行到等待状态，它填写了超时时间，所以即可没有其他的任务来释放sem2，当超时时间一旦到达，task3会放弃等待sem2，继续运行。所以从log来看task2先打印，然后task1。而task3与task1和task2无关。
***main.c***
```c

sem_t sem1;
sem_t sem2;

void task1_entry(void *param)
{
    init_systick(10);

    sem_init(&sem1, 0, 10);
    for(;;) {
        sem_acquire(&sem1, 0);
        printk("%s\n", __func__);
        task_delay_s(1);
    }
}

void delay(uint32_t delay)
{
    while(delay--);
}

void task2_entry(void *param)
{
    for(;;) {

        printk("%s\n", __func__);
        task_delay_s(1);
        sem_release(&sem1);
    }
}

void task3_entry(void *param)
{
    sem_init(&sem2, 0, 10);
    for(;;) {
        sem_acquire(&sem2, 500);
        printk("%s\n", __func__);
        task_delay_s(1);
    }
}

```

![](https://i.imgur.com/SvqSMMN.png)

# 3 邮箱
在多任务操作系统中，通常需要在任务与任务之间传递一个数据(这种数据叫做消息)的方式来进行通信。为了达到这个目的，可以在内存中创建一个存储空间作为该数据的缓冲区。如果把这个缓冲区叫做消息缓冲区，那么在任务之间传递数据的一个最简单的办法就是传递消息缓冲区的指针。因此，用来传递消息缓冲区指针的数据结构就叫做消息邮箱。 ***引用自《嵌入式实时操作系统uc/OS-II 原理及应用(第四版)》***

下图就是一次邮箱发送与获取的过程。初试状态下，邮箱里并没有数据可以读，而task1申请读取邮箱消息，所以task1进入了邮箱的等待队列。过了一会task0往邮箱里发送了一个消息，这个时候邮箱的write指针向后移一位，并且将task1从等待队列中唤醒，同时task1把数据从邮箱中读出来。
![](https://i.imgur.com/RFfD5sT.gif)

## 代码实现

### 接口定义
mbox_t定义了邮箱结构。同样邮箱是一种特殊的事件控制块，所以mbox_t和信号量一样包含了一个event_t成员。 count是邮箱当前消息的数量，read是邮箱缓冲区的读索引，write是邮箱缓冲区的写索引，max_count是邮箱缓冲区的长度，也就是邮箱最大允许容纳的消息数量。

mbox_init初始化邮箱mbox，msg_buffer事传给邮箱缓冲区的地址。
mbox_get从邮箱中读数据，msg是待读取消息的地址，wait_ticks同样是等到超时的参数。
mbox_get_no_wait从邮箱读取消息，但当邮箱没有消息时，任务不会进行等待。
mbox_send往邮箱mbox发消息，msg是消息的地址，notify_opition是发送消息的选项，后面实现会讲到是干什么的。
mbox_flush清空邮箱mbox中所有消息。
mbox_destory销毁邮箱mbox。
mbox_get_info查询邮箱某一时刻的信息。
***mailbox.h***
```c
  2 #define MAILBOX_H
  3
  4 #include "event.h"
  5 #include "config.h"
  6
  7 #define MBOX_SEND_FRONT         0x12345678
  8 #define MBOX_SEND_NORMAL        0
  9 typedef struct mbox_tag {
 10
 11     event_t event;
 12     uint32_t count;
 13     uint32_t read;
 14     uint32_t write;
 15     uint32_t max_count;
 16     void **msg_buffer;
 17
 18 }mbox_t;
 19
 20 typedef struct mbox_info_tag {
 21     uint32_t count;
 22     uint32_t max_count;
 23     uint32_t task_count;
 24 }mbox_info_t;
 25
 26 extern void mbox_init(mbox_t *mbox, void **msg_buffer, uint32_t max_count);
 27 extern uint32_t mbox_get(mbox_t *mbox, void **msg, uint32_t wait_ticks);
 28 extern uint32_t mbox_get_no_wait(mbox_t *mbox, void **msg);
 29 extern uint32_t mbox_send(mbox_t *mbox, void *msg, uint32_t notify_opition);
 30 extern void mbox_flush(mbox_t *mbox);
 31 extern uint32_t mbox_destory(mbox_t *mbox);
 32 extern void mbox_get_info(mbox_t *mbox, mbox_info_t *info);
```

### 实现

- mbox_init实现：初始化邮箱的事件控制块event，并根据传入的参数填写相应的字段。
***mbox.c***
```c
  5 void mbox_init(mbox_t *mbox, void **msg_buffer, uint32_t max_count)
  6 {
  7     event_init(&mbox->event, EVENT_TYPE_MAILBOX);
  8     mbox->msg_buffer = msg_buffer;
  9     mbox->max_count = max_count;
 10     mbox->read = 0;
 11     mbox->write = 0;
 12     mbox->count = 0;
 13 }
```

- mbox_get实现：从邮箱mbox中获取消息，消息保存到msg所指向的地址中。当邮箱中有消息的时候，将邮箱消息数量count-1， 并从邮箱中消息缓冲区msg_buffer读出一个消息给msg，并且将邮箱的读指针read自增1，当read大于邮箱的最大长度时，read会指向0。也就是说，邮箱中的消息缓冲区是一个长度为max_count的循环缓冲区。<br/>当邮箱中没有消息的时候，系统将当前任务加入到邮箱的等待队列中，安排一次任务调度。当调度再次回到这个任务的时候，会将事件控制块中的消息赋值给msg指针并且返回。
***mbox.c***
```c
 15 uint32_t mbox_get(mbox_t *mbox, void **msg, uint32_t wait_ticks)
 16 {
 17     uint32_t status = task_enter_critical();
 18
 19     if (mbox->count > 0) {
 20
 21         mbox->count--;
 22         *msg = mbox->msg_buffer[mbox->read++];
 23         if (mbox->read >= mbox->max_count) {
 24             mbox->read = 0;
 25         }
 26         task_exit_critical(status);
 27         return NO_ERROR;
 28     } else {
 29         event_wait(&mbox->event, g_current_task, (void *)0, EVENT_TYPE_MAILBOX, wait_ticks);
 30         task_exit_critical(status);
 31         task_sched();
 32
 33         *msg = g_current_task->event_msg;
 34         return g_current_task->wait_event_result;
 35     }
 36 }
```

- mbox_get_no_wait实现：该函数与mbox_get差不多，只不过在邮箱没有消息的时候不会等待任务。
***mbox.c***
```c
 38 uint32_t mbox_get_no_wait(mbox_t *mbox, void **msg)
 39 {
 40     uint32_t status = task_enter_critical();
 41
 42     if (mbox->count > 0) {
 43
 44         mbox->count--;
 45         *msg = mbox->msg_buffer[mbox->read++];
 46         if (mbox->read >= mbox->max_count) {
 47             mbox->read = 0;
 48         }
 49         task_exit_critical(status);
 50         return NO_ERROR;
 51     } else {
 52         task_exit_critical(status);
 53         return g_current_task->wait_event_result;
 54     }
 55 }
```
- mbox_send实现稍微有点长，我们一点一点来看，首先判断邮箱中是否有任务在等待啊。<br/>如果有，那就唤醒等待中的第一个任务，然后直接把传递的消息赋值给唤醒任务控制块中的消息指针，不对邮箱的读写指针改动。<br/>如果没有等待的任务，首先判断邮箱内的消息已经满了，如果邮箱满了，那么就不能再往邮箱里写消息，并返回ERROR_RESOURCE_FULL。如果邮箱没满，那么就往邮箱里写一个数据。这里也分两种情况，如果notify_opition是MBOX_SEND_FRONT，那么数据往前写，然后把读指针read往前挪一个。否则就是正常写，把写指针write往后挪一个。写完后把邮箱的消息计数count+1。
***mbox.c***
```c
uint32_t mbox_send(mbox_t *mbox, void *msg, uint32_t notify_opition)
{
    uint32_t status = task_enter_critical();
    task_t *task = (task_t *)NULL;

    if (event_wait_count(&mbox->event) > 0) {
        task = event_wakeup(&mbox->event, (void *)msg, NO_ERROR);
        if (task->prio < g_current_task->prio) {
            task_sched();
        }
    } else {
        if (mbox->count >= mbox->max_count) {
            task_exit_critical(status);
            return ERROR_RESOURCE_FULL;
        }

        if (notify_opition & MBOX_SEND_FRONT) {
            if (mbox->read <= 0) {
                mbox->read = mbox->max_count - 1;
            } else {
                mbox->read--;
            }
            mbox->msg_buffer[mbox->read] = msg;
        } else {
            mbox->msg_buffer[mbox->write++] = msg;
            if (mbox->write >= mbox->max_count) {
                mbox->write = 0;
            }
        }

        mbox->count++;
    }
    task_exit_critical(status);
    return NO_ERROR;
}
```

- mbox_flush 代码很简单，如果邮箱内没有等待的任务，那么把邮箱内的各个字段清0即可。
***mbox.c***
```c
 93 void mbox_flush(mbox_t *mbox)
 94 {
 95     uint32_t status = task_enter_critical();
 96
 97     if (event_wait_count(&mbox->event) == 0) {
 98         mbox->read = 0;
 99         mbox->write = 0;
100         mbox->count = 0;
101     }
102
103     task_exit_critical(status);
104 }
```
- mbox_destory移除邮箱等待队列中的所有任务，并且触发一次任务调度。
***mbox.c***
```c
106 uint32_t mbox_destory(mbox_t *mbox)
107 {
108     uint32_t status = task_enter_critical();
109
110     uint32_t count = event_remove_all(&mbox->event, (void *)0, ERROR_DEL);
111
112     task_exit_critical(status);
113     if (count > 0) {
114         task_sched();
115     }
116
117     return count;
118 }
```

- mbox_get_info实现代码如下：
***mbox.c***
```c
120 void mbox_get_info(mbox_t *mbox, mbox_info_t *info)
121 {
122     uint32_t status = task_enter_critical();
123     info->count = mbox->count;
124     info->max_count = mbox->max_count;
125     info->task_count = event_wait_count(&mbox->event);
126     task_exit_critical(status);
127 }
```

## 测试
测试代码中定义了一个邮箱mbox1，task1第一次往邮箱中以MBOX_SEND_NORMAL写入数据，然后延时20s，然后task1以MBOX_SEND_FRONT方式写入数据，然后延时20s。<br/>而task2一次性读取了20个数据，因此在task1延时的20s内，task2处于等待邮箱状态，也不会执行。所以只有等到task1再次往邮箱里写数据的时候，task2才被唤醒并读取数据。log上看不出来task2延时的效率。读者只需要make run一下就能看到运行的时序是如何的。
***mbox.c***
```c
mbox_t mbox1;
void *mbox1_msg_buffer[20];
uint32_t msg[20];

void task1_entry(void *param)
{
    uint32_t i = 0;
    init_systick(10);

    mbox_init(&mbox1, mbox1_msg_buffer, 20);
    for(;;) {
        printk("%s:send mbox\n", __func__);
        for (i = 0; i < 20; i++) {
            msg[i] = i;
            mbox_send(&mbox1, &msg[i], MBOX_SEND_NORMAL);
        }
        task_delay_s(20);

        printk("%s:send mbox front\n", __func__);
        for (i = 0; i < 20; i++) {
            msg[i] = i;
            mbox_send(&mbox1, &msg[i], MBOX_SEND_FRONT);
        }
        task_delay_s(20);
        task_delay_s(1);
    }
}

void task2_entry(void *param)
{
    void *msg;
    uint32_t err = 0;
    for(;;) {
        err = mbox_get(&mbox1, &msg, 10);
        if (err == NO_ERROR) {
            uint32_t value = *(uint32_t *)msg;
            printk("%s:value:%d\n", __func__, value);

        }
    }
}

```

![](https://i.imgur.com/o6vp8Lp.png)

# 4 内存分区
本节代码位于15_mem_block中


应用程序在运行中为了某种特殊需要，经常需要临时获得一些内存空间，因此作为一个比较完善的操作系统，必须具有动态分配内存的能力。对于RTOS来说，还需要保证系统在动态分配内存时，其执行时间是固定的。本文中的存储块分配与uCosII类似，采用固定大小的内存块。这样一来简化了代码实现，二来能使内存管理能够在O(1)内完成。

存储块进行两级管理，即把一个连续的内存空间分为若干个分区，每个分区又分为若干个大小的内存块。RTOS以分区为单位来管理动态内存，而任务以内存块为单位来获取和释放内存。内存分区及内存块的使用情况由内存控制块来记录。所以在代码中定义一个内存分区及其内存块的方法非常简单，只需要定义二维数组即可。例如：
```c
uint32_t mem_buf[10][10]; /*定义了1个内存分区，每个内存分区有10个内存块，每个内存块大小为10个word*/
```
其分配释放过程如下图。
1. 首先图中有一个只有一个内存块的内存分区，task1申请获得一个内存块，此时内存分区内不再有内存块。
2. 过了一段时间task0申请内存块，而此时内存分区内并没有内存块可以分配，task0进入内存块的等待队列。
3. task1释放内存块，此时RTOS将唤醒task0，并且将内存块分配给task0。

相信读者从上面可以看出来存储块分配释放也是一种事件，因此存储块的实现也可以依赖于事件控制块来实现。
![](https://i.imgur.com/Y9QIrwI.gif)

##代码实现

###结构定义
首先我们看内存分区的结构定义，
- 内存分区mem_block_t中包含了一个event_t字段，用来处理内存分区事件。
- start定义了内存分区的起始地址
- block_size定义了内存分区每一个内存快的大小。
- max_count定义了内存分区中最多有几个内存块
- block_list是内存分区中内存块链表。当内存块不被使用的使用，它就被插入到block_list中，当内存被任务申请时，它就从block_list中移除。

***memblock.h***
```c
 11 typedef struct mem_block_tag {
 12     event_t event;
 13     void *start;
 14     uint32_t block_size;
 15     uint32_t max_count;
 16     list_t block_list;
 17 }mem_block_t;
 18
```

###接口实现

mem_block_init函数初始化内存分区。
- start:内存分区的起始地址
- block_size:每一个内存块的大小
- block_cnt:内存块个数。
- 初始化过程很简单，首先根据传入的参数初始化各个字段。
- 然后初始化block_list
- 将内存块一个一个链接到block_list上，这里很巧妙的是，当内存块不使用的时候，将内存块强制转换成list_node_t结构，链接到该链表上，当内存块需要被使用的时候，根据相应的类型强制转换一下即可。这样可以省下一个list_node_t的空间。

根据上述的描述，可以看到内存分区与内存块的关系如下图。

![](https://i.imgur.com/W74qosH.gif)


***memblock.c***
```c
  4 void mem_block_init(mem_block_t *mem_block, uint8_t *start, uint32_t block_size, uint32_t block_cnt)
  5 {
  6     uint8_t *mem_block_start = (uint8_t *)start;
  7     uint8_t *mem_block_end = mem_block_start + block_size * block_cnt;
  8
  9     if (block_size < sizeof(list_node_t)) {
 10         goto cleanup;
 11     }
 12
 13     event_init(&mem_block->event, EVENT_TYPE_MEM_BLOCK);
 14
 15     mem_block->start = start;
 16     mem_block->block_size = block_size;
 17     mem_block->max_count = block_cnt;
 18     list_init(&mem_block->block_list);
 19
 20
 21     while (mem_block_start < mem_block_end) {
 22         list_node_init((list_node_t *)mem_block_start);
 23         list_append_last(&mem_block->block_list, (list_node_t *)mem_block_start);
 24         mem_block_start += block_size;
 25     }
 26 cleanup:
 27     return;
 28
 29 }
```

mem_block_alloc从mem_block中分配一个内存块到mem。
- mem:分配的内存块的地址。
- wait_ticks:分配超时时间，与其他事件控制块类似。
- 分配逻辑很简单，如果内存分区还有内存块，那么直接从内存块链表中拿出第一个内存块给任务并返回。
- 如果内存块链表中不存在内存块，那么将当前任务插入到内存块等待链表尾部，并且触发一次任务调度。当任务被唤醒时，从event_msg中拿到释放的内存块地址返回。


***memblock.c***
```c
 31 uint32_t mem_block_alloc(mem_block_t *mem_block, uint8_t **mem, uint32_t wait_ticks)
 32 {
 33     uint32_t status = task_enter_critical();
 34
 35     if (list_count(&mem_block->block_list) > 0) {
 36         *mem = (uint8_t *)list_remove_first(&mem_block->block_list);
 37         task_exit_critical(status);
 38         return NO_ERROR;
 39     } else {
 40         event_wait(&mem_block->event, g_current_task, (void *)0, EVENT_TYPE_MEM_BLOCK, wait_ticks);
 41         task_exit_critical(status);
 42         task_sched();
 43         *mem = g_current_task->event_msg;
 44         return g_current_task->wait_event_result;
 45     }
 46 }
```

mem_block_alloc_no_wait与mem_block_alloc唯一差异是mem_block_alloc_no_wait当没有可用的内存块时，不会阻塞任务的运行。

***memblock.c***
```c
 48 uint32_t mem_block_alloc_no_wait(mem_block_t *mem_block, uint8_t **mem)
 49 {
 50     uint32_t status = task_enter_critical();
 51
 52     if (list_count(&mem_block->block_list) > 0) {
 53         *mem = (uint8_t *)list_remove_first(&mem_block->block_list);
 54         task_exit_critical(status);
 55         return NO_ERROR;
 56     } else {
 57         task_exit_critical(status);
 58         return ERROR_RESOURCE_FULL;
 59     }
 60
 61 }
```

mem_block_free释放地址为mem的内存块。内部实现很简单
- 当内存分区等待队列中存在等待任务时，唤醒等待队列中的第一个任务，然后将该内存块分配给唤醒的任务。如果唤醒的任务优先级高于当前任务，那么就触发一次任务调度。
- 当内存分区等待队列中没有任务等待时，直接将该内存块插入到内存块链表的尾部。

***memblock.c***
```c
 63 void mem_block_free(mem_block_t *mem_block, uint8_t *mem)
 64 {
 65     uint32_t status = task_enter_critical();
 66     if (event_wait_count(&mem_block->event) > 0) {
 67         task_t *task = event_wakeup(&mem_block->event, (void *)mem, NO_ERROR);
 68         if (task->prio > g_current_task->prio) {
 69             task_sched();
 70         }
 71     } else {
 72         list_append_last(&mem_block->block_list, (list_node_t *)mem);
 73     }
 74
 75     task_exit_critical(status);
 76 }
```

mem_block_destory销毁内存块，将内存块中等待队列全部唤醒。
***memblock.c***
```c
 88 uint32_t mem_block_destory(mem_block_t *mem_block)
 89 {
 90     uint32_t status = task_enter_critical();
 91
 92     uint32_t count = event_remove_all(&mem_block->event, (void *)0, ERROR_DEL);
 93
 94     task_exit_critical(status);
 95
 96     if (count > 0) {
 97         task_sched();
 98     }
 99     return count;
100 }
```
## 测试
- task1初始化了一个内存分区mem1[20][100]。然后申请二十个内存块，然后释放。延时5s后再次分配，可以看到每次分配的内存确实就是内存分区中内存块。

***main.c***
```c
 29 /*20 block of size 100 bytes*/
 30 uint8_t mem1[20][100];
 31 mem_block_t mem_block1;
 32 typedef uint8_t (*block_t)[100];
 33
 34 void task1_entry(void *param)
 35 {
 36     uint8_t i;
 37     block_t block[20];
 38     init_systick(10);
 39
 40     mem_block_init(&mem_block1, (uint8_t *)mem1, 100, 20);
 41     for(;;) {
 42         printk("%s\n", __func__);
 43         for (i = 0; i < 20; i++) {
 44             mem_block_alloc(&mem_block1, (uint8_t **)&block[i], 0);
 45             printk("block:%x, mem[i]:%x\n", block[i], &mem1[i][0]);
 46         }
 47
 48         for (i = 0; i < 20; i++) {
 49             mem_block_free(&mem_block1, (uint8_t *)block[i]);
 50         }
 51         task_delay_s(5);
 52     }
 53 }
 54                                                             
```
![](https://i.imgur.com/5L7rDMm.png)

# 5 定时器
考虑平台硬件定时器个数限制的，RTOS 通过一个定时器任务管理软定时器组，满足用户定时需求。定时器任务会在其执行期间检查用户启动的时间周期溢出的定时器，并调用其回调函数。本文RTOS同时支持一个硬定时器组，硬定时器组是直接systick对定时器组计数，而软定时器是在定时器任务中对定时器进行操作。

##定时器状态
定时器一共有5种状态,定义如下。很好理解，一共是创建，启动，运行，停止和销毁。

***timer.h***
```c
  6 typedef enum timer_state{
  7     TIMER_CREATED,
  8     TIMER_STARTED,
  9     TIMER_RUNNING,
 10     TIMER_STOPPED,
 11     TIMER_DESTORYED,
 12 }timer_state_e;
```
其状态之间的关系如下图：
![](https://i.imgur.com/R6uqBMf.gif)

##定时器运行原理
下面以软定时器组为例，来阐述定时器的运行原理。硬定时器组原理一样，只是计数的地方不一样而已。 定时器任务有一个定时器链表，下面挂了三个定时器，timer0，timer1和timer2。发生一次systick中断，就会定时器链表中所有的定时器的计数器减1，当某一个定时器定时时间到达之后，会调用挂载定时器下的定时器回调函数。如下图所示，timer0定时时间到，会调用timer0的func0，打印hello。
![](https://i.imgur.com/xuHLLln.gif)

## 代码实现

###定时器定义

timer_t定义了定时器的结构
- link_node用于链接到定时器链表中。
- duration_ticks 周期定时时的周期tick数
- start_delay_ticks 初次启动延后的ticks数
- delay_ticks 当前定时递减计数值
- timer_func 定时回调函数
- arg 传递给回调函数的参数
- config 定时器配置参数
- state 定时器状态

TIMER_CONFIG_TYPE_HARD和TIMER_CONFIG_TYPE_SOFT用于设置定时器是软定时器组还是硬定时器组。
***timer.h***
```c
 14 typedef struct timer_tag {
 15
 16     list_node_t link_node;
 17     uint32_t duration_ticks;
 18     uint32_t start_delay_ticks;
 19     uint32_t delay_ticks;
 20     void (*timer_func)(void *arg);
 21     void *arg;
 22     uint32_t config;
 23
 24     timer_state_e state;
 25 }timer_t;
 26
 27 #define TIMER_CONFIG_TYPE_HARD      (1 << 0)
 28 #define TIMER_CONFIG_TYPE_SOFT      (0 << 0)
```

###接口实现
timer_init根据传入的参数初始化定时器timer，并将timer的状态设置成TIMER_CREATED。
***timer.c***
```c
 10 void timer_init(timer_t *timer, uint32_t delay_ticks, uint32_t duration_ticks,
 11         void(*timer_func)(void *arg), void *arg, uint32_t config)
 12 {
 13     list_node_init(&timer->link_node);
 14     timer->start_delay_ticks = delay_ticks;
 15     timer->duration_ticks  = duration_ticks;
 16     timer->timer_func = timer_func;
 17     timer->arg = arg;
 18     timer->config = config;
 19
 20     if (delay_ticks == 0) {
 21         timer->delay_ticks = duration_ticks;
 22     } else {
 23         timer->delay_ticks = timer->start_delay_ticks;
 24     }
 25
 26     timer->state = TIMER_CREATED;
 27 }
```

timer_module_init初始化定时器组模块，代码很简单，初始化硬定时器链表和软定时器链表，并且初始化软定时器任务timer_soft_task，timer_soft_task的优先级为1，属于较高优先级的任务。 

***timer.c***
```c
 29 static task_t g_timer_task;
 30 static task_stack_t g_timer_task_stack[OS_TIMERTASK_STACK_SIZE];

103 static void timer_soft_task(void *param)
104 {
105     for(;;) {
106         sem_acquire(&g_timer_tick_sem, 0);
107         sem_acquire(&g_timer_protect_sem, 0);
108
109         timer_call_func_list(&g_timer_soft_list);
110         sem_release(&g_timer_protect_sem);
111     }
112 }

123 void timer_module_init()
124 {
125     list_init(&g_timer_hard_list);
126     list_init(&g_timer_soft_list);
127     sem_init(&g_timer_protect_sem, 1, 1);
128     sem_init(&g_timer_tick_sem, 0, 0);
129
130     task_init(&g_timer_task, &timer_soft_task, (void *)0,
131             OS_TIMERTASK_PRIO, &g_timer_task_stack[OS_TIMERTASK_STACK_SIZE]);
132
133 }
```

timer_start启动定时器timer，只有当定时器处于TIMER_CREATED和TIMER_STOPPED的时候才能够启动定时器。启动定时器分为以下步骤：
1. 设置定时器的delay_ticks如果定时器没有起始定时时间，那么之间将周期定时时间赋给定时器的delay_ticks。
2. 将定时器的状态设置为TIMER_STARTED
3. 根据定时器的配置，将定时器插入到不同的定时器链表中。当配置为软定时器组时，将定时器插入到g_timer_hard_list中，反之，则插入到g_timer_soft_list中。
***timer.c***
```c
 33 void timer_start(timer_t *timer)
 34 {
 35     switch(timer->state) {
 36     case TIMER_CREATED:
 37     case TIMER_STOPPED:
 38         timer->delay_ticks = timer->start_delay_ticks ? timer->start_delay_ticks : timer->duration_ticks;
 39         timer->state = TIMER_STARTED;
 40
 41         if (timer->config & TIMER_CONFIG_TYPE_HARD) {
 42             uint32_t status = task_enter_critical();
 43             list_append_last(&g_timer_hard_list, &timer->link_node);
 44             task_exit_critical(status);
 45         } else {
 46             sem_acquire(&g_timer_protect_sem, 0);
 47             list_append_last(&g_timer_soft_list, &timer->link_node);
 48             sem_release(&g_timer_protect_sem);
 49         }
 50         break;
 51     default:
 52         break;
 53     }
 54 }
```

timer_stop停止定时器timer。其操作很简单，正好与timer_start反操作，直接将定时器从相应的定时器链表中移除即可。
***timer.c***
```c
 56 void timer_stop(timer_t *timer)
 57 {
 58     switch(timer->state) {
 59     case TIMER_STARTED:
 60     case TIMER_RUNNING:
 61         if (timer->config & TIMER_CONFIG_TYPE_HARD) {
 62             uint32_t status = task_enter_critical();
 63             list_remove(&g_timer_hard_list, &timer->link_node);
 64             task_exit_critical(status);
 65         } else {
 66             sem_acquire(&g_timer_protect_sem, 0);
 67             list_remove(&g_timer_soft_list, &timer->link_node);
 68             sem_release(&g_timer_protect_sem);
 69         }
 70         timer->state = TIMER_STOPPED;
 71         break;
 72     default:
 73         break;
 74     }
 75 }
```

timer_destory销毁定时器timer，停止定时器并且将定时器的状态设置为TIMER_DESTORYED。
***timer.c***
```c
 97 void timer_destory(timer_t *timer)
 98 {
 99     timer_stop(timer);
100     timer->state = TIMER_DESTORYED;
101 }
```

从上文中可以看到软定时器任务timer_soft_task只做了一件事，就是调用timer_call_func_list这个函数。这个函数就是
1. 将定时器的状态设置为TIMER_RUNNING。
2. 根据传入了传入的定时器链表，遍历该定时器链表，然后调用定时器回调函数timer->timer_func(timer->arg);。
3. 根据定时器的周期定时数来判断定时器是否是周期定时器。如果duration_ticks是0，那么该定时器是一次性的定时器，在一次定时完成后，将停止定时器。如果duration_ticks不为0，那么将duration_ticks重新赋给delay_ticks，那么定时器又开始重新计数了。

***timer.c***
```c
 77 static void timer_call_func_list(list_t *timer_list)
 78 {
 79     list_node_t *node;
 80     timer_t *timer;
 81
 82     for (node = timer_list->head.next; node != &(timer_list->head); node = node->next) {
 83         timer = container_of(node, timer_t, link_node);
 84         if ((timer->delay_ticks == 0) || (--timer->delay_ticks == 0)) {
 85             timer->state = TIMER_RUNNING;
 86             timer->timer_func(timer->arg);
 87             timer->state = TIMER_STARTED;
 88             if (timer->duration_ticks > 0) {
 89                 timer->delay_ticks = timer->duration_ticks;
 90             } else {
 91                 timer_stop(timer);
 92             }
 93         }
 94     }
 95 }
```

timer_module_tick_notify用于硬定时器组。 该函数在task_system_tick_handle中调用。这样硬定时器组就是由systick计数，而非软定时器任务。这样硬定时器的优先级的最高的，因此硬定时器组的定时器总能在较快的时间内进行相应。
***timer.c***
```c
114 void timer_module_tick_notify(void)
115 {
116     uint32_t status = task_enter_critical();
117
118     timer_call_func_list(&g_timer_hard_list);
119     task_exit_critical(status);
120     sem_release(&g_timer_tick_sem);
121 }
```

***task.c***
```c
125 void task_system_tick_handler(void)
126 {
		......
166     timer_module_tick_notify();
167     task_exit_critical(status);
168
169     task_sched();
170 }
```

## 测试
task1中使用了3个定时器timer1，timer2和timer3。
- timer1 起始延时为100个tick(即1s)，周期延时1s，即每1s回调一次timer_func1，参数为0，将其配置为硬定时器组。
- timer2 起始延时为200个tick(即2s)，周期延时2s，即每1s回调一次timer_func2，参数为0，将其配置为软定时器组。
- timer3 起始延时为300个tick(即3s)，没有周期延时，即只一次性调用timer_func3, 参数为0，将其配置为软定时器组。

task1延时10s后，停止timer2。
然后再延时10s，销毁timer1，此时timer1再也不会运行了。同时启动timer2。
随后延时10s，销毁timer2。至此任务中所有定时器都不再执行。
***main.c***
```c
 30 timer_t timer1;
 31 timer_t timer2;
 32 timer_t timer3;
 33
 34 static void timer_func1(void *arg)
 35 {
 36     printk("timer_func1\n");
 37 }
 38
 39 static void timer_func2(void *arg)
 40 {
 41     printk("timer_func2\n");
 42 }
 43
 44 static void timer_func3(void *arg)
 45 {
 46     printk("timer_func3\n");
 47 }
 54 void task1_entry(void *param)
 55 {
 56     uint32_t stopped = 0;
 57     init_systick(10);
 58
 59     timer_init(&timer1, 100, 100, timer_func1, (void *)0, TIMER_CONFIG_TYPE_HARD);
 60     timer_start(&timer1);
 61     timer_init(&timer2, 200, 200, timer_func2, (void *)0, TIMER_CONFIG_TYPE_SOFT);
 62     timer_start(&timer2);
 63
 64     timer_init(&timer3, 300, 0, timer_func3, (void *)0, TIMER_CONFIG_TYPE_HARD);
 65     timer_start(&timer3);
 66
 67     for(;;) {
 68         if (stopped == 0) {
 69             task_delay(1000);
 70             timer_stop(&timer2);
 71             task_delay(1000);
 72             timer_start(&timer2);
 73             timer_destory(&timer1);
 74             task_delay(1000);
 75             timer_destory(&timer2);
 76             stopped = 1;
 77         }
 78     }
 79 }
```
![](https://i.imgur.com/SoS8e47.png)

# 6 互斥锁
## 优先级反转
## 嵌套操作
## 代码实现
## 测试


