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

const _iq DEGREE_PER_HZ = 1;

void stepper_init(){
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN3);
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN4);
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN5);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN5);
}

void unit_step(const uint8_t pos)
{
    uint16_t baseAddress = __MSP430_BASEADDRESS_PORT1_R__;

    uint16_t initial = HWREG16(baseAddress + OFS_PAOUT);
    initial &= ~(0x2038);

    uint16_t write = pos & 0b0111;
    write <<= 3;

    if (pos & 0b1000) {
        write |= 0x2000;
    }

    write |= initial;

    HWREG16(baseAddress + OFS_PAOUT) = write;
}

void turn_deg(int deg)
{
    long num_cycles = (deg * 4096L) / (360 * 8); //4096 steps per 360 degrees, 8 steps per cycle
    long i;

    if (deg >= 0) {
        for (i = 0; i < num_cycles; i++) {
            unit_step(0x1);
            __delay_cycles(1000);
            unit_step(0x3);
            __delay_cycles(1000);
            unit_step(0x2);
            __delay_cycles(1000);
            unit_step(0x6);
            __delay_cycles(1000);
            unit_step(0x4);
            __delay_cycles(1000);
            unit_step(0xC);
            __delay_cycles(1000);
            unit_step(0x8);
            __delay_cycles(1000);
            unit_step(0x9);
            __delay_cycles(1000);
        }
    } else {
        num_cycles *= -1;
        for (i = 0; i < num_cycles; i++) {
            unit_step(0x9);
            __delay_cycles(1000);
            unit_step(0x8);
            __delay_cycles(1000);
            unit_step(0xC);
            __delay_cycles(1000);
            unit_step(0x4);
            __delay_cycles(1000);
            unit_step(0x6);
            __delay_cycles(1000);
            unit_step(0x2);
            __delay_cycles(1000);
            unit_step(0x3);
            __delay_cycles(1000);
            unit_step(0x1);
            __delay_cycles(1000);
        }
    }

    unit_step(0x0);
}

void tune_peg(_iq curr_freq, _iq desired_freq)
{
    _iq difference = desired_freq - curr_freq;
    int angle = _IQint(_IQmpy(difference, DEGREE_PER_HZ));
    turn_deg(angle);
}
