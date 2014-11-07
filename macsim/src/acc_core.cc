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
 * File         : acc_core.cc
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
#include "acc_core.h"
#include "knob.h"
#include "pqueue.h"
#include "utils.h"
#include "bug_detector.h"
#include "walker.h"

#include "config.h"

#include "debug_macros.h"

#include "all_knobs.h"

///////////////////////////////////////////////////////////////////////////////////////////////


#define DEBUG(args...)   _DEBUG(*m_simBase->m_knobs->KNOB_DEBUG_TRACE_READ, ## args)


///////////////////////////////////////////////////////////////////////////////////////////////


void acc_core_c::init(void)
{

}


// acc_core_c constructor
acc_core_c::acc_core_c (macsim_c* simBase, Unit_Type type)
{
    // Initialization
    m_core_id         = -1;
    m_unit_type       = type;
    m_sim_start_time  = time(NULL);
    m_core_type       = string("acc");
    m_marker_count    = 0;


    // Reference to simulation base globals
    m_simBase         = simBase;


    // clock cycle
    m_cycle = 0;
    m_last_pop = 0;

    // Loop iteration cursor
    m_cursor = 0;


    // Acc Core Components Init

    int num_pes = *KNOB(KNOB_ACC_NUM_PE); // Replace with KNOB
    report("ACC Num PE: " << num_pes);
    // int imb_cap = 15*64; // 15*64 calculated from number of ways in the LLC-1 * line size
    int imb_cap = *KNOB(KNOB_ACC_IMB_CAP); // 15*64 calculated from number of ways in the LLC-1 * line size
    int max_walkers = 8; // Replace with same knob as trace_max_ds
    int acc_work_queue_latency = 1; // Replace with KNOB, unit is acc_cycles
    int acc_work_queue_size = 1024; // Replace with KNOB
    int acc_msg_queue_latency = 1; // Replace with KNOB, unit is acc_cycles
    int acc_msg_queue_size = 1024; // Replace with KNOB

    // Queues

    m_walker_imb_queue = new pqueue_c<Acc_Msg_Type>(
                    acc_msg_queue_size,
                    acc_msg_queue_latency,
                    "Acc_Msg_Queue_Walker_IMB",
                    simBase);

    m_imb_walker_queue = new pqueue_c<Acc_Msg_Type>(
                    acc_msg_queue_size,
                    acc_msg_queue_latency,
                    "Acc_Msg_Queue_IMB_Walker",
                    simBase);

    m_pe_imb_queue     = new pqueue_c<Acc_Msg_Type>(
                    acc_msg_queue_size,
                    acc_msg_queue_latency,
                    "Acc_Msg_Queue_PE_IMB",
                    simBase);

    m_imb_pe_queue     = new pqueue_c<Acc_Msg_Type>(
                    acc_msg_queue_size,
                    acc_msg_queue_latency,
                    "Acc_Msg_Queue_IMB_PE",
                    simBase);


    pes =  new pe_c(num_pes, m_imb_pe_queue, m_pe_imb_queue,
                    simBase, &m_cursor, &m_pe_walker_load_keys,
                    &m_pe_walker_store_keys);

    int_buf = new imb_c(imb_cap, m_pe_imb_queue, m_imb_pe_queue,
                        m_walker_imb_queue, m_imb_walker_queue, simBase);

    walkers = new walker_c( max_walkers, m_imb_walker_queue, m_walker_imb_queue, simBase,
                            &m_pe_walker_load_keys,
                            &m_pe_walker_store_keys,
                            &m_cursor);

    // Stats -- Initialize accelerator stats here

    m_stats.insert(make_pair(string("IMB_REFILL"),0));

    m_stats.insert(make_pair(string("PE_CYCLE_COUNT"),0));
    m_stats.insert(make_pair(string("PE_INT"),0));
    m_stats.insert(make_pair(string("PE_FP"),0));
    m_stats.insert(make_pair(string("IBUF_ACCESS"),0));
    m_stats.insert(make_pair(string("STACK_COUNT"),0));

    m_stats.insert(make_pair(string("WALKER_CYCLE_COUNT"),0));
    m_stats.insert(make_pair(string("WALKER_MSHR_ISSUE"),0));
    m_stats.insert(make_pair(string("WALKER_IDLE_CYCLES"),0));

    m_stats.insert(make_pair(string("BTREE_ADD_ACCESS_LATENCY"),0));
    m_stats.insert(make_pair(string("HASHT_ADD_ACCESS_LATENCY"),0));

#if 0
    m_stats.insert(make_pair(string("BANK0"),0));
    m_stats.insert(make_pair(string("BANK1"),0));
    m_stats.insert(make_pair(string("BANK2"),0));
    m_stats.insert(make_pair(string("BANK3"),0));
    m_stats.insert(make_pair(string("BANK4"),0));
    m_stats.insert(make_pair(string("BANK5"),0));
    m_stats.insert(make_pair(string("BANK6"),0));
    m_stats.insert(make_pair(string("BANK7"),0));
#endif 
    // m_stats.insert(make_pair(string("STRAY_BLOCK"),0));

}
// acc_core_c destructor
acc_core_c::~acc_core_c()
{
    delete pes;
    delete int_buf;
    delete m_imb_pe_queue;
    delete m_pe_imb_queue;
    delete m_walker_imb_queue;
    delete m_imb_walker_queue;
}


// start core simulation
void acc_core_c::start(void)
{

}


// stop core simulation
void acc_core_c::stop(void)
{

}


// main execution routine
// In every cycle, run all pipeline stages
void acc_core_c::run_a_cycle(void)
{
    if(!m_marker_in_queue.empty())
    {
        pes->run_a_cycle();
        int_buf->run_a_cycle();
        walkers->run_a_cycle();

        advance_queues();

        if(pes->m_status == PE_INACTIVE)
        {
            report("PE Inactive: Popping Marker " << m_marker_count);
            walkers->cleanup();
            m_simBase->m_acc_core_pointer->pop_marker();
            m_marker_count++;
        }
    }

    ++m_cycle;
}


// age entries in various queues
void acc_core_c::advance_queues(void)
{
    m_walker_imb_queue->advance();
    m_imb_walker_queue->advance();
    m_pe_imb_queue->advance();
    m_imb_pe_queue->advance();
    m_walker_imb_queue->advance();
    m_imb_walker_queue->advance();
}


// last heartbeat call when a thread is terminated
void acc_core_c::final_heartbeat(int thread_id)
{

}


// In every HEARTBEAT_INTERVAL cycles, print performance information
void acc_core_c::check_heartbeat(bool final)
{
  core_heartbeat(final);

}

// core heartbeat
void acc_core_c::core_heartbeat(bool final)
{
    if (!*m_simBase->m_knobs->KNOB_PRINT_HEARTBEAT)
        return ;
}


// check forward progress of the simulation
void acc_core_c::check_forward_progress()
{


}

// add marker to in_queue

void acc_core_c::push_marker(trace_info_s *trace_info, uint32_t core_id)
{
    ASSERTM(core_id < *KNOB(KNOB_NUM_SIM_CORES), "Unknown core tried to enqueue marker");

    trace_info_s* t = new trace_info_s;
    memcpy((void*)t, (const void*)trace_info, sizeof(trace_info_s));
    t->m_branch_target = core_id;
    m_marker_in_queue.push(t);
    if(m_requesting_cores.count(core_id))
    {
        m_requesting_cores[core_id]++;
    }
    else
    {
        m_requesting_cores.insert(std::pair<int,int>(core_id,1));
    }
}

void acc_core_c::pop_marker(void)
{
    if(!m_marker_in_queue.empty())
    {
        trace_info_s *t = m_marker_in_queue.front();
        m_marker_in_queue.pop();
        uint32_t core_id = t->m_branch_target;
        if(m_requesting_cores.count(core_id))
        {
            m_requesting_cores[core_id]--;
            if(m_requesting_cores[core_id] == 0)
            {
                m_requesting_cores.erase(core_id);
            }
        }
        delete t;
        m_cursor = 0; // Reset cursor to 0
    }
}

uint32_t acc_core_c::get_pe_cursor_stride()
{
    return pes->get_cursor_stride();
}

bool acc_core_c::ds_op_pending(uint32_t core_id)
{
    return m_requesting_cores.count(core_id) > 0;
}

string acc_core_c::ds_op_type_to_str(ds_ops x)
{
    switch(x)
    {
      case VEC_ITER:
        return string("VEC_ITER");
      case VEC_SORT:
        return string("VEC_SORT");
      case HASH_SEARCH:
        return string("HASH_SEARCH");
    }

}

trace_info_s* acc_core_c::get_front_marker()
{
    return m_marker_in_queue.front();
}

void acc_core_c::add_stat(string name, uint64_t inc_val)
{
    ASSERTM(m_stats.count(name) > 0, "Unknown ACC Stat");
    m_stats[name] += inc_val;
}

void acc_core_c::print_stats()
{
    std::ofstream stats_file("acc.stat.out", std::ofstream::out);

    for(auto I = m_stats.begin(); I != m_stats.end(); I++)
        stats_file << I->first << "\t" << I->second << endl;

    stats_file.close();
}
