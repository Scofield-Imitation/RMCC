/*
 * Copyright (c) 2012 ARM Limited
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
 *          Andreas Hansson
 */

/**
 * @file
 * AbstractMemory declaration
 */

#ifndef __MEM_ABSTRACT_MEMORY_HH__
#define __MEM_ABSTRACT_MEMORY_HH__

#include "mem/mem_object.hh"
#include "params/AbstractMemory.hh"
#include "sim/stats.hh"


class System;

/**
 * Locked address class that represents a physical address and a
 * context id.
 */
class LockedAddr {

  private:

    // on alpha, minimum LL/SC granularity is 16 bytes, so lower
    // bits need to masked off.
    static const Addr Addr_Mask = 0xf;

  public:

    // locked address
    Addr addr;

    // locking hw context
    const ContextID contextId;

    static Addr mask(Addr paddr) { return (paddr & ~Addr_Mask); }

    // check for matching execution context
    bool matchesContext(const RequestPtr &req) const
    {
        return (contextId == req->contextId());
    }

    LockedAddr(const RequestPtr &req) : addr(mask(req->getPaddr())),
                                        contextId(req->contextId())
    {}

    // constructor for unserialization use
    LockedAddr(Addr _addr, int _cid) : addr(_addr), contextId(_cid)
    {}
};

/**
 * An abstract memory represents a contiguous block of physical
 * memory, with an associated address range, and also provides basic
 * functionality for reading and writing this memory without any
 * timing information. It is a MemObject since any subclass must have
 * at least one slave port.
 */
class AbstractMemory : public MemObject
{
  protected:

    // Address range of this memory
    AddrRange range;

    // Pointer to host memory used to implement this memory
    uint8_t* pmemAddr;

    // Enable specific memories to be reported to the configuration table
    const bool confTableReported;

    // Should the memory appear in the global address map
    const bool inAddrMap;

    // Should KVM map this memory for the guest
    const bool kvmMap;

    std::list<LockedAddr> lockedAddrList;

    // helper function for checkLockedAddrs(): we really want to
    // inline a quick check for an empty locked addr list (hopefully
    // the common case), and do the full list search (if necessary) in
    // this out-of-line function
    bool checkLockedAddrList(PacketPtr pkt);

    // Record the address of a load-locked operation so that we can
    // clear the execution context's lock flag if a matching store is
    // performed
    void trackLoadLocked(PacketPtr pkt);

    // Compare a store address with any locked addresses so we can
    // clear the lock flag appropriately.  Return value set to 'false'
    // if store operation should be suppressed (because it was a
    // conditional store and the address was no longer locked by the
    // requesting execution context), 'true' otherwise.  Note that
    // this method must be called on *all* stores since even
    // non-conditional stores must clear any matching lock addresses.
    bool writeOK(PacketPtr pkt) {
        const RequestPtr &req = pkt->req;
        if (lockedAddrList.empty()) {
            // no locked addrs: nothing to check, store_conditional fails
            bool isLLSC = pkt->isLLSC();
            if (isLLSC) {
                req->setExtraData(0);
            }
            return !isLLSC; // only do write if not an sc
        } else {
            // iterate over list...
            return checkLockedAddrList(pkt);
        }
    }

    /** Number of total bytes read from this memory */
    Stats::Vector bytesRead;
    /** Number of instruction bytes read from this memory */
    Stats::Vector bytesInstRead;
    /** Number of bytes written to this memory */
    Stats::Vector bytesWritten;
    /** Number of read requests */
    Stats::Vector numReads;
    /** Number of write requests */
    Stats::Vector numWrites;
    /** Number of other requests */
    Stats::Vector numOther;
    /** Read bandwidth from this memory */
    Stats::Formula bwRead;
    /** Read bandwidth from this memory */
    Stats::Formula bwInstRead;
    /** Write bandwidth from this memory */
    Stats::Formula bwWrite;
    /** Total bandwidth from this memory */
    Stats::Formula bwTotal;
    //xinw added for using trace-begin
    Stats::Scalar mem_reads_0;
    Stats::Scalar mem_reads_1;
    Stats::Scalar mem_reads_2;
    Stats::Scalar mem_reads_3;
    Stats::Scalar mem_writes_0;
    Stats::Scalar mem_writes_1;
    Stats::Scalar mem_writes_2;
    Stats::Scalar mem_writes_3;
    //xinw added for using trace-end
//xinw added for controlling possibility set by pintool stats-begin
    Stats::Scalar num_given_up_aes_in_l2; 

    Stats::Scalar total_data_access_counter ; 
    Stats::Scalar aes_table_hit_num ; 
    Stats::Scalar metadata_miss_during_page_releveling_num_0; 
    Stats::Scalar metadata_miss_during_page_releveling_num_1; 
    Stats::Scalar metadata_miss_during_page_releveling_num_2; 
    Stats::Scalar block_level_dccm_num ;
    Stats::Scalar metadata_miss_for_memory_reads_num_0;
    Stats::Scalar metadata_miss_for_memory_reads_num_1;
    Stats::Scalar metadata_miss_for_memory_reads_num_2;
    Stats::Scalar metadata_miss_for_memory_writes_num_0;
    Stats::Scalar metadata_miss_for_memory_writes_num_1;
    Stats::Scalar metadata_miss_for_memory_writes_num_2;
    Stats::Scalar overflow_event_num_0;
    Stats::Scalar overflow_event_num_1;
    Stats::Scalar overflow_event_num_2;
    Stats::Scalar overflow_event_num_3;
    Stats::Scalar evicted_metadata_num; 
//xinw added for controlling possibility set by pintool stats-end


   //xinw added for otp table-begin
 
    Stats::Scalar L1_node_hit_stats;
    Stats::Scalar L1_node_miss_stats; 
    Stats::Scalar L1_node_hit_while_otp_hit_stats;
    Stats::Scalar L1_node_miss_while_otp_hit_stats;
    Stats::Scalar _otp_table_hit_stats;
    Stats::Scalar _otp_tables_hit_stats_0;
    Stats::Scalar _otp_tables_hit_stats_1;
    Stats::Scalar _otp_tables_hit_stats_2;
    Stats::Scalar _otp_tables_hit_stats_3;
    Stats::Scalar _otp_table_miss_between_largest_groups;
    Stats::Scalar _otp_table_miss_stats;
    Stats::Scalar _otp_tables_miss_stats_0;
    Stats::Scalar _otp_tables_miss_stats_1;
    Stats::Scalar _otp_tables_miss_stats_2;
    Stats::Scalar _otp_tables_miss_stats_3;


    Stats::Scalar _otp_table_miss_with_small_ctr_stats;
    Stats::Scalar _otp_table_miss_with_medium_ctr_stats;
    Stats::Scalar _otp_table_miss_with_big_ctr_stats;
    Stats::Scalar _otp_table_update_stats;
    Stats::Scalar _otp_table_active_relevel_stats;
    Stats::Scalar _otp_table_passive_relevel_stats;
    //xinw added for otp table-end
    //xinw added for morph block-begin
    Stats::Scalar _overflow_due_to_releveling_stats;

    Stats::Scalar _mcr_to_zcc_switches_stats_0;
    Stats::Scalar _mcr_to_zcc_switches_stats_1;
    Stats::Scalar _mcr_to_zcc_switches_stats_2;
    Stats::Scalar _mcr_to_zcc_switches_stats_3;
    Stats::Scalar _zcc_to_mcr_switches_stats_0;
    Stats::Scalar _zcc_to_mcr_switches_stats_1;
    Stats::Scalar _zcc_to_mcr_switches_stats_2;
    Stats::Scalar _zcc_to_mcr_switches_stats_3;

    Stats::Scalar _zcc_counter_overflows_stats_0;
    Stats::Scalar _zcc_counter_overflows_stats_1;
    Stats::Scalar _zcc_counter_overflows_stats_2;
    Stats::Scalar _zcc_counter_overflows_stats_3;
    Stats::Scalar _mcr_counter_overflows_stats_0;
    Stats::Scalar _mcr_counter_overflows_stats_1;
    Stats::Scalar _mcr_counter_overflows_stats_2;
    Stats::Scalar _mcr_counter_overflows_stats_3;
    //xinw added for morph block-end
    //xinw added for wc cache-begin
    Stats::Scalar _wc_cache_hits_stats;
    Stats::Scalar _wc_cache_fetches_stats;
    Stats::Scalar _wccache_exist_but_miss_stats;
    Stats::Scalar _wc_cache_write_stats;
    Stats::Scalar _counters_access_stats_0;
    Stats::Scalar _counters_access_stats_1;
    Stats::Scalar _counters_access_stats_2;
    Stats::Scalar _counters_access_stats_3;
    Stats::Scalar _counters_access_read_stats_0;
    Stats::Scalar _counters_access_read_stats_1;
    Stats::Scalar _counters_access_read_stats_2;
    Stats::Scalar _counters_access_read_stats_3;
    Stats::Scalar _counters_access_write_stats_0;
    Stats::Scalar _counters_access_write_stats_1;
    Stats::Scalar _counters_access_write_stats_2;
    Stats::Scalar _counters_access_write_stats_3;
 
    Stats::Scalar _counters_fetches_stats_0;
    Stats::Scalar _counters_fetches_stats_1;
    Stats::Scalar _counters_fetches_stats_2;
    Stats::Scalar _memory_writebacks_stats;
    Stats::Scalar _memory_writebacks_while_releveling_total;

    //xinw added for wc cache-end
    //xinw added for warning statistics-begin
    Stats::Scalar _last_access_tick;
    Stats::Scalar version_block_miss_not_expected;
    Stats::Scalar stall_due_to_write_overhead_queue_full;
    Stats::Scalar stall_due_to_ramulator_read_queue_full;
    Stats::Scalar stall_for_more_than_5_microseconds;
    Stats::Scalar  waiting_ticks_for_version;
    Stats::Scalar  exceeded_dram_verfication_latency_for_version_longer_than_aes_latency;
    Stats::Scalar  dram_latency_for_version;
    Stats::Scalar  dram_read_for_version;
    Stats::Scalar  dram_latency_for_data;
    Stats::Scalar  dram_read_for_data;
    Stats::Formula dram_avg_latency_for_version;
    Stats::Formula dram_avg_latency_for_data;
    Stats::Scalar  num_of_prefetch_request_consuming_private_metadata_cache_miss_budget;
    Stats::Scalar  num_of_prefetch_request_consuming_LLC_metadata_miss_budget;	
    //xinw added for warning statistics-end
    //xinw added for morphable-begin
    Stats::Scalar wc_cache_occupancy;//number of total metadata bytes read from this memory
    Stats::Scalar metabytesRead;//number of total metadata bytes read from this memory
    Stats::Scalar metabytesWritten;//number of total metadata bytes read from this memory
    Stats::Scalar metabytesReadlevel0;
    Stats::Scalar metabytesReadlevel1;
    Stats::Scalar metabytesReadlevel2; 
    Stats::Scalar metabytesReadlevel3; 
    Stats::Scalar metabytesReadoverflow; 
    Stats::Scalar metabytesReadoverflowfordata;
    Stats::Scalar metabytesReadoverflowformeta_0;

    Stats::Scalar metabytesWrittenlevel0;
    Stats::Scalar metabytesWrittenlevel1;
    Stats::Scalar metabytesWrittenlevel2;
    Stats::Scalar metabytesWrittenlevel3;
    Stats::Scalar metabytesWrittenoverflow; 
    Stats::Scalar metabytesWrittenoverflowfordata;
    Stats::Scalar metabytesWrittenoverflowformeta_0;


    Stats::Formula metabwRead;
    Stats::Formula metabwWrite;
    Stats::Formula metabwTotal;
public:
    Stats::Scalar mc_blocks;
    Stats::Formula mc_blocks_occ;
    Stats::Scalar mc_hits;
    Stats::Scalar mc_misses;
    Stats::Formula mc_hit_rate;
    void increase_mc_occ();
    //xinw added for morphable-end
	
    /** Pointor to the System object.
     * This is used for getting the number of masters in the system which is
     * needed when registering stats
     */
    System *_system;

  private:

    // Prevent copying
    AbstractMemory(const AbstractMemory&);

    // Prevent assignment
    AbstractMemory& operator=(const AbstractMemory&);

  public:

    typedef AbstractMemoryParams Params;

    AbstractMemory(const Params* p);
    virtual ~AbstractMemory() {}

    /**
     * Initialise this memory.
     */
    void init() override;

    /**
     * See if this is a null memory that should never store data and
     * always return zero.
     *
     * @return true if null
     */
    bool isNull() const { return params()->null; }

    /**
     * Set the host memory backing store to be used by this memory
     * controller.
     *
     * @param pmem_addr Pointer to a segment of host memory
     */
    void setBackingStore(uint8_t* pmem_addr);

    /**
     * Get the list of locked addresses to allow checkpointing.
     */
    const std::list<LockedAddr>& getLockedAddrList() const
    { return lockedAddrList; }

    /**
     * Add a locked address to allow for checkpointing.
     */
    void addLockedAddr(LockedAddr addr) { lockedAddrList.push_back(addr); }

    /** read the system pointer
     * Implemented for completeness with the setter
     * @return pointer to the system object */
    System* system() const { return _system; }

    /** Set the system pointer on this memory
     * This can't be done via a python parameter because the system needs
     * pointers to all the memories and the reverse would create a cycle in the
     * object graph. An init() this is set.
     * @param sys system pointer to set
     */
    void system(System *sys) { _system = sys; }

    const Params *
    params() const
    {
        return dynamic_cast<const Params *>(_params);
    }

    /**
     * Get the address range
     *
     * @return a single contigous address range
     */
    AddrRange getAddrRange() const;

    /**
     * Get the memory size.
     *
     * @return the size of the memory
     */
    uint64_t size() const { return range.size(); }

    /**
     * Get the start address.
     *
     * @return the start address of the memory
     */
    Addr start() const { return range.start(); }

    /**
     *  Should this memory be passed to the kernel and part of the OS
     *  physical memory layout.
     *
     * @return if this memory is reported
     */
    bool isConfReported() const { return confTableReported; }

    /**
     * Some memories are used as shadow memories or should for other
     * reasons not be part of the global address map.
     *
     * @return if this memory is part of the address map
     */
    bool isInAddrMap() const { return inAddrMap; }

    /**
     * When shadow memories are in use, KVM may want to make one or the other,
     * but cannot map both into the guest address space.
     *
     * @return if this memory should be mapped into the KVM guest address space
     */
    bool isKvmMap() const { return kvmMap; }

    /**
     * Perform an untimed memory access and update all the state
     * (e.g. locked addresses) and statistics accordingly. The packet
     * is turned into a response if required.
     *
     * @param pkt Packet performing the access
     */
    void access(PacketPtr pkt);

    /**
     * Perform an untimed memory read or write without changing
     * anything but the memory itself. No stats are affected by this
     * access. In addition to normal accesses this also facilitates
     * print requests.
     *
     * @param pkt Packet performing the access
     */
    void functionalAccess(PacketPtr pkt);

    /**
     * Register Statistics
     */
    void regStats() override;

};

#endif //__MEM_ABSTRACT_MEMORY_HH__
