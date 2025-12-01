#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

void PortF_Init(void);
void Timer0_Init(void);
void Timer0A_Handler(void);

int main()
{
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ); // clock to 80 MHz

    PortF_Init();
    Timer0_Init();

    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);

    while(1) { }
}

void PortF_Init()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));

    GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_DIR_MODE_OUT);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
}

void Timer0_Init()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));

    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() - 1); // set timer period to 1 second (40MHz / 40000000 = 1Hz)

    TimerIntRegister(TIMER0_BASE, TIMER_A, Timer0A_Handler);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    IntEnable(INT_TIMER0A);
    TimerEnable(TIMER0_BASE, TIMER_A);
}

void Timer0A_Handler()
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    uint32_t currentState = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_1);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, currentState ? 0 : GPIO_PIN_1);
}
