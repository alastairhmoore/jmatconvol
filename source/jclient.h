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


#ifndef __JCLIENT_H
#define __JCLIENT_H


#include <jack/jack.h>
#include "denseconv.h"


class Jclient
{
public:

    Jclient (const char *jname, const char *jserv);
    ~Jclient (void);

    enum { INIT, PROC, TERM };

    const char  *jname (void) const { return _jname; } 
    unsigned int fsamp (void) const { return _fsamp; } 
    unsigned int fragm (void) const { return _fragm; } 

    int add_input_port  (const char *name, const char *conn = 0);
    int add_output_port (const char *name, const char *conn = 0);
    int delete_ports (void);

    void start (Denseconv *dconv);
    void stop (void);
    int state (void) const { return _actstate; }

private:

    void  init_jack (const char *jname, const char *jserv);
    void  close_jack (void);
    void  jack_process (void);

    jack_client_t  *_jack_client;
    jack_port_t    *_jack_inppp [Denseconv::MAXINP];
    jack_port_t    *_jack_outpp [Denseconv::MAXOUT];
    int             _abspri;
    int             _policy;
    volatile int    _comstate;
    volatile int    _actstate;
    const char     *_jname;
    unsigned int    _fsamp;
    unsigned int    _fragm;
    unsigned int    _flags;
    unsigned int    _ninp;
    unsigned int    _nout;
    Denseconv      *_dconv;

    static void jack_static_shutdown (void *arg);
    static int  jack_static_buffsize (jack_nframes_t nframes, void *arg);
    static int  jack_static_process (jack_nframes_t nframes, void *arg);
};


#endif
