/*
Copyright (c) <2012>, <Georgia Institute of Technology> All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted
provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions
and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or other materials provided
with the distribution.

Neither the name of the <Georgia Institue of Technology> nor the names of its contributors
may be used to endorse or promote products derived from this software without specific prior
written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/


/**********************************************************************************************
 * File         : dma_core.cc
 * Author       : ska124
 * Date         : 07/05/2013
 * CVS          :
 * Description  : main function
 *********************************************************************************************/


/**********************************************************************************************
 * 1. run_a_cycle :
 *
 * 2. heartbeat : for every N cycle, print out current/accumulative performance information.
 *
 *********************************************************************************************/


#include <stdio.h>
#include <cmath>
#include <map>
#include <sstream>
#include <fstream>

#include "assert.h"
#include "dma_core.h"
#include "knob.h"
#include "pqueue.h"
#include "utils.h"
#include "bug_detector.h"
#include "acc_core.h"

#include "config.h"

#include "debug_macros.h"

#include "all_knobs.h"

///////////////////////////////////////////////////////////////////////////////////////////////


#undef DEBUG
#define DEBUG(args...)   _DEBUG(*m_simBase->m_knobs->KNOB_DEBUG_TRACE_READ, ## args)


///////////////////////////////////////////////////////////////////////////////////////////////


void dma_core_c::init(void)
{

}


// dma_core_c constructor
dma_core_c::dma_core_c (macsim_c* simBase, Unit_Type type)
{
   m_simBase = simBase;
   m_unit_type = type;
   m_active = false;
   m_done = false;
}

// dma_core_c destructor
dma_core_c::~dma_core_c()
{

}


// start core simulation
void dma_core_c::start(void)
{
}


// stop core simulation
void dma_core_c::stop(void)
{

}


// main execution routine
// In every cycle, run all pipeline stages
void dma_core_c::run_a_cycle(void)
{
    if(m_done)
    {
        std::cerr << "DMA Done\n";
        // Set the next acc/CPU to active
        if(m_next != -1)
            m_simBase->m_acc_core_pointers[m_next]->m_active = true;
        else
        {
            m_simBase->m_core_pointers[0]->start_frontend();
            m_simBase->m_core_pointers[0]->m_active = true;
        }

        m_active = false;
        m_done = false;
    }

    if(m_active)
    {
        // Issue all the requests we need to 
        // Wait for them to come back
        std::cerr << "DMA Active\n";
        m_done = true;     
    }
}


// age entries in various queues
void dma_core_c::advance_queues(void)
{}


// last heartbeat call when a thread is terminated
void dma_core_c::final_heartbeat(int thread_id)
{

}


// In every HEARTBEAT_INTERVAL cycles, print performance information
void dma_core_c::check_heartbeat(bool final)
{
  core_heartbeat(final);
}

// core heartbeat
void dma_core_c::core_heartbeat(bool final)
{
    if (!*m_simBase->m_knobs->KNOB_PRINT_HEARTBEAT)
        return ;
}


// check forward progress of the simulation
void dma_core_c::check_forward_progress()
{
}

void dma_core_c::add_stat(string name, uint64_t inc_val)
{
}

void dma_core_c::print_stats()
{
}