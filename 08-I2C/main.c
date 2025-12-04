#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/i2c.h"

#include "ssd1306.h"
#include "ssd1306_fonts.h"

// init I2C0: PB2 (SCL), PB3 (SDA), 400 kHz Fast Mode
void I2C0_Init()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    // wait for peripherals to be ready
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_I2C0));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));

    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

    I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), true);
    I2CMasterEnable(I2C0_BASE);
}

int main()
{
    SysCtlClockSet(SYSCTL_XTAL_16MHZ | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_SYSDIV_2_5); // 80 MHz
    I2C0_Init();

    SSD1306_Init(I2C0_BASE, SSD1306_I2C_ADDR);
    SSD1306_Clear();

    ssd1306_SetFont(&Font8x12_bold);
    SSD1306_SetCursor(0, 0);
    SSD1306_WriteString("Hello TM4C!");
    SSD1306_Display();

    while(1) { }
}
