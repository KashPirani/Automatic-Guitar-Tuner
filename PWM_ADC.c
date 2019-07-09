#define GLOBAL_IQ 12

#include "driverlib.h"
#include "Board.h"
#include "DFT.h"
#include "stepper.h"
#include "uart_io.h"

#define BLINK_CYCLES 200000
#define BUFFER_SIZE 128

#define AMBER_LED_PORT GPIO_PORT_P2
#define AMBER_LED_PIN GPIO_PIN5

#define GREEN_LED_PORT GPIO_PORT_P2
#define GREEN_LED_PIN GPIO_PIN7

int buf16[BUFFER_SIZE];
int32_t buf[BUFFER_SIZE];
int32_t out[BUFFER_SIZE];
//int out_dft[BUFFER_SIZE];
int buf_count = 0;
int curr_freq = 0;
int curr_string = 4;
int blink_counter = 0;

const _iq standard_tuning[6] = {_IQ(82), _IQ(110), _IQ(147), _IQ(196), _IQ(247), _IQ(330)};
_iq tuning_boundaries[6];
const _iq error_boundaries[6] =  {_IQ(5), _IQ(7), _IQ(9), _IQ(12), _IQ(15), _IQ(20)};
enum States {
    IDLE,
    ENTER_WAIT_PLUCK,
    WAIT_PLUCK,
    ENTER_SAMPLING,
    SAMPLING,
    PROCESSING,
};

volatile enum States curr_state = IDLE;

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
    tuning_boundaries[0] = _IQdiv(_IQ(1),_IQ(2));
    tuning_boundaries[1] = _IQdiv(_IQ(65),_IQ(100));
    tuning_boundaries[2] = _IQdiv(_IQ(87),_IQ(100));
    tuning_boundaries[3] = _IQdiv(_IQ(117),_IQ(100));
    tuning_boundaries[4] = _IQdiv(_IQ(147),_IQ(100));
    tuning_boundaries[5] = _IQdiv(_IQ(196),_IQ(100));
    //Stop Watchdog Timer
    WDT_A_hold(WDT_A_BASE);

    CS_initFLLSettle(8000, 244);

    //Set ACLK = REFOCLK with clock divider of 1
    CS_initClockSignal(CS_ACLK,CS_REFOCLK_SELECT,CS_CLOCK_DIVIDER_1);
    //Set SMCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_SMCLK,CS_DCOCLKDIV_SELECT,CS_CLOCK_DIVIDER_8);
    //Set MCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_MCLK,CS_DCOCLKDIV_SELECT,CS_CLOCK_DIVIDER_1);

    // GPIO set-up
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P2, GPIO_PIN6);

    GPIO_setAsOutputPin(AMBER_LED_PORT, AMBER_LED_PIN);
    GPIO_setAsOutputPin(GREEN_LED_PORT, GREEN_LED_PIN);
    GPIO_setOutputLowOnPin(AMBER_LED_PORT, AMBER_LED_PIN);
    GPIO_setOutputLowOnPin(GREEN_LED_PORT, GREEN_LED_PIN);

    // Stepper set-up
    stepper_init();
    turn_deg(1);
    // UART set-up
    uart_io_init();

    // Timer set-up
    Timer_A_initContinuousModeParam param = {0};
    param.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    param.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_3;
    //param.timerPeriod = 12500;
    param.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    //param.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE;
    param.timerClear = TIMER_A_DO_CLEAR;
    param.startTimer = false;
    Timer_A_initContinuousMode(TIMER_A1_BASE, &param);

    // ADC set-up

    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_ADC8, GPIO_PIN_ADC8, GPIO_FUNCTION_ADC8); // ADC Analog Input

    ADC_init(ADC_BASE, ADC_SAMPLEHOLDSOURCE_SC, ADC_CLOCKSOURCE_SMCLK, ADC_CLOCKDIVIDER_32);
    ADC_enable(ADC_BASE);
    ADC_setupSamplingTimer(ADC_BASE, ADC_CYCLEHOLD_16_CYCLES, ADC_MULTIPLESAMPLESENABLE);        // timer trigger needed to start every ADC conversion
    ADC_configureMemory(ADC_BASE, ADC_INPUT_A8, ADC_VREFPOS_INT, ADC_VREFNEG_AVSS);
    ADC_setWindowComp(ADC_BASE, 648, 376);
    ADC_clearInterrupt(ADC_BASE, ADC_ABOVETHRESHOLD_INTERRUPT);
    ADC_clearInterrupt(ADC_BASE, ADC_COMPLETED_INTERRUPT);       //  bit mask of the interrupt flags to be cleared- for new conversion data in the memory buffer
    //ADC_enableInterrupt(ADC_BASE, ADC_COMPLETED_INTERRUPT);       //  enable source to reflected to the processor interrupt

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
    //int runcount = 0;
    //int32_t cur_freq = 0;
    int i;
    ADC_startConversion(ADC_BASE, ADC_REPEATED_SINGLECHANNEL);
    //turn_deg(360L);
    //__delay_cycles(8000000);
    //turn_deg(-180L);


    turn_deg(90);
    while (1)
    {
        switch (curr_state){
            case IDLE:
                if (!GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN6)) {
                    curr_state = ENTER_WAIT_PLUCK;
                    printWord("Button pressed\r\n");
                }
                break;
            case ENTER_WAIT_PLUCK:
                curr_state = WAIT_PLUCK;
                printWord("Entering wait_pluck\r\n");
                Timer_A_enableInterrupt(TIMER_A1_BASE);
                Timer_A_startCounter(TIMER_A1_BASE,TIMER_A_CONTINUOUS_MODE);
                ADC_enableInterrupt(ADC_BASE, ADC_ABOVETHRESHOLD_INTERRUPT);
                break;
            case WAIT_PLUCK:
                printWord("Waiting for peak\r\n");
                __delay_cycles(800000);
                break;
            case ENTER_SAMPLING:
                ADC_disableInterrupt(ADC_BASE, ADC_ABOVETHRESHOLD_INTERRUPT);
                printWord("Entering Sampling\r\n");
                curr_state = SAMPLING;
                ADC_enableInterrupt(ADC_BASE, ADC_COMPLETED_INTERRUPT);
                Timer_A_disableInterrupt(TIMER_A1_BASE);
                GPIO_setOutputHighOnPin(AMBER_LED_PORT, AMBER_LED_PIN);
                break;
            case SAMPLING:
                printWord("Sampling\r\n");
                __delay_cycles(800000);
                break;
            case PROCESSING:
                GPIO_setOutputHighOnPin(GREEN_LED_PORT, GREEN_LED_PIN);
                buf_count = 0;
                curr_state = ENTER_WAIT_PLUCK;
                printWord("Running FFT\r\n");
                //curr_freq = FFT_test(buf, out, BUFFER_SIZE, 1059);
                //printWord("Frequency: ");
                //itoa(curr_freq);
                //printWord("\r\n");


                printWord("\r\nInput:\r\n");
                for (i = 0; i < BUFFER_SIZE; i++) {
                      itoa(buf[i]);
                      printWord("\r\n");
                }
                printWord("\r\nFrequency: ");
                curr_freq = FFT_test16(buf16, out, BUFFER_SIZE, 1059);
                itoa(curr_freq);
                printWord("\r\nFFT:\r\n");
                for(i=0; i<BUFFER_SIZE; i++)
                {
                    itoa(out[i]);
                    printWord(", ");
                    if (i % 16 == 0) {
                        printWord("\r\n");
                    }
                }


                if ( _IQabs(standard_tuning[curr_string] - curr_freq) <= tuning_boundaries[curr_string]) {
                    printWord("String ");
                    itoa(curr_string);
                    printWord(" tuned, moving to next string\r\n");
                    curr_string++;
                    if (curr_string == 6) {
                        curr_string = 0;
                        curr_state = IDLE;
                    }
                } else if ( _IQabs(standard_tuning[curr_string] - curr_freq) <= error_boundaries[curr_string]) {
                    printWord("Turning peg\r\n");
                    tune_peg(curr_freq, standard_tuning[curr_string]);
                } else {
                    for (i = 0; i < 12; i++) {
                        GPIO_toggleOutputOnPin(GREEN_LED_PORT, GREEN_LED_PIN);
                        GPIO_toggleOutputOnPin(AMBER_LED_PORT, AMBER_LED_PIN);
                        __delay_cycles(1600000);
                    }
                }
                GPIO_setOutputLowOnPin(GREEN_LED_PORT, GREEN_LED_PIN);
                GPIO_setOutputLowOnPin(AMBER_LED_PORT, AMBER_LED_PIN);
                break;
        }
        /*if (buf_count==BUFFER_SIZE)
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
        }*/
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
        case  6:        //ADCHI
            if (WAIT_PLUCK == curr_state) {
                curr_state = ENTER_SAMPLING;
            }
            break;
        case  8: break; //ADCLO
        case 10: break; //ADCIN
        case 12:        //ADCIFG0 is ADC interrupt flag
            buf16[buf_count] = (ADC_getResults(ADC_BASE) - 512);
            buf[buf_count] = buf16[buf_count];
            buf_count++;
            if (buf_count == BUFFER_SIZE)
            {
                if (SAMPLING == curr_state) {
                    curr_state = PROCESSING;
                }
                ADC_disableInterrupt(ADC_BASE, ADC_COMPLETED_INTERRUPT);
            }
            break;
        default: break;
    }
}

//BLINK ISR
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER1_A1_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(TIMER1_A1_VECTOR)))
#endif
void TIMER1_A1_ISR (void)
{
    switch (__even_in_range(TA1IV,TA1IV_TAIFG)){     // interrupt vector register never has a value that is odd or larger than 12 (stated)
        case TA1IV_NONE: break; //No interrupt
        case TA1IV_TACCR1: break;
        case TA1IV_TACCR2: break;
        case TA1IV_TAIFG:
            if (blink_counter < (curr_string+1)*2)
            {
                    switch (blink_counter%2) {
                        case 0:
                            GPIO_setOutputHighOnPin(AMBER_LED_PORT, AMBER_LED_PIN);
                            break;
                        case 1:
                            GPIO_setOutputLowOnPin(AMBER_LED_PORT, AMBER_LED_PIN);
                            break;
                    }
            }
            else
            {
                if(blink_counter > curr_string*2 + 10)
                {
                    blink_counter = -1;
                }
            }
            blink_counter++;
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

