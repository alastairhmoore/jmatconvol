// ----------------------------------------------------------------------------
//
//  Copyright (C) 2010-2014 Fons Adriaensen <fons@linuxaudio.org>
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


#ifndef __PXTHREAD_H
#define __PXTHREAD_H


#include <pthread.h>
#include <semaphore.h>


class Pxthread
{
public:

    Pxthread (void);
    virtual ~Pxthread (void);
    Pxthread (const Pxthread&);
    Pxthread& operator=(const Pxthread&);

    virtual void thr_main (void) = 0;
    int thr_start (int policy, int priority, size_t stacksize = 0);

private:
  
    pthread_t  _thrid;
};


class ZCsema
{
public:

    ZCsema (void) { init (0, 0); }
    ~ZCsema (void) { sem_destroy (&_sema); }

    ZCsema (const ZCsema&); // disabled
    ZCsema& operator= (const ZCsema&); // disabled

    int init (int s, int v) { return sem_init (&_sema, s, v); }
    int post (void) { return sem_post (&_sema); }
    int wait (void) { return sem_wait (&_sema); }
    int trywait (void) { return sem_trywait (&_sema); }

private:

    sem_t  _sema;
};


#endif
