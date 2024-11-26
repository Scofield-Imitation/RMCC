# Copyright (c) 2012-2013, 2015, 2018 ARM Limited
# All rights reserved.
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
# Copyright (c) 2005-2007 The Regents of The University of Michigan
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
# Authors: Nathan Binkert
#          Andreas Hansson

from m5.params import *
from m5.proxy import *
from MemObject import MemObject
from Prefetcher import BasePrefetcher
from ReplacementPolicies import *
from Tags import *


# Enum for cache clusivity, currently mostly inclusive or mostly
# exclusive.
class Clusivity(Enum): vals = ['mostly_incl', 'mostly_excl']


class BaseCache(MemObject):
    type = 'BaseCache'
    abstract = True
    cxx_header = "mem/cache/base.hh"

    size = Param.MemorySize("Capacity")
    assoc = Param.Unsigned("Associativity")

    tag_latency = Param.Cycles("Tag lookup latency")
    data_latency = Param.Cycles("Data access latency")
    response_latency = Param.Cycles("Latency for the return path on a miss");

    warmup_percentage = Param.Percent(0,
        "Percentage of tags to be touched to warm up the cache")

    max_miss_count = Param.Counter(0,
        "Number of misses to handle before calling exit")

    mshrs = Param.Unsigned("Number of MSHRs (max outstanding requests)")
    demand_mshr_reserve = Param.Unsigned(1, "MSHRs reserved for demand access")
    tgts_per_mshr = Param.Unsigned("Max number of accesses per MSHR")
    write_buffers = Param.Unsigned(8, "Number of write buffers")

    is_read_only = Param.Bool(False, "Is this cache read only (e.g. inst)")

    prefetcher = Param.BasePrefetcher(NULL,"Prefetcher attached to cache")
    prefetch_on_access = Param.Bool(False,
         "Notify the hardware prefetcher on every access (not just misses)")

    prefetcher2 = Param.BasePrefetcher(NULL,"Prefetcher attached to cache")
    prefetch_on_access2 = Param.Bool(False,
         "Notify the hardware prefetcher on every access (not just misses)")


    tags = Param.BaseTags(BaseSetAssoc(), "Tag store")
    replacement_policy = Param.BaseReplacementPolicy(LRURP(),
        "Replacement policy")

    sequential_access = Param.Bool(False,
        "Whether to access tags and data sequentially")

    cpu_side = SlavePort("Upstream port closer to the CPU and/or device")
    mem_side = MasterPort("Downstream port closer to memory")

    addr_ranges = VectorParam.AddrRange([AllMemory],
         "Address range for the CPU-side port (to allow striping)")

    system = Param.System(Parent.any, "System we belong to")

    # Determine if this cache sends out writebacks for clean lines, or
    # simply clean evicts. In cases where a downstream cache is mostly
    # exclusive with respect to this cache (acting as a victim cache),
    # the clean writebacks are essential for performance. In general
    # this should be set to True for anything but the last-level
    # cache.
    writeback_clean = Param.Bool(False, "Writeback clean lines")

    # Control whether this cache should be mostly inclusive or mostly
    # exclusive with respect to upstream caches. The behaviour on a
    # fill is determined accordingly. For a mostly inclusive cache,
    # blocks are allocated on all fill operations. Thus, L1 caches
    # should be set as mostly inclusive even if they have no upstream
    # caches. In the case of a mostly exclusive cache, fills are not
    # allocating unless they came directly from a non-caching source,
    # e.g. a table walker. Additionally, on a hit from an upstream
    # cache a line is dropped for a mostly exclusive cache.
    clusivity = Param.Clusivity('mostly_incl',
                                "Clusivity with upstream cache")
    #xinw added
    pc = Param.Int(0, "address for pc")
    #xinw added for l2 aes
    offload_aes_to_llc = Param.Int(0, "specify whether to offload AES from L2 to LLC")
    deprioritize_prefetch_aes_in_l2 = Param.Int(0, "specify whether to deprioritize prefetch AES")
    threshold_for_in_used_units_for_give_up_prefetch_aes = Param.Int(0, "threshold_for_in_used_units_for_give_up_prefetch_aes")
    noc_latency = Param.Int(32000, "specify NoC latency between L2 and MC")
    adaptive_l2_aes = Param.Int(0, "specify whether to apply adaptive l2 address-based aes")
    l2_address_aes = Param.Int(0, "specify whether to apply l2 address-based aes")
    number_of_aes_units = Param.Int(4, "specify number of aes units for l2 address-based aes")
    aes_latency = Param.Int(35000, "specify aes latency for l2 address-based aes")
class Cache(BaseCache):
    type = 'Cache'
    cxx_header = 'mem/cache/cache.hh'


class NoncoherentCache(BaseCache):
    type = 'NoncoherentCache'
    cxx_header = 'mem/cache/noncoherent_cache.hh'

    # This is typically a last level cache and any clean
    # writebacks would be unnecessary traffic to the main memory.
    writeback_clean = False

