# Copyright (c) 2012-2013, 2015-2016 ARM Limited
# All rights reserved
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2010 Advanced Micro Devices, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Lisa Hsu

# Configure the M5 cache hierarchy config in one place
#

import m5
from m5.objects import *
from Caches import *

def config_cache(options, system):
    if options.external_memory_system and (options.caches or options.l2cache):
        print "External caches and internal caches are exclusive options.\n"
        sys.exit(1)

    if options.external_memory_system:
        ExternalCache = ExternalCacheFactory(options.external_memory_system)

    if options.cpu_type == "O3_ARM_v7a_3":
        try:
            from cores.arm.O3_ARM_v7a import *
        except:
            print "O3_ARM_v7a_3 is unavailable. Did you compile the O3 model?"
            sys.exit(1)

        dcache_class, icache_class, l2_cache_class, walk_cache_class = \
            O3_ARM_v7a_DCache, O3_ARM_v7a_ICache, O3_ARM_v7aL2, \
            O3_ARM_v7aWalkCache
    else:
        # gagan : L3 cache
        #dcache_class, icache_class, l2_cache_class, walk_cache_class = \
        #    L1_DCache, L1_ICache, L2Cache, None
        dcache_class, icache_class, l2_cache_class, walk_cache_class, l3_cache_class = \
            L1_DCache, L1_ICache, L2Cache, None, L3Cache

        if buildEnv['TARGET_ISA'] == 'x86':
            walk_cache_class = PageTableWalkerCache

    # Set the cache line size of the system
    system.cache_line_size = options.cacheline_size

    # If elastic trace generation is enabled, make sure the memory system is
    # minimal so that compute delays do not include memory access latencies.
    # Configure the compulsory L1 caches for the O3CPU, do not configure
    # any more caches.
    if options.l2cache and options.elastic_trace_en:
        fatal("When elastic trace is enabled, do not configure L2 caches.")

    # gagan : commenting out L2 cache code
    #if options.l2cache:
    #    # Provide a clock for the L2 and the L1-to-L2 bus here as they
    #    # are not connected using addTwoLevelCacheHierarchy. Use the
    #    # same clock as the CPUs.
    #    # gagan : configs with double system cycles
    #    # system.l2 = l2_cache_class(clk_domain=system.cpu_clk_domain
    #    system.l2 = l2_cache_class(clk_domain=system.clk_domain,
    #                               size=options.l2_size,
    #                               assoc=options.l2_assoc)
    #
    #    system.tol2bus = L2XBar(clk_domain = system.clk_domain)
    #    system.l2.cpu_side = system.tol2bus.master
    #    system.l2.mem_side = system.membus.slave

    if options.l3cache:
	#xinw added for deciding l3_assoc
	final_l3_assoc = options.l3_assoc
        if options.l3_size == "1792kB":
            final_l3_assoc = 7
        if options.l3_size == "3712kB":
            final_l3_assoc = 29
        if options.l3_size == "7680kB":
            final_l3_assoc = 15
 
        #system.l3 = l3_cache_class(clk_domain=system.cpu_clk_domain,
         #                          size=options.l3_size,
          #                         assoc=options.l3_assoc)
        system.l3 = l3_cache_class(clk_domain=system.cpu_clk_domain,
                                   size=options.l3_size,
                                   assoc=final_l3_assoc)
 
        system.tol3bus = L3XBar(clk_domain=system.clk_domain, width=128)
        system.l3.cpu_side = system.tol3bus.master
        system.l3.mem_side = system.membus.slave

        if options.l3prefetcher:
            system.l3.prefetch_on_access = True
            system.l3.prefetcher = StridePrefetcher()

        if options.l3_rp == "clock":
            system.l3.replacement_policy = SecondChanceRP()
        if options.l3_rp == "brrip":
            system.l3.replacement_policy = BRRIPRP()
        if options.l3_rp == "lru":
            system.l3.replacement_policy = LRURP()
	system.l3.response_latency = options.l3_response_latency
	if options.cpu_clock == "2.7GHz":
	    system.l3.data_latency = 37
	    system.l3.tag_latency = 37
	    system.l3.response_latency = 49
        system.l3.clusivity = "mostly_excl"
	system.l3.offload_aes_to_llc = options.offload_aes_to_llc
    
    # gagan : private L2 cache for each core, common L3 cache
    if options.caches:

        #system.l2 = [l2_cache_class(clk_domain=system.clk_domain,
        #                            size=options.l2_size,
        #                            assoc=options.l2_assoc) for
        #             i in xrange(options.num_cpus)]
        
        #system.tol2bus = [L2XBar(clk_domain=system.clk_domain,
        #                              width=64) for
        #                  i in xrange(options.num_cpus)]

        system.l2 = [l2_cache_class(clk=system.cpu_clk_domain,
                                    size=options.l2_size,
                                    assoc=options.l2_assoc,
                                    clusivity="mostly_excl") for i in xrange(options.num_cpus)]
        system.tol2bus = [L2XBar(clk_domain=system.l2_bus_clk_domain, width=16) for i in xrange(options.num_cpus)]
        
        
        # gagan : each l2 cache conneced to l3 in the loop below
        for i in xrange(options.num_cpus):
            system.l2[i].cpu_side = system.tol2bus[i].master
            system.l2[i].mem_side = system.tol3bus.slave
        
            # gagan : L2 prefetch
            if options.l2taggedprefetcher:
                system.l2[i].prefetch_on_access2 = True
                system.l2[i].prefetcher2 = TaggedPrefetcher()
	    #xinw modified
            if options.l2strideprefetcher or options.l1_l2_prefetchers == 1:
                system.l2[i].prefetch_on_access = True
                system.l2[i].prefetcher = StridePrefetcher()
		system.l2[i].prefetcher.degree = 2
                system.l2[i].prefetcher.thresh_conf = 6

            if options.l2prefetcher:
                system.l2[i].prefetch_on_access = True
                system.l2[i].prefetcher = StridePrefetcher()
                system.l2[i].prefetch_on_access2 = True
                system.l2[i].prefetcher2 = TaggedPrefetcher()
            #xinw added for l2 aes  
            system.l2[i].noc_latency = 2 * options.noc_latency
            system.l2[i].adaptive_l2_aes = options.adaptive_l2_aes
            system.l2[i].l2_address_aes = options.l2_address_aes
            system.l2[i].number_of_aes_units = options.number_of_aes_units
            system.l2[i].aes_latency = options.data_aes_latency
	    system.l2[i].deprioritize_prefetch_aes_in_l2 = options.deprioritize_prefetch_aes_in_l2
            system.l2[i].threshold_for_in_used_units_for_give_up_prefetch_aes = options.threshold_for_in_used_units_for_give_up_prefetch_aes
	    if options.cpu_clock == "2.7GHz":
	        system.l2[i].tag_latency = 46
	        system.l2[i].data_latency = 46
	    if options.predictive_decryption:
		system.l2[i].aes_latency = options.data_aes_latency + options.carry_less_multiplication_latency
    if options.memchecker:
        system.memchecker = MemChecker()

    # gagan : for every CPU
    for i in xrange(options.num_cpus):
        if options.caches:
            # gagan : configs with double system cycles
            # dcache, icache = icache_class(size=options.l1i_size
            icache = icache_class(clk_domain=system.l1_clk_domain,
                                  size=options.l1i_size,
                                  assoc=options.l1i_assoc)
            dcache = dcache_class(clk_domain=system.l1_clk_domain,
                                  size=options.l1d_size,
                                  assoc=options.l1d_assoc)
	    #xinw added
	    dcache.pc = options.pc
            # gagan : icache prefetch
            #icache.prefetch_on_access=True
            #icache.prefetcher=StridePrefetcher()

            # gagan : dcache prefetch
            if options.l1taggedprefetcher:
                dcache.prefetch_on_access2 = True
                dcache.prefetcher2 = TaggedPrefetcher()

	    # xinw modified
            #if options.l1strideprefetcher:
            if options.l1strideprefetcher or options.l1_l2_prefetchers == 1:
                dcache.prefetch_on_access = True
                dcache.prefetcher = StridePrefetcher(degree=2)
		dcache.prefetcher.degree = 1
		dcache.prefetcher.thresh_conf = 6

            if options.l1prefetcher:
                dcache.prefetch_on_access = True
                dcache.prefetcher = StridePrefetcher(degree=2)
                dcache.prefetch_on_access2 = True
                dcache.prefetcher2 = TaggedPrefetcher()

            # If we have a walker cache specified, instantiate two
            # instances here
            if walk_cache_class:
                iwalkcache = walk_cache_class()
                dwalkcache = walk_cache_class()
                # gagan : configs with double system cycles
                iwalkcache.clk_domain = system.clk_domain
                dwalkcache.clk_domain = system.clk_domain
            else:
                iwalkcache = None
                dwalkcache = None

            if options.memchecker:
                dcache_mon = MemCheckerMonitor(warn_only=True)
                dcache_real = dcache

                # Do not pass the memchecker into the constructor of
                # MemCheckerMonitor, as it would create a copy; we require
                # exactly one MemChecker instance.
                dcache_mon.memchecker = system.memchecker

                # Connect monitor
                dcache_mon.mem_side = dcache.cpu_side

                # Let CPU connect to monitors
                dcache = dcache_mon

            # When connecting the caches, the clock is also inherited
            # from the CPU in question
            system.cpu[i].addPrivateSplitL1Caches(icache, dcache,
                                                  iwalkcache, dwalkcache)

            if options.memchecker:
                # The mem_side ports of the caches haven't been connected yet.
                # Make sure connectAllPorts connects the right objects.
                system.cpu[i].dcache = dcache_real
                system.cpu[i].dcache_mon = dcache_mon

        elif options.external_memory_system:
            # These port names are presented to whatever 'external' system
            # gem5 is connecting to.  Its configuration will likely depend
            # on these names.  For simplicity, we would advise configuring
            # it to use this naming scheme; if this isn't possible, change
            # the names below.
            if buildEnv['TARGET_ISA'] in ['x86', 'arm']:
                system.cpu[i].addPrivateSplitL1Caches(
                        ExternalCache("cpu%d.icache" % i),
                        ExternalCache("cpu%d.dcache" % i),
                        ExternalCache("cpu%d.itb_walker_cache" % i),
                        ExternalCache("cpu%d.dtb_walker_cache" % i))
            else:
                system.cpu[i].addPrivateSplitL1Caches(
                        ExternalCache("cpu%d.icache" % i),
                        ExternalCache("cpu%d.dcache" % i))

        system.cpu[i].createInterruptController()
        #if options.l2cache:
        #    system.cpu[i].connectAllPorts(system.tol2bus[i], system.membus)
        if options.l3cache:
            system.cpu[i].connectAllPorts(system.tol2bus[i], system.membus)
        elif options.external_memory_system:
            system.cpu[i].connectUncachedPorts(system.membus)
        else:
            system.cpu[i].connectAllPorts(system.membus)

    return system

# ExternalSlave provides a "port", but when that port connects to a cache,
# the connecting CPU SimObject wants to refer to its "cpu_side".
# The 'ExternalCache' class provides this adaptation by rewriting the name,
# eliminating distracting changes elsewhere in the config code.
class ExternalCache(ExternalSlave):
    def __getattr__(cls, attr):
        if (attr == "cpu_side"):
            attr = "port"
        return super(ExternalSlave, cls).__getattr__(attr)

    def __setattr__(cls, attr, value):
        if (attr == "cpu_side"):
            attr = "port"
        return super(ExternalSlave, cls).__setattr__(attr, value)

def ExternalCacheFactory(port_type):
    def make(name):
        return ExternalCache(port_data=name, port_type=port_type,
                             addr_ranges=[AllMemory])
    return make
