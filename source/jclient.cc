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


#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "jclient.h"
#include <stdio.h>


Jclient::Jclient (const char *jname, const char *jserv) :
    _jack_client (0),
    _comstate (INIT),
    _actstate (INIT),
    _jname (0),
    _fsamp (0),
    _fragm (0),
    _flags (0),
    _ninp (0),
    _nout (0),
    _dconv (0)
{
    init_jack (jname, jserv);  
}


Jclient::~Jclient (void)
{
    if (_jack_client) close_jack ();
}


void Jclient::init_jack (const char *jname, const char *jserv)
{
    struct sched_param  spar;
    jack_status_t       stat;
    int                 opts;

    opts = JackNoStartServer;
    if (jserv) opts |= JackServerName;
    if ((_jack_client = jack_client_open (jname, (jack_options_t) opts, &stat, jserv)) == 0)
    {
        fprintf (stderr, "Can't connect to JACK\n");
        exit (1);
    }

    jack_on_shutdown (_jack_client, jack_static_shutdown, (void *) this);
    jack_set_buffer_size_callback (_jack_client, jack_static_buffsize, (void *) this);
    jack_set_process_callback (_jack_client, jack_static_process, (void *) this);

    if (jack_activate (_jack_client))
    {
        fprintf(stderr, "Can't activate JACK.\n");
        exit (1);
    }

    _jname = jack_get_client_name (_jack_client);
    _fsamp = jack_get_sample_rate (_jack_client);
    _fragm = jack_get_buffer_size (_jack_client);
    if ((_fragm < 64) || (_fragm > 4096))
    {
	fprintf (stderr, "Fragment size must be 64...4096.\n");
	exit (1);
    }
    if (_fragm & (_fragm - 1))
    {
	fprintf (stderr, "Fragment size must be a power of 2.\n");
	exit (1);
    }

    pthread_getschedparam (jack_client_thread_id (_jack_client), &_policy, &spar);
    _abspri = spar.sched_priority;

    memset (_jack_inppp, 0, Denseconv::MAXINP * sizeof (jack_port_t *));
    memset (_jack_outpp, 0, Denseconv::MAXOUT * sizeof (jack_port_t *));
}


void Jclient::close_jack ()
{
    jack_deactivate (_jack_client);
    jack_client_close (_jack_client);
}


void Jclient::jack_static_shutdown (void *arg)
{
     Jclient *C = (Jclient *) arg;
     C->_comstate = TERM;
}


int Jclient::jack_static_buffsize (jack_nframes_t nframes, void *arg)
{
    Jclient *C = (Jclient *) arg;
    if (C->_fragm) C->_comstate = TERM;
    return 0;
}


int Jclient::jack_static_process (jack_nframes_t nframes, void *arg)
{
    Jclient *C = (Jclient *) arg;
    C->jack_process ();
    return 0;
}


void Jclient::start (Denseconv *dconv)
{
    _dconv = dconv;
    _comstate = PROC;
}


void Jclient::stop (void)
{
    _comstate = TERM;
    usleep (100000);
}


void Jclient::jack_process (void)
{
    unsigned int  i;
    float         *inpp [DCparam::MAXINP];
    float         *outp [DCparam::MAXOUT];

    _actstate = _comstate;
    if (_actstate != PROC) return;
    for (i = 0; i < _nout; i++)
    {
        outp [i] = (float *) jack_port_get_buffer (_jack_outpp [i], _fragm);
    }
    for (i = 0; i < _ninp; i++)
    {
        inpp [i] = (float *) jack_port_get_buffer (_jack_inppp [i], _fragm);
    }
    _dconv->process (inpp, outp);
}


int Jclient::add_input_port (const char *name, const char *conn)
{
    char s [512];

    _jack_inppp [_ninp] = jack_port_register (_jack_client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    if (conn)
    {
	sprintf (s, "%s:%s", _jname, name);
	jack_connect (_jack_client, conn, s);
    }
    return _ninp++;
}


int Jclient::add_output_port (const char *name, const char *conn)
{
    char s [512];

    _jack_outpp [_nout] = jack_port_register (_jack_client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (conn)
    {
	sprintf (s, "%s:%s", _jname, name);
	jack_connect (_jack_client, s, conn);
    }
    return _nout++;
}
