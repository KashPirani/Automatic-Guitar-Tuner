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

char print_str[16];

int32_t buf[BUFFER_SIZE];
int32_t out[BUFFER_SIZE];
int buf_count = 0;
_iq curr_freq = 0;
int curr_string = 0;
int blink_counter = 0;

const int sampling_freqs[6] = {234, 273, 328, 546, 546, 819};
const uint16_t sampling_settings[6][2] =
{
    {ADC_CLOCKDIVIDER_7, ADC_CYCLEHOLD_8_CYCLES},
    {ADC_CLOCKDIVIDER_6, ADC_CYCLEHOLD_8_CYCLES},
    {ADC_CLOCKDIVIDER_5, ADC_CYCLEHOLD_8_CYCLES},
    {ADC_CLOCKDIVIDER_3, ADC_CYCLEHOLD_8_CYCLES},
    {ADC_CLOCKDIVIDER_3, ADC_CYCLEHOLD_8_CYCLES},
    {ADC_CLOCKDIVIDER_2, ADC_CYCLEHOLD_8_CYCLES}
};

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

void reset_adc(uint16_t clk_div, uint16_t t_sample)
{
    ADC_init(ADC_BASE, ADC_SAMPLEHOLDSOURCE_SC, ADC_CLOCKSOURCE_ACLK, clk_div);
    ADC_enable(ADC_BASE);
    ADC_setupSamplingTimer(ADC_BASE, t_sample, ADC_MULTIPLESAMPLESENABLE);        // timer trigger needed to start every ADC conversion
    ADC_configureMemory(ADC_BASE, ADC_INPUT_A8, ADC_VREFPOS_INT, ADC_VREFNEG_AVSS);
    ADC_setWindowComp(ADC_BASE, 648, 376);
    ADC_clearInterrupt(ADC_BASE, ADC_ABOVETHRESHOLD_INTERRUPT);
    ADC_clearInterrupt(ADC_BASE, ADC_COMPLETED_INTERRUPT);       //  bit mask of the interrupt flags to be cleared- for new conversion data in the memory buffer
}

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
    param.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    param.timerClear = TIMER_A_DO_CLEAR;
    param.startTimer = false;
    Timer_A_initContinuousMode(TIMER_A1_BASE, &param);

    // ADC set-up

    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_ADC8, GPIO_PIN_ADC8, GPIO_FUNCTION_ADC8); // ADC Analog Input

    reset_adc(sampling_settings[curr_string][0], sampling_settings[curr_string][1]);

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
    int i;
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
                ADC_startConversion(ADC_BASE, ADC_REPEATED_SINGLECHANNEL);
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
                GPIO_setOutputLowOnPin(AMBER_LED_PORT, AMBER_LED_PIN);
                GPIO_setOutputHighOnPin(GREEN_LED_PORT, GREEN_LED_PIN);
                buf_count = 0;
                curr_state = ENTER_WAIT_PLUCK;
                printWord("Running FFT\r\n");


                printWord("\r\nInput:\r\n");
                for (i = 0; i < BUFFER_SIZE; i++) {
                      itoa(buf[i]);
                      printWord(", ");
                      if (i % 16 == 15) {
                          printWord("\r\n");
                      }
                }
                printWord("\r\nFrequency: ");
                curr_freq = FFT_test(buf, out, BUFFER_SIZE, sampling_freqs[curr_string]);
                _IQtoa(print_str, "%4.6f", curr_freq);
                printWord(print_str);
                printWord(" Hz\r\nFFT:\r\n");
                for(i=0; i<BUFFER_SIZE; i++)
                {
                    itoa(_IQint(out[i]));
                    printWord(", ");
                    if (i % 16 == 0) {
                        printWord("\r\n");
                    }
                }
                printWord("\r\n");

                if ( _IQabs(standard_tuning[curr_string] - curr_freq) <= tuning_boundaries[curr_string]) {
                    printWord("String ");
                    itoa(curr_string);
                    printWord(" tuned, moving to next string\r\n");
                    curr_string++;
                    if (curr_string == 6) {
                        printWord("~GUITAR IS NOW IN TUNE~\r\n");
                        curr_string = 0;
                        curr_state = IDLE;
                    }
                    reset_adc(sampling_settings[curr_string][0], sampling_settings[curr_string][1]);
                } else if ( _IQabs(standard_tuning[curr_string] - curr_freq) <= error_boundaries[curr_string]) {
                    printWord("Turning peg\r\n");
                    tune_peg(curr_freq, standard_tuning[curr_string], curr_string);
                } else {
                    printWord("Error, going back to WAIT_PLUCK\r\n");
                    GPIO_setOutputHighOnPin(AMBER_LED_PORT, AMBER_LED_PIN);
                    for (i = 0; i < 12; i++) {
                        GPIO_toggleOutputOnPin(GREEN_LED_PORT, GREEN_LED_PIN);
                        GPIO_toggleOutputOnPin(AMBER_LED_PORT, AMBER_LED_PIN);
                        __delay_cycles(1600000);
                    }
                }
                GPIO_setOutputLowOnPin(GREEN_LED_PORT, GREEN_LED_PIN);
                GPIO_setOutputLowOnPin(AMBER_LED_PORT, AMBER_LED_PIN);
                break;
            default:
                printWord("what just happened\r\n");
                __delay_cycles(100000);
                break;
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
        case  6:        //ADCHI
            if (WAIT_PLUCK == curr_state) {
                curr_state = ENTER_SAMPLING;
            }
            break;
        case  8: break; //ADCLO
        case 10: break; //ADCIN
        case 12:        //ADCIFG0 is ADC interrupt flag
            buf[buf_count] = (ADC_getResults(ADC_BASE) - 512);
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
