#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

void Delay(uint32_t count)
{
    uint32_t i;
    for(i = 0; i < count; i++) { }
}

void main()
{
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ); // set clock to 80 MHz

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // enable GPIO PORTF

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) { } // wait for GPIO

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3); // RGB LEDs on launchpad as outputs

    while(1)
    {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_1); // red
        Delay(2000000);

        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_2); // blue
        Delay(2000000);

        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_3); // green
        Delay(2000000);

        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0x00); // all off
        Delay(2000000);
    }
}
