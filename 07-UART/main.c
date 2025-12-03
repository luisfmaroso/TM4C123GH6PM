#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

#define GPIO_PA0_U0RX 0x00000001
#define GPIO_PA1_U0TX 0x00000401

#define LEDS (*((volatile long *)0x40025038))
#define RED 0x02
#define BLUE 0x04
#define GREEN 0x08

void Config_UART0(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    UARTStdioConfig(0, 115200, 16000000);
}

void main(void)
{
    char x;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1);

    Config_UART0(); // UART init

    UARTprintf("\nChoose a color:\n");
    UARTprintf("\tR->RED\n");
    UARTprintf("\tG->GREEN\n");
    UARTprintf("\tB->BLUE\n");

    while(1)
    {
        x = UARTCharGet(UART0_BASE);

        switch (x)
        {
            case 'R':
            case 'r':
                UARTprintf("\rRED     ");
                LEDS = RED;
                break;

            case 'G':
            case 'g':
                UARTprintf("\rGREEN   ");
                LEDS = GREEN;
                break;

            case 'B':
            case 'b':
                UARTprintf("\rBLUE    ");
                LEDS = BLUE;
                break;

            default:
                UARTprintf("\rNOPE   ");
                LEDS = 0;
        }
    }
}
