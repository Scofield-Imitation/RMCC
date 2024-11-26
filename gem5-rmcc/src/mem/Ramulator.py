# -*- mode:python -*-
from m5.params import *
from AbstractMemory import *

# A wrapper for Ramulator multi-channel memory controller
class Ramulator(AbstractMemory):
    type = 'Ramulator'
    cxx_header = "mem/ramulator.hh"

    # A single port for now
    port = SlavePort("Slave port")

    config_file = Param.String("", "configuration file")
    num_cpus = Param.Unsigned(1, "Number of cpu")
    #  daz3
    atomicwarmupTime = Param.UInt64(20000000000, "Default warmup time")
    warmupTime = Param.UInt64(20000000000, "Default warmup time")
    # xinw added for morphable-begin
    metadata_cache_size = Param.UInt32(2, "default metadata cache size 128KB")
    overflow_fetch_per_page = Param.UInt64(64, "default number of blocks to fetch for each page 64")
    cpt_index = Param.UInt64(1, "Default index of cpt")
    checkpoint_dir = Param.String("","checkpoint directory")
    outdir = Param.String("","output directory for custom files")
    write_overhead_dequeue_interval = Param.UInt32(1, "the interval between two dequeueing operation when this queue is not full")
    simulationTime = Param.UInt64(20000000000, "Default simulation time")
    possibility_relevel = Param.UInt64(1, "default 100 percent for releveling")
    rand_init= Param.UInt64(1, "default turned for random initializaton of morph tree")
    dccm_overhead_ratio= Param.UInt64(5, "default 5 percent overhead ratio for dccm releveling")
    only_relevel_demand = Param.Bool(True, "default turned off")
    otp_for_address = Param.Bool(True, "default turned off")
    predictive_decryption = Param.Bool(True, "default turned off")
    predictive_decryption_l1 = Param.Bool(True, "default turned off")
    memory_encryption = Param.Bool(True, "default turned off")
    #aes_latency = Param.UInt64(35000, "default latency 35000 pico seconds")
    data_aes_latency = Param.UInt64(35000, "default latency 35000 pico seconds")
    tree_node_aes_latency = Param.UInt64(35000, "default latency 35000 pico seconds")
    mac_latency = Param.UInt64(3000, "default latency 3000 pico seconds")
    modulo_latency = Param.UInt64(1000, "default latency 1000 pico seconds")
    metadata_access_latency = Param.UInt64(2500, "default latency 2500 pico seconds")
    noc_latency = Param.UInt64(14000, "default latency 14000 pico seconds")
    high_level_tree_node_demotion_rate = Param.UInt64(0, "default priority 0")
    aggressive_level = Param.UInt64(0, "default priority 0")
    wait_counter = Param.Bool(True, "default turned on")
    static_mapping = Param.Bool(False, "default turned off")
    mapping_for_inst_and_walker = Param.Bool(False, "default turned off")
    # xinw added for morphable-end
    timeout= Param.UInt64(1000, "default timeout for row in ramulator backend")
    threshold_for_enqueuing_overflow_requests = Param.Float(10000, " default unlimited")
    trace_name= Param.String("trace", "name for trace file")
    control_overflow_for_use_trace = Param.UInt64(0, "default turned off")
    generate_trace = Param.UInt64(0, "default turned off")
    use_trace = Param.UInt64(0, "default turned off")
# xinw added pintool stats to use-begin
    max_write_overhead_queue_size = Param.UInt64(2048, "default 2048")
    l2_address_aes = Param.UInt64(0, "default 0")
    number_of_mc_aes_units = Param.UInt64(1, "default 1")
    mc_queue_aes = Param.UInt64(0, "default off")
    caching_metadata_in_llc = Param.UInt64(1, "default on")
    offload_aes_to_llc = Param.UInt64(0, "specify whether to offload AES to LLC")
    carry_less_multiplication_latency = Param.UInt64(1000, "latency of carry-less multiplication")
    adaptive_l2_aes = Param.UInt64(0, "default turned off")
    caching_version_in_l2 = Param.UInt64(0, "default turned off")
    pintool_aes_l1_hit_rate = Param.Float(0, "default value")
    pintool_aes_table_hit_rate = Param.Float(0, "default value")
    pintool_metadata_miss_rate_during_page_releveling_0 = Param.Float(0, "default value") 
    pintool_metadata_miss_rate_during_page_releveling_1 = Param.Float(0, "default value")
    pintool_metadata_miss_rate_during_page_releveling_2 = Param.Float(0, "default value")
    pintool_block_level_dccm_ratio = Param.Float(0, "default value")
    pintool_metadata_miss_rate_for_memory_reads_0 = Param.Float(0, "default value")
    pintool_metadata_miss_rate_for_memory_reads_1 = Param.Float(0, "default value")
    pintool_metadata_miss_rate_for_memory_reads_2 = Param.Float(0, "default value")
    pintool_metadata_miss_rate_for_memory_writes_0 = Param.Float(0, "default value")
    pintool_metadata_miss_rate_for_memory_writes_1 = Param.Float(0, "default value")
    pintool_metadata_miss_rate_for_memory_writes_2 = Param.Float(0, "default value")
    #xinw added for caching metadata in LLC-begin
    pintool_llc_metadata_miss_rate_for_memory_reads_0 = Param.Float(0, "default value")
    pintool_llc_metadata_miss_rate_for_memory_reads_1 = Param.Float(0, "default value")
    pintool_llc_metadata_miss_rate_for_memory_reads_2 = Param.Float(0, "default value")
    pintool_llc_metadata_miss_rate_for_memory_writes_0 = Param.Float(0, "default value")
    pintool_llc_metadata_miss_rate_for_memory_writes_1 = Param.Float(0, "default value")
    pintool_llc_metadata_miss_rate_for_memory_writes_2 = Param.Float(0, "default value")
    llc_metadata_access_latency = Param.UInt64(11000, "default latency 11000 pico seconds")
    #xinw added for caching metadata in LLC-end
 

    pintool_overflow_events_normalized_to_data_traffic_0 = Param.Float(0, "default value")
    pintool_overflow_events_normalized_to_data_traffic_1 = Param.Float(0, "default value")
    pintool_overflow_events_normalized_to_data_traffic_2 = Param.Float(0, "default value")
    pintool_overflow_events_normalized_to_data_traffic_3 = Param.Float(0, "default value")
    pintool_evicted_metadata_traffic_normalized_to_data_traffic = Param.Float(0, "default value")
# xinw added pintool stats to use-end


