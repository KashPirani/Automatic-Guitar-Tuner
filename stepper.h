/*
 * stepper.h
 *
 *  Created on: May 24, 2019
 *      Author: tldibatt
 */

#ifndef STEPPER_H_
#define STEPPER_H_

void stepper_init(void);
void unit_step(uint8_t pos);
void turn_deg(long deg);
void tune_peg(_iq curr_freq, _iq desired_freq, int curr_string);

#endif /* STEPPER_H_ */
