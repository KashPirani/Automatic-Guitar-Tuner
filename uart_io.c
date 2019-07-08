/*
 * uart_io.c
 *
 *  Created on: Jul 8, 2019
 *      Author: Tyler
 */

#include "uart_io.h"

void uart_io_init(void)
{
    //Configure UART pins
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_UCA0TXD,
        GPIO_PIN_UCA0TXD,
        GPIO_FUNCTION_UCA0TXD
    );
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_UCA0RXD,
        GPIO_PIN_UCA0RXD,
        GPIO_FUNCTION_UCA0RXD
    );

    //Configure UART
    //SMCLK = 1MHz, Baudrate = 9600
    //UCBRx = 6, UCBRFx = 8, UCBRSx = 0x20, UCOS16 = 1
    EUSCI_A_UART_initParam uparam = {0};
    uparam.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
    uparam.clockPrescalar = 6;
    uparam.firstModReg = 8;
    uparam.secondModReg = 0x20;
    uparam.parity = EUSCI_A_UART_NO_PARITY;
    uparam.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    uparam.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    uparam.uartMode = EUSCI_A_UART_MODE;
    uparam.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;

    if (STATUS_FAIL == EUSCI_A_UART_init(EUSCI_A0_BASE, &uparam)) {
        return;
    }

    EUSCI_A_UART_enable(EUSCI_A0_BASE);
}

void printWord(char* word)
{
    while(*word)
    {
        EUSCI_A_UART_transmitData(EUSCI_A0_BASE, *word);
        __delay_cycles(5000);
        word++;
    }
    return;
}

void itoa(int32_t num) //TODO: fix this for large numbers
{
    if (num < 0) {
        printWord("-");
        num *= -1;
    }
    int32_t numLen = 0;
    int32_t i = num;
    while (i > 0)
    {
        i/=10;
        numLen++;
    }
    char ret[32];
    i = 0;
    int32_t pow = 1;
    for(;i<numLen-1; i++){
        pow = pow*10;
    }
    int32_t dig = 0;
    int32_t count = 0;
    while(pow > 0)
    {
        dig = (num/pow) % 10;
        ret[count] = dig + 48;
        pow = pow/10;
        count++;
    }
    ret[count] = '\0';
    printWord(ret);
}
