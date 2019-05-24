#include "driverlib.h"
#include "Board.h"
#include "DFT.h"

#define BUFFER_SIZE 128

int buf[BUFFER_SIZE];
int buf_count = 0;

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

void itoa(unsigned int num) //TODO: fix this for large numbers
{
    unsigned int numLen = 0;
    unsigned int i = num;
    while (i > 0)
    {
        i/=10;
        numLen++;
    }
    char ret[32];
    i = 0;
    unsigned int pow = 1;
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


    // ADC set-up

    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_ADC8, GPIO_PIN_ADC8, GPIO_FUNCTION_ADC8); // ADC Anolog Input


    ADC_init(ADC_BASE, ADC_SAMPLEHOLDSOURCE_SC, ADC_CLOCKSOURCE_SMCLK, ADC_CLOCKDIVIDER_4);
    ADC_enable(ADC_BASE);
    ADC_setupSamplingTimer(ADC_BASE, ADC_CYCLEHOLD_8_CYCLES, ADC_MULTIPLESAMPLESENABLE);        // timer trigger needed to start every ADC conversion
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
    ADC_startConversion(0x0700, ADC_REPEATED_SINGLECHANNEL);
    int runcount = 0;
    while (1)
    {
        //itoa(buf[buf_count]);
        //printWord("\r\n");
        if (buf_count==BUFFER_SIZE)
        {
            itoa(DFT(buf,BUFFER_SIZE,31250));
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
            buf[buf_count] = ADC_getResults(ADC_BASE);
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

