#include "driverlib.h"
#include "Board.h"
#include "DFT.h"
#include "stepper.h"
#include "uart_io.h"

#define BUFFER_SIZE 128

int buf16[BUFFER_SIZE];
int32_t buf[BUFFER_SIZE];
int32_t out[BUFFER_SIZE];
//int out_dft[BUFFER_SIZE];
int buf_count = 0;

const int standard_tuning[6] = {82, 110, 147, 196, 247, 330};

/*const int data_buffer[128] = {256,306,354,398,437,469,493,507,
512,507,493,469,437,398,354,306,
256,206,158,114,75,43,19,5,
0,5,19,43,75,114,158,206,256,306,354,398,437,469,493,507,
512,507,493,469,437,398,354,306,
256,206,158,114,75,43,19,5,
0,5,19,43,75,114,158,206,256,306,354,398,437,469,493,507,
512,507,493,469,437,398,354,306,
256,206,158,114,75,43,19,5,
0,5,19,43,75,114,158,206,256,306,354,398,437,469,493,507,
512,507,493,469,437,398,354,306,
256,206,158,114,75,43,19,5,
0,5,19,43,75,114,158,206};
*/
/*const int data_buffer[128] = {16,19,22,25,27,29,31,32,
                        32,32,31,29,27,25,22,19,
                        16,13,10,7,5,3,1,0,
                        0,0,1,3,5,7,10,13,
                        16,19,22,25,27,29,31,32,
                        32,32,31,29,27,25,22,19,
                        16,13,10,7,5,3,1,0,
                        0,0,1,3,5,7,10,13,
                        16,19,22,25,27,29,31,32,
                        32,32,31,29,27,25,22,19,
                        16,13,10,7,5,3,1,0,
                        0,0,1,3,5,7,10,13,
                        16,19,22,25,27,29,31,32,
                        32,32,31,29,27,25,22,19,
                        16,13,10,7,5,3,1,0,
                        0,0,1,3,5,7,10,13};
*/

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


    // Stepper set-up
    stepper_init();


    // ADC set-up

    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_ADC8, GPIO_PIN_ADC8, GPIO_FUNCTION_ADC8); // ADC Analog Input


    ADC_init(ADC_BASE, ADC_SAMPLEHOLDSOURCE_SC, ADC_CLOCKSOURCE_SMCLK, ADC_CLOCKDIVIDER_4);
    ADC_enable(ADC_BASE);
    ADC_setupSamplingTimer(ADC_BASE, ADC_CYCLEHOLD_8_CYCLES, ADC_MULTIPLESAMPLESENABLE);        // timer trigger needed to start every ADC conversion
    ADC_configureMemory(ADC_BASE, ADC_INPUT_A8, ADC_VREFPOS_INT, ADC_VREFNEG_AVSS);
    ADC_clearInterrupt(ADC_BASE, ADC_COMPLETED_INTERRUPT);       //  bit mask of the interrupt flags to be cleared- for new conversion data in the memory buffer
    ADC_enableInterrupt(ADC_BASE, ADC_COMPLETED_INTERRUPT);       //  enable source to reflected to the processor interrupt

    while (PMM_REFGEN_NOTREADY == PMM_getVariableReferenceVoltageStatus()) ;

    PMM_enableInternalReference();      // disabled by default

    __bis_SR_register(GIE);

    /*
     * Disable the GPIO power-on default high-impedance mode to activate
     * previously configured port settings
     */
    PMM_unlockLPM5();

    // Enable global interrupts
    __enable_interrupt();
    ADC_startConversion(0x0700, ADC_REPEATED_SINGLECHANNEL);
    int runcount = 0;
    int32_t cur_freq = 0;
    //turn_deg(360L);
    //__delay_cycles(1000000);
    //turn_deg(-180L);
    while (1)
    {
        if (buf_count==BUFFER_SIZE)
        {
            //itoa(DFT(buf,BUFFER_SIZE,31250));
            //printWord("\r\n");
            //fft_helper(data_buffer, out, BUFFER_SIZE, 1);
            //DFT_test(data_buffer, out_dft, BUFFER_SIZE, 31250);
            int i;
            printWord("\r\nInput:\r\n");
            for (i = 0; i < BUFFER_SIZE; i++) {
                  itoa(buf[i]);
                  printWord(",");
                  itoa(buf[i]);
                  printWord("\r\n");
            }
            printWord("\r\nFrequency: ");
            cur_freq = FFT_test(buf16, out, BUFFER_SIZE, 11628);
            itoa(cur_freq);
            printWord("\r\nFFT:\r\n");
            for(i=0; i<BUFFER_SIZE; i++)
            {
                itoa(out[i]);
                printWord(", ");
                if (i % 16 == 0) {
                    printWord("\r\n");
                }
            }
            printWord("\r\n");
            runcount++;
            buf_count = 0;
            ADC_enableInterrupt(ADC_BASE, ADC_COMPLETED_INTERRUPT);
        }
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
            buf16[buf_count] = (ADC_getResults(ADC_BASE) - 512);
            buf[buf_count] = buf16[buf_count];
            buf_count++;
            if (buf_count == BUFFER_SIZE)
            {
                ADC_disableInterrupt(ADC_BASE, ADC_COMPLETED_INTERRUPT);
            }
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

