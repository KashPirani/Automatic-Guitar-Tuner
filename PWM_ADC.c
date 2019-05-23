///* DESCRIPTION
// * Sample code structure when with ADC analog input and driving an LED with PWM
// * This example: Turns a LED on at two different brightness level depending on ADC value
//*/
//
//
//#include <msp430.h>
//#include "driverlib.h"
//#include "Board.h"
//
//#include <stdio.h>

#define TIMER_PERIOD 511
//int DUTY_CYCLE = 0;

//void main (void)
//{
//
//     WDT_A_hold(WDT_A_BASE);
//
//
//
//     // Start timer
//
//     Timer_A_initUpModeParam param = {0};
//     param.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
//     param.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
//     param.timerPeriod = TIMER_PERIOD;
//     param.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
//     param.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE;
//     param.timerClear = TIMER_A_DO_CLEAR;
//     param.startTimer = true;
//     Timer_A_initUpMode(TIMER_A0_BASE, &param);
//
//
//
//     // PWM set-up
//
//     GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN6, GPIO_PRIMARY_MODULE_FUNCTION);     // Connect P1.6 to LED
//
//     Timer_A_initCompareModeParam initComp2Param = {0};
//     initComp2Param.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_2;
//     initComp2Param.compareInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE;
//     initComp2Param.compareOutputMode = TIMER_A_OUTPUTMODE_RESET_SET;
//
//
//
//     // ADC set-up
//
//     GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_ADC8, GPIO_PIN_ADC8, GPIO_FUNCTION_ADC8); // ADC Anolog Input
//
//     PMM_unlockLPM5();
//
//     ADC_init(0x0700, ADC_SAMPLEHOLDSOURCE_SC, ADC_CLOCKSOURCE_ADCOSC, ADC_CLOCKDIVIDER_1);
//     ADC_enable(0x0700);
//     ADC_setupSamplingTimer(0x0700, ADC_CYCLEHOLD_16_CYCLES, ADC_MULTIPLESAMPLESDISABLE);        // timer trigger needed to start every ADC conversion
//     ADC_configureMemory(0x0700, ADC_INPUT_A8, ADC_VREFPOS_INT, ADC_VREFNEG_AVSS);
//     ADC_clearInterrupt(0x0700, ADC_COMPLETED_INTERRUPT);       //  bit mask of the interrupt flags to be cleared- for new conversion data in the memory buffer
//     ADC_enableInterrupt(0x0700, ADC_COMPLETED_INTERRUPT);       //  enable source to reflected to the processor interrupt
//
//     while (PMM_REFGEN_NOTREADY == PMM_getVariableReferenceVoltageStatus()) ;
//
//     PMM_enableInternalReference();      // disabled by default
//
//     __bis_SR_register(GIE);
//
//
//
//
//     for (;;)
//     {
//         __delay_cycles(5000);
//         ADC_startConversion(0x0700, ADC_SINGLECHANNEL);
//
//         initComp2Param.compareValue = DUTY_CYCLE;
//         Timer_A_initCompareMode(TIMER_A0_BASE, &initComp2Param);
//     }
//}
//
//
//
////ADC ISR
//#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
//#pragma vector=ADC_VECTOR
//__interrupt
//#elif defined(__GNUC__)
//__attribute__((interrupt(ADC_VECTOR)))
//#endif
//void ADC_ISR (void)
//{
//    switch (__even_in_range(ADCIV,12)){     // interrupt vector register never has a value that is odd or larger than 12 (stated)
//        case  0: break; //No interrupt
//        case  2: break; //conversion result overflow
//        case  4: break; //conversion time overflow
//        case  6: break; //ADCHI
//        case  8: break; //ADCLO
//        case 10: break; //ADCIN
//        case 12:        //ADCIFG0 is ADC interrupt flag
//
//            if ((ADC_getResults(0x0700) <= 0x155))      // 0x155 = 0.5V
//                DUTY_CYCLE = 511;       // LED max brightness
//
//            else
//                DUTY_CYCLE = 100;       // LED dim
//
//            break;
//        default: break;
//    }
//}
/* --COPYRIGHT--,BSD
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
//******************************************************************************
//!  EUSCI_A0 External Loopback test using EUSCI_A_UART_init API
//!
//!  Description: This demo connects TX to RX of the MSP430 UART
//!  The example code shows proper initialization of registers
//!  and interrupts to receive and transmit data.
//!
//!  SMCLK = MCLK = BRCLK = DCOCLKDIV = ~1MHz, ACLK = 32.768kHz
//!
//!
//!           MSP430FR2xx_4xx Board
//!             -----------------
//!       RST -|          UCA0TXD|----|
//!            |                 |    |
//!            |                 |    |
//!            |          UCA0RXD|----|
//!            |                 |
//!
//! This example uses the following peripherals and I/O signals. You must
//! review these and change as needed for your own board:
//! - UART peripheral
//! - GPIO Port peripheral (for UART pins)
//! - UCA0TXD
//! - UCA0RXD
//!
//! This example uses the following interrupt handlers. To use this example
//! in your own application you must add these interrupt handlers to your
//! vector table.
//! - USCI_A0_VECTOR.
//******************************************************************************
#include "driverlib.h"
#include "Board.h"

uint16_t i;
uint8_t RXData = 0, TXData = 0;
uint8_t check = 0;
volatile int buf = 0;


void printWord(char* word)
{
    while(*word)
    {
        EUSCI_A_UART_transmitData(EUSCI_A0_BASE, *word);
        __delay_cycles(20000);
        word++;
    }
    return;
}

void itoa(int num)
{
    int numLen = 0;
    int i = num;
    while (i > 0)
    {
        i/=10;
        numLen++;
    }
    char ret[32];
    i = 0;
    int pow = 1;
    for(;i<numLen-1; i++){
        pow = pow*10;
    }
    int dig = 0;
    int count = 0;
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

void main(void)
{
    //Stop Watchdog Timer
    WDT_A_hold(WDT_A_BASE);

    //Set ACLK = REFOCLK with clock divider of 1
    CS_initClockSignal(CS_ACLK,CS_REFOCLK_SELECT,CS_CLOCK_DIVIDER_1);
    //Set SMCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_SMCLK,CS_DCOCLKDIV_SELECT,CS_CLOCK_DIVIDER_1);
    //Set MCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_MCLK,CS_DCOCLKDIV_SELECT,CS_CLOCK_DIVIDER_1);

    // Start timer

         Timer_A_initUpModeParam param = {0};
         param.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
         param.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
         param.timerPeriod = TIMER_PERIOD;
         param.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
         param.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE;
         param.timerClear = TIMER_A_DO_CLEAR;
         param.startTimer = true;
         Timer_A_initUpMode(TIMER_A0_BASE, &param);


         // ADC set-up

          GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_ADC8, GPIO_PIN_ADC8, GPIO_FUNCTION_ADC8); // ADC Anolog Input


          ADC_init(ADC_BASE, ADC_SAMPLEHOLDSOURCE_SC, ADC_CLOCKSOURCE_ADCOSC, ADC_CLOCKDIVIDER_1);
          ADC_enable(ADC_BASE);
          ADC_setupSamplingTimer(ADC_BASE, ADC_CYCLEHOLD_16_CYCLES, ADC_MULTIPLESAMPLESDISABLE);        // timer trigger needed to start every ADC conversion
          ADC_configureMemory(ADC_BASE, ADC_INPUT_A8, ADC_VREFPOS_INT, ADC_VREFNEG_AVSS);
          ADC_clearInterrupt(ADC_BASE, ADC_COMPLETED_INTERRUPT);       //  bit mask of the interrupt flags to be cleared- for new conversion data in the memory buffer
          ADC_enableInterrupt(ADC_BASE, ADC_COMPLETED_INTERRUPT);       //  enable source to reflected to the processor interrupt

          while (PMM_REFGEN_NOTREADY == PMM_getVariableReferenceVoltageStatus()) ;

          PMM_enableInternalReference();      // disabled by default

          __bis_SR_register(GIE);

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

    /*
     * Disable the GPIO power-on default high-impedance mode to activate
     * previously configured port settings
     */
    PMM_unlockLPM5();

    //Configure UART
    //SMCLK = 1MHz, Baudrate = 115200
    //UCBRx = 8, UCBRFx = 0, UCBRSx = 0xD6, UCOS16 = 0
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

    // Enable global interrupts
    __enable_interrupt();
    while (1)
    {
        ADC_startConversion(0x0700, ADC_SINGLECHANNEL);
        itoa(buf);
        printWord("\r\n");
    }
}
//ADC ISR
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(ADC_VECTOR)))
#endif
void ADC_ISR (void)
{
    switch (__even_in_range(ADCIV,12)){     // interrupt vector register never has a value that is odd or larger than 12 (stated)
        case  0: break; //No interrupt
        case  2: break; //conversion result overflow
        case  4: break; //conversion time overflow
        case  6: break; //ADCHI
        case  8: break; //ADCLO
        case 10: break; //ADCIN
        case 12:        //ADCIFG0 is ADC interrupt flag
            buf = ADC_getResults(ADC_BASE);
            break;
        default: break;
    }
}

////******************************************************************************
////
////This is the USCI_A0 interrupt vector service routine.
////
////******************************************************************************
//#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
//#pragma vector=USCI_A0_VECTOR
//__interrupt
//#elif defined(__GNUC__)
//__attribute__((interrupt(USCI_A0_VECTOR)))
//#endif
//void EUSCI_A0_ISR(void)
//{
//    switch(__even_in_range(UCA0IV,USCI_UART_UCTXCPTIFG))
//    {
//        case USCI_NONE: break;
//        case USCI_UART_UCRXIFG: break;
//        case USCI_UART_UCTXIFG:
//            buf = 'f';
//            break;
//        case USCI_UART_UCSTTIFG: break;
//        case USCI_UART_UCTXCPTIFG:
//           break;
//    }
//}

