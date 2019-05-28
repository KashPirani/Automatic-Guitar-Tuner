/*
 * stepper.c
 *
 *  Created on: May 24, 2019
 *      Author: tldibatt
 */

#include "driverlib.h"
#include "Board.h"
#include "gpio.h"

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

void turn_deg(long deg)
{
    long num_cycles = (deg * 4096) / (360 * 8); //4096 steps per 360 degrees, 8 steps per cycle
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
