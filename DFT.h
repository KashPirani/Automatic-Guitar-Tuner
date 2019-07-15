/*
 * DFT.h
 *
 *  Created on: May 23, 2019
 *      Author: tldibatt
 */

#ifndef DFT_H_
#define DFT_H_

#define GLOBAL_Q 6
#define LOG_SAMPLES 7

#include "QmathLib.h"
#include "IQmathLib.h"

const _q6 Q_2PI = 402; //2 * pi * 2^6

const _iq12 IQ_2PI = 25736; //2 * pi * 2^12

int iq_DFT(int32_t* data, int samples, int sample_rate)
{
    int n = 0;
    int k = 0;
    _iq max = 0;
    _iq real;
    _iq imag;
    _iq temp;
    int bucket = 0;

    _iq cos_mod = 0;
    _iq cos_arg = 0;
    //_q q_data = 0;


    for (; n<samples; n++) {
        real = 0;
        imag = 0;
        temp = 0;
        for (k=0;k<samples;k++) {
            cos_mod = _IQ(n*k % samples);
            cos_arg = _IQdiv(_IQmpy(IQ_2PI, cos_mod), _IQ(samples));
            //q_data = _Q(data[k]);

            real +=_IQmpy(_IQ(data[k]), _IQcos(cos_arg));
            imag +=_IQmpy(_IQ(data[k]), _IQsin(cos_arg));
            //real = data[k]*cos((2*M_PI*n*k)/samples);
            //imag = data[k]*sin((2*M_PI*n*k)/samples);
        }

        temp = _IQmag(imag, real);
        if (temp > max && n!=0)
        {
            max = temp;
            bucket = n;
        }
    }
    return bucket*(sample_rate/samples);
}

unsigned int reverseBits(unsigned int num, unsigned int bits)
{
    unsigned int reverse_num = 0;
    int i;

    for(i = 0; i < bits; i++)
    {
       reverse_num |= num & 1;
       num >>= 1;
       reverse_num<<=1;
    }
    reverse_num >>= 1;
    return reverse_num;
}

void iqfft_helper(_iq* out_real, _iq* out_imag, int samples, int step_size)
{
    if (samples > 1) {
        iqfft_helper(out_real, out_imag, samples / 2, step_size * 2); //even
        iqfft_helper(out_real + samples/2, out_imag + samples/2, samples / 2, step_size * 2); //odd

        _iq factor_real, factor_imag, temp_real, temp_imag, cos_arg;
        int i;
        for (i = 0; i < samples/2; i++) {
            temp_real = out_real[i];
            temp_imag = out_imag[i];

            cos_arg = _IQdiv(_IQmpy(IQ_2PI, _IQ(i)), _IQ(samples));

            factor_real = _IQmpy(_IQcos(cos_arg), out_real[i + samples/2]) + _IQmpy(_IQsin(cos_arg), out_imag[i + samples/2]);
            factor_imag = _IQmpy(_IQsin(cos_arg), -out_real[i + samples/2]) + _IQmpy(_IQcos(cos_arg), out_imag[i + samples/2]);

            out_real[i] = temp_real + factor_real;
            out_imag[i] = temp_imag + factor_imag;

            out_real[i + samples/2] = temp_real - factor_real;
            out_imag[i + samples/2] = temp_imag - factor_imag;
        }
    } else {
        //out_real[0] = _IQ(data[0]);
    }
}

//Modifies data array
_iq FFT_test(int32_t* data, int32_t* out, int samples, int sampling_rate)
{
	unsigned int i;
	for (i = 0; i < samples; i++) {
		out[i] = _IQ(data[reverseBits(i, LOG_SAMPLES)]);
	}
	for (i = 0; i < samples; i++) {
	    data[i] = 0;
    }
	iqfft_helper(out, data, samples, 1);

	_iq bucket, max = 0;
	for (i = 0; i < samples; i++) {
		out[i] = _IQmag(out[i], data[i]);
		if (out[i] > max && i <= samples/2) {
		    max = out[i];
		    bucket = i;
		}
	}
	_iq d;
    d = _IQdiv((out[bucket+1] - out[bucket-1]), (out[bucket]+out[bucket-1]+out[bucket+1]));
	bucket = _IQ(bucket)+d;

	return _IQmpy(bucket, _IQdiv(_IQ(sampling_rate), _IQ(samples)));
}

#endif /* DFT_H_ */
