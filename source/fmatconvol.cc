// ----------------------------------------------------------------------------
//
//  Copyright (C) 2010-2021 Fons Adriaensen <fons@linuxaudio.org>
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
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include "audiofile.h"
#include "config.h"



static const char   *clopt = "hT";
static bool          stop  = false;
static bool          T_opt = false;


static void help (void)
{
    fprintf (stderr, "\nfmatconvol %s\n", VERSION);
    fprintf (stderr, "(C) 2010-2021 Fons Adriaensen  <fons@linuxaudio.org>\n\n");
    fprintf (stderr, "Usage: fmatconvol <configuration file> <input file> <output file>\n");
    fprintf (stderr, "Options:\n");
    fprintf (stderr, "  -h  Display this text\n");
    fprintf (stderr, "  -T  Truncate ouput to input size\n");
    exit (1);
}


static void procoptions (int ac, char *av [], const char *where)
{
    int k;
    
    optind = 1;
    opterr = 0;
    while ((k = getopt (ac, av, (char *) clopt)) != -1)
    {
        if (optarg && (*optarg == '-'))
        {
            fprintf (stderr, "\n%s\n", where);
	    fprintf (stderr, "  Missing argument for '-%c' option.\n", k); 
            fprintf (stderr, "  Use '-h' to see all options.\n");
            exit (1);
        }
	switch (k)
	{
        case 'h' : help (); exit (0);
        case 'T' : T_opt = true; break;
        case '?':
            fprintf (stderr, "\n%s\n", where);
            if (optopt != ':' && strchr (clopt, optopt))
	    {
                fprintf (stderr, "  Missing argument for '-%c' option.\n", optopt); 
	    }
            else if (isprint (optopt))
	    {
                fprintf (stderr, "  Unknown option '-%c'.\n", optopt);
	    }
            else
	    {
                fprintf (stderr, "  Unknown option character '0x%02x'.\n", optopt & 255);
	    }
            fprintf (stderr, "  Use '-h' to see all options.\n");
            exit (1);
        default:
            abort ();
 	}
    }
}


static void sigint_handler (int)
{
    stop = true;
}


int main (int ac, char *av [])
{
    int            i, j, k, nd, ninp, nout, fsamp, fragm;
    long           nf;
    float          *buff1, *buff2, *p, *q;
    float          *data [1024];
    Config         *config;
    Audiofile      Ainp, Aout;

    
    procoptions (ac, av, "On command line:");
    if (ac != optind + 3) help ();
    av += optind;

    if (Ainp.open_read (av [1]))
    {
	fprintf (stderr, "Unable to read input file '%s'\n", av [1]);
        return 1;
    } 

    fsamp = Ainp.rate ();
    fragm = 1024;
    
    config = new Config (fsamp, fragm);
    if (config->read_config (av [0]))
    {
	Ainp.close ();
	delete config;
        return 1;
    }

    ninp = config->ninp ();
    nout = config->nout ();
    
    if (Ainp.chan () != (int) ninp)
    {
	fprintf (stderr, "Number of inputs (%d) in config file doesn't match input file (%d)'\n",
                 ninp, Ainp.chan ());
	Ainp.close ();
	delete config;
        return 1;
    }

    if (Aout.open_write (av [2], Audiofile::TYPE_WAV, Audiofile::FORM_FLOAT, fsamp, nout))   
    {
	fprintf (stderr, "Unable to create output file '%s'\n", av [2]);
	Ainp.close ();
	delete config;
        return 1;
    } 

    nd = (ninp > nout) ? ninp : nout;
    buff1 = new float [nd * fragm]; 
    buff2 = new float [nd * fragm];
    for (i = 0; i < nd; i++) data [i] = buff2 + i * fragm;
    
    signal (SIGINT, sigint_handler); 
    stop = false;
    nf = Ainp.size ();
    if (! T_opt) nf += config->maxsize () - 1;
    while (nf)
    {
	if (stop) break;
	k = Ainp.read (buff1, fragm);
	// De-interleave channels.
	for (i = 0; i < ninp; i++)
	{
	    p = buff1 + i;
	    q = data [i];
	    for (j = 0; j < k; j++) q [j] = p [j * ninp];
	    if (k < fragm) memset (q + k, 0, (fragm - k) * sizeof (float));
	}
	config->convproc ()->process (data, data);
	k = fragm;
	if (k > nf) k = nf;
	// Interleave channels.
	for (i = 0; i < nout; i++)
	{
	    p = data [i];
	    q = buff1 + i;
	    for (j = 0; j < k; j++) q [j * nout] = p [j];
	}
        Aout.write (buff1, k);
	nf -= k;
    }

    Ainp.close ();
    Aout.close ();
    if (stop) unlink (av [2]);
    delete[] buff1;
    delete[] buff2;
    delete config;

    return stop ? 1 : 0;
}

