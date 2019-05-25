/*
 * DFT.h
 *
 *  Created on: May 23, 2019
 *      Author: tldibatt
 */

#ifndef DFT_H_
#define DFT_H_

#define GLOBAL_Q 6
#include "QmathLib.h"



int DFT(int* data, int samples, int sample_rate)
{
    int n = 0;
    int k = 0;
    _q max = 0;
    _q real;
    _q imag;
    _q temp = 0;
    int bucket = 0;

    _q cos_mod = 0;
    _q cos_arg = 0;
    _q q_data = 0;

    //const _q10 Q_2PI = 6434; //2 * pi * 2^10
    const _q6 Q_2PI = 402; //2 * pi * 2^6
    for (; n<samples; n++) {
        real = 0;
        imag = 0;
        temp = 0;
        for (k=0;k<samples;k++) {
            cos_mod = _Q(n*k % samples);
            cos_arg = _Qdiv(_Qmpy(Q_2PI, cos_mod), _Q(samples));
            //q_data = _Q(data[k]);

            real =_Qmpy(data[k], _Qcos(cos_arg));
            imag =_Qmpy(data[k], _Qsin(cos_arg));
            //real = data[k]*cos((2*M_PI*n*k)/samples);
            //imag = data[k]*sin((2*M_PI*n*k)/samples);
            temp =_Qmag(imag, real);
        }

        if (temp > max)
        {
            max = temp;
            bucket = n;
        }
    }
    return bucket*(sample_rate/samples);
}




#endif /* DFT_H_ */
