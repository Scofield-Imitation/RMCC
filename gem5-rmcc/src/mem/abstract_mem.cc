/*
 * Copyright (c) 2010-2012,2017 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2001-2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Ron Dreslinski
 *          Ali Saidi
 *          Andreas Hansson
 */

#include "mem/abstract_mem.hh"

#include <vector>

#include "arch/locked_mem.hh"
#include "cpu/base.hh"
#include "cpu/thread_context.hh"
#include "debug/LLSC.hh"
#include "debug/MemoryAccess.hh"
#include "mem/packet_access.hh"
#include "sim/system.hh"

using namespace std;

AbstractMemory::AbstractMemory(const Params *p) :
    MemObject(p), range(params()->range), pmemAddr(NULL),
    confTableReported(p->conf_table_reported), inAddrMap(p->in_addr_map),
    kvmMap(p->kvm_map), _system(NULL)
{
}

void
AbstractMemory::init()
{
    assert(system());

    if (size() % _system->getPageBytes() != 0)
        panic("Memory Size not divisible by page size\n");
}

void
AbstractMemory::setBackingStore(uint8_t* pmem_addr)
{
    pmemAddr = pmem_addr;
}
void
AbstractMemory::regStats()
{
    MemObject::regStats();

    using namespace Stats;

    assert(system());
  
    bytesRead
        .init(system()->maxMasters())
        .name(name() + ".bytes_read")
        .desc("Number of bytes read from this memory")
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        bytesRead.subname(i, system()->getMasterName(i));
    }
    bytesInstRead
        .init(system()->maxMasters())
        .name(name() + ".bytes_inst_read")
        .desc("Number of instructions bytes read from this memory")
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        bytesInstRead.subname(i, system()->getMasterName(i));
    }
    bytesWritten
        .init(system()->maxMasters())
        .name(name() + ".bytes_written")
        .desc("Number of bytes written to this memory")
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        bytesWritten.subname(i, system()->getMasterName(i));
    }
    numReads
        .init(system()->maxMasters())
        .name(name() + ".num_reads")
        .desc("Number of read requests responded to by this memory")
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        numReads.subname(i, system()->getMasterName(i));
    }
    numWrites
        .init(system()->maxMasters())
        .name(name() + ".num_writes")
        .desc("Number of write requests responded to by this memory")
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        numWrites.subname(i, system()->getMasterName(i));
    }
    numOther
        .init(system()->maxMasters())
        .name(name() + ".num_other")
        .desc("Number of other requests responded to by this memory")
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        numOther.subname(i, system()->getMasterName(i));
    }
    bwRead
        .name(name() + ".bw_read")
        .desc("Total read bandwidth from this memory (bytes/s)")
        .precision(0)
        .prereq(bytesRead)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        bwRead.subname(i, system()->getMasterName(i));
    }

    bwInstRead
        .name(name() + ".bw_inst_read")
        .desc("Instruction read bandwidth from this memory (bytes/s)")
        .precision(0)
        .prereq(bytesInstRead)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        bwInstRead.subname(i, system()->getMasterName(i));
    }
    bwWrite
        .name(name() + ".bw_write")
        .desc("Write bandwidth from this memory (bytes/s)")
        .precision(0)
        .prereq(bytesWritten)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        bwWrite.subname(i, system()->getMasterName(i));
    }
    bwTotal
        .name(name() + ".bw_total")
        .desc("Total bandwidth to/from this memory (bytes/s)")
        .precision(0)
        .prereq(bwTotal)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        bwTotal.subname(i, system()->getMasterName(i));
    }
    bwRead = bytesRead / simSeconds;
    bwInstRead = bytesInstRead / simSeconds;
    bwWrite = bytesWritten / simSeconds;
    bwTotal = (bytesRead + bytesWritten) / simSeconds; 
    //xinw added for using trace-begin
    mem_reads_0
	.name(name() + ".mem_reads_0")
	.desc("Number of memory reads for core 0")
	.flags(total | nonan)
	; 
    mem_reads_1
	.name(name() + ".mem_reads_1")
	.desc("Number of memory reads for core 1")
	.flags(total | nonan)
	; 
    mem_reads_2
	.name(name() + ".mem_reads_2")
	.desc("Number of memory reads for core 2")
	.flags(total | nonan)
	;
    mem_reads_3
	.name(name() + ".mem_reads_3")
	.desc("Number of memory reads for core 3")
	.flags(total | nonan)
	; 
    mem_writes_0
	.name(name() + ".mem_writes_0")
	.desc("Number of memory writes for core 0")
	.flags(total | nonan)
	;
    mem_writes_1
	.name(name() + ".mem_writes_1")
	.desc("Number of memory writes for core 1")
	.flags(total | nonan)
	;
    mem_writes_2
	.name(name() + ".mem_writes_2")
	.desc("Number of memory writes for core 2")
	.flags(total | nonan)
	;
    mem_writes_3
	.name(name() + ".mem_writes_3")
	.desc("Number of memory writes for core 3")
	.flags(total | nonan)
	;
    //xinw added for using trace-end
//xinw added for controlling possibility set by pintool stats-begin
    num_given_up_aes_in_l2
	.name(name() + ".num_given_up_aes_in_l2")
	.desc("Number of given up aes calculations(which is moved to MC)")
	.flags(total | nonan)
	;

    total_data_access_counter
	.name(name() + ".total_data_access_counter")
	.desc("total number of memory accesses")
	.flags(total | nonan)
	; 
    aes_table_hit_num 	
	.name(name() + ".aes_table_hit_num")
	.desc("total number of aes table hits")
	.flags(total | nonan)
	; 
    metadata_miss_during_page_releveling_num_0
	.name(name() + ".metadata_miss_during_page_releveling_num_0")
	.desc("total number of metadata misses during page releveling in level 0")
	.flags(total | nonan)
	; 
    metadata_miss_during_page_releveling_num_1
	.name(name() + ".metadata_miss_during_page_releveling_num_1")
	.desc("total number of metadata misses during page releveling in level 1")
	.flags(total | nonan)
	; 
    metadata_miss_during_page_releveling_num_2
	.name(name() + ".metadata_miss_during_page_releveling_num_2")
	.desc("total number of metadata misses during page releveling in level 2")
	.flags(total | nonan)
	; 
    block_level_dccm_num
	.name(name() + ".block_level_dccm_num")
	.desc("total number of block-level dccm releveling")
	.flags(total | nonan)
	; 
    metadata_miss_for_memory_reads_num_0	
	.name(name() + ".metadata_miss_for_memory_reads_num_0")
	.desc("total number of metadata misses during memory reads for level 0")
	.flags(total | nonan)
	; 
    metadata_miss_for_memory_reads_num_1	
	.name(name() + ".metadata_miss_for_memory_reads_num_1")
	.desc("total number of metadata misses during memory reads for level 1")
	.flags(total | nonan)
	; 
    metadata_miss_for_memory_reads_num_2	
	.name(name() + ".metadata_miss_for_memory_reads_num_2")
	.desc("total number of metadata misses during memory reads for level 2")
	.flags(total | nonan)
	; 
    metadata_miss_for_memory_writes_num_0	
	.name(name() + ".metadata_miss_for_memory_writes_num_0")
	.desc("total number of metadata misses during memory writes for level 0")
	.flags(total | nonan)
	; 
    metadata_miss_for_memory_writes_num_1	
	.name(name() + ".metadata_miss_for_memory_writes_num_1")
	.desc("total number of metadata misses during memory writes for level 1")
	.flags(total | nonan)
	; 
    metadata_miss_for_memory_writes_num_2	
	.name(name() + ".metadata_miss_for_memory_writes_num_2")
	.desc("total number of metadata misses during memory writes for level 2")
	.flags(total | nonan)
	; 
    overflow_event_num_0
	.name(name() + ".overflow_event_num_0")
	.desc("total number of overflow in level 0")
	.flags(total | nonan)
	; 
    overflow_event_num_1
	.name(name() + ".overflow_event_num_1")
	.desc("total number of overflow in level 1")
	.flags(total | nonan)
	; 
    overflow_event_num_2
	.name(name() + ".overflow_event_num_2")
	.desc("total number of overflow in level 2")
	.flags(total | nonan)
	; 
    overflow_event_num_3
	.name(name() + ".overflow_event_num_3")
	.desc("total number of overflow in level 3")
	.flags(total | nonan)
	; 
    evicted_metadata_num
	.name(name() + ".evicted_metadata_num")
	.desc("total number of metadata eviction")
	.flags(total | nonan)
	; 
//xinw added for controlling possibility set by pintool stats-end


    //xinw added for otp table-begin 
    L1_node_hit_stats
	.name(name() + ".L1_node_hit_stats")
	.desc("Number of L1 node hits in metadata cache")
	.flags(total | nonan)
	;
    L1_node_miss_stats
	.name(name() + ".L1_node_miss_stats")
	.desc("Number of L1 node misses in metadata cache")
	.flags(total | nonan)
	;
    L1_node_hit_while_otp_hit_stats
	.name(name() + ".L1_node_hit_while_otp_hit_stats")
	.desc("Number of L1 node hits in metadata cache while otp hit")
	.flags(total | nonan)
	;
    L1_node_miss_while_otp_hit_stats
	.name(name() + ".L1_node_miss_while_otp_hit_stats")
	.desc("Number of L1 node misses in metadata cache while otp hit")
	.flags(total | nonan)
	;

    _otp_table_hit_stats
	.name(name() + ".otp_table_hit_stats")
    	.desc("Number of otp table hits")
        .flags(total | nonan)
	;
     _otp_tables_hit_stats_0
        .name(name() + ".otp_tables_hit_stats_0")
        .desc("Number of otp table hits in table 0")
        .flags(total | nonan)
        ;
    _otp_tables_hit_stats_1
        .name(name() + ".otp_tables_hit_stats_1")
        .desc("Number of otp table hits in table 1")
        .flags(total | nonan)
        ;
    _otp_tables_hit_stats_2
        .name(name() + ".otp_tables_hit_stats_2")
        .desc("Number of otp table hits in table 2")
        .flags(total | nonan)
        ;
    _otp_tables_hit_stats_3
        .name(name() + ".otp_tables_hit_stats_3")
        .desc("Number of otp table hits in table 3")
        .flags(total | nonan)
        ;


    _otp_table_miss_between_largest_groups
  	.name(name() + ".otp_table_miss_between_largest_groups")
	.desc("Number of otp table misses between largest and second largest group")
        .flags(total | nonan)
	;

    _otp_table_miss_stats
	.name(name() + ".otp_table_miss_stats")
	.desc("Number of otp table misses")
        .flags(total | nonan)
	;
     _otp_tables_miss_stats_0
        .name(name() + ".otp_tables_miss_stats_0")
        .desc("Number of otp table misses in table 0")
        .flags(total | nonan)
        ;
    _otp_tables_miss_stats_1
        .name(name() + ".otp_tables_miss_stats_1")
        .desc("Number of otp table misses in table 1")
        .flags(total | nonan)
        ;
    _otp_tables_miss_stats_2
        .name(name() + ".otp_tables_miss_stats_2")
        .desc("Number of otp table misses in table 2")
        .flags(total | nonan)
        ;
    _otp_tables_miss_stats_3
        .name(name() + ".otp_tables_miss_stats_3")
        .desc("Number of otp table misses in table 3")
        .flags(total | nonan)
        ;
	




    _otp_table_miss_with_small_ctr_stats
	.name(name() + ".otp_table_miss_with_small_ctr_stats")
	.desc("Number of otp table misses due to small counter value")
        .flags(total | nonan)
	;
    _otp_table_miss_with_medium_ctr_stats
	.name(name() + ".otp_table_miss_with_medium_ctr_stats")
	.desc("Number of otp table misses due to medium counter value")
        .flags(total | nonan)
	;
    _otp_table_miss_with_big_ctr_stats	
	.name(name() + ".otp_table_miss_with_big_ctr_stats")
	.desc("Number of otp table misses due to big counter value")
        .flags(total  | nonan)
	;
    _otp_table_update_stats
	.name(name() + ".otp_table_update_stats")
	.desc("Number of otp table updates")
        .flags(total | nonan)
	;
    _otp_table_active_relevel_stats
	.name(name() + ".otp_table_active_relevel_stats")
	.desc("Number of otp table active releveling")
        .flags(total | nonan)
	;
    _otp_table_passive_relevel_stats	
	.name(name() + ".otp_table_passive_relevel_stats")
	.desc("Number of otp table passive releveling")
        .flags(total  | nonan)
	;
    //xinw added for otp table-end
    //xinw added for morph block-begin
      _mcr_to_zcc_switches_stats_0
	.name(name() + ".mcr_to_zcc_switches_stats_0")
	.desc("Number of mcr to zcc switches in tree level 0")
	.flags(total | nonan)
	;
    _mcr_to_zcc_switches_stats_1
	.name(name() + ".mcr_to_zcc_switches_stats_1")
	.desc("Number of mcr to zcc switches in tree level 1")
	.flags(total | nonan)
	;
    _mcr_to_zcc_switches_stats_2
	.name(name() + ".mcr_to_zcc_switches_stats_2")
	.desc("Number of mcr to zcc switches in tree level 2")
	.flags(total | nonan)
	;
    _mcr_to_zcc_switches_stats_3
	.name(name() + ".mcr_to_zcc_switches_stats_3")
	.desc("Number of mcr to zcc switches in tree level 3")
	.flags(total | nonan)
	;
    _overflow_due_to_releveling_stats
	.name(name() + ".overflow_due_to_releveling_stats")
	.desc("zcc switch to mcr due to releveling")
	.flags(total | nonan)
	;
    _zcc_to_mcr_switches_stats_0
	.name(name() + ".zcc_to_mcr_switches_stats_0")
	.desc("Number of zcc to mcr switches in tree level 0")
	.flags(total | nonan)
	;
    _zcc_to_mcr_switches_stats_1	
	.name(name() + ".zcc_to_mcr_switches_stats_1")
	.desc("Number of zcc to mcr switches in tree level 1")
	.flags(total | nonan)
	;
    _zcc_to_mcr_switches_stats_2	
	.name(name() + ".zcc_to_mcr_switches_stats_2")
	.desc("Number of zcc to mcr switches in tree level 2")
	.flags(total | nonan)
	;
    _zcc_to_mcr_switches_stats_3	
	.name(name() + ".zcc_to_mcr_switches_stats_3")
	.desc("Number of zcc to mcr switches in tree level 3")
	.flags(total | nonan)
	;
    _zcc_counter_overflows_stats_0
	.name(name() + ".zcc_counter_overflows_stats_0")
	.desc("Number of overflows within zcc mode in tree level 0")
	.flags(total | nonan)
	;
    _zcc_counter_overflows_stats_1	
	.name(name() + ".zcc_counter_overflows_stats_1")
	.desc("Number of overflows within zcc mode in tree level 1")
	.flags(total | nonan)
	;
    _zcc_counter_overflows_stats_2	
	.name(name() + ".zcc_counter_overflows_stats_2")
	.desc("Number of overflows within zcc mode in tree level 2")
	.flags(total | nonan)
	;
    _zcc_counter_overflows_stats_3	
	.name(name() + ".zcc_counter_overflows_stats_3")
	.desc("Number of overflows within zcc mode in tree level 3")
	.flags(total | nonan)
	;
    _mcr_counter_overflows_stats_0
	.name(name() + ".mcr_counter_overflows_stats_0")
	.desc("Number of overflows within mcr mode in tree level 0")
	.flags(total | nonan)
	;
    _mcr_counter_overflows_stats_1
	.name(name() + ".mcr_counter_overflows_stats_1")
	.desc("Number of overflows within mcr mode in tree level 1")
	.flags(total | nonan)
	;
    _mcr_counter_overflows_stats_2
	.name(name() + ".mcr_counter_overflows_stats_2")
	.desc("Number of overflows within mcr mode in tree level 2")
	.flags(total | nonan)
	;
    _mcr_counter_overflows_stats_3
	.name(name() + ".mcr_counter_overflows_stats_3")
	.desc("Number of overflows within mcr mode in tree level 3")
	.flags(total | nonan)
	;
 
    //xinw added for morph block-end

    //xinw added for wc cache-begin
    _wc_cache_hits_stats
	.name(name() + ".wc_cache_hits_stats")
	.desc("Number of wc cache hits")
	.flags(total | nonan)
	;
    _wc_cache_fetches_stats
	.name(name() + ".wc_cache_fetches_stats")
	.desc("Number of wc cache fetches")
	.flags(total | nonan)
	;
    _wccache_exist_but_miss_stats
	.name(name() + ".wc_cache_exist_but_miss_stats")
	.desc("Number of wc cache exist but miss")
	.flags(total | nonan)
	;
    _wc_cache_write_stats
	.name(name() + ".wc_cache_write_stats")
	.desc("Number of wc cache write")
	.flags(total | nonan)
	;
 
 
    _counters_fetches_stats_0
	.name(name() + ".counters_fetches_stats_0")
	.desc("Number of counter fetches in tree level 0")
	.flags(total | nonan)
	;
    _counters_fetches_stats_1
	.name(name() + ".counters_fetches_stats_1")
	.desc("Number of counter fetches in tree level 1")
	.flags(total | nonan)
	;
    _counters_fetches_stats_2
	.name(name() + ".counters_fetches_stats_2")
	.desc("Number of counter fetches in tree level 2")
	.flags(total | nonan)
	;
    _counters_access_stats_0
	.name(name() + ".counters_access_stats_0")
	.desc("Number of counter access in tree level 0")
	.flags(total | nonan)
	;
    _counters_access_stats_1
	.name(name() + ".counters_access_stats_1")
	.desc("Number of counter access in tree level 1")
	.flags(total | nonan)
	;
    _counters_access_stats_2
	.name(name() + ".counters_access_stats_2")
	.desc("Number of counter access in tree level 2")
	.flags(total | nonan)
	;
    _counters_access_stats_3
	.name(name() + ".counters_access_stats_3")
	.desc("Number of counter access in tree level 3")
	.flags(total | nonan)
	;
    _counters_access_read_stats_0
	.name(name() + ".counters_access_read_stats_0")
	.desc("Number of counter access in tree level 0")
	.flags(total | nonan)
	;
    _counters_access_read_stats_1
	.name(name() + ".counters_access_read_stats_1")
	.desc("Number of counter access in tree level 1")
	.flags(total | nonan)
	;
    _counters_access_read_stats_2
	.name(name() + ".counters_access_read_stats_2")
	.desc("Number of counter access in tree level 2")
	.flags(total | nonan)
	;
    _counters_access_read_stats_3
	.name(name() + ".counters_access_read_stats_3")
	.desc("Number of counter access in tree level 3")
	.flags(total | nonan)
	;
    _counters_access_write_stats_0
	.name(name() + ".counters_access_write_stats_0")
	.desc("Number of counter access in tree level 0")
	.flags(total | nonan)
	;
    _counters_access_write_stats_1
	.name(name() + ".counters_access_write_stats_1")
	.desc("Number of counter access in tree level 1")
	.flags(total | nonan)
	;
    _counters_access_write_stats_2
	.name(name() + ".counters_access_write_stats_2")
	.desc("Number of counter access in tree level 2")
	.flags(total | nonan)
	;
    _counters_access_write_stats_3
	.name(name() + ".counters_access_write_stats_3")
	.desc("Number of counter access in tree level 3")
	.flags(total | nonan)
	;
   

    _memory_writebacks_stats
	.name(name() + ".memory_writebacks_stats")
	.desc("Number of memory writebacks")
	.flags(total | nonan)
	;
    _memory_writebacks_while_releveling_total
	.name(name() + ".memory_writebacks_while_releveling_total")
	.desc("Number of memory writebacks while releveling")
	.flags(total | nonan)
	;

    //xinw added for wc cache-end
  //xinw added for warning statistics-begin
    _last_access_tick
	.name(name() + ".ticks_passed_before_last_access")
	.desc("number of ticks passed from beginning of detailed simulation to the last access")
	.flags(total | nonan)
	;
    version_block_miss_not_expected
	.name(name() + ".version_block_miss_not_expected")
	.desc("frequency of scenarios that version block misses after data block arrives but previously hits while sending data block requests")
	.flags(total | nonan)
	; 
    stall_due_to_write_overhead_queue_full
	.name(name() + ".stall_due_to_write_overhead_queue_full")
	.desc("number of stalls due to write overhead queue is full")
	.flags(total | nonan)
	;
    stall_due_to_ramulator_read_queue_full
	.name(name() + ".stall_due_to_ramulator_read_queue_full")
	.desc("number of stalls due to no enough space in ramulaotr's read queue")
	.flags(total | nonan)
	;
    stall_for_more_than_5_microseconds
	.name(name() + ".stall_for_more_than_5_microseconds")
	.desc("number of stalls with more than 5 microseconds")
	.flags(total | nonan)
    	;
    waiting_ticks_for_version
	.name(name() + ".waiting_ticks_for_version")
	.desc("ticks for waiting version blocks after data block arrives")
	.flags(total | nonan)
	;
    exceeded_dram_verfication_latency_for_version_longer_than_aes_latency
	.name(name() + ".exceeded_dram_verfication_latency_for_version_longer_than_aes_latency")
	.desc("exceeded ticks for version block access plus finite field calculation latency which is higher than aes latency ")
	.flags(total | nonan)
	; 
    dram_latency_for_version
	.name(name() + ".dram_latency_for_version")
	.desc("ticks for version block access in ramulator backend")
	.flags(total | nonan)
	; 
    dram_latency_for_data
	.name(name() + ".dram_latency_for_data")
	.desc("ticks for normal data block access in ramulator backend")
	.flags(total | nonan)
	;
    dram_read_for_version
	.name(name() + ".dram_read_for_version")
	.desc("number for version block access in ramulator backend")
	.flags(total | nonan)
	; 
    dram_read_for_data
	.name(name() + ".dram_read_for_data")
	.desc("number for normal data block access in ramulator backend")
	.flags(total | nonan)
	;

    dram_avg_latency_for_version
	.name(name() + ".dram_avg_latency_for_version")
	.desc("average dram access latency for version blocks")
	.flags(total | nonan)
	;
    dram_avg_latency_for_version  = dram_latency_for_version/dram_read_for_version;

    dram_avg_latency_for_data
	.name(name() + ".dram_avg_latency_for_data")
	.desc("average dram access latency for data blocks")
	.flags(total | nonan)
	;
    dram_avg_latency_for_data  = dram_latency_for_data/dram_read_for_data;

    num_of_prefetch_request_consuming_private_metadata_cache_miss_budget
	.name(name() + ".num_of_prefetch_request_consuming_private_metadata_cache_miss_budget")
	.desc("number for prefetch request consuming private metadata cache miss budget")
	.flags(total | nonan)
	;

    num_of_prefetch_request_consuming_LLC_metadata_miss_budget
	.name(name() + ".num_of_prefetch_request_consuming_LLC_metadata_miss_budget")
	.desc("number for prefetch request consuming LLC metadata miss budget")
	.flags(total | nonan)
	;



  //xinw added for warning statistics-begin
  //xinw added for warning statistics-end
  //xinw added for morphable-begin
    wc_cache_occupancy
        .name(name() + ".wc_cache_occupancy")
        .desc("occupancy of write counter cache")
        .flags(total | nozero | nonan)
        ;

    metabytesRead
        .name(name() + ".metabytes_read")
        .desc("Number of metadata bytes read from this memory")
        .flags(total | nonan)
        ;
    metabytesWritten
        .name(name() + ".metabytes_written")
        .desc("Number of metadata bytes written to this memory")
        .flags(total  | nonan)
        ;
    
    metabytesWrittenlevel0
        .name(name() + ".metabytes_written_level0")
        .desc("Number of level0 metadata bytes traffic from this memory")
        .flags(total  | nonan)
        ;
    metabytesWrittenlevel1
        .name(name() + ".metabytes_written_level1")
        .desc("Number of level1 metadata bytes traffic from this memory")
        .flags(total | nonan)
        ;
    metabytesWrittenlevel2
        .name(name() + ".metabytes_written_level2")
        .desc("Number of level2 metadata bytes traffic from this memory")
        .flags(total | nonan)
        ;
    metabytesWrittenlevel3
        .name(name() + ".metabytes_written_level3")
        .desc("Number of level3 metadata bytes traffic from this memory")
        .flags(total  | nonan)
        ;
    metabytesWrittenoverflow
        .name(name() + ".metabytes_written_overflow")
        .desc("Number of overflow  metadata bytes traffic from this memory")
        .flags(total  | nonan)
        ;
    metabytesWrittenoverflowfordata
        .name(name() + ".metabytes_written_overflow_fordata")
        .desc("Number of overflow  metadata bytes traffic from this memory for data")
        .flags(total  | nonan)
        ;
    metabytesWrittenoverflowformeta_0
        .name(name() + ".metabytes_written_overflow_formeta_0")
        .desc("Number of overflow  metadata bytes traffic from this memory for meta in level 0")
        .flags(total  | nonan)
        ;

    metabytesReadlevel0
        .name(name() + ".metabytes_read_level0")
        .desc("Number of level0 metadata bytes traffic from this memory")
        .flags(total | nonan)
        ;
    metabytesReadlevel1
        .name(name() + ".metabytes_read_level1")
        .desc("Number of level1 metadata bytes traffic from this memory")
        .flags(total | nonan)
        ;
    metabytesReadlevel2
        .name(name() + ".metabytes_read_level2")
        .desc("Number of level2 metadata bytes traffic from this memory")
        .flags(total | nonan)
        ;
    metabytesReadlevel3
        .name(name() + ".metabytes_read_level3")
        .desc("Number of level3 metadata bytes traffic from this memory")
        .flags(total | nonan)
        ;
    metabytesReadoverflow
        .name(name() + ".metabytes_read_overflow")
        .desc("Number of overflow bytes traffic from this memory")
        .flags(total | nonan)
        ;
    metabytesReadoverflowfordata
        .name(name() + ".metabytes_read_overflow_fordata")
        .desc("Number of overflow bytes traffic from this memory for data")
        .flags(total | nonan)
        ;
    metabytesReadoverflowformeta_0
        .name(name() + ".metabytes_read_overflow_formeta_0")
        .desc("Number of overflow bytes traffic from this memory for meta in level 0")
        .flags(total | nonan)
        ;



    metabwRead
        .name(name() + ".metabw_read")
        .desc("Total metadata read bandwidth from this memory (bytes/s)")
        .flags(total | nozero | nonan)
        ;
  //  for (int i = 0; i < system()->maxMasters(); i++) {
  //      metabwRead.subname(i, system()->getMasterName(i));
  //  }
    metabwWrite
        .name(name() + ".metabw_write")
        .desc("metadata Write bandwidth from this memory (bytes/s)")
        .flags(total | nozero | nonan)
        ;
 //   for (int i = 0; i < system()->maxMasters(); i++) {
 //       metabwWrite.subname(i, system()->getMasterName(i));
 //   }
    metabwTotal
        .name(name() + ".metabw_total")
        .desc("Total metadata bandwidth to/from this memory (bytes/s)")
        .flags(total | nozero | nonan)
        ;
  //  for (int i = 0; i < system()->maxMasters(); i++) {
  //      metabwTotal.subname(i, system()->getMasterName(i));
  //  }
    metabwRead = metabytesRead / simSeconds;
    metabwWrite = metabytesWritten / simSeconds;
    metabwTotal = (metabytesRead + metabytesWritten) / simSeconds; 
   mc_blocks
	.name(name() + ".mc_blocks")
	.desc("Number of metadata blocks increase in this period")
	.flags(total | nozero | nonan)
	; 
   mc_blocks_occ
	.name(name() + ".mc_blocks_occ")
	.desc("Number of metadata blocks occupancy increase in this period")
	.flags(total | nozero | nonan)
	;  
  mc_blocks_occ = mc_blocks/2048;
   mc_hits
	.name(name() + ".mc_hits")
	.desc("Number of metadata cache hits")
	.flags(total | nozero | nonan)
	;  
   mc_misses
	.name(name() + ".mc_misses")
	.desc("Number of metadata cache misses")
	.flags(total | nozero | nonan)
	;  
   mc_hit_rate
	.name(name() + ".mc_hit_rate")
	.desc("Number of metadata cache hit rate")
	.flags(total | nozero | nonan)
	; 
   mc_hit_rate = mc_hits/(mc_hits + mc_misses);
 //xinw added for morphable-end
}

AddrRange
AbstractMemory::getAddrRange() const
{
    return range;
}

// Add load-locked to tracking list.  Should only be called if the
// operation is a load and the LLSC flag is set.
void
AbstractMemory::trackLoadLocked(PacketPtr pkt)
{
    const RequestPtr &req = pkt->req;
    Addr paddr = LockedAddr::mask(req->getPaddr());

    // first we check if we already have a locked addr for this
    // xc.  Since each xc only gets one, we just update the
    // existing record with the new address.
    list<LockedAddr>::iterator i;

    for (i = lockedAddrList.begin(); i != lockedAddrList.end(); ++i) {
        if (i->matchesContext(req)) {
            DPRINTF(LLSC, "Modifying lock record: context %d addr %#x\n",
                    req->contextId(), paddr);
            i->addr = paddr;
            return;
        }
    }

    // no record for this xc: need to allocate a new one
    DPRINTF(LLSC, "Adding lock record: context %d addr %#x\n",
            req->contextId(), paddr);
    lockedAddrList.push_front(LockedAddr(req));
}


// Called on *writes* only... both regular stores and
// store-conditional operations.  Check for conventional stores which
// conflict with locked addresses, and for success/failure of store
// conditionals.
bool
AbstractMemory::checkLockedAddrList(PacketPtr pkt)
{
    const RequestPtr &req = pkt->req;
    Addr paddr = LockedAddr::mask(req->getPaddr());
    bool isLLSC = pkt->isLLSC();

    // Initialize return value.  Non-conditional stores always
    // succeed.  Assume conditional stores will fail until proven
    // otherwise.
    bool allowStore = !isLLSC;

    // Iterate over list.  Note that there could be multiple matching records,
    // as more than one context could have done a load locked to this location.
    // Only remove records when we succeed in finding a record for (xc, addr);
    // then, remove all records with this address.  Failed store-conditionals do
    // not blow unrelated reservations.
    list<LockedAddr>::iterator i = lockedAddrList.begin();

    if (isLLSC) {
        while (i != lockedAddrList.end()) {
            if (i->addr == paddr && i->matchesContext(req)) {
                // it's a store conditional, and as far as the memory system can
                // tell, the requesting context's lock is still valid.
                DPRINTF(LLSC, "StCond success: context %d addr %#x\n",
                        req->contextId(), paddr);
                allowStore = true;
                break;
            }
            // If we didn't find a match, keep searching!  Someone else may well
            // have a reservation on this line here but we may find ours in just
            // a little while.
            i++;
        }
        req->setExtraData(allowStore ? 1 : 0);
    }
    // LLSCs that succeeded AND non-LLSC stores both fall into here:
    if (allowStore) {
        // We write address paddr.  However, there may be several entries with a
        // reservation on this address (for other contextIds) and they must all
        // be removed.
        i = lockedAddrList.begin();
        while (i != lockedAddrList.end()) {
            if (i->addr == paddr) {
                DPRINTF(LLSC, "Erasing lock record: context %d addr %#x\n",
                        i->contextId, paddr);
                ContextID owner_cid = i->contextId;
                ContextID requester_cid = pkt->req->contextId();
                if (owner_cid != requester_cid) {
                    ThreadContext* ctx = system()->getThreadContext(owner_cid);
                    TheISA::globalClearExclusive(ctx);
                }
                i = lockedAddrList.erase(i);
            } else {
                i++;
            }
        }
    }

    return allowStore;
}


#if TRACING_ON

#define CASE(A, T)                                                        \
  case sizeof(T):                                                         \
    DPRINTF(MemoryAccess,"%s from %s of size %i on address 0x%x data " \
            "0x%x %c\n", A, system()->getMasterName(pkt->req->masterId()),\
            pkt->getSize(), pkt->getAddr(), pkt->get<T>(),                \
            pkt->req->isUncacheable() ? 'U' : 'C');                       \
  break


#define TRACE_PACKET(A)                                                 \
    do {                                                                \
        switch (pkt->getSize()) {                                       \
          CASE(A, uint64_t);                                            \
          CASE(A, uint32_t);                                            \
          CASE(A, uint16_t);                                            \
          CASE(A, uint8_t);                                             \
          default:                                                      \
            DPRINTF(MemoryAccess, "%s from %s of size %i on address 0x%x %c\n",\
                    A, system()->getMasterName(pkt->req->masterId()),          \
                    pkt->getSize(), pkt->getAddr(),                            \
                    pkt->req->isUncacheable() ? 'U' : 'C');                    \
            DDUMP(MemoryAccess, pkt->getConstPtr<uint8_t>(), pkt->getSize());  \
        }                                                                      \
    } while (0)

#else

#define TRACE_PACKET(A)

#endif

void
AbstractMemory::access(PacketPtr pkt)
{
    if (pkt->cacheResponding()) {
        DPRINTF(MemoryAccess, "Cache responding to %#llx: not responding\n",
                pkt->getAddr());
        return;
    }

    if (pkt->cmd == MemCmd::CleanEvict || pkt->cmd == MemCmd::WritebackClean) {
        DPRINTF(MemoryAccess, "CleanEvict  on 0x%x: not responding\n",
                pkt->getAddr());
      return;
    }

    assert(AddrRange(pkt->getAddr(),
                     pkt->getAddr() + (pkt->getSize() - 1)).isSubset(range));

    uint8_t *hostAddr = pmemAddr + pkt->getAddr() - range.start();

    if (pkt->cmd == MemCmd::SwapReq) {
        if (pkt->isAtomicOp()) {
            if (pmemAddr) {
                memcpy(pkt->getPtr<uint8_t>(), hostAddr, pkt->getSize());
                (*(pkt->getAtomicOp()))(hostAddr);
            }
        } else {
            std::vector<uint8_t> overwrite_val(pkt->getSize());
            uint64_t condition_val64;
            uint32_t condition_val32;

            if (!pmemAddr)
                panic("Swap only works if there is real memory (i.e. null=False)");

            bool overwrite_mem = true;
            // keep a copy of our possible write value, and copy what is at the
            // memory address into the packet
            std::memcpy(&overwrite_val[0], pkt->getConstPtr<uint8_t>(),
                        pkt->getSize());
            std::memcpy(pkt->getPtr<uint8_t>(), hostAddr, pkt->getSize());

            if (pkt->req->isCondSwap()) {
                if (pkt->getSize() == sizeof(uint64_t)) {
                    condition_val64 = pkt->req->getExtraData();
                    overwrite_mem = !std::memcmp(&condition_val64, hostAddr,
                                                 sizeof(uint64_t));
                } else if (pkt->getSize() == sizeof(uint32_t)) {
                    condition_val32 = (uint32_t)pkt->req->getExtraData();
                    overwrite_mem = !std::memcmp(&condition_val32, hostAddr,
                                                 sizeof(uint32_t));
                } else
                    panic("Invalid size for conditional read/write\n");
            }

            if (overwrite_mem)
                std::memcpy(hostAddr, &overwrite_val[0], pkt->getSize());

            assert(!pkt->req->isInstFetch());
            TRACE_PACKET("Read/Write");
            numOther[pkt->req->masterId()]++;
        }
    } else if (pkt->isRead()) {
        assert(!pkt->isWrite());
        if (pkt->isLLSC()) {
            assert(!pkt->fromCache());
            // if the packet is not coming from a cache then we have
            // to do the LL/SC tracking here
            trackLoadLocked(pkt);
        }
        if (pmemAddr)
            memcpy(pkt->getPtr<uint8_t>(), hostAddr, pkt->getSize());
        TRACE_PACKET(pkt->req->isInstFetch() ? "IFetch" : "Read");
        numReads[pkt->req->masterId()]++;
        bytesRead[pkt->req->masterId()] += pkt->getSize();
        if (pkt->req->isInstFetch())
            bytesInstRead[pkt->req->masterId()] += pkt->getSize();
    } else if (pkt->isInvalidate() || pkt->isClean()) {
        assert(!pkt->isWrite());
        // in a fastmem system invalidating and/or cleaning packets
        // can be seen due to cache maintenance requests

        // no need to do anything
    } else if (pkt->isWrite()) {
        if (writeOK(pkt)) {
            if (pmemAddr) {
                memcpy(hostAddr, pkt->getConstPtr<uint8_t>(), pkt->getSize());
                DPRINTF(MemoryAccess, "%s wrote %i bytes to address %x\n",
                        __func__, pkt->getSize(), pkt->getAddr());
            }
            assert(!pkt->req->isInstFetch());
            TRACE_PACKET("Write");
            numWrites[pkt->req->masterId()]++;
            bytesWritten[pkt->req->masterId()] += pkt->getSize();
        }
    } else {
        panic("Unexpected packet %s", pkt->print());
    }

    if (pkt->needsResponse()) {
        pkt->makeResponse();
    }
}

void
AbstractMemory::functionalAccess(PacketPtr pkt)
{
    assert(AddrRange(pkt->getAddr(),
                     pkt->getAddr() + pkt->getSize() - 1).isSubset(range));

    uint8_t *hostAddr = pmemAddr + pkt->getAddr() - range.start();

    if (pkt->isRead()) {
        if (pmemAddr)
            memcpy(pkt->getPtr<uint8_t>(), hostAddr, pkt->getSize());
        TRACE_PACKET("Read");
        pkt->makeResponse();
    } else if (pkt->isWrite()) {
        if (pmemAddr)
            memcpy(hostAddr, pkt->getConstPtr<uint8_t>(), pkt->getSize());
        TRACE_PACKET("Write");
        pkt->makeResponse();
    } else if (pkt->isPrint()) {
        Packet::PrintReqState *prs =
            dynamic_cast<Packet::PrintReqState*>(pkt->senderState);
        assert(prs);
        // Need to call printLabels() explicitly since we're not going
        // through printObj().
        prs->printLabels();
        // Right now we just print the single byte at the specified address.
        ccprintf(prs->os, "%s%#x\n", prs->curPrefix(), *hostAddr);
    } else {
        panic("AbstractMemory: unimplemented functional command %s",
              pkt->cmdString());
    }
}
