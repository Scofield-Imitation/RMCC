# Copyright (c) 2013, 2017 ARM Limited
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
# Authors: Andreas Sandberg
#          Andreas Hansson

import m5.objects
import inspect
import sys
import HMC
from textwrap import  TextWrapper

# Dictionary of mapping names of real memory controller models to
# classes.
_mem_classes = {}

def is_mem_class(cls):
    """Determine if a class is a memory controller that can be instantiated"""

    # We can't use the normal inspect.isclass because the ParamFactory
    # and ProxyFactory classes have a tendency to confuse it.
    try:
        return issubclass(cls, m5.objects.AbstractMemory) and \
            not cls.abstract
    except TypeError:
        return False

def get(name):
    """Get a memory class from a user provided class name."""

    try:
        mem_class = _mem_classes[name]
        return mem_class
    except KeyError:
        print "%s is not a valid memory controller." % (name,)
        sys.exit(1)

def print_mem_list():
    """Print a list of available memory classes."""

    print "Available memory classes:"
    doc_wrapper = TextWrapper(initial_indent="\t\t", subsequent_indent="\t\t")
    for name, cls in _mem_classes.items():
        print "\t%s" % name

        # Try to extract the class documentation from the class help
        # string.
        doc = inspect.getdoc(cls)
        if doc:
            for line in doc_wrapper.wrap(doc):
                print line

def mem_names():
    """Return a list of valid memory names."""
    return _mem_classes.keys()

# Add all memory controllers in the object hierarchy.
for name, cls in inspect.getmembers(m5.objects, is_mem_class):
    _mem_classes[name] = cls

def create_mem_ctrl(cls, r, i, nbr_mem_ctrls, intlv_bits, intlv_size, options):
    """
    Helper function for creating a single memoy controller from the given
    options.  This function is invoked multiple times in config_mem function
    to create an array of controllers.
    """

    import math
    intlv_low_bit = int(math.log(intlv_size, 2))

    # Use basic hashing for the channel selection, and preferably use
    # the lower tag bits from the last level cache. As we do not know
    # the details of the caches here, make an educated guess. 4 MByte
    # 4-way associative with 64 byte cache lines is 6 offset bits and
    # 14 index bits.
    xor_low_bit = 20

    # Create an instance so we can figure out the address
    # mapping and row-buffer size
    ctrl = cls()

    if issubclass(cls, m5.objects.Ramulator):
	    if not options.ramulator_config:
	    	fatal("--mem-type=ramulator require --ramulator-config option")
	    ctrl.warmupTime = options.real_warm_up
	    ctrl.atomicwarmupTime = options.atomic_warm_up
	    ctrl.config_file = options.ramulator_config
	    ctrl.outdir = m5.options.outdir + "/"
	    print("Ramulator system configuration file = ", options.ramulator_config)
	    ctrl.num_cpus = options.num_cpus
# xinw added-begin
	    ctrl.control_overflow_for_use_trace = options.control_overflow_for_use_trace
	    ctrl.generate_trace = options.generate_trace
	    ctrl.use_trace = options.use_trace
	    ctrl.l2_address_aes = options.l2_address_aes
	    #ctrl.number_of_mc_aes_units = options.number_of_mc_aes_units
	    ctrl.number_of_mc_aes_units = 104 - 20*options.number_of_aes_units
	    ctrl.mc_queue_aes = options.mc_queue_aes
	    ctrl.caching_metadata_in_llc = options.caching_metadata_in_llc
	    #ctrl.trace_name="trace_" + "freq_" + options.cpu_clock +  "_" + options.l3_size + "_" + options.encryption_mechanism + "_l2_aes_" + str(options.l2_address_aes) + "_num_aes_" + str(options.number_of_aes_units) +  "_l3_resp_" + str(options.l3_response_latency)  +  "_prefetch_" + str(options.l1_l2_prefetchers) + "_dccm_" + str(options.predictive_decryption) + "_aes_lat_" + str(options.data_aes_latency) +   "_llc_metadata_access_lat_" + str(options.llc_metadata_access_latency) + "_mac_lat_" + str(options.mac_latency) + "_tree_aes_" + str(options.tree_node_aes_latency) + ".cpt"
	    #ctrl.trace_name="trace_" + "freq_" + options.cpu_clock +  "_" + options.l3_size + "_" + options.encryption_mechanism   +  "_l3_resp_" + str(options.l3_response_latency)  +  "_prefetch_" + str(options.l1_l2_prefetchers) + "_dccm_" + str(options.predictive_decryption) + "_aes_lat_" + str(options.data_aes_latency) +   "_llc_metadata_access_lat_" + str(options.llc_metadata_access_latency) + "_mac_lat_" + str(options.mac_latency) + "_tree_aes_" + str(options.tree_node_aes_latency) + "_debug_overflow_" + str(options.debug_overflow_ratio)  + ".cpt"
	    #ctrl.trace_name="trace_" + "freq_" + options.cpu_clock +  "_" + options.l3_size + "_" + options.encryption_mechanism   +  "_l3_resp_" + str(options.l3_response_latency)  +  "_prefetch_" + str(options.l1_l2_prefetchers) + "_dccm_" + str(options.predictive_decryption) + "_aes_lat_" + str(options.data_aes_latency) +   "_llc_metadata_access_lat_" + str(options.llc_metadata_access_latency) + "_mac_lat_" + str(options.mac_latency) + "_tree_aes_" + str(options.tree_node_aes_latency) + "_debug_overflow_" + str(options.debug_overflow_ratio)  + "_ctr_trace_" + str(options.control_overflow_for_use_trace) +  ".cpt"
	    ctrl.trace_name="trace_" + "freq_" + options.cpu_clock +  "_" + options.l3_size + "_" + options.encryption_mechanism   +  "_l3_resp_" + str(options.l3_response_latency)  +  "_prefetch_" + str(options.l1_l2_prefetchers) + "_dccm_" + str(options.predictive_decryption) + "_aes_lat_" + str(options.data_aes_latency) +   "_llc_metadata_access_lat_" + str(options.llc_metadata_access_latency) + "_mac_lat_" + str(options.mac_latency) + "_tree_aes_" + str(options.tree_node_aes_latency) + "_debug_overflow_" + str(options.debug_overflow_ratio)  + "_ctr_trace_" + str(options.control_overflow_for_use_trace) + "_trans_rate_" + str(options.transfer_rate) +  ".cpt"
	    ctrl.max_write_overhead_queue_size = options.max_write_overhead_queue_size
	    ctrl.threshold_for_enqueuing_overflow_requests=options.threshold_for_enqueuing_overflow_requests
	    ctrl.timeout=options.timeout
	    ctrl.rand_init=options.rand_init
	    ctrl.dccm_overhead_ratio=options.dccm_overhead_ratio
	    #ctrl.aes_latency = options.aes_latency
	    ctrl.data_aes_latency = options.data_aes_latency
	    if options.predictive_decryption:
		ctrl.data_aes_latency = options.data_aes_latency + options.carry_less_multiplication_latency
	    if options.tree_node_aes_latency == 40000:
	    	ctrl.tree_node_aes_latency = options.data_aes_latency
	    else:
		ctrl.tree_node_aes_latency = options.tree_node_aes_latency
	    ctrl.mac_latency = options.mac_latency
	    ctrl.modulo_latency = options.modulo_latency
	    ctrl.metadata_access_latency = options.metadata_access_latency
	    #ctrl.noc_latency = options.noc_latency
	    ctrl.noc_latency = 18000
	    ctrl.cpt_index = options.checkpoint_restore
	    ctrl.checkpoint_dir = options.checkpoint_dir + "/"
	    if options.predictive_decryption == 1:
	    	ctrl.predictive_decryption = True
	    else:
	    	ctrl.predictive_decryption = False
	    if options.predictive_decryption_l1 == 1:
	    	ctrl.predictive_decryption_l1 = True
	    else:
	    	ctrl.predictive_decryption_l1 = False
	    if options.static_mapping == 1:
	    	ctrl.static_mapping = True
	    else:
	    	ctrl.static_mapping = False
	    if options.memory_encryption == 1:
	    	ctrl.memory_encryption = True
	    else:
	    	ctrl.memory_encryption = False
	    if options.mapping_for_inst_and_walker == 1:
	    	ctrl.mapping_for_inst_and_walker = True
	    else:
	    	ctrl.mapping_for_inst_and_walker = False
# xinw added-end
# xinw added pintool stats to use-begin
	    ctrl.offload_aes_to_llc = options.offload_aes_to_llc
	    ctrl.adaptive_l2_aes = options.adaptive_l2_aes
	    ctrl.carry_less_multiplication_latency = options.carry_less_multiplication_latency
	    ctrl.caching_version_in_l2 = options.caching_version_in_l2
	    ctrl.pintool_aes_l1_hit_rate = options.pintool_aes_l1_hit_rate
	    ctrl.pintool_aes_table_hit_rate = options.pintool_aes_table_hit_rate
	    ctrl.pintool_metadata_miss_rate_during_page_releveling_0 = options.pintool_metadata_miss_rate_during_page_releveling_0
	    ctrl.pintool_metadata_miss_rate_during_page_releveling_1 = options.pintool_metadata_miss_rate_during_page_releveling_1
	    ctrl.pintool_metadata_miss_rate_during_page_releveling_2 = options.pintool_metadata_miss_rate_during_page_releveling_2
	    ctrl.pintool_block_level_dccm_ratio = options.pintool_block_level_dccm_ratio
	    ctrl.pintool_metadata_miss_rate_for_memory_reads_0 = options.pintool_metadata_miss_rate_for_memory_reads_0
	    ctrl.pintool_metadata_miss_rate_for_memory_reads_1 = options.pintool_metadata_miss_rate_for_memory_reads_1
	    ctrl.pintool_metadata_miss_rate_for_memory_reads_2 = options.pintool_metadata_miss_rate_for_memory_reads_2
	    ctrl.pintool_metadata_miss_rate_for_memory_writes_0 = options.pintool_metadata_miss_rate_for_memory_writes_0
	    ctrl.pintool_metadata_miss_rate_for_memory_writes_1 = options.pintool_metadata_miss_rate_for_memory_writes_1
	    ctrl.pintool_metadata_miss_rate_for_memory_writes_2 = options.pintool_metadata_miss_rate_for_memory_writes_2
	    #xinw added for caching metadata in LLC -begin
	    ctrl.pintool_llc_metadata_miss_rate_for_memory_reads_0 = options.pintool_llc_metadata_miss_rate_for_memory_reads_0
	    ctrl.pintool_llc_metadata_miss_rate_for_memory_reads_1 = options.pintool_llc_metadata_miss_rate_for_memory_reads_1
	    ctrl.pintool_llc_metadata_miss_rate_for_memory_reads_2 = options.pintool_llc_metadata_miss_rate_for_memory_reads_2
	    ctrl.pintool_llc_metadata_miss_rate_for_memory_writes_0 = options.pintool_llc_metadata_miss_rate_for_memory_writes_0
	    ctrl.pintool_llc_metadata_miss_rate_for_memory_writes_1 = options.pintool_llc_metadata_miss_rate_for_memory_writes_1
	    ctrl.pintool_llc_metadata_miss_rate_for_memory_writes_2 = options.pintool_llc_metadata_miss_rate_for_memory_writes_2
	    ctrl.llc_metadata_access_latency = options.llc_metadata_access_latency
	    #xinw added for caching metadata in LLC -end
	    ctrl.pintool_overflow_events_normalized_to_data_traffic_0 = options.pintool_overflow_events_normalized_to_data_traffic_0
	    if options.use_overflow_stats_from_debug_script == 1:
		ctrl.pintool_overflow_events_normalized_to_data_traffic_0 = options.debug_overflow_ratio
	    ctrl.pintool_overflow_events_normalized_to_data_traffic_1 = options.pintool_overflow_events_normalized_to_data_traffic_1
	    ctrl.pintool_overflow_events_normalized_to_data_traffic_2 = options.pintool_overflow_events_normalized_to_data_traffic_2
	    ctrl.pintool_overflow_events_normalized_to_data_traffic_3 = options.pintool_overflow_events_normalized_to_data_traffic_3
	    ctrl.pintool_evicted_metadata_traffic_normalized_to_data_traffic = options.pintool_evicted_metadata_traffic_normalized_to_data_traffic
# xinw added pintool stats to use-end

    # Only do this for DRAMs
    elif issubclass(cls, m5.objects.DRAMCtrl):
        # Inform each controller how many channels to account
        # for
        ctrl.channels = nbr_mem_ctrls

        # If the channel bits are appearing after the column
        # bits, we need to add the appropriate number of bits
        # for the row buffer size
        if ctrl.addr_mapping.value == 'RoRaBaChCo':
            # This computation only really needs to happen
            # once, but as we rely on having an instance we
            # end up having to repeat it for each and every
            # one
            rowbuffer_size = ctrl.device_rowbuffer_size.value * \
                ctrl.devices_per_rank.value

            intlv_low_bit = int(math.log(rowbuffer_size, 2))

    # We got all we need to configure the appropriate address
    # range
    ctrl.range = m5.objects.AddrRange(r.start, size = r.size(),
                                      intlvHighBit = \
                                          intlv_low_bit + intlv_bits - 1,
                                      xorHighBit = \
                                          xor_low_bit + intlv_bits - 1,
                                      intlvBits = intlv_bits,
                                      intlvMatch = i)
    return ctrl

def config_mem(options, system):
    """
    Create the memory controllers based on the options and attach them.

    If requested, we make a multi-channel configuration of the
    selected memory controller class by creating multiple instances of
    the specific class. The individual controllers have their
    parameters set such that the address range is interleaved between
    them.
    """

    # Mandatory options
    opt_mem_type = options.mem_type
    opt_mem_channels = options.mem_channels

    # Optional options
    opt_tlm_memory = getattr(options, "tlm_memory", None)
    opt_external_memory_system = getattr(options, "external_memory_system",
                                         None)
    opt_elastic_trace_en = getattr(options, "elastic_trace_en", False)
    opt_mem_ranks = getattr(options, "mem_ranks", None)

    if opt_mem_type == "HMC_2500_1x32":
        HMChost = HMC.config_hmc_host_ctrl(options, system)
        HMC.config_hmc_dev(options, system, HMChost.hmc_host)
        subsystem = system.hmc_dev
        xbar = system.hmc_dev.xbar
    else:
        subsystem = system
        xbar = system.membus

    if opt_tlm_memory:
        system.external_memory = m5.objects.ExternalSlave(
            port_type="tlm_slave",
            port_data=opt_tlm_memory,
            port=system.membus.master,
            addr_ranges=system.mem_ranges)
        system.kernel_addr_check = False
        return

    if opt_external_memory_system:
        subsystem.external_memory = m5.objects.ExternalSlave(
            port_type=opt_external_memory_system,
            port_data="init_mem0", port=xbar.master,
            addr_ranges=system.mem_ranges)
        subsystem.kernel_addr_check = False
        return

    nbr_mem_ctrls = opt_mem_channels
    import math
    from m5.util import fatal
    intlv_bits = int(math.log(nbr_mem_ctrls, 2))
    if 2 ** intlv_bits != nbr_mem_ctrls:
        fatal("Number of memory channels must be a power of 2")

    cls = get(opt_mem_type)
    mem_ctrls = []

    if opt_elastic_trace_en and not issubclass(cls, m5.objects.SimpleMemory):
        fatal("When elastic trace is enabled, configure mem-type as "
                "simple-mem.")

    # gagan : dramsim2 ini
    if issubclass(cls, m5.objects.DRAMSim2):
        opt_dramsim2_dev_conf = options.dramsim2_dev_conf
        opt_dramsim2_sys_conf = options.dramsim2_sys_conf
        opt_dramsim2_visoutDir = m5.options.outdir + "/dramsimout"
        opt_warmup_time = options.real_warm_up
        print("DRAMSim2 device configuration file = ", opt_dramsim2_dev_conf)
        print("DRAMSim2 system configuration file = ", opt_dramsim2_sys_conf)
        print("DRAMSim2 out.vis path = ", m5.options.outdir)

    # The default behaviour is to interleave memory channels on 128
    # byte granularity, or cache line granularity if larger than 128
    # byte. This value is based on the locality seen across a large
    # range of workloads.
    intlv_size = max(128, system.cache_line_size.value)

    # For every range (most systems will only have one), create an
    # array of controllers and set their parameters to match their
    # address mapping in the case of a DRAM
    for r in system.mem_ranges:
        for i in xrange(nbr_mem_ctrls):
            mem_ctrl = create_mem_ctrl(cls, r, i, nbr_mem_ctrls, intlv_bits,
                                       intlv_size, options)
            # Set the number of ranks based on the command-line
            # options if it was explicitly set
            if issubclass(cls, m5.objects.DRAMCtrl) and opt_mem_ranks:
                mem_ctrl.ranks_per_channel = opt_mem_ranks

            if opt_elastic_trace_en:
                mem_ctrl.latency = '1ns'
                print "For elastic trace, over-riding Simple Memory " \
                    "latency to 1ns."

            # gagan : dramsim2 ini
            if issubclass(cls, m5.objects.DRAMSim2):
                mem_ctrl.deviceConfigFile = opt_dramsim2_dev_conf
                mem_ctrl.systemConfigFile = opt_dramsim2_sys_conf
                mem_ctrl.outDir = opt_dramsim2_visoutDir
                mem_ctrl.warmupTime = opt_warmup_time
                mem_ctrl.enableDebug=True            

            mem_ctrls.append(mem_ctrl)

    subsystem.mem_ctrls = mem_ctrls

    # Connect the controllers to the membus
    for i in xrange(len(subsystem.mem_ctrls)):
        if opt_mem_type == "HMC_2500_1x32":
            subsystem.mem_ctrls[i].port = xbar[i/4].master
            # Set memory device size. There is an independent controller for
            # each vault. All vaults are same size.
            subsystem.mem_ctrls[i].device_size = options.hmc_dev_vault_size
        else:
            subsystem.mem_ctrls[i].port = xbar.master
