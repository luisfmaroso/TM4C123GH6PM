#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/pin_map.h"

void Timer0_PWM_Init(void);
void SetLEDBrightness(float dutyCycle);

int main()
{
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ); // clock to 80 MHz

    Timer0_PWM_Init();

    float brightness = 0.0;

    while(1)
    {
        SetLEDBrightness(brightness);

        brightness += 10.0;
        if(brightness > 100.0)
            brightness = 0.0;

        SysCtlDelay(SysCtlClockGet() / 6);  // 500ms delay
    }
}

void Timer0_PWM_Init()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));

    GPIOPinConfigure(GPIO_PB6_T0CCP0);
    GPIOPinTypeTimer(GPIO_PORTB_BASE, GPIO_PIN_6);

    TimerDisable(TIMER0_BASE, TIMER_A);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM);

    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() / 1000 - 1);

    SetLEDBrightness(0);

    TimerEnable(TIMER0_BASE, TIMER_A);
}

void SetLEDBrightness(float dutyCycle)
{
    if(dutyCycle < 0.0) dutyCycle = 0.0;
    if(dutyCycle > 100.0) dutyCycle = 100.0;

    uint32_t period = SysCtlClockGet() / 1000;
    uint32_t highTime = (uint32_t)(period * dutyCycle / 100.0);
    uint32_t matchValue = period - highTime;

    TimerMatchSet(TIMER0_BASE, TIMER_A, matchValue - 1);
}
