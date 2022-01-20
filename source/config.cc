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


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <libgen.h>
#include "audiofile.h"
#include "config.h"
#include "sstring.h"


Config::Config (unsigned int fsamp, unsigned int fragm) :
    _convproc (0),
    _fsamp (fsamp),
    _fragm (fragm),
    _ninp (0),
    _nout (0),
    _maxsize (0),
    _nthread (0)
{
    memset (_inp_name, 0, Denseconv::MAXINP * sizeof (char *));
    memset (_inp_conn, 0, Denseconv::MAXINP * sizeof (char *));
    memset (_out_name, 0, Denseconv::MAXOUT * sizeof (char *));
    memset (_out_conn, 0, Denseconv::MAXOUT * sizeof (char *));
}


Config::~Config (void)
{
    delete _convproc;
}


int Config::read_config (const char *file)
{
    FILE  *F;
    char  *p, *q;

    if (! (F = fopen (file, "r"))) 
    {
	fprintf (stderr, "Can't open '%s' for reading\n", file);
        return -1;
    } 

    strncpy (_cdir, dirname ((char *) file), 512);
    _cdir [511] = 0;
    _stat = 0;
    _lnum = 0;

    while (!_stat && fgets (_line, 1024, F))
    {
        _lnum++;
        p = _line; 
        if (*p != '/')
	{
            while (isspace (*p)) p++;
            if ((*p > ' ') && (*p != '#'))
	    {
                _stat = ERR_SYNTAX;
		break;
	    }
            continue;
	}
        for (q = p; (*q >= ' ') && !isspace (*q); q++);
        for (*q++ = 0; (*q >= ' ') && isspace (*q); q++);

        if (! strcmp (p, "/cd"))
        {
            if (sstring (q, _cdir, 512) == 0) _stat = ERR_PARAM;
        }	
        else if (! strcmp (p, "/convolver/new")) _stat = make_convproc (q);
        else if (! strcmp (p, "/input/name"))    _stat = read_input (q); 
        else if (! strcmp (p, "/output/name"))   _stat = read_output (q); 
        else if (! strcmp (p, "/matrix/load"))        _stat = load_matrix (q, false, false); 
        else if (! strcmp (p, "/matrix/load_transp")) _stat = load_matrix (q, true, false); 
        else if (! strcmp (p, "/matrix/load_output")) _stat = load_matrix (q, false, true); 
        else if (! strcmp (p, "/matrix/load_input"))  _stat = load_matrix (q, true, true); 
        else _stat = ERR_COMMAND;
    }

    fclose (F);
    if (_stat == ERR_OTHER) return _stat;
    if (_stat)
    {
	fprintf (stderr, "Line %d: ", _lnum);
	switch (_stat)
	{
	case ERR_SYNTAX:
	    fprintf (stderr, "Syntax error.\n");
	    break;
	case ERR_PARAM:
	    fprintf (stderr, "Bad or missing parameters.\n");
	    break;
	case ERR_ALLOC:
	    fprintf (stderr, "Out of memory.\n");
	    break;
	case ERR_CANTCD:
	    fprintf (stderr, "Can't change directory to '%s'.\n", _cdir);
	    break;
	case ERR_COMMAND:
	    fprintf (stderr, "Unknown command.\n");
	    break;
	case ERR_NOCONV:
	    fprintf (stderr, "No convolver yet defined.\n");
	    break;
	case ERR_IONUM:
	    fprintf (stderr, "Bad input or output number.\n");
	    break;
	default:
	    fprintf (stderr, "Unknown error.\n");		     
	}
    }
    return _stat;
}


int Config::make_convproc (const char *line)
{
    int r;

    r = sscanf (line, "%u %u %u %u", &_ninp, &_nout, &_maxsize, &_nthread);
    if (r < 4) return ERR_PARAM;

    if ((_ninp == 0) || (_ninp > Denseconv::MAXINP))
    {
        fprintf (stderr, "Line %d: Number of inputs (%d) is out of range.\n", _lnum, _ninp);
        return ERR_OTHER;
    }
    if ((_nout == 0) || (_nout > Denseconv::MAXOUT))
    {
        fprintf (stderr, "Line %d: Number of outputs (%d) is out of range.\n", _lnum, _nout);
        return ERR_OTHER;
    }
    if ((_maxsize < 16) || (_maxsize > Denseconv::MAXLEN))
    {
        fprintf (stderr, "Line %d: Convolver size (%d) is out of range.\n", _lnum, _maxsize);
        return ERR_OTHER;
    }
    if ((_nthread < 1) || (_nthread > Denseconv::MAXTHR))
    {
        fprintf (stderr, "Line %d: Number of threads (%d) is out of range.\n", _lnum, _nthread);
        return ERR_OTHER;
    }

    _convproc = new Denseconv (_ninp, _nout, _maxsize, _fragm, _nthread);
    return 0;
}


int Config::read_input (const char *line)
{
    unsigned int  i, n;
    char          s [256];

    if (sscanf (line, "%u %n", &i, &n) != 1) return ERR_PARAM;
    if (--i >= _ninp) return ERR_IONUM;
    line += n;
    n = sstring (line, s, 256);
    if (*s) _inp_name [i] = strdup (s);
    else return ERR_PARAM;
    line += n;
    n = sstring (line, s, 256);
    if (*s) _inp_conn [i] = strdup (s);
    return 0;
}


int Config::read_output (const char *line)
{
    unsigned int  i, n;
    char          s [256];

    if (sscanf (line, "%u %n", &i, &n) != 1) return ERR_PARAM;
    if (--i >= _nout) return ERR_IONUM;
    line += n;
    n = sstring (line, s, 256);
    if (*s) _out_name [i] = strdup (s);
    else return ERR_PARAM;
    line += n;
    n = sstring (line, s, 256);
    if (*s) _out_conn [i] = strdup (s);
    return 0;
}


int Config::load_matrix (const char *line, bool transp, bool single)
{
    int     i, j, n;
    int     index, nchan, nsect, nfram, nused;
    char    file [512];
    float   gain;
    float  *buff;

    if (single)
    {
        if (sscanf (line, "%u %f %n", &index, &gain, &n) != 2) return ERR_PARAM;
    }
    else
    {
        if (sscanf (line, "%f %n", &gain, &n) != 1) return ERR_PARAM;
    }
    n = sstring (line + n, file, 512);
    if (!n) return ERR_PARAM;

    if (open_file (file)) return ERR_OTHER;

    nchan = transp ? _nout : _ninp;
    if (_afile.chan () != nchan)
    {
	fprintf (stderr, "Line %d: Channel count (%d) of '%s' doens't match.\n",
                 _lnum, _afile.chan (), file);
        _afile.close ();
        return ERR_OTHER;
    } 

    nsect = transp ? _ninp : _nout;
    if (single)
    {
	if ((index < 1) || (index > nsect))
	{
            fprintf (stderr, "Line %d: Input or output number (%d) is out of range.\n",
                     _lnum, index);
           _afile.close ();
           return ERR_OTHER;
	}
	nsect = 1;
	index -= 1;
    }	
    else
    {
        if (_afile.size () % nsect)
        {
            fprintf (stderr, "Line %d: File size of '%s' doesn't match.\n",
                     _lnum, file);
           _afile.close ();
           return ERR_OTHER;
	}
	index = 0;
    }
    nfram = (int)(_afile.size () / nsect);

    nused = nfram;
    if (nused > (int)_maxsize)
    {
	fprintf (stderr, "Line %d: Warning: data in '%s' will be truncated.\n",
                 _lnum, file);
	nused = _maxsize;
    } 

    try 
    {
        buff = new float [nused * nchan];
    }
    catch (...)
    {
	_afile.close ();
        return ERR_ALLOC;
    }

    for (j = 0; j < nsect; j++)
    {
	_afile.seek (j * nfram);
        _afile.read (buff, nused);
        for (i = 0; i < nchan; i++)
        {
	    if (transp) _convproc->setimp (j + index, i, gain, buff + i, nused, nchan);
	    else        _convproc->setimp (i, j + index, gain, buff + i, nused, nchan);
        }
    }
    _afile.close ();
    delete[] buff;
    return 0;
}

    
int Config::open_file (const char *file)
{
    char path [1024];
    
    if (*file == '/') strcpy (path, file);
    else
    {
        strcpy (path, _cdir);
        strcat (path, "/");
        strcat (path, file);
    }
    if (_afile.open_read (path))
    {
        fprintf (stderr, "Line %d: Unable to open '%s'.\n", _lnum, file);
        return ERR_OTHER;
    } 
    if (_afile.rate () != (int)_fsamp)
    {
        fprintf (stderr, "Line %d: Warning: sample rate (%d) of '%s' does not match.\n",
                 _lnum, _afile.rate (), file);
    }
    return 0;
}


