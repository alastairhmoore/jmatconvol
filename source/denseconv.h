// ----------------------------------------------------------------------------
//
//  Copyright (C) 2010-2020 Fons Adriaensen <fons@linuxaudio.org>
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------


#ifndef __DENSECONV_H
#define __DENSECONV_H


#include <fftw3.h>
#include "pxthread.h"


class DCfdata
{
    friend class DCparam;
    friend class Workthr;
    friend class Denseconv;

private:

    DCfdata (void): _npart (0), _fsize (0), _freq_data (0) {}	
    ~DCfdata (void) { fini (); }

    void init (int npart, int fsize);
    void clear (void);
    void fini (void);

    int              _npart;
    int              _fsize;
    fftwf_complex  **_freq_data;   
};


class DCparam
{
    friend class Workthr;
    friend class Denseconv;

public:

    enum
    {
        MAXINP = 64,
        MAXOUT = 64,
        MAXLEN = 4096,
        MAXPER = 4096,
        MAXTHR = 16
    };

private:

    DCparam (int ninp, int nout, int maxlen, int period, int nthread);
    ~DCparam (void);
    void init (void);
    void fini (void);

    enum { FFT, MAC };

    int              _ninp;
    int              _nout;
    int              _maxlen;
    int              _period;
    int              _nthread;
    int              _npart;
    int              _ipart;
    int              _wtype;
    float          **_inpdata;
    float          **_outdata;
    float          **_outbuff;
    float           *_time_data;      // workspace
    fftwf_plan       _plan_r2c;       // FFTW plan, forward FFT
    fftwf_plan       _plan_c2r;       // FFTW plan, inverse FFT
    DCfdata         *_inp_fdata;
    DCfdata         *_mac_fdata;
};    


class Workthr: public Pxthread
{
    friend class Denseconv;

private:

    Workthr (int index, DCparam *param);
    virtual ~Workthr (void);

    void trig (void) { _trig.post (); }
    void wait (void) { _done.wait (); }
    void stop (void)
    {
       _stop = true;
       _trig.post ();
    }

    virtual void thr_main (void);

    int              _index;
    DCparam         *_param;
    ZCsema           _trig;
    ZCsema           _done;
    volatile bool    _stop;
    float           *_time_data;      // workspace
    fftwf_complex   *_freq_data;      // workspace
};


class Denseconv: public DCparam
{
public:

    Denseconv (int ninp, int nout, int maxlen, int period, int nthread = 1);
    ~Denseconv (void);

    int  setimp (int inp, int out, float gain, float *data, int size, int step = 1);
    void clear (void);
    void reset (void);
    void process (float *inp [], float *out []);

private:

    Workthr   *_workers [MAXTHR];
};


#endif


