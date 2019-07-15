/*
 * stepper.c
 *
 *  Created on: May 24, 2019
 *      Author: tldibatt
 */
#define GLOBAL_IQ 12

#include "driverlib.h"
#include "Board.h"
#include "gpio.h"
#include "IQmathLib.h"

#define STEP_DELAY 8000

const _iq DEGREE_PER_HZ = 10;
const _iq degrees_per_hz[6] = {_IQ(30), _IQ(25), _IQ(21), _IQ(16), _IQ(12), _IQ(7)};
const uint8_t unit_steps[8] = {0x1, 0x3, 0x2, 0x6, 0x4, 0xC, 0x8, 0x9};

void stepper_init(){
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN3);
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN4);
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN5);
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN6);
}

void unit_step(const uint8_t pos)
{
    uint16_t baseAddress = __MSP430_BASEADDRESS_PORT1_R__;

    uint16_t initial = HWREG16(baseAddress + OFS_PAOUT);
    initial &= ~(0x0078);

    uint16_t write = pos & 0b1010;
    write |= (pos & 0b0100) >> 2;
    write |= (pos & 0b0001) << 2;
    write <<= 3;

    write |= initial;

    HWREG16(baseAddress + OFS_PAOUT) = write;
}

void turn_deg(int deg)
{
    long num_cycles = (deg * 4096L) / 360; //4096 steps per 360 degrees
    long i;

    if (deg >= 0) {
        for (i = 0; i < num_cycles; i++) {
            unit_step(unit_steps[i % 8]);    //8 steps per cycle
            __delay_cycles(STEP_DELAY);
        }
    } else {
        num_cycles *= -1;
        for (i = num_cycles - 1; i >= 0; i--) {
            unit_step(unit_steps[i % 8]);
            __delay_cycles(STEP_DELAY);
        }
    }

    unit_step(0x0);
}

void tune_peg(_iq curr_freq, _iq desired_freq, int curr_string)
{
    _iq difference = curr_freq - desired_freq;
    long angle = _IQint(_IQmpy(difference, degrees_per_hz[curr_string]));
    turn_deg(angle);
}
