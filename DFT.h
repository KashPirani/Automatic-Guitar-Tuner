/*
 * DFT.h
 *
 *  Created on: May 23, 2019
 *      Author: tldibatt
 */

#ifndef DFT_H_
#define DFT_H_

#define GLOBAL_Q 6
#define GLOBAL_IQ 12
#include "QmathLib.h"
#include "IQmathLib.h"

//const _q10 Q_2PI = 6434; //2 * pi * 2^10
const _q6 Q_2PI = 402; //2 * pi * 2^6

const _iq12 IQ_2PI = 25736; //2 * pi * 2^12
//const _iq6 IQ_2PI = 402; //2 * pi * 2^6

int DFT(int* data, int samples, int sample_rate)
{
    int n = 0;
    int k = 0;
    _q max = 0;
    _q real;
    _q imag;
    _q temp;
    int bucket = 0;

    _q cos_mod = 0;
    _q cos_arg = 0;
    _q q_data = 0;


    for (; n<samples; n++) {
        real = 0;
        imag = 0;
        temp = 0;
        for (k=0;k<samples;k++) {
            cos_mod = _Q(n*k % samples);
            cos_arg = _Qdiv(_Qmpy(Q_2PI, cos_mod), _Q(samples));
            //q_data = _Q(data[k]);

            real +=_Qmpy(data[k], _Qcos(cos_arg));
            imag +=_Qmpy(data[k], _Qsin(cos_arg));
            //real = data[k]*cos((2*M_PI*n*k)/samples);
            //imag = data[k]*sin((2*M_PI*n*k)/samples);
        }

        temp = _Qmag(imag, real);
        if (temp > max && n!=0)
        {
            max = temp;
            bucket = n;
        }
    }
    return bucket*(sample_rate/samples);
}

void DFT_test(int* data, int* out, int samples, int sample_rate)
{
    int n = 0;
    int k = 0;
    _q real;
    _q imag;
    _q temp = 0;
    int bucket = 0;

    _q cos_mod = 0;
    _q cos_arg = 0;
    _q q_data = 0;


    for (; n<samples; n++) {
        real = 0;
        imag = 0;
        temp = 0;
        for (k=0;k<samples;k++) {
            cos_mod = _Q(n*k % samples);
            cos_arg = _Qdiv(_Qmpy(Q_2PI, cos_mod), _Q(samples));
            //q_data = _Q(data[k]);

            real +=_Qmpy(_Q(data[k]-16), _Qcos(cos_arg));
            imag +=_Qmpy(_Q(data[k]-16), _Qsin(cos_arg));
            //real = data[k]*cos((2*M_PI*n*k)/samples);
            //imag = data[k]*sin((2*M_PI*n*k)/samples);
        }
        out[n] =_Qint(_Qmag(imag, real));
    }
}

void qfft_helper(int* data, _q* out_real, _q* out_imag, int samples, int step_size)
{
	if (samples > 1) {
		qfft_helper(data, out_real, out_imag, samples / 2, step_size * 2); //even
        qfft_helper(data + step_size, out_real + samples/2, out_imag + samples/2, samples / 2, step_size * 2); //odd

		_q factor_real, factor_imag, temp_real, temp_imag, cos_arg;
		int i;
		for (i = 0; i < samples/2; i++) {
			temp_real = out_real[i];
			temp_imag = out_imag[i];

			cos_arg = _Qdiv(_Qmpy(Q_2PI, _Q(i)), _Q(samples));
			
			factor_real = _Qmpy(_Qcos(cos_arg), out_real[i + samples/2]) + _Qmpy(_Qsin(cos_arg), out_imag[i + samples/2]);
			factor_imag = _Qmpy(_Qsin(cos_arg), -out_real[i + samples/2]) + _Qmpy(_Qcos(cos_arg), out_imag[i + samples/2]);

			out_real[i] = temp_real + factor_real;
			out_imag[i] = temp_imag + factor_imag;

			out_real[i + samples/2] = temp_real - factor_real;
			out_imag[i + samples/2] = temp_imag - factor_imag;
		}
	} else {
		out_real[0] = _Q(data[0]);
	}
}

void iqfft_helper(const int* data, _iq* out_real, _iq* out_imag, int samples, int step_size)
{
    if (samples > 1) {
        iqfft_helper(data, out_real, out_imag, samples / 2, step_size * 2); //even
        iqfft_helper(data + step_size, out_real + samples/2, out_imag + samples/2, samples / 2, step_size * 2); //odd

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
        out_real[0] = _IQ(data[0]);
    }
}

int32_t FFT_test(const int* data, int32_t* out, int samples, int sampling_rate)
{
	//_iq out_real[128];
	_iq out_imag[256];
	int i;
	for (i = 0; i < samples; i++) {
		//out_real[i] = 0;
		out_imag[i] = 0;
	}
	iqfft_helper(data, out, out_imag, samples, 1);
	//Fft_transformRadix2(out_real, out_imag, samples);
	int32_t bucket, max = 0;
	for (i = 0; i < samples; i++) {
		out[i] = _IQint(_IQmag(out[i], out_imag[i]));
		if (out[i] > max && i <= samples/2) {
		    max = out[i];
		    bucket = i;
		}
	}

	return bucket * (sampling_rate / samples);
}

int fft_helper(int* data, int* out, int samples, int step_size)
{
    _q cur_max = 0;
    _q cos_arg, real, imag, temp;

    if (step_size < samples) {
        fft_helper(data, out, samples, step_size * 2);
        fft_helper(data + step_size, out + step_size, samples, step_size * 2);

        int i;
        for (i = 0; i < samples; i += 2 * step_size) {
            cos_arg = _Qdiv(_Qmpy(Q_2PI, i), _Q(samples));
            //q_data = _Q(data[k]);

            real =_Qmpy(data[i + step_size], _Qcos(cos_arg));
            imag =_Qmpy(data[i + step_size], _Qsin(cos_arg));
            //real = data[k]*cos((2*M_PI*n*k)/samples);
            //imag = data[k]*sin((2*M_PI*n*k)/samples);
            temp = real + data[i];
            out[i] +=_Qint(_Qmag(imag, temp));
            temp = data[i] - real;
            out[(i+samples)/2] += _Qint(_Qmag(imag, temp));
        }
    }

    return cur_max;
}

int FFT(int* data, int samples, int sample_rate)
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
