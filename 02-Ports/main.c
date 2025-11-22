#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"

#define LED1 GPIO_PIN_0
#define LED2 GPIO_PIN_1
#define LED3 GPIO_PIN_2

void Delay(uint32_t count)
{
    uint32_t i;
    for(i = 0; i < count; i++)
    {
    }
}

void main()
{
    // set system clock to 80 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)) { }

    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, LED1 | LED2 | LED3);

    while(1)
    {
        GPIOPinWrite(GPIO_PORTB_BASE, LED1 | LED2 | LED3, LED1);
        Delay(1000000);

        GPIOPinWrite(GPIO_PORTB_BASE, LED1 | LED2 | LED3, LED2);
        Delay(1000000);

        GPIOPinWrite(GPIO_PORTB_BASE, LED1 | LED2 | LED3, LED3);
        Delay(1000000);

        GPIOPinWrite(GPIO_PORTB_BASE, LED1 | LED2 | LED3, 0x00);
        Delay(1000000);
    }
}
