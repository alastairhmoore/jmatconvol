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
#include <ctype.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>
#include "jclient.h"
#include "config.h"



static const char   *clopt = "hN:s:";
static const char   *N_val = "jmatconvol";
static const char   *s_val = 0;
static bool          stop  = false;


static void help (void)
{
    fprintf (stderr, "\njmatconvol %s\n", VERSION);
    fprintf (stderr, "(C) 2006-2020 Fons Adriaensen  <fons@linuxaudio.org>\n\n");
    fprintf (stderr, "Usage: jmatconvol <options> <config file>\n");
    fprintf (stderr, "Options:\n");
    fprintf (stderr, "  -h                 Display this text\n");
    fprintf (stderr, "  -N <name>          Name to use as JACK client [jconvolmat]\n");   
    fprintf (stderr, "  -s <server>        Jack server name\n");
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
        case 'N' : N_val = optarg; break; 
        case 's' : s_val = optarg; break; 
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


static void makeports (Jclient *jclient, Config *config)
{
    unsigned int  i;
    char          s [16];

    for (i = 0; i < config->ninp (); i++)
    {
	if (config->inp_name (i))
	{
            jclient->add_input_port (config->inp_name (i),
				     config->inp_conn (i));
	}
	else
	{
            sprintf (s, "in-%d", i + 1);
            jclient->add_input_port (s, 0);
	}
    }
    for (i = 0; i < config->nout (); i++)
    {
	if (config->out_name (i))
	{
            jclient->add_output_port (config->out_name (i),
				      config->out_conn (i));
	}
	else
	{
            sprintf (s, "out-%d", i + 1);
            jclient->add_output_port (s, 0);
	}
    }
}


int main (int ac, char *av [])
{
    Jclient  *jclient;
    Config   *config;
    
    if (mlockall (MCL_CURRENT | MCL_FUTURE))
    {
        fprintf (stderr, "Warning: ");
        perror ("mlockall:");
    }

    procoptions (ac, av, "On command line:");
    if (ac <= optind) help ();

    jclient = new Jclient (N_val, s_val);
    config = new Config (jclient->fsamp (), jclient->fragm ());
    if (config->read_config (av [optind]))
    {
	delete jclient;
	return 1;
    }
    
    makeports (jclient, config);
    jclient->start (config->convproc ());

    signal (SIGINT, sigint_handler); 
    while (! stop)
    {    
	usleep (100000);
	if (jclient->state () & Jclient::TERM) stop = true;
    }

    jclient->stop ();
    delete jclient;
    delete config;

    return 0;
}

