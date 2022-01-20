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


#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "denseconv.h"


// -------------------------------------------------------------------------------


void DCfdata::init (int npart, int fsize)
{
    clear ();
    _npart = npart;
    _fsize = fsize;
    _freq_data = new fftwf_complex* [npart];
    for (int i = 0; i < npart; i++)
    {
        _freq_data [i] = (fftwf_complex *)(fftwf_malloc (fsize * sizeof (fftwf_complex)));
    }
}


void DCfdata::fini ()
{
    for (int i = 0; i < _npart; i++)
    {
        fftwf_free (_freq_data [i]);
    }
    delete[] _freq_data;
    _npart = 0;
    _fsize = 0;
    _freq_data = 0;
}


void DCfdata::clear ()
{
    for (int i = 0; i < _npart; i++)
    {
	memset (_freq_data [i], 0, _fsize * sizeof (fftwf_complex));
    }
}


// -------------------------------------------------------------------------------


DCparam::DCparam (int ninp, int nout, int maxlen, int period, int nthread):
    _ninp (ninp),
    _nout (nout),
    _maxlen (maxlen),
    _period (period),
    _nthread (nthread),
    _time_data (0),
    _plan_r2c (0),
    _plan_c2r (0)
{
    assert ((ninp > 0) && (ninp <= MAXINP));
    assert ((nout > 0) && (nout <= MAXOUT));
    assert ((maxlen > 16) && (maxlen <= MAXLEN));
    assert (((period & (period - 1)) == 0) && (period < MAXPER));
    assert ((nthread > 0) && (nthread <= MAXTHR));
    init ();
}


DCparam::~DCparam (void)
{
    fini ();
}


void DCparam::init (void)
{
    int i;

    _time_data = (float *) (fftwf_malloc (2 * _period * sizeof (float)));
    _npart = (_maxlen + _period - 1) / _period;
    _ipart = 0;
    _inp_fdata = new DCfdata [_ninp];
    for (i = 0; i < _ninp; i++) _inp_fdata [i].init (_npart, _period + 1);
    _mac_fdata = new DCfdata [_ninp * _nout];
    for (i = 0; i < _ninp * _nout; i++) _mac_fdata [i].init (_npart, _period + 1);
    _outbuff = new float * [_nout];
    for (i = 0; i < _nout; i++) _outbuff [i] = new float [_period];
    _plan_r2c = fftwf_plan_dft_r2c_1d (2 * _period, _time_data, _inp_fdata->_freq_data [0], 0);
    _plan_c2r = fftwf_plan_dft_c2r_1d (2 * _period, _inp_fdata->_freq_data [0], _time_data, 0);
}


void DCparam::fini (void)
{
    int i;

    fftwf_free (_time_data);
    delete[] _inp_fdata;
    delete[] _mac_fdata;
    for (i = 0; i < _nout; i++) delete[] _outbuff [i];
    delete[] _outbuff;
    fftwf_destroy_plan (_plan_r2c);
    fftwf_destroy_plan (_plan_c2r);
}


// -------------------------------------------------------------------------------


Workthr::Workthr (int index, DCparam *param) :
    _index (index),
    _param (param)
{
    int per;

    per = _param->_period;
    _time_data = (float *) (fftwf_malloc (2 * per * sizeof (float)));
    _freq_data = (fftwf_complex *)(fftwf_malloc ((per + 1) * sizeof (fftwf_complex)));
    thr_start (SCHED_FIFO, -20, 0);
}


Workthr::~Workthr (void)
{
    fftwf_free (_time_data);
    fftwf_free (_freq_data);
}


void Workthr::thr_main (void)
{
    int      inp, out, nf, np, ia, ib, k;
    float    *p, *q;;
    DCfdata  *FDA, *FDB;
    fftwf_complex *A, *B;

    _stop = false;
    while (true)
    {
	_trig.wait ();
	if (_stop) break;
	nf = _param->_period;
	np = _param->_npart;
	if (_param->_wtype == DCparam::FFT)
	{
	    for (inp = _index; inp < _param->_ninp; inp += _param->_nthread)
	    {
		memcpy (_time_data, _param->_inpdata [inp], nf * sizeof (float));
		memset (_time_data + nf, 0, nf * sizeof (float));
		FDA = _param->_inp_fdata + inp;
	        fftwf_execute_dft_r2c (_param->_plan_r2c, _time_data, FDA->_freq_data [_param->_ipart]);	
	    }
	}
	else
	{
	    for (out = _index; out < _param->_nout; out += _param->_nthread)
	    {
		memset (_freq_data, 0, (nf + 1) * sizeof (fftwf_complex));
                FDA = _param->_inp_fdata;
                FDB = _param->_mac_fdata + (out * _param->_ninp);
		for (inp = 0; inp < _param->_ninp; inp++)
		{
		    ia = _param->_ipart;
		    for (ib = 0; ib < np; ib++)
		    {
			A = FDA->_freq_data [ia];
			B = FDB->_freq_data [ib];
			for (k = 0; k <= nf; k++)
			{
			    _freq_data [k][0] += A [k][0] * B [k][0] - A [k][1] * B [k][1];
			    _freq_data [k][1] += A [k][0] * B [k][1] + A [k][1] * B [k][0];
			}
			if (--ia < 0) ia += np;
		    }
		    FDA++;
		    FDB++;
		}
	        fftwf_execute_dft_c2r (_param->_plan_c2r, _freq_data, _time_data);	
		p = _param->_outdata [out];
		q = _param->_outbuff [out];
		for (k = 0; k < nf; k++) p [k] = q [k] + _time_data [k];
		memcpy (q, _time_data + nf, nf * sizeof (float));
	    }
	}
        _done.post ();
    }
    delete this;
}


// -------------------------------------------------------------------------------


Denseconv::Denseconv (int ninp, int nout, int maxlen, int period, int nthread):
    DCparam (ninp, nout, maxlen, period, nthread)
{
    for (int i = 0; i < _nthread; i++)
    {
        _workers [i] = new Workthr (i, this);
    }
    clear ();
    reset ();
}


Denseconv::~Denseconv (void)
{
    for (int i = 0; i < _nthread; i++) _workers [i]->stop ();
    usleep (100000);
}



int Denseconv::setimp (int inp, int out, float gain, float *data, int size, int step)
{
    int     i, j, n;
    DCfdata *fdata;

    if ((inp < 0) || (inp >= _ninp)) return 1;
    if ((out < 0) || (out >= _nout)) return 1;
    if (size > _maxlen) size = _maxlen;

    gain /= 2 * _period;
    fdata = _mac_fdata + (out * _ninp + inp);
    fdata->clear ();
    for (i = 0; size; i++)
    {
	memset (_time_data, 0, 2 * _period * sizeof (float));
	n = size;
	if (n > _period) n = _period;
	for (j = 0; j < n; j++)
	{
	    _time_data [j] = *data * gain;
	    data += step;
	}
        fftwf_execute_dft_r2c (_plan_r2c, _time_data, fdata->_freq_data [i]);	
	size -= n;
    }
    return 0;
}


void Denseconv::reset (void)
{
    int i;
    for (i = 0; i < _ninp; i++) _inp_fdata [i].clear ();
    for (i = 0; i < _nout; i++) memset (_outbuff [i], 0, _period * sizeof (float));
}


void Denseconv::clear (void)
{
    int i;
    for (i = 0; i < _ninp * _nout; i++) _mac_fdata [i].clear ();
}


void Denseconv::process (float *inp [], float *out [])
{
    int i;

    _inpdata = inp;
    _outdata = out;
    _wtype = FFT;
    for (i = 0; i < _nthread; i++) _workers [i]->trig ();
    for (i = 0; i < _nthread; i++) _workers [i]->wait ();
    _wtype = MAC;
    for (i = 0; i < _nthread; i++) _workers [i]->trig ();
    for (i = 0; i < _nthread; i++) _workers [i]->wait ();
    if (++_ipart == _npart) _ipart = 0;
}


// -------------------------------------------------------------------------------
