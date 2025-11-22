#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/adc.h"

#define LED1 GPIO_PIN_0
#define LED2 GPIO_PIN_1
#define LED3 GPIO_PIN_2

void main()
{
    uint32_t adcValue[1];

    // set system clock to 80 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)) { }
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)) { }

    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, LED1 | LED2 | LED3);

    // configure PE3 as ADC input (AIN0)
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);

    // configure ADC0, sequencer 3, processor trigger
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 3);
    ADCIntClear(ADC0_BASE, 3);

    while(1)
    {
        ADCProcessorTrigger(ADC0_BASE, 3);

        while(!ADCIntStatus(ADC0_BASE, 3, false)) { }

        ADCIntClear(ADC0_BASE, 3);
        ADCSequenceDataGet(ADC0_BASE, 3, adcValue);

        // 12-bit ADC: 0-4095 maps to 0-3.3V
        // light LEDs based on voltage thresholds
        if(adcValue[0] < 1365) // 0 - 1.1V
        {
            GPIOPinWrite(GPIO_PORTB_BASE, LED1 | LED2 | LED3, 0x00);
        }
        else if(adcValue[0] < 2730) // 1.1V - 2.2V
        {
            GPIOPinWrite(GPIO_PORTB_BASE, LED1 | LED2 | LED3, LED1);
        }
        else if(adcValue[0] < 3685) // 2.2V - 3.0V
        {
            GPIOPinWrite(GPIO_PORTB_BASE, LED1 | LED2 | LED3, LED1 | LED2);
        }
        else // 3.0V - 3.3V
        {
            GPIOPinWrite(GPIO_PORTB_BASE, LED1 | LED2 | LED3, LED1 | LED2 | LED3);
        }
    }
}
