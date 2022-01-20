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


#ifndef __CONFIG_H
#define __CONFIG_H


#include "denseconv.h"
#include "audiofile.h"


class Config
{
public:

    Config (unsigned int fsamp, unsigned int fragm);
    ~Config (void);

    int read_config (const char *conffile);

    unsigned int   ninp (void) const { return _ninp; }
    unsigned int   nout (void) const { return _nout; }
    unsigned int   maxsize (void) const { return _maxsize; }
    const char     *inp_name (unsigned int inp) const { return _inp_name [inp]; }
    const char     *inp_conn (unsigned int inp) const { return _inp_conn [inp]; }
    const char     *out_name (unsigned int out) const { return _out_name [out]; }
    const char     *out_conn (unsigned int out) const { return _out_conn [out]; }
    Denseconv      *convproc (void) { return _convproc; }
    
    
private:    

    enum { NOERR, ERR_OTHER, ERR_SYNTAX, ERR_PARAM, ERR_ALLOC,
	   ERR_CANTCD, ERR_COMMAND, ERR_NOCONV, ERR_IONUM };

    int make_convproc (const char *line);
    int read_input (const char *line);
    int read_output (const char *line);
    int load_matrix (const char *line, bool transp, bool single);
    int open_file (const char *file);
    
    Denseconv    *_convproc;
    Audiofile     _afile;
    unsigned int  _fsamp;
    unsigned int  _fragm;
    unsigned int  _ninp;
    unsigned int  _nout;
    unsigned int  _maxsize;
    unsigned int  _nthread;
    int           _stat;
    int           _lnum;
    char          _line [1024];
    char          _cdir [512];
    char         *_inp_name [Denseconv::MAXINP];
    char         *_out_name [Denseconv::MAXOUT];
    char         *_inp_conn [Denseconv::MAXINP];
    char         *_out_conn [Denseconv::MAXOUT];
};


#endif


