/*BEGIN_LEGAL 
  Intel Open Source License 

  Copyright (c) 2002-2017 Intel Corporation. All rights reserved.
 
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.  Redistributions
  in binary form must reproduce the above copyright notice, this list of
  conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.  Neither the name of
  the Intel Corporation nor the names of its contributors may be used to
  endorse or promote products derived from this software without
  specific prior written permission.
 
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
  ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  END_LEGAL */
/*
 *  This file contains an ISA-portable PIN tool for tracing memory accesses.
 */
//xinw added for generating trace
//#include "test.H"
#include "pin.H"
#include <assert.h>

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <unordered_map>
#include <vector>
#include <list>
#include <math.h>
#include <algorithm>
#include <bitset>
#include <ctime>
#include <cstdlib>
//xinw added for measuring running time of pintool job
#include <time.h>
#include <random>
#include <queue>
//#include <bits/stdc++.h>
PIN_LOCK pinLock;
#define max_num_of_threads 16
#define NUM_OF_THREADS 16
int counter_for_relevel_L1_counter=0;
double possibility_for_updating_L1_while_read=0;
//#define times_of_16g 5
#define times_of_16g 8

int num_billion_insts=0;
double accumulated_data_occupancy=0;
int during_page_zero=0;
int special_debug=0;
int debug_overflow_for_releveling_to_next_group=0;
int debug_counter_increment=0;
#define access_data 0
#define access_inst 1
#define access_page_table 2
#define debug_memoize_l1 0
#define  debug_zcc_inner_switch  0
#define debug_relevel_to_next_group 0
#define debug_part_chain_incrementing 0
#define debug_remain_budget 0
#define debug_block_relevel 0
#define debug_prioritize_overflow_nodes 0
#define debug_prioritize_block_releveling 0
#define debug_budget_consume 0
#define debug_aes_table_update 1
#define debug_aes_table_epoch 0
#define debug_trace_read 0
#define debug_page_zero 0
#define debug_aes_hit 0
#define debug_dccm_counter 0
#define debug_budget 0
#define debug_overflow 0
#define debug_actual_overflow 0
#define debug_metadata_cache 0
#define custom_debug 0
#define custom_debug_wc_insertion 0
UINT64 index_of_billion=1;
UINT64 current_block_addr_access_aes_table=0;
//2482820973259164488
//xinw added

clock_t t1, t2;
//std::default_random_engine generator;
//std::normal_distribution<uint64_t> rand_distribution;
//std::random_device rd; 
//std::mt19937 gen(rd());
//std::normal_distribution<uint64_t> d(1048576, 32768);

uint64_t dcache_access_number=0;
uint64_t last_pc_access_number=0;
int is_first_touch=0;

unsigned rand_seed=0;
UINT64 range_base = (UINT64)1 << 47;//128TB, i.e., kernel space
UINT64 randomConstVal = 660225116055843;
UINT64 rand_base = 0;
UINT64 rand_func()
{
        rand_base = (rand_base + randomConstVal) % 1048576;
        return rand_base;
}
UINT64 rand_func_trace()
{
        rand_base = rand() % (2*1048576);
//	rand_base = (d(gen))% (2*1048576);
        return rand_base;
}

char output_buff[4];
#define FALSE 0
#define TRUE 1
//xinw added for otp precalculation-begin
//#include <array>
#include <map>
using namespace std;
int CACHELINE_OFFSET=0;
int deviation=0;
//#define OTP_PRECALCULATION 0
int deprioritize_tree_nodes_in_metadata_cache=0;
int insert_metadata_into_LLC=0;
int access_inst_and_page_table=0;
#define page_table_begin_addr (0xF000000000000000)
UINT64 get_inst_block_address(UINT64 _program_counter){
	return _program_counter&0xFFFFFFFFFFFFFFC0;
}
UINT64 get_page_table_block_address(UINT64 data_block_address){
	return page_table_begin_addr + ((data_block_address>>24)<<6);
}
int use_sampling_for_inserting_metadata_into_LLC=0;
int use_drrip=0;
int delay_high_level_tree_node_overflow=0;
int dccm_block_level_relevel=0;
int huge_page=0;
int during_write=1;
int is_gap=0;
int mcr_overflow=0;
int invalidate_dirty_version_in_metadata_cache=0;
int far_relevel=0;
int use_page_zero=0;
int generate_tree_nodes=0;
int OTP_PRECALCULATION=0;
int OTP_L1_PRECALCULATION=0;

int BEGIN_WITH_BIG_COUNTER=0;
int flag_variable_detected=0;
int iteration_of_gap_to_begin_with=2;
int current_iteration_of_gap=0;
int current_iteration_of_microbenchmark=0;
UINT64 current_written_block_address=0;
//xinw added for otp precalculation-end
/* CONSTATNS */
// ignore RUN_TYPE (was previously used for LOAD only statistics run) 
static const UINT32 RUN_TYPE = 0; 
static const UINT32 DEBUG_TEST = 0; 

// Masks to get addresses for data cache line/block and wc(metadata) block
static const UINT64 PC_FLOOR_VAL_MASK = 0xFFFFFFFFFFFFFFFF; 
static const UINT64 DATA_BLOCK_FLOOR_ADDR_MASK = 0xFFFFFFFFFFFFFFC0;
static const UINT64 WC_BLOCK_FLOOR_ADDR_MASK = 0xFFFFFFFFFFFFFE00;

// L1 cache parameters: total size = SET_NUM*SET_SIZE*64B/1024B/1kB = 128*8*64B*1kB/1024B = 64kB
//static const INT32 L1_SET_NUM = 1; // Total number of sets
//static const INT32 L1_SET_NUM = 8; // Total number of sets
static const INT32 L1_SET_NUM = 2048; // Total number of sets
static const INT32 L1_SET_BITS = log2(L1_SET_NUM); // log2(SET_NUM)
static const INT32 L1_SET_SIZE = 8; // Set size (number of cache lines in each set)
static const INT32 L1_counter_SET_NUM = 64; // Total number of sets
static const INT32 L1_counter_SET_BITS = log2(L1_SET_NUM); // log2(SET_NUM)
static const INT32 L1_counter_SET_SIZE = 8; // Set size (number of cache lines in each set)


// L2 cache parameters: total size = 1MB
//static const INT32 L2_SET_NUM = 512;
//static const INT32 L2_SET_NUM = 256;
//static const INT32 L2_SET_NUM = 8;
//static const INT32 L2_SET_NUM = 8;
static const INT32 L2_SET_NUM = 2048;
//static const INT32 L2_SET_NUM = 128;
//xinw modified L2 size to 2MB
//xinw modified L2 size to 256KB
//static const INT32 L2_SET_NUM = 1;
//static const INT32 L2_SET_NUM = 256;
//static const INT32 L2_SET_NUM = 512;
static const INT32 L2_SET_BITS = log2(L2_SET_NUM);
//static const INT32 L2_SET_SIZE = 32;
//static const INT32 L2_SET_SIZE = 8;
//static const INT32 L2_SET_SIZE = 11;
static const INT32 L2_SET_SIZE = 16;

static const INT32 DATA_CACHE_BLOCK_SIZE = 64;
static const INT32 DATA_CACHE_BLOCK_BITS = log2(DATA_CACHE_BLOCK_SIZE);

static const INT32 OUT_OF_BOUND = L2_SET_SIZE;

static const UINT32 PC_SAMPLING_SIZE = 8;

// WC cache parameters: total size = 128kB
//xinw modified the WCCACHE size to 32KB-begin
//static const INT32 WCCACHE_SET_NUM = 128;
static const INT32 WCCACHE_SET_NUM = 16;
//static const INT32 WCCACHE_SET_NUM = 1;
//static const INT32 WCCACHE_SET_NUM = 4;
//static const INT32 WCCACHE_SET_NUM = 2;
//xinw modified the WCCACHE size to 32KB-end
static const INT32 WCCACHE_SET_BITS = log2(WCCACHE_SET_NUM);
static const INT32 WCCACHE_SET_SIZE = 32;
//static const INT32 WCCACHE_SET_SIZE = 48;
static const INT32 WCCACHE_BLOCK_SIZE = 64;
static const INT32 WCCACHE_BLOCK_BITS = log2(WCCACHE_BLOCK_SIZE);
static const UINT64 WCCACHE_SET_MASK = WCCACHE_SET_NUM-1;

static const UINT32 CACHE_HIT = 1;
static const UINT32 MISS_NO_EVICT = 0;
static const UINT32 MISS_NON_DIRTY_EVICT = 2;
static const UINT32 MISS_DIRTY_EVICT = 3;

static const UINT32 READ_OP = 0;
static const UINT32 WRITE_OP = 1;
static const UINT32 DIRTY_BY_RELEVEL = 2;

static const UINT32 LOAD_INS = 0;
static const UINT32 STORE_INS = 1;

static const UINT32 NOT_IN_CACHE = 0;
static const UINT32 IN_CACHE = 1;

static const UINT32 SET_NOT_FULL = 0;
static const UINT32 SET_IS_FULL = 1;

static const UINT32 WARMUP_IN_PROGRESS = 0;
static const UINT32 RELEVEL_WARMUP= 1;
static const UINT32 WARMUP_OVER = 2;

static const UINT32 RPOLICY_LRU = 0;
static const UINT32 RPOLICY_DIP = 3;
static const UINT32 RPOLICY_DRRIP = 6;
//xinw modified for quick debugging-begin
//static const UINT64 PRINT_STEP = 1000000000;
//static const UINT64 PRINT_STEP = 50000000000;
static const UINT64 PRINT_STEP = 50000000;
//xinw modified for quick debugging-end

static const UINT32 CRIT_STORE_INS_RANGE = 50;
//static const UINT32 CRIT_OVERLAP_VAL = 50;
static const UINT32 CRIT_OVERLAP_VAL = 0;

static const UINT64 WR_RATIO_GROUP_NUM = 100;
//
// Run parameters
// WARMUP_VAL: total numner of memory instructions for the entire warmup (PC warmup + relevel warmup)
// RELEVEL_WARMUP_VAL: number of memory instructions for relevel warmup (only collect TOTAL stats, not AFTER_WARMUP stats)
// WARMUP_VAL - RELEVEL_WARMUP_VAL = number of memory instruction for PC warmup (find top 100 frequent PCs)
// RUN_VAL: number of memory instructions for actual run (collect AFTER_WARMUP stats)
//static const UINT64 WARMUP_VAL = 25000000000;
//static const UINT64 WARMUP_VAL = 200000000;
//static const UINT64 WARMUP_VAL = 1617000000;
static const UINT64 WARMUP_VAL = 0;
//static const UINT64 WARMUP_VAL = 160000;
//xinw modified for double warmup time
//static const UINT64 WARMUP_VAL = 50000000000;
static const UINT64 RELEVEL_WARMUP_VAL = 0;
//xinw modified for double warmup time
//static const UINT64 RELEVEL_WARMUP_VAL = 40000000000;
//static const UINT64 RUN_VAL = 2300000000;
//static const UINT64 RUN_VAL = 10000000000000;
static const UINT64 RUN_VAL = 10000000000000;
UINT64 RUN_INST_VAL = 1000000000;
//static const UINT64 STEP_VAL = (WARMUP_VAL/50000000000)+(RUN_VAL/50000000000);
static const UINT64 STEP_VAL = (WARMUP_VAL/PRINT_STEP)+(RUN_VAL/PRINT_STEP);
// Run parameters for microbenchmarks
//static const UINT64 WARMUP_VAL = 2000000000;
//static const UINT64 RELEVEL_WARMUP_VAL = 1000000000;
/*
static const UINT64 WARMUP_VAL = 2;
static const UINT64 RELEVEL_WARMUP_VAL = 1;
static const UINT64 RUN_VAL = 5000000000;
static const UINT64 STEP_VAL = (WARMUP_VAL/1000000000)+(RUN_VAL/1000000000);
*/
// Relevel queue/ prediction queue parameters
static const UINT64 WC_RELEVELQ_SIZE = 16;
static const UINT64 WC_PREDICTQ_SIZE = 4;

// Prediction range N:  x+0, x+1, ..., x+N-1
static const UINT32 PREDICTION_RANGE = 3;	
static const UINT32 PREDICTION_DIST_SIZE = PREDICTION_RANGE*2;	

static const UINT32 MOST_FREQUENT_INDEX = 0;	
static const UINT32 MOST_RECENT_INDEX = PREDICTION_RANGE;	

static const INT32 DECRYPT_LATENCY = 100;
static const INT32 EXECUTE_WIDTH = 2;

static const UINT32 LRU_SET = 1;
static const UINT32 BIP_SET = 2;
static const UINT32 DIP_SET = 3;

static const UINT32 SRRIP_SET = 4;
static const UINT32 BRRIP_SET = 5;
static const UINT32 DRRIP_SET = 6;

static const UINT32 BIP_PROBABILITY_VAL = 31;
static const UINT32 BRRIP_PROBABILITY_VAL = 31;

static const UINT32 RRIP_M_BIT = 2;
static const UINT32 RRIP_DISTANT = (1<<2)-1;
static const UINT32 RRIP_LONG = RRIP_DISTANT - 1;

//static const UINT32 RRIP_AGGRESSIVNESS = 1;
//xinw modified for more aggressive level
static const UINT32 RRIP_AGGRESSIVNESS = 8;

// PSEL counter used in DIP/DRRIP replacement policies 
//xinw fixed the bug of '^' here-it is xor actually
//static const UINT32 PSEL_COUNTER_MAX_VAL = (2^11)-1;
static const UINT32 PSEL_COUNTER_MAX_VAL = (1<<11)-1;
static const UINT32 PSEL_COUNTER_INIT_VAL = PSEL_COUNTER_MAX_VAL/2;

// Bandwidth overhead control parameters
static const UINT64 OVERHEAD_PERIOD_MAX = 100000;
static const double BANDWIDTH_OVERHEAD_SET_POINT = 1;
static const double K_P_VALUE = 1;

// Decryption overhead control parameters
static const double DECRYPTION_OVERHEAD_SET_POINT = 1;
static const double RANGE_CORRECT_PREDICTION_FLOOR = 0.1;
static const double RATIO_GROUP_PREDICTION_FLOOR = 0.05;
/* END CONSTANTS */

/* GLOBAL VARIABLES FOR STATS */

static UINT64 total_overflow_events=0;
static UINT64 total_page_zero_events=0;
static UINT64 total_L0_overflow_events_during_page_zero=0;
static UINT64 total_overflow_events_during_page_zero=0;
static UINT64 total_dccm_overflow_events=0;
static UINT64 page_releveling_overhead=0;
//static UINT64 before_dccm_overflow_events=0;
//static UINT64 after_dccm_overflow_events=0;
static UINT64 number_of_preventing_block_relevel[4]={0};
static UINT64 number_of_nodes_selected_from_different_levels[4]={0};
static UINT64 total_block_relevel_stats=0;
//xinw added for debugging
static UINT64 potential_overflow_events=0;
static UINT64 failed_counter_increment = 0;

static UINT32 warmup_status = 0;	

static UINT64 fast_forward_value = 0;
static UINT64 skip_gap_init = 0;
//static UINT64 print_count = 0;
static UINT64 step_value = 0;

//static double period_allowed_bandwidth_overhead = BANDWIDTH_OVERHEAD_SET_POINT;
static UINT64 period_counter = 0;

// Total - stats collected during the warmup+run (entire experiment), stats - stats collected during the actual run (after warmup)
//static UINT64 instruction_count_total = 0;
static UINT64 instruction_count_total = 1;
static UINT64 instruction_count_stats = 0;
//static UINT64 global_instruction_count_stats = 0;
static UINT64 instruction_count_stats_thread[NUM_OF_THREADS*16] = {0};
UINT64 get_total_inst_num(){
	UINT64 sum_insts=0;
	for(int i=0;i<NUM_OF_THREADS;i++)
		sum_insts+=instruction_count_stats_thread[i*16];
	return sum_insts;
}
static UINT64 memory_instruction_count_total = 0;
static UINT64 memory_instruction_count_stats[NUM_OF_THREADS*16] = {0};

static UINT64 L1_hits_total[NUM_OF_THREADS*16] = {0};
static UINT64 L1_hits_stats[NUM_OF_THREADS*16] = {0};
//xinw added stats

static UINT64 L1_read_hits[NUM_OF_THREADS*16] = {0};
static UINT64 L1_write_hits[NUM_OF_THREADS*16] = {0};
static UINT64 L1_data_hits_stats[NUM_OF_THREADS*16] ={0};
static UINT64 L1_inst_hits_stats[NUM_OF_THREADS*16] ={0};
static UINT64 L1_page_table_hits_stats[NUM_OF_THREADS*16] ={0};
//static UINT64 L1_read_inst_accesses =0;
//static UINT64 L1_read_page_table_accesses =0;
static UINT64 L1_read_accesses[NUM_OF_THREADS*16] ={0};
static UINT64 L1_data_accesses[NUM_OF_THREADS*16] ={0};
static UINT64 L1_inst_accesses[NUM_OF_THREADS*16] ={0};
static UINT64 L1_page_table_accesses[NUM_OF_THREADS*16] ={0};
static UINT64 L1_write_accesses[NUM_OF_THREADS*16] ={0};
static UINT64 L1_eviction_stats[NUM_OF_THREADS*16] ={0};
static UINT64 num_of_useless_counter_accesses[NUM_OF_THREADS*16]={0};
static UINT64 num_of_total_counter_accesses[NUM_OF_THREADS*16]={0};
static UINT64 num_invalidate_l1_counter_cache[NUM_OF_THREADS*16]={0};

static UINT64 L2_hits_total = 0;
static UINT64 L2_hits_stats = 0;
static UINT64 L2_data_hits_stats = 0;
static UINT64 L2_inst_hits_stats = 0;
static UINT64 L2_page_table_hits_stats = 0;

static UINT64 L2_accesses_stats=0;
static UINT64 L2_data_accesses_stats=0;
static UINT64 L2_inst_accesses_stats=0;
static UINT64 L2_page_table_accesses_stats=0;

static UINT64 L2_insertion_stats=0;
static UINT64 num_metadata_write_backs_from_LLC=0;
static UINT64 memory_fetches_total = 0;
static UINT64 memory_fetches_stats = 0;

static UINT64 memory_writebacks_total = 0;
static UINT64 memory_writebacks_stats = 0;
//xinw added
static UINT64 memory_writebacks_while_releveling_total = 0;
static UINT64 memory_writebacks_while_releveling_stats= 0;
static UINT64 overflow_due_to_releveling_total = 0;
static UINT64 overflow_due_to_releveling_stats = 0;
 
static UINT64 predictions_total = 0;
static UINT64 predictions_stats = 0;

static UINT64 correct_predictions_total = 0;
static UINT64 correct_predictions_stats = 0;

static UINT64 counters_fetches_total[3] = { 0 };
static UINT64 counters_fetches_stats[3] = { 0 };
//xinw added for overflow traffic-begin
static UINT64 overflow_fetches_total = 0;
static UINT64 overflow_fetches_stats = 0;
static UINT64 overflow_fetches_stats_level[4] = {0};
static UINT64 NumMcrReBase[4] = {0};
static UINT64 overflow_smaller_than_smallest_stats = 0;
static UINT64 overflow_in_small_stats = 0;
static UINT64 overflow_in_mediate_stats = 0;
static UINT64 overflow_fetches_smaller_than_smallest_stats = 0;
static UINT64 overflow_fetches_in_small_stats = 0;
static UINT64 overflow_fetches_in_mediate_stats = 0;
//xinw added for overflow traffic-begin
//xinw added-begin
static UINT64 wc_cache_write_total = 0;
static UINT64 wc_cache_write_stats = 0;
//xinw added-end
//xinw changed the size of each array from 3 to 4
/*
static UINT64 mcr_to_zcc_switches_total[3] = { 0 };
static UINT64 mcr_to_zcc_switches_stats[3] = { 0 };
static UINT64 zcc_to_mcr_switches_total[3] = { 0 };
static UINT64 zcc_to_mcr_switches_stats[3] = { 0 };

static UINT64 zcc_counter_overflows_total[3] = { 0 };
static UINT64 zcc_counter_overflows_stats[3] = { 0 };
static UINT64 mcr_counter_overflows_total[3] = { 0 };
static UINT64 mcr_counter_overflows_stats[3] = { 0 };
*/
static UINT64 mcr_to_zcc_switches_total[4] = { 0 };
static UINT64 mcr_to_zcc_switches_stats[4] = { 0 };
static UINT64 reset_to_zcc_switches_stats[4] = {0};
static UINT64 zcc_to_mcr_switches_total[4] = { 0 };
static UINT64 zcc_to_mcr_switches_stats[4] = { 0 };

static UINT64 zcc_counter_overflows_total[4] = { 0 };
static UINT64 zcc_counter_overflows_stats[4] = { 0 };
static UINT64 mcr_counter_overflows_total[4] = { 0 };
static UINT64 mcr_counter_overflows_stats[4] = { 0 };


// Extra stats
static UINT64 stack_predictions_total = 0;
static UINT64 stack_predictions_stats = 0;
static UINT64 stack_correct_predictions_total = 0;
static UINT64 stack_correct_predictions_stats = 0;

static UINT64 heap_predictions_total = 0;
static UINT64 heap_predictions_stats = 0;
static UINT64 heap_correct_predictions_total = 0;
static UINT64 heap_correct_predictions_stats = 0;

static UINT64 rlvl_predictions_total = 0;
static UINT64 rlvl_predictions_stats = 0;
static UINT64 rlvl_correct_predictions_total = 0;
static UINT64 rlvl_correct_predictions_stats = 0;
// Extra stats

static UINT64 bandwidth_overhead_total = 0;
static UINT64 bandwidth_overhead_stats = 0;

static UINT64 decryption_overhead_total = 0;
static UINT64 decryption_overhead_stats = 0;

static UINT64 load_induced_misses_total = 0;
static UINT64 load_induced_misses_stats = 0;

static UINT64 store_induced_misses_total = 0;
static UINT64 store_induced_misses_stats = 0;

static UINT64 prediction_store_induced_misses_total = 0;
static UINT64 prediction_store_induced_misses_stats = 0;

static UINT64 wc_cache_hits_total = 0;	
static UINT64 wc_cache_hits_stats = 0;	
static UINT64 wc_cache_fetches_total = 0;	
static UINT64 wc_cache_fetches_stats = 0;	
//xinw added
static UINT64 wc_cache_tree_access_stats[4] = { 0 };	
static UINT64 wc_cache_tree_access_read_stats[4] = { 0 };	
static UINT64 wc_cache_tree_access_write_stats[4] = { 0 };	
static UINT64 wc_cache_tree_hit_read_stats[4] = { 0 };	
static UINT64 wc_cache_tree_hit_write_stats[4] = { 0 };	
static UINT64 wc_cache_tree_page_relevel_miss_stats[4] = { 0 };	
static UINT64 num_write_accesses_in_LLC[4]={0};
static UINT64 num_read_accesses_in_LLC[4]={0};
static UINT64 num_write_misses_in_LLC[4]={0};
static UINT64 num_read_misses_in_LLC[4]={0};


static UINT64 wccache_exist_but_miss_total = 0;	
static UINT64 wccache_exist_but_miss_stats = 0;	

static UINT64 mainmemory_assert_fails_total = 0;
static UINT64 pctable_assert_fails_total = 0;
static UINT64 ratio_group_assert_fails_total = 0;

static UINT64 mainmemory_error_ratio_total = 0;
static UINT64 mainmemory_error_ratio_stats = 0;

static UINT64 pctable_error_ratio_total = 0;
static UINT64 pctable_error_ratio_stats = 0;

static UINT64 pc_store_instructions_total = 0;
static UINT64 pc_load_instructions_total = 0;	

static UINT64 already_evicted_lines_total = 0;	
static UINT64 already_evicted_lines_stats = 0;	
   
static UINT64 same_wc_prediction_values_total = 0;
static UINT64 same_wc_prediction_values_stats = 0;

static UINT64 relevels_attempt_total = 0;
static UINT64 relevels_attempt_stats = 0;

static UINT64 relevels_skipped_total = 0;
static UINT64 relevels_skipped_stats = 0;

static UINT64 stack_memory_instruction_count_total = 0;
static UINT64 heap_memory_instruction_count_total = 0;
static UINT64 stack_memory_fetches_total = 0;
static UINT64 heap_memory_fetches_total = 0;
static UINT64 stack_memory_writebacks_total = 0;
static UINT64 heap_memory_writebacks_total = 0;
/* END GLOBAL VARIABLES FOR STATS */


// Output file handle
ofstream output_file;
ofstream output_file_overflow;
ofstream output_file_miss;
ofstream output_file_otp;

// Pintool KNOB used for command line parameters
// -o used to specify output file with all stats collected: ex. -o  youroutfile.out 
KNOB<double> KnobPossibilityForUpdatingL1WhileRead(KNOB_MODE_WRITEONCE, "pintool",
  "possibility_for_updating_l1_while_read", "0", "specify possibility for updating L1 counter while read");
KNOB<int> KnobInvalidateDirtyVersionInMetadataCache(KNOB_MODE_WRITEONCE, "pintool",
  "invalidate_dirty_version_in_metadata_cache", "0", "identify whether to invalidate dirty version metadata cache during data writeback");
KNOB<int> KnobDeprioritizeTreeNodesInMetadataCache(KNOB_MODE_WRITEONCE, "pintool",
  "deprioritize_tree_nodes_in_metadata_cache", "0", "identify whether to deprioritize integrity tree nodes in metadata cache");
KNOB<int> KnobAccessInstAndPageTable(KNOB_MODE_WRITEONCE, "pintool",
  "access_inst_and_page_table", "0", "identify whether to access inst and page table entry");
KNOB<int> KnobInsertMetadataIntoLLC(KNOB_MODE_WRITEONCE, "pintool",
  "insert_metadata_into_llc", "0", "identify whether to insert metadata blocks to LLC from private metadata cache");
KNOB<int> KnobUseSamplingForInsertingMetadataIntoLLC(KNOB_MODE_WRITEONCE, "pintool",
  "use_sampling_for_inserting_metadata_into_llc", "0", "identify whether to use sampling for inserting metadata blocks to LLC from private metadata cache");



KNOB<int> KnobRelevelToNextGroup(KNOB_MODE_WRITEONCE, "pintool",
  "relevel_to_next_group", "0", "identify whether to relevel to the bootom of next group in AES table");


 
KNOB<int> KnobRecentAes(KNOB_MODE_WRITEONCE, "pintool",
  "record_recent_aes", "0", "identify whether recording recent aes values missing in AES table");


KNOB<double> KnobRemainBudgetLimitForPageLevelReleveling(KNOB_MODE_WRITEONCE, "pintool",
  "remain_budget_limit_for_page_level_relevel", "0", "remaining dccm budget limit for page-level releveling");
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
  "o", "pinatrace.out", "specify output file name");
KNOB<int> KnobDeviation(KNOB_MODE_WRITEONCE, "pintool",
  "deviation", "0", "deviation for uniform distribution");
KNOB<string> KnobRandomCpt(KNOB_MODE_WRITEONCE, "pintool",
  "random_cpt", "random_cpt", "specify stats file directory");
KNOB<string> KnobTreeCpt(KNOB_MODE_WRITEONCE, "pintool",
  "tree_cpt", "tree_cpt", "specify stats file directory");
KNOB<string> KnobTableCpt(KNOB_MODE_WRITEONCE, "pintool",
  "table_cpt", "table_cpt", "specify stats file directory");
KNOB<int> KnobAesEpoch(KNOB_MODE_WRITEONCE, "pintool",
  "aes_epoch", "1000000", "AES epoch size");
KNOB<int> KnobResetTableStatsPerEpoch(KNOB_MODE_WRITEONCE, "pintool",
  "reset_table_stats_per_epoch", "0", "identify whether it is resetting table stats in the end of each epoch");
KNOB<double> KnobThresholdForInsertionOfEvictedGroupOverhead(KNOB_MODE_WRITEONCE, "pintool",
  "threshold_insertion_of_evicted_group", "0", "specify threshold for hit rate of evicted group to insert into the table");

KNOB<string> KnobOutputDir(KNOB_MODE_WRITEONCE, "pintool",
  "output_dir", "output_dir", "specify stats file directory");

// -f used to specify fastforward value (skip N instructions to skip benchmarks' initialization phases): ex. -f 2.5E+10
KNOB<double> KnobFastForward(KNOB_MODE_WRITEONCE, "pintool",
  "f", "1000000000", "specify fast forward value");
KNOB<double> KnobRunInsts(KNOB_MODE_WRITEONCE, "pintool",
  "run_insts", "1000000000", "specify number of insts to run");
KNOB<double> KnobRandomInit(KNOB_MODE_WRITEONCE, "pintool",
  "rand_init", "0", "specify random initialization of morphable tree");
KNOB<int> KnobMcrOverflow(KNOB_MODE_WRITEONCE, "pintool",
  "mcr_overflow", "0", "option to turn/off mcr_overflow");
KNOB<int> KnobHugePage(KNOB_MODE_WRITEONCE, "pintool",
  "huge_page", "0", "option to turn/off huge page");
KNOB<int> KnobIsGap(KNOB_MODE_WRITEONCE, "pintool",
  "is_gap", "0", "identify whether it is running for GAP or microbenchmarks");
KNOB<int> KnobFarRelevel(KNOB_MODE_WRITEONCE, "pintool",
  "far_relevel", "0", "identify whether it is doing far releveling");
KNOB<int> KnobGenerateTreeNodes(KNOB_MODE_WRITEONCE, "pintool",
  "generate_tree_nodes", "0", "identify whether it is generating or using tree nodes checkpoint");
KNOB<int> KnobUsePageZero(KNOB_MODE_WRITEONCE, "pintool",
  "use_page_zero", "0", "identify whether it is using page zeroing");
KNOB<int> KnobPredictiveDecryption(KNOB_MODE_WRITEONCE, "pintool",
  "predictive_decryption", "0", "option to turn/off dccm");
KNOB<double> KnobDccmOverhead(KNOB_MODE_WRITEONCE, "pintool",
  "dccm_overhead", "0.05", "specify dccm overhead ratio");
KNOB<int> KnobCachelineOffset(KNOB_MODE_WRITEONCE, "pintool",
  "cacheline_offset", "0", "cacheline address offset");
KNOB<double> KnobSkipGapInit(KNOB_MODE_WRITEONCE, "pintool",
  "skip_gap_init", "480000000", "specify gap fast forward value");
KNOB<double> KnobPossibilityOverflowRelevel(KNOB_MODE_WRITEONCE, "pintool",
  "possibility_overflow_relevel", "1", "specify possibility of dccm releveling with overflow overhead");
KNOB<double> KnobTimeDisablePageLevelRelevelWithOverhead(KNOB_MODE_WRITEONCE, "pintool",
  "time_disable_page_level_relevel_with_overhead", "0.5", "specify time length when  page level releveling with overhead is cancelled normalized to epoch size");
KNOB<double> KnobHighWcRatioThreshold(KNOB_MODE_WRITEONCE, "pintool",
  "high_wc_ratio_threshold", "1", "specify possibility of dccm releveling with overflow overhead");
KNOB<int> KnobDelayHighLevelTreeNodeOverflow(KNOB_MODE_WRITEONCE, "pintool",
  "delay_high_level_tree_node_overflow", "0", "option to turn/off delay_high_level_tree_node_overflow");
// Fast randomization algorithm (used instead of rand()). Used in DIP/DRRIP for probability of long vs distant insertion 
UINT32 xor5rand() {
  static UINT32 a = 123456789;
  static UINT32 b = 362436069;
  static UINT32 c = 521288629;
  static UINT32 d = 88675123;
  UINT32 t;

  t = a ^ (a << 11);   
  a = b; b = c; c = d;   
  d = d ^ (d >> 19) ^ (t ^ (t >> 8));

  return (d >> 27);
}

// Fast randomization algorithm (used instead of rand()). Used to randomize initial WC for memeory blocks 
UINT32 xor14rand() {
  static UINT32 x = 123456789;
  static UINT32 y = 362436069;
  static UINT32 z = 521288629;
  static UINT32 w = 88675123;
  UINT32 t;

  t = x ^ (x << 11);   
  x = y; y = z; z = w;   
  w = w ^ (w >> 19) ^ (t ^ (t >> 8));

  return (w >> 18);
}


// Struct to represent cache line/block
typedef struct CacheLine {
  UINT64 block_address;
  INT64 recency_value;
  INT32 dirty_status;
  //xinw added
  bool is_metadata;
  bool is_used;
  CacheLine() : block_address(0), recency_value(0), dirty_status(0), is_metadata(false), is_used(false) {}
} CACHE_LINE;

class DataCache {
  public:

    DataCache(INT32 policy, INT32 set_bits, INT32 set_num, INT32 set_size);                                                                                                
    void Initialize(INT32 policy, INT32 set_bits, INT32 set_num, INT32 set_size);                                                                                                
    UINT64 GetCacheSet(const UINT64& block_addr);
    UINT32 GetBlockIndex(const UINT64& cache_set, const UINT64& block_addr, bool is_metadata);
    void UpdateIsUsed(const UINT64& cache_set, const UINT64& block_addr);
    bool IsCacheHit(const UINT64& cache_set, const UINT64& block_addr, bool is_metadata);		
    bool CheckCacheHit(const UINT64& block_addr, bool is_metadata);		
    void InvalidateBlock(const UINT64& cache_set, UINT32& block_index);
    void InvalidateCounter(const UINT64& cache_set, UINT32& block_index, THREADID threadid);
    bool SetFullStatus(const UINT64& cache_set);		

    void UpdateBlockDirtyStatus(const UINT64& cache_set, const UINT64& block_addr, const UINT32& block_index, const UINT32& ins_op, const UINT32& t,bool is_metadata); 

    void InsertNoEviction(const UINT64& cache_set, const UINT64& block_addr, const UINT32& ins_op, const UINT32& t, bool is_metadata);		
    void InsertWithEviction(const UINT64& cache_set, const UINT64& block_addr, const INT32& insert_index, const UINT32& ins_op, const UINT32& t, bool is_metadata);		
    double calculate_data_block_occupancy();
    //xinw added begin
    UINT64 find_oldest_clean_block(const UINT64& cache_set);
    UINT64 find_oldest_dirty_block(const UINT64& cache_set);
    UINT64 find_any_oldest_block(const UINT64& cache_set);
    UINT64 getDrripVictim(const UINT64& cache_set);
    UINT64 getDrripBaselineVictim(const UINT64& cache_set);
   

    //xinw added end

    UINT64 GetEvictedIndex(const UINT64& cache_set);

    UINT64 GetBlockAddress(const UINT64& cache_set, const INT32& evict_index);
    INT32 GetBlockDirtyStatus(const UINT64& cache_set, const INT32& evict_index);
    bool GetIsUsed(const UINT64& cache_set, const INT32& evict_index) ;
    bool GetMetaStatus(const UINT64& cache_set, const INT32& evict_index);
    UINT32 ComplementSelectComparator(const UINT64& cache_set);
    //xinw added for sampling sets inserting or not inserting metadata into LLC
    bool is_sampling_sets_inserting_metadata(const UINT64& cache_set);
    bool is_sampling_sets_not_inserting_metadata(const UINT64& cache_set);
    UINT64 PSEL=512;
    void increment_PSEL();
    void decrement_PSEL();
    ~DataCache();
	
	private:
    INT32 set_bits_;
    INT32 set_num_;
    INT32 set_size_;
    INT32 block_size_;

    CACHE_LINE **cache_;
    INT32 *set_usage_;

    UINT32 replacement_policy_;
    UINT32 psel_counter_;
    UINT64 set_mask_;
};

DataCache::DataCache(INT32 policy, INT32 set_bits, INT32 set_num, INT32 set_size) :  set_bits_(set_bits), 
                                                                                     set_num_(set_num),                     
                                                                                     set_size_(set_size),
                                                                                     replacement_policy_(policy),
                                                                                     psel_counter_(PSEL_COUNTER_INIT_VAL) {

  cache_ = new CACHE_LINE *[set_num];
  for (INT32 i = 0; i < set_num; i++) {
    cache_[i] = new CACHE_LINE[set_size];
  }

  set_usage_ = new INT32[set_num];
  for(INT32 i = 0; i < set_num; i++) {
    set_usage_[i] = -1;
  }
  
  set_mask_ = set_num-1;

  CACHE_LINE empty = {};
  for (INT32 i = 0; i < set_num; i++) {
    for (INT32 j = 0; j < set_size; j++) {
      cache_[i][j] = empty;
    }
  }


  if (DEBUG_TEST) {
    //std::cout << "Cache constructor" << std::endl;
  }
}
void DataCache::Initialize(INT32 policy, INT32 set_bits, INT32 set_num, INT32 set_size){ 

   		set_bits_=set_bits; 
		set_num_=set_num;                     
		set_size_=set_size;
		replacement_policy_=policy;
		psel_counter_=PSEL_COUNTER_INIT_VAL;

  cache_ = new CACHE_LINE *[set_num];
  for (INT32 i = 0; i < set_num; i++) {
    cache_[i] = new CACHE_LINE[set_size];
  }

  set_usage_ = new INT32[set_num];
  for(INT32 i = 0; i < set_num; i++) {
    set_usage_[i] = -1;
  }
  
  set_mask_ = set_num-1;

  CACHE_LINE empty = {};
  for (INT32 i = 0; i < set_num; i++) {
    for (INT32 j = 0; j < set_size; j++) {
      cache_[i][j] = empty;
    }
  }


  if (DEBUG_TEST) {
    //std::cout << "Cache constructor" << std::endl;
  }
}


DataCache::~DataCache() {
  for (INT32 i = 0; i < set_num_; i++) {
    delete[] cache_[i];
  }
  delete[] cache_;
  
  delete[] set_usage_;

  if (DEBUG_TEST) {
    //std::cout << "Cache destructor" << std::endl;
  }
}
void DataCache::increment_PSEL(){
	if(PSEL<=1024)
		PSEL++;
}
void DataCache::decrement_PSEL(){
	if(PSEL>0)
		PSEL--;
}

bool DataCache::is_sampling_sets_inserting_metadata(const UINT64& cache_set){
	if((cache_set%32)==((cache_set/32)%32))
		return true;
	else
		return false;
}
bool DataCache::is_sampling_sets_not_inserting_metadata(const UINT64& cache_set){
	if((cache_set%32)==(31-((cache_set/32)%32)))
		return true;
	else
		return false;
}

double DataCache::calculate_data_block_occupancy(){
	INT32 num_data_blocks=0;
	for (INT32 i = 0; i < set_num_; i++) {
		for (INT32 j = 0; j < set_size_; j++) {
			if(!(cache_[i][j].is_metadata))
				num_data_blocks++;
		}
	}
	return (num_data_blocks)*1.0/(set_num_*set_size_);
}
UINT64 DataCache::GetCacheSet(const UINT64& block_addr) {
  return (block_addr >> DATA_CACHE_BLOCK_BITS) & set_mask_;
}

bool DataCache::SetFullStatus(const UINT64& cache_set) {
  if (set_usage_[cache_set] < set_size_ - 1) {
    return SET_NOT_FULL;
  } else {
    return SET_IS_FULL;
  }
}

UINT32 DataCache::GetBlockIndex(const UINT64& cache_set, const UINT64& block_addr, bool _is_metadata) {
  INT32 usage_index = set_usage_[cache_set];
   for(INT32 i = 0; i <= usage_index; i++) {
    if((cache_[cache_set][i].block_address == block_addr)&&(cache_[cache_set][i].is_metadata==_is_metadata)) { 
      return i;
    }
   } 
  return OUT_OF_BOUND;
}
void DataCache::UpdateIsUsed(const UINT64& cache_set, const UINT64& block_addr) {
  INT32 usage_index = set_usage_[cache_set];
   for(INT32 i = 0; i <= usage_index; i++) {
    if((cache_[cache_set][i].block_address == block_addr)) { 
      cache_[cache_set][i].is_used=true;
    }
   } 
}

void DataCache::UpdateBlockDirtyStatus(const UINT64& cache_set, const UINT64& block_addr, const UINT32& block_index, const UINT32& ins_op, const UINT32& t, bool _is_metadata) {

  if ((cache_[cache_set][block_index].block_address == block_addr)&&(cache_[cache_set][block_index].is_metadata==_is_metadata)) {
    if(ins_op == WRITE_OP) {
      cache_[cache_set][block_index].dirty_status = WRITE_OP;
    } else if (cache_[cache_set][block_index].dirty_status == READ_OP) {
      cache_[cache_set][block_index].dirty_status = ins_op;
    }
    //xinw added for touching block written
    cache_[cache_set][block_index].recency_value = instruction_count_total;
  } else {
    //std::cout << "Wrong index: " << block_index << " " << t << std::endl;
    //std::cout << std::hex << block_addr << " " << cache_[cache_set][block_index].block_address << std::endl;
  }
}

UINT32 DataCache::ComplementSelectComparator(const UINT64& cache_set) {

  if( (cache_set & 63) == ((cache_set >> 6) & 63) ) {
    return replacement_policy_-2;
  } else if ( ((cache_set & 63)^63)  == ((cache_set >> 6) & 63) ) {
    return replacement_policy_-1;
  }
  return replacement_policy_;
}

bool DataCache::IsCacheHit(const UINT64& cache_set, const UINT64& block_addr, bool _is_metadata) {

  BOOL is_hit = 0;

  INT32 usage_index = set_usage_[cache_set];
  for(INT32 i = 0; i <= usage_index; i++) {
    if((cache_[cache_set][i].block_address == block_addr)&&(cache_[cache_set][i].is_metadata==_is_metadata)) {
      // Hit Priority (RRIP-HP)
      cache_[cache_set][i].recency_value = instruction_count_total;
      is_hit = CACHE_HIT;
    }
  }

  if (replacement_policy_ == RPOLICY_DIP || replacement_policy_ == RPOLICY_DRRIP ) { 
    if(is_hit != CACHE_HIT) { 
      UINT32 set_type = ComplementSelectComparator(cache_set);
      if (set_type == LRU_SET || set_type == SRRIP_SET) {
        if (psel_counter_ < PSEL_COUNTER_MAX_VAL) { 
          psel_counter_++; 
        }
      } else if (set_type == BIP_SET || set_type == BRRIP_SET) { 
        if (psel_counter_ > 0) { 
          psel_counter_--; 
        }
      }
    }
  }
  if(use_sampling_for_inserting_metadata_into_LLC){
	if(is_sampling_sets_inserting_metadata(cache_set))
		increment_PSEL();
	if(is_sampling_sets_not_inserting_metadata(cache_set))
		decrement_PSEL();
  }
	
  return is_hit;
}

bool DataCache::CheckCacheHit(const UINT64& block_addr, bool _is_metadata){
	UINT64 cache_set = GetCacheSet(block_addr); 
	BOOL is_hit = 0;
	INT32 usage_index = set_usage_[cache_set];
	for(INT32 i = 0; i <= usage_index; i++) {
		if((cache_[cache_set][i].block_address == block_addr)&&(cache_[cache_set][i].is_metadata==_is_metadata)) {
			is_hit = CACHE_HIT;
		}
	}
	return is_hit;
}
void DataCache::InvalidateBlock(const UINT64& cache_set, UINT32& block_index) {
  INT32 usage_index = set_usage_[cache_set];
  for (INT32 i = block_index; i < usage_index; i++) {

    cache_[cache_set][i].block_address = cache_[cache_set][i+1].block_address;
    cache_[cache_set][i].recency_value = cache_[cache_set][i+1].recency_value;
    cache_[cache_set][i].dirty_status = cache_[cache_set][i+1].dirty_status; 
    cache_[cache_set][i].is_metadata = cache_[cache_set][i+1].is_metadata; 
    cache_[cache_set][i].is_used = cache_[cache_set][i+1].is_used; 
  }

  cache_[cache_set][usage_index].block_address = 0;
  cache_[cache_set][usage_index].recency_value = 0;
  cache_[cache_set][usage_index].dirty_status = 0;   
  cache_[cache_set][usage_index].is_metadata = false;   
  cache_[cache_set][usage_index].is_used = false;   

  if (set_usage_[cache_set] >= 0) {
    set_usage_[cache_set]--;
  } else {
    //std::cout << "Set_usage < 0" << std::endl;
  }
}
//UINT64 num_invalidate_l1_counter_cache[4]={0};
void DataCache::InvalidateCounter(const UINT64& cache_set, UINT32& block_index, THREADID threadid) {
  num_invalidate_l1_counter_cache[threadid*16]++;
  INT32 usage_index = set_usage_[cache_set];
  for (INT32 i = block_index; i < usage_index; i++) {
    cache_[cache_set][i].block_address = cache_[cache_set][i+1].block_address;
    cache_[cache_set][i].recency_value = cache_[cache_set][i+1].recency_value;
    cache_[cache_set][i].dirty_status = cache_[cache_set][i+1].dirty_status; 
    cache_[cache_set][i].is_metadata = cache_[cache_set][i+1].is_metadata; 
    cache_[cache_set][i].is_used = cache_[cache_set][i+1].is_used; 
  }

  cache_[cache_set][usage_index].block_address = 0;
  cache_[cache_set][usage_index].recency_value = 0;
  cache_[cache_set][usage_index].dirty_status = 0;   
  cache_[cache_set][usage_index].is_metadata = false;   
  cache_[cache_set][usage_index].is_used = false;   
  if (set_usage_[cache_set] >= 0) {
    set_usage_[cache_set]--;
  } else {
    //std::cout << "Set_usage < 0" << std::endl;
  }
}

void DataCache::InsertNoEviction(const UINT64& cache_set, const UINT64& block_addr, const UINT32& ins_op, const UINT32& t, bool _is_metadata) {

  INT32 usage_index = set_usage_[cache_set];
  usage_index++;

  cache_[cache_set][usage_index].block_address = block_addr;
  cache_[cache_set][usage_index].dirty_status = ins_op;
  cache_[cache_set][usage_index].recency_value = instruction_count_total;
  cache_[cache_set][usage_index].is_metadata = _is_metadata;
  cache_[cache_set][usage_index].is_used = false;
  if(_is_metadata){
	if(!use_sampling_for_inserting_metadata_into_LLC)
  		cache_[cache_set][usage_index].recency_value = instruction_count_total;
	else if(is_sampling_sets_inserting_metadata(cache_set))
  		cache_[cache_set][usage_index].recency_value = instruction_count_total;
	else if(is_sampling_sets_inserting_metadata(cache_set))
  		cache_[cache_set][usage_index].recency_value = 0;
	else if (PSEL>=512)
  		cache_[cache_set][usage_index].recency_value = 0;
	else
  		cache_[cache_set][usage_index].recency_value = instruction_count_total;
		
  }
  if (replacement_policy_ == RPOLICY_DIP) {
    UINT32 set_type = ComplementSelectComparator(cache_set);
    if ((set_type == BIP_SET) || (set_type == DIP_SET && psel_counter_ > PSEL_COUNTER_INIT_VAL)) {
      if (xor5rand() != BIP_PROBABILITY_VAL) {	
        INT64 largestRecencyVal = -1;
        for(INT32 i = 0; i <= set_usage_[cache_set]; i++) {
          if(cache_[cache_set][i].recency_value > largestRecencyVal) {
            largestRecencyVal = cache_[cache_set][i].recency_value;
          }
        }
        cache_[cache_set][usage_index].recency_value = largestRecencyVal + 1;
      }
    }
  } else if (replacement_policy_ == RPOLICY_DRRIP) {
    UINT32 set_type = ComplementSelectComparator(cache_set);
    if ((set_type == BRRIP_SET) || (set_type == DRRIP_SET && psel_counter_ > PSEL_COUNTER_INIT_VAL)) {
      if (xor5rand() != BRRIP_PROBABILITY_VAL) {	
        cache_[cache_set][usage_index].recency_value = RRIP_DISTANT;
      } else {
        cache_[cache_set][usage_index].recency_value = RRIP_LONG; 
      }
    } else if ((set_type == SRRIP_SET) || (set_type == DRRIP_SET && psel_counter_ <= PSEL_COUNTER_INIT_VAL)) {
        cache_[cache_set][usage_index].recency_value = RRIP_LONG; 
    }
  }

  if (set_usage_[cache_set] < set_size_ - 1) {
    set_usage_[cache_set]++;
  } else {
    //std::cout << "Set_usage > set_size" << std::endl;
  }
  
}


void DataCache::InsertWithEviction(const UINT64& cache_set, const UINT64& block_addr, const INT32& insert_index, const UINT32& ins_op, const UINT32& t, bool _is_metadata) {

  INT64 largestRecencyVal = cache_[cache_set][insert_index].recency_value;
  cache_[cache_set][insert_index].block_address = block_addr;
  cache_[cache_set][insert_index].recency_value = instruction_count_total;
  cache_[cache_set][insert_index].dirty_status = ins_op;
  cache_[cache_set][insert_index].is_metadata = _is_metadata;
  cache_[cache_set][insert_index].is_used = false;
   if(_is_metadata){
	if(!use_sampling_for_inserting_metadata_into_LLC)
  		cache_[cache_set][insert_index].recency_value = instruction_count_total;
	else if(is_sampling_sets_inserting_metadata(cache_set))
  		cache_[cache_set][insert_index].recency_value = instruction_count_total;
	else if(is_sampling_sets_inserting_metadata(cache_set))
  		cache_[cache_set][insert_index].recency_value = 0;
	else if (PSEL>=512)
  		cache_[cache_set][insert_index].recency_value = 0;
	else
  		cache_[cache_set][insert_index].recency_value = instruction_count_total;
		
  }
 
  if(replacement_policy_ == RPOLICY_DIP) {
    UINT32 set_type = ComplementSelectComparator(cache_set);
    if((set_type == BIP_SET) || (set_type == DIP_SET && psel_counter_ > PSEL_COUNTER_INIT_VAL)) {
      if(xor5rand() != BIP_PROBABILITY_VAL) {	
        cache_[cache_set][insert_index].recency_value = largestRecencyVal + 1;
      }
    }
  } else if (replacement_policy_ == RPOLICY_DRRIP) {
    UINT32 set_type = ComplementSelectComparator(cache_set);
    if ((set_type == BRRIP_SET) || (set_type == DRRIP_SET && psel_counter_ > PSEL_COUNTER_INIT_VAL)) {
      if (xor5rand() != BRRIP_PROBABILITY_VAL) {	
        cache_[cache_set][insert_index].recency_value = RRIP_DISTANT;
      } else {
        cache_[cache_set][insert_index].recency_value = RRIP_LONG; 
      }
    } else if ((set_type == SRRIP_SET) || (set_type == DRRIP_SET && psel_counter_ <= PSEL_COUNTER_INIT_VAL)) {
        cache_[cache_set][insert_index].recency_value = RRIP_LONG; 
    }
  }

}

//void InsertL2WithEviction(const UINT64& block_addr, const UINT32& ins_op,   bool is_metadata)


//xinw added begin
UINT64 DataCache::find_oldest_clean_block(const UINT64& cache_set)
{
	INT32 victim_index=10000;
	INT32 usage_index = set_usage_[cache_set];
	for (INT32 i = 0; i <= usage_index; i++) {
	/*	if(cache_[cache_set]==NULL)
		{
			printf("find_clean: break due to cache_[cache_set] is null\n");
		}	
		if(cache_[cache_set][i]==NULL)
		{
			printf("find_clean: break due to cache_[cache_set][i] is null\n");
		}
	*/
		if ((cache_[cache_set][i].dirty_status != WRITE_OP)&&(cache_[cache_set][i].recency_value >= RRIP_DISTANT)) {
			return i;
		}
	}
	return victim_index;
}
UINT64 DataCache::find_oldest_dirty_block(const UINT64& cache_set)
{
		int victim_index=10000;	
		INT32 usage_index = set_usage_[cache_set];
		for (INT32 i = 0; i <= usage_index; i++) {
			if ((cache_[cache_set][i].dirty_status == WRITE_OP)&&(cache_[cache_set][i].recency_value >= (RRIP_DISTANT+RRIP_AGGRESSIVNESS-1))) {
				return i;
			}
		}
		return victim_index;
}
UINT64 DataCache::find_any_oldest_block(const UINT64& cache_set)
{
		int victim_index=10000;	
		INT32 usage_index = set_usage_[cache_set];
		for (INT32 i = 0; i <= usage_index; i++) {
			if (cache_[cache_set][i].recency_value >= RRIP_DISTANT) {
				return i;
			}
		}
		return victim_index;
}
UINT64 DataCache::getDrripVictim(const UINT64& cache_set)
{
    // There must be at least one replacement candidate
    while(1)
    {
           UINT64 alternative_clean_victim=find_oldest_clean_block(cache_set);
           if(alternative_clean_victim!=10000)
	   {
		//printf("clean_victim: %lu\n", alternative_clean_victim);
                return alternative_clean_victim;
	   }
	       // printf("not finding clean victim, continue finding dirty victim\n"); 
           UINT64 alternative_prioritized_victim=find_oldest_dirty_block(cache_set);
           if(alternative_prioritized_victim!=10000)
	   {	
	//	printf("dirty_victim: %lu\n", alternative_prioritized_victim);	
                return alternative_prioritized_victim;
           }
	//   else
	//	printf("not finding dirty victim, continue finding by aging\n");
	   for (INT32 i=0;i<=set_usage_[cache_set];i++) {
                cache_[cache_set][i].recency_value += 1;
                   }
    }
}

UINT64 DataCache::getDrripBaselineVictim(const UINT64& cache_set)
{
    // There must be at least one replacement candidate
    while(1)
    {
           UINT64 alternative_victim=find_any_oldest_block(cache_set);
           if(alternative_victim!=10000)
	   {
		//printf("clean_victim: %lu\n", alternative_clean_victim);
                return alternative_victim;
	   }
	//	printf("not finding victim, continue finding by aging\n");
	   for (INT32 i=0;i<=set_usage_[cache_set];i++) {
                cache_[cache_set][i].recency_value += 1;
                   }
    }
}

UINT64 DataCache::GetEvictedIndex(const UINT64& cache_set) {

  INT32 usage_index = set_usage_[cache_set];
  INT32 insert_index = -1;

  if (replacement_policy_ == RPOLICY_LRU || replacement_policy_ == RPOLICY_DIP) {
    INT64 smallestRecencyVal = 1000000000000000;
    for(INT32 i = 0; i <= usage_index; i++) {
      if(cache_[cache_set][i].recency_value < smallestRecencyVal) {
        insert_index = i;
        smallestRecencyVal = cache_[cache_set][i].recency_value;
      }
    }
  } else if (replacement_policy_ == RPOLICY_DRRIP) {

	  insert_index=getDrripBaselineVictim(cache_set);
  }

  return insert_index;
};

UINT64 DataCache::GetBlockAddress(const UINT64& cache_set, const INT32& evict_index) {

  return cache_[cache_set][evict_index].block_address;

};
bool DataCache::GetIsUsed(const UINT64& cache_set, const INT32& evict_index) {

  return cache_[cache_set][evict_index].is_used;

};


INT32 DataCache::GetBlockDirtyStatus(const UINT64& cache_set, const INT32& evict_index) {

  return cache_[cache_set][evict_index].dirty_status;

};

bool DataCache::GetMetaStatus(const UINT64& cache_set, const INT32& evict_index){
  return cache_[cache_set][evict_index].is_metadata;
}
//static DataCache L1(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE);
//static DataCache L1[max_num_of_threads](RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE);
DataCache *L1;
DataCache *L1_counter_caches;
/*static DataCache L1[max_num_of_threads]={
DataCache(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE),
DataCache(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE),
DataCache(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE),
DataCache(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE),
DataCache(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE),
DataCache(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE),
DataCache(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE),
DataCache(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE),
DataCache(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE),
DataCache(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE),
DataCache(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE),
DataCache(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE),
DataCache(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE),
DataCache(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE),
DataCache(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE),
DataCache(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE)
};*/
static DataCache L2(RPOLICY_LRU, L2_SET_BITS, L2_SET_NUM, L2_SET_SIZE);


INT32 EvictionUpdateStats(const UINT64& evict_addr, const UINT32& evict_index, const UINT32& evict_dirty_status) {
 if(evict_dirty_status == WRITE_OP) {
    //printf("inst: %lu, evict dirty,  address: %lu\n",instruction_count_total, evict_addr);
    memory_writebacks_total++; 
    if (warmup_status == WARMUP_OVER) {
      memory_writebacks_stats++; 
    }
	}
 else{
 //   printf("inst: %lu, evict clean,  address: %lu\n",instruction_count_total, evict_addr);
}
   return 0;
/*
  period_counter++;

  if(evict_dirty_status == WRITE_OP) {
      
    memory_writebacks_total++; 
    if (warmup_status == WARMUP_OVER) {
      memory_writebacks_stats++; 
    }

    if (ExistsInMainMemory(evict_addr)) {
      if (main_memory[evict_addr].stack_status == 1) {
        stack_memory_writebacks_total++; 
      } else {
        heap_memory_writebacks_total++; 
      }

      main_memory[evict_addr].memory_fetches_total++;
      main_memory[evict_addr].memory_writebacks_total++;
      main_memory[evict_addr].cache_status = NOT_IN_CACHE;

      UINT64 evict_block_prog_count = main_memory[evict_addr].fetch_program_counter;
      //main_memory[evict_addr].write_count++;
      
      if (ExistsInPcTable(evict_block_prog_count)) {
        if (evict_index < PC_SAMPLING_SIZE) {

          mem_pc_table[evict_block_prog_count].memory_fetches_total++;	
          mem_pc_table[evict_block_prog_count].memory_writebacks_total++;	 

          if (warmup_status == WARMUP_OVER) {
            mem_pc_table[evict_block_prog_count].memory_fetches_stats++;	
          }
        }
      } else { 
        pctable_assert_fails_total++;
      }

    } else {
      mainmemory_assert_fails_total++;
    }

    return MISS_DIRTY_EVICT;

  } else {
    if (evict_dirty_status == DIRTY_BY_RELEVEL) {
      if (BANDWIDTH_OVERHEAD_SET_POINT < 1) {

        double current_bandwidth_overhead = bandwidth_overhead_total / (double) (memory_fetches_total + memory_writebacks_total);

        if (period_counter >= OVERHEAD_PERIOD_MAX) {
          period_counter = 0;

          double current_error = BANDWIDTH_OVERHEAD_SET_POINT - current_bandwidth_overhead;
          double proportional_output = current_error * K_P_VALUE;
          period_allowed_bandwidth_overhead = BANDWIDTH_OVERHEAD_SET_POINT + proportional_output;
        }

        if (current_bandwidth_overhead < period_allowed_bandwidth_overhead) {
          bandwidth_overhead_total++;
          if (warmup_status == WARMUP_OVER) {
            bandwidth_overhead_stats++; 
          } 
        } else {
          if (ExistsInMainMemory(evict_addr)) {
            main_memory[evict_addr].write_count = main_memory[evict_addr].previous_write_count;
          } else {
            mainmemory_assert_fails_total++;
          }
        }
      } else {
        bandwidth_overhead_total++;
        if (warmup_status == WARMUP_OVER) {
          bandwidth_overhead_stats++; 
        } 
      }
    }
    if (ExistsInMainMemory(evict_addr)) {
      main_memory[evict_addr].memory_fetches_total++;
      main_memory[evict_addr].cache_status = NOT_IN_CACHE;

      UINT64 evict_block_prog_count = main_memory[evict_addr].fetch_program_counter;

      if (ExistsInPcTable(evict_block_prog_count)) {
        if (evict_index < PC_SAMPLING_SIZE) {
          mem_pc_table[evict_block_prog_count].memory_fetches_total++;	
          if (warmup_status == WARMUP_OVER) {
            mem_pc_table[evict_block_prog_count].memory_fetches_stats++;	
          }
        }
      } else { 
        pctable_assert_fails_total++;
      }
    } else {
      mainmemory_assert_fails_total++;
    } 	

    return MISS_NON_DIRTY_EVICT;
  }
*/
}
// Struct to represent PC block in PC table (include stats variables)
typedef struct PcBlock {
// STORE/LOAD
  UINT32 instruction_type; 

  UINT64 memory_fetches_total;
  UINT64 memory_fetches_stats; 

  UINT64 predictions_total;
  UINT64 predictions_stats;

  UINT64 correct_predictions_total;
  UINT64 correct_predictions_stats;

  UINT64 memory_writebacks_total;
  
  UINT64 store_induced_misses_total;

  double wr_ratio;

  UINT64 ratio_group;

  UINT64 ratio_group_switches_total;
  UINT64 ratio_group_switches_stats;

  std::string rtn_name;
  INT32 line_number;
  std::string file_name;

  UINT64 wccache_hits_total;
  UINT64 wccache_hits_stats;

  UINT32 rq_group;

} PC_BLOCK;

// Struct to represent memeory block in memory (includes stats variables)
typedef struct MmDataBlock {
	
  UINT64 write_count;
  UINT64 previous_write_count;
  
  UINT64 memory_fetches_total;

  UINT64 memory_writebacks_total;
  
  UINT64 store_induced_misses_total;
 
  UINT64 store_instruction_count;		
  UINT64 fetch_program_counter;	

  UINT32 writeread_ratio_group;	
 
  UINT32 cache_status;
  
  UINT32 stack_status;

  UINT64 relevel_count_total;

} MM_DATA_BLOCK;

// Struct to represent WC relevel queue
typedef struct WcRelevelQBlock {
  UINT64 block_addr;
  UINT64 write_count;

  UINT64 correct_predictions_total;

} WC_RELEVELQ_BLOCK;


// Struct to represent ratio queue block (used to collect stats on all ratio groups)
typedef struct RatioGroupStatsBlock {
  UINT64 memory_fetches_total;
  UINT64 memory_fetches_stats;

  UINT64 predictions_total;
  UINT64 predictions_stats;

  UINT64 correct_predictions_total;
  UINT64 correct_predictions_stats;

  UINT64 predictions_distribution_total[PREDICTION_DIST_SIZE] = { 0 };
  UINT64 predictions_distribution_stats[PREDICTION_DIST_SIZE] = { 0 };

} RATIO_GROUP_STATS_BLOCK;

//xinw added for otp precalculation-begin

UINT64 accumulated_dccm_traffic_overhead=0;			      
UINT64 otp_table_hit_total=0;
UINT64 otp_table_miss_total=0;
UINT64 otp_table_miss_with_small_ctr_total=0;
UINT64 otp_table_miss_with_medium_ctr_total=0;
UINT64 otp_table_miss_with_big_ctr_total=0;
UINT64 otp_table_update_total=0;
UINT64 otp_table_active_relevel_total=0;
UINT64 otp_table_passive_relevel_total=0;
UINT64 otp_table_hit_while_wc_cache_miss_total=0;
UINT64 otp_table_miss_while_wc_cache_miss_total=0;
UINT64 otp_table_hit_stats=0;
UINT64 num_L1_counter_hit_in_memoization_table=0;
UINT64 otp_table_miss_stats=0;
UINT64 otp_table_miss_with_small_ctr_stats=0;
UINT64 otp_table_miss_with_medium_ctr_stats=0;
UINT64 otp_table_miss_with_big_ctr_stats=0;
UINT64 otp_table_update_stats=0;
UINT64 otp_table_active_relevel_stats=0;
UINT64 otp_table_passive_relevel_stats=0;
UINT64 otp_table_hit_while_wc_cache_miss_stats=0;
UINT64 otp_table_miss_while_wc_cache_miss_stats=0;
//xinw added for average difference between groups
double otp_average_distance;
double time_disable_page_level_relevel_with_overhead=0.5;
//double HIGH_WC_RATIO_THRESHOLD=0.05;
double HIGH_WC_RATIO_THRESHOLD=0.02;
//#define HIGH_WC_RATIO_THRESHOLD 0.1
#define THRESHOULD_FOR_SWITCHING_POSSIBILITY 2
//#define HIGHEST_GROUP_MOVE_THRESHOLD 0.95
#define HIGHEST_GROUP_MOVE_THRESHOLD 1
#define POSSIBILITY_FILTER 0.1
#define POSSIBILITY_RELEVEL 1
//double POSSIBILITY_WITH_OVERFLOW_RELEVEL=1;
double DCCM_OVERHEAD_RATIO=0.05;
double DCCM_OVERHEAD_RATIO_AFTER_WARMUP=0.05;
int64_t dccm_overflow_traffic=0;
int64_t dccm_remain_budget=0;
bool is_dccm_overhead=false;
#define TABLE_SIZE 8
#define num_recent_evicted_groups TABLE_SIZE
//#define TABLE_SIZE 32
//#define POTENTIAL_GROUP_NUM 21
#define POTENTIAL_GROUP_NUM 31
//#define POTENTIAL_GROUP_NUM 31
//UINT32 TABLE_SIZE= 16;
//once modified in march_3_otp32_epoch1million
#define TABLE_LINE_SIZE 16
#define threshold_for_times_of_smallest_group_with_lowest_hits (TABLE_SIZE-1)
//#define TABLE_LINE_SIZE 2
//#define system_max ((UINT64)(0x1)<<56)
UINT64 system_max=0;
//#define TABLE_LINE_SIZE 1
//#define AES_OTP_INTERVAL 5000
//#define AES_OTP_INTERVAL 512000
//#define AES_OTP_INTERVAL 10000000
//UINT64 AES_OTP_INTERVAL=1000000;
UINT64 AES_OTP_INTERVAL=1000000;

uint64_t dccm_overhead_for_groups[TABLE_SIZE]={0};
uint64_t dccm_block_relevel_overhead_for_groups[TABLE_SIZE]={0};
uint64_t dccm_page_relevel_overhead_for_groups[TABLE_SIZE]={0};
//#define OTP_INTERVAL 5000
//#define OTP_INTERVAL 5012000
UINT64 OTP_INTERVAL=1000000;
#define OTP_ACTIVE_RELEVEL_SMALL_GROUP_NUMBER 2
#define OTP_ACTIVE_RELEVEL_BIG_GROUP_NUMBER (TABLE_SIZE-2)
UINT64 history_max_wc=0;
UINT64 last_history_max_wc=0;
UINT32* wc_overflow;
UINT32* wc_miss;
//UINT64 wc_insert[2200000*times_of_16g]={0};
//UINT64 access_number=0;
//UINT64 access_number=0;
#define RELEVEL_FOR_SMALLER_VALUE_THAN_MIN 1
#define RELEVEL_FOR_VALUE_IN_SMALLEST 2
#define RELEVEL_FOR_VALUE_IN_MEDIATE 3
UINT64 relevel_reason=0;
VOID PrintOverflowResults() {
/*
  std::ostringstream oss_occ;
  oss_occ << KnobOutputFile.Value().c_str() << "_morphtree_4_4_micro_baseline_debug_new_overflow.out";
  std::string out_file_name = oss_occ.str();
  //output_file.open(out_file_name.c_str());
  //xinw modified open to append
  output_file_overflow.open(out_file_name.c_str(), ios::out|ios::app);
  output_file_overflow<< "wc overflow at memory instruction " <<memory_instruction_count_total<<":"<<std::endl;
  for(UINT64 i=0;i<2200000*128;i++)
        output_file_overflow << wc_overflow[i]<<endl;
  output_file_overflow << std::endl << std::endl;
   output_file_overflow<< "wc insert at memory instruction " <<memory_instruction_count_total<<":"<<std::endl;
  for(UINT64 i=0;i<2200000;i++)
        output_file_overflow << wc_insert[i]<<endl;
  output_file_overflow << std::endl << std::endl;
  
  output_file_overflow.close();
  output_file_overflow.clear();
*/
}
VOID PrintMissResults() {
/*
  std::ostringstream oss_occ;
  oss_occ << KnobOutputFile.Value().c_str() << "_morphtree_4_4_micro_baseline_miss_stats.out";
  std::string out_file_name = oss_occ.str();
  //output_file_miss.open(out_file_name.c_str(), ios::out|ios::app);
  output_file_miss.open(out_file_name.c_str(), ios::out);
  output_file_miss<< "wc miss at memory instruction " <<memory_instruction_count_total<<":"<<std::endl;
  for(UINT64 i=0;i<2200000*128;i++)
        output_file_miss << wc_miss[i]<<endl;
  output_file_miss << std::endl << std::endl;
   output_file_miss<< "wc insert at memory instruction " <<memory_instruction_count_total<<":"<<std::endl;
  for(UINT64 i=0;i<2200000;i++)
        output_file_miss << wc_insert[i]<<endl;
  output_file_miss << std::endl << std::endl;
  
  output_file_miss.close();
  output_file_miss.clear();
*/
}
UINT64 global_counter_id=0;
UINT64 global_minor_ctr_pos=0;
typedef struct RECENT_EVICT_GROUP {
  UINT64 beginning_counter;
  UINT64 number_of_hits;
  RECENT_EVICT_GROUP() : beginning_counter(0), number_of_hits(0){}
} recent_evicted_group;

std::deque<recent_evicted_group> recent_evicted_groups;
//std::deque<UINT64> recent_evicted_groups;
//UINT64 recent_evicted_groups_hits[num_recent_evicted_groups]; 
/*
class NewOtpTable
{
public:
  UINT64 me=0;
  UINT64 te=0;
  UINT64 tick_in_current_interval=0;
  UINT64 table[TABLE_SIZE];
  UINT64 min_wc;
  UINT64 max_wc;
  UINT64 epoch_max_wc=0; 
  UINT64 num_accesses_with_bigger_counter=0;
  UINT64 num_aes_misses=0;
  UINT64 num_otp_interval=0;
  double total_average_distance=0;
//  UINT64 group_gaps[31-1]={1,17,33,49,65,81,97,113,129,257,513,1025,2049,4097,8193,16385,32769,65537,131073,262145};
  UINT64 group_gaps[30]={0};
  UINT64 total_num_aes_accesses=0;
  UINT64 total_num_large_counter_accesses=0;
  UINT64 num_table_hits[TABLE_SIZE]={0};
  UINT64 num_table_hits_between_groups[TABLE_SIZE]={0};
  UINT64 potential_groups[31];
  UINT64 num_potential_group_hits[POTENTIAL_GROUP_NUM]={0};
  UINT64 times_of_smallest_group_with_lowest_hits=0;
 
};

NewOtpTable new_otp_table;
*/
typedef struct RECENT_AES {
  UINT64 counter_value;
  UINT64 last_inst;
  RECENT_AES() : counter_value(0), last_inst(0){}
} recent_aes;

#define THRESHOLD_FOR_RECENT_AES_VALUES 16
std::deque<recent_aes> recent_aes_values;
bool access_recent_aes(UINT64 _write_counter_value){

	bool find_aes=false;
        for(UINT64 recent_aes_index=0;recent_aes_index<recent_aes_values.size();recent_aes_index++){
		if(recent_aes_values[recent_aes_index].counter_value==_write_counter_value){
			find_aes=true;
			recent_aes_values[recent_aes_index].last_inst=instruction_count_total;		
		}
	}
       if(!find_aes)
       {
	   recent_aes _aes;
	   _aes.counter_value=_write_counter_value;
	   _aes.last_inst=instruction_count_total;

	   if(recent_aes_values.size()>=THRESHOLD_FOR_RECENT_AES_VALUES){
		   UINT64 smallest_inst=1000000000000000;	
		   UINT64 victim_index=0; 
		   for(UINT64 recent_aes_index=0;recent_aes_index<recent_aes_values.size();recent_aes_index++){
			   if(recent_aes_values[recent_aes_index].last_inst<smallest_inst){
				   victim_index=recent_aes_index;
				   smallest_inst=recent_aes_values[recent_aes_index].last_inst;		
			   }
		   }
		   recent_aes_values.erase(recent_aes_values.begin()+victim_index);
	   }
	   recent_aes_values.push_back(_aes);
	}
	return find_aes;
}



class OtpTable
{
public:
  UINT64 me=0;
  UINT64 te=0;
  UINT64 tick_in_current_interval=0;
  UINT64 table[TABLE_SIZE];
//  std::array<UINT64, TABLE_SIZE> table;

  UINT64 min_wc;
  UINT64 max_wc;
  UINT64 epoch_max_wc=0; 
  UINT64 num_accesses_with_bigger_counter=0;
  UINT64 num_aes_misses=0;
//xinw added for average distance between groups
  UINT64 num_otp_interval=0;
  double total_average_distance=0;

  //UINT64 group_gaps[POTENTIAL_GROUP_NUM-1]={1,17,33,49,65,81,97,113,129,257,513,1025,2049,4097,8193,16385,32769,65537,131073,262145};
  UINT64 group_gaps[POTENTIAL_GROUP_NUM-1]={0};
  UINT64 total_num_aes_accesses=0;
  UINT64 total_num_large_counter_accesses=0;
  UINT64 num_table_hits[TABLE_SIZE]={0};
  UINT64 num_table_hits_between_groups[TABLE_SIZE]={0};
  //UINT64 potential_groups[POTENTIAL_GROUP_NUM];
  UINT64 potential_groups[POTENTIAL_GROUP_NUM];
  UINT64 num_potential_group_hits[POTENTIAL_GROUP_NUM]={0};
  UINT64 times_of_smallest_group_with_lowest_hits=0;
public:
  OtpTable();
  void reset(UINT64 _max_ctr, UINT64 _min_ctr);
  bool CheckTable(UINT64 _max_ctr, UINT64 _min_ctr);
  bool CheckDuplicationInTable();
  void PrintOtpTable();
  
  void enqueue_to_recently_evicted_groups(UINT64 _beginning_counter);
  void update();
  bool need_activel_relevel(UINT64 _effective_ctr);
  UINT64 get_relevel_wc(UINT64 _effective_ctr, bool default_clean, bool default_overflow);
  UINT64 get_nearest_bottom_wc(UINT64 _effective_ctr);
  UINT64 get_nearest_bottom_group(UINT64 _effective_ctr);
  bool access(UINT64 _effective_ctr, UINT32 _wccahe_hit);
  bool counter_satisfy_requirement_for_consuming_dccm_budget(UINT64 _effective_ctr);
  bool remaining_dccm_budget_nonzero();
  bool remaining_dccm_budget_for_page_relevel();
  bool counter_satisfy_requirement_for_epoch_period(UINT64 _effective_ctr);
  void PrintTableUpdateAndResetStats();
  void find_victim_and_update_the_table(bool _insertion_potential_group);
  bool hit_in_table_groups_except_top_of_group(UINT64 _effective_ctr);
};
OtpTable::OtpTable()
  {
    min_wc=0;
    max_wc=TABLE_SIZE*TABLE_LINE_SIZE-1; 
    for(int table_index=0; table_index<TABLE_SIZE; table_index++)
	table[table_index]= table_index*TABLE_LINE_SIZE;
  }
void OtpTable::reset(UINT64 _max_ctr, UINT64 _min_wc)
  {
    //min_wc=begin_ctr;
    //max_wc=begin_ctr+TABLE_SIZE*TABLE_LINE_SIZE-1;
     
    //for(int table_index=0; table_index<TABLE_SIZE; table_index++)
    //	table[table_index]= begin_ctr+table_index*TABLE_LINE_SIZE;
    min_wc=0;
    max_wc=_max_ctr;

    //for(int table_index=0; table_index<TABLE_SIZE-1; table_index++)
    for(int table_index=0; table_index<=TABLE_SIZE-1; table_index++)
    	table[table_index]= table_index*TABLE_LINE_SIZE;
//xinw added for debugging
    //	table[table_index]= table_index*100+4;
    	//table[table_index]= table_index*8+188;
    //min_wc=table[0];
    //max_wc=table[TABLE_SIZE-1]+TABLE_LINE_SIZE-1;
    table[TABLE_SIZE-1]=max_wc-TABLE_LINE_SIZE+1;
    //xinw added for debugging
 /*   table[TABLE_SIZE-1]=15167-7;    
    table[TABLE_SIZE-2]=15167-15;    
    table[TABLE_SIZE-3]=15167-23;    
    table[TABLE_SIZE-4]=15167-31;    
*/
    for(int potential_group_index=0;potential_group_index<POTENTIAL_GROUP_NUM;potential_group_index++) 
    {
	if(potential_group_index<POTENTIAL_GROUP_NUM-1)	
		potential_groups[potential_group_index]=max_wc+group_gaps[potential_group_index];
	else
		potential_groups[potential_group_index]=system_max;
	
    }	
    for(UINT64 group_gap_index=0;group_gap_index<17;group_gap_index++)
	    group_gaps[group_gap_index]=1+group_gap_index*TABLE_LINE_SIZE;
    UINT64 base_gap=256;
    for(UINT64 group_gap_index=17;group_gap_index<30;group_gap_index++){
	    group_gaps[group_gap_index]=group_gaps[group_gap_index-1]+base_gap;
	    base_gap=base_gap*2;
    }
    for(UINT64 group_index=0;group_index<31;group_index++)
    {
	    if(group_index<30)
		    potential_groups[group_index]=max_wc+group_gaps[group_index];
	    else
		    potential_groups[group_index]=system_max;
    }
  }
bool OtpTable::CheckTable(UINT64 _max_ctr, UINT64 _min_wc)
  {
    //min_wc=begin_ctr;
    //max_wc=begin_ctr+TABLE_SIZE*TABLE_LINE_SIZE-1;

    //for(int table_index=0; table_index<TABLE_SIZE; table_index++)
    //  table[table_index]= begin_ctr+table_index*TABLE_LINE_SIZE;
    if((min_wc!=0)||
    (max_wc!=_max_ctr))
        return false;
    for(UINT64 table_index=0; table_index<TABLE_SIZE-1; table_index++)
    {
        if(table[table_index] != (table_index*TABLE_LINE_SIZE))
                return false;
    }
    if(table[TABLE_SIZE-1]!=(max_wc-TABLE_LINE_SIZE+1))
                return false;
    return true;
  }

bool OtpTable::CheckDuplicationInTable()
{
     for(UINT64 table_index=1; table_index<TABLE_SIZE-1; table_index++)
    {
        if(table[table_index] == (table[table_index-1]))
                return true;
    }
    return false;
}
void OtpTable::PrintOtpTable() {
/*
  std::ostringstream oss_otp;
  oss_otp << KnobOutputFile.Value().c_str() << "_morphtree_baseline_august_26_verify_otp8_baseline_page_zero_without_insert_otp_table.out";
  std::string out_file_name = oss_otp.str();
  output_file_otp.open(out_file_name.c_str(), ios::out|ios::app);
  output_file_otp<< "otp table at memory instruction " <<memory_instruction_count_total<<": ";
  for(int table_index=0; table_index<TABLE_SIZE; table_index++)
        output_file_otp<<table[table_index]<<" ";
  output_file_otp << std::endl << std::endl; 
  output_file_otp.close();
  output_file_otp.clear(); 
*/
printf("update the AES table: \n");
 for(int table_index=0; table_index<TABLE_SIZE; table_index++)
        cout<<table[table_index]<<" ";
 printf(" at instruction: %lu after memory accesses:%lu\n", instruction_count_total-fast_forward_value, memory_fetches_stats);
 
}
/*
  void OtpTable::update()
  {
    if(!OTP_PRECALCULATION)
	return;
    sort(table, table+TABLE_SIZE);  
    //xinw added for average distance between groups-begin
    num_otp_interval++;
    UINT64 current_sum_distance=0;
    for(int table_index=0; table_index<TABLE_SIZE-2; table_index++)
	    current_sum_distance=current_sum_distance+table[table_index+1]-(table[table_index]+TABLE_LINE_SIZE-1);  
    total_average_distance+=current_sum_distance*1.0/(TABLE_SIZE-1);
    otp_average_distance=total_average_distance*1.0/num_otp_interval;
    //xinw added for average distance between groups-end

    double high_wc_ratio=total_num_large_counter_accesses*1.0/total_num_aes_accesses;
    if(debug_aes_table_epoch)
    {
	    std::ostringstream oss;
	    //oss << KnobOutputFile.Value().c_str() <<"_morphtree_4_4_micro_baseline_debug_new_final.out";
	    oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
	    std::string out_file_name = oss.str();
	    output_file.open(out_file_name.c_str(), ios::out|ios::app);
	    //PrintTotalStats();
	    output_file << "AES accesses: " <<total_num_aes_accesses<<" , AES big counter accessess: " <<total_num_large_counter_accesses;
	    if(high_wc_ratio>HIGH_WC_RATIO_THRESHOLD)
	    	output_file << ", exceeds the threshold" <<std::endl;
	    else
	    	output_file << ", not exceeds the threshold" <<std::endl;
	    output_file <<  "system_max: "<<system_max <<std::endl;
	    output_file <<  "content of the AES table: " <<std::endl;
            for(int table_index=0; table_index<TABLE_SIZE; table_index++)
       		 output_file << table[table_index]<< std::endl;
	    output_file << std::endl;
    
	    output_file.close();
	    output_file.clear();
    }
    if(high_wc_ratio>HIGH_WC_RATIO_THRESHOLD)
    {

	    UINT64 victim_group_index=0;
	    UINT64 min_hits=1000000000;
	    for(int table_index=0;table_index<TABLE_SIZE;table_index++)
	    {
		    if(num_table_hits[table_index]<min_hits)
		    {
			    min_hits=num_table_hits[table_index];
			    victim_group_index=table_index;
		    }
	    }
	    UINT64  actual_group_index_with_lowest_hits=victim_group_index;
	    if((victim_group_index==0)&&(num_table_hits[0]<(1.0*HIGHEST_GROUP_MOVE_THRESHOLD*total_num_large_counter_accesses))&&(times_of_smallest_group_with_lowest_hits>=threshold_for_times_of_smallest_group_with_lowest_hits))
	    {
		       if(debug_aes_table_update)
		    {
			    std::ostringstream oss;
			    //oss << KnobOutputFile.Value().c_str() <<"_morphtree_4_4_micro_baseline_debug_new_final.out";
			    oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
			    std::string out_file_name = oss.str();
			    output_file.open(out_file_name.c_str(), ios::out|ios::app);
			    //PrintTotalStats();
			    output_file << "before update, AES accesses: " <<total_num_aes_accesses<<" , AES big counter accessess: " <<total_num_large_counter_accesses<<std::endl;
			    output_file <<  "system_max: "<<system_max <<std::endl;
			    output_file <<  "content of the AES table: " <<std::endl;
			    for(int table_index=0; table_index<TABLE_SIZE; table_index++)
			    {
				    if(table_index==0)
					output_file <<"counters between 0 and "<<table[0]<<" : , hits number: "<<num_table_hits_between_groups[0]*1.0/total_num_aes_accesses << std::endl;
				    else
					output_file <<"counters between "<<(table[table_index-1]+TABLE_LINE_SIZE)<<" and "<<table[table_index]<<" : , hits number: "<<num_table_hits_between_groups[table_index]*1.0/total_num_aes_accesses << std::endl;
				    output_file <<"beginning counter: "<< table[table_index]<<" , hits number: "<<num_table_hits[table_index]*1.0/total_num_aes_accesses<< std::endl;
			    }
			     output_file <<  "content of the potential groups: " <<std::endl;
			     
		    	    for(int potential_group_index=0;potential_group_index<POTENTIAL_GROUP_NUM;potential_group_index++)
				    output_file <<"beginning counter: "<< potential_groups[potential_group_index]<<" , hits number: "<<num_potential_group_hits[potential_group_index]*1.0/total_num_aes_accesses<< std::endl;
			    
			    output_file << std::endl;
				
			    output_file.close();
			    output_file.clear();
		    }

		    times_of_smallest_group_with_lowest_hits=0;
		    UINT64 smallest_index_for_potential_group=0;
		    UINT64 accumulated_hits=0;
		    for(int potential_group_index=0;potential_group_index<POTENTIAL_GROUP_NUM;potential_group_index++)
		    {
			    accumulated_hits+=num_potential_group_hits[potential_group_index];
			    if((accumulated_hits*1.0)>(1.0*HIGHEST_GROUP_MOVE_THRESHOLD*total_num_large_counter_accesses))
			    {
				    smallest_index_for_potential_group=potential_group_index;
				    break;
			    }
		    }
		    if(smallest_index_for_potential_group<(POTENTIAL_GROUP_NUM-1))
		    	table[victim_group_index]=potential_groups[smallest_index_for_potential_group];
		    else
			table[victim_group_index]=system_max;
		    sort(table, table+TABLE_SIZE); 
		    PrintOtpTable();
		    min_wc=table[0];
		    max_wc=table[TABLE_SIZE-1]+TABLE_LINE_SIZE-1;
		    for(int potential_group_index=0;potential_group_index<POTENTIAL_GROUP_NUM;potential_group_index++) 
		    {
			    if(potential_group_index<POTENTIAL_GROUP_NUM-1)	
				    potential_groups[potential_group_index]=max_wc+group_gaps[potential_group_index];
			    else
				    potential_groups[potential_group_index]=system_max;

		    }
		    otp_table_update_total++;
		    if (warmup_status == WARMUP_OVER)
			    otp_table_update_stats++;
		    if(actual_group_index_with_lowest_hits==0)
			times_of_smallest_group_with_lowest_hits++;
		    else
			times_of_smallest_group_with_lowest_hits=0;
		     
		     if(debug_aes_table_update)
		    {
			    std::ostringstream oss;
			    //oss << KnobOutputFile.Value().c_str() <<"_morphtree_4_4_micro_baseline_debug_new_final.out";
			    oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
			    std::string out_file_name = oss.str();
			    output_file.open(out_file_name.c_str(), ios::out|ios::app);
			    //PrintTotalStats();
			    output_file << std::endl;
			    output_file <<  "after update, content of the AES table: " <<std::endl;
			    for(int table_index=0; table_index<TABLE_SIZE; table_index++)
				    output_file <<"beginning counter: "<< table[table_index]<< std::endl;
			    
			    output_file << std::endl;
				
			    output_file.close();
			    output_file.clear();
		    }
			step_value = 0;
 	 		PrintResults();

	    }
	    else
	    { 
		victim_group_index=0;
	    	min_hits=1000000000;
		 for(int table_index=1;table_index<TABLE_SIZE;table_index++)
	    	{
		    if(num_table_hits[table_index]<min_hits)
		    {
			    min_hits=num_table_hits[table_index];
			    victim_group_index=table_index;
		    }
	    	}
    	
	
	    if(num_table_hits[victim_group_index]<(1.0*HIGHEST_GROUP_MOVE_THRESHOLD*total_num_large_counter_accesses))
	    {
		    if(debug_aes_table_update)
		    {
			    std::ostringstream oss;
			    //oss << KnobOutputFile.Value().c_str() <<"_morphtree_4_4_micro_baseline_debug_new_final.out";
			    oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
			    std::string out_file_name = oss.str();
			    output_file.open(out_file_name.c_str(), ios::out|ios::app);
			    //PrintTotalStats();
			    output_file << "before update, AES accesses: " <<total_num_aes_accesses<<" , AES big counter accessess: " <<total_num_large_counter_accesses<<std::endl;
			    output_file <<  "system_max: "<<system_max <<std::endl;
			    output_file <<  "content of the AES table: " <<std::endl;
			    for(int table_index=0; table_index<TABLE_SIZE; table_index++)
			    {
				    if(table_index==0)
					output_file <<"counters between 0 and "<<table[0]<<" : , hits number: "<<num_table_hits_between_groups[0]*1.0/total_num_aes_accesses << std::endl;
				    else
					output_file <<"counters between "<<(table[table_index-1]+TABLE_LINE_SIZE)<<" and "<<table[table_index]<<" : , hits number: "<<num_table_hits_between_groups[table_index]*1.0/total_num_aes_accesses << std::endl;
				    output_file <<"beginning counter: "<< table[table_index]<<" , hits number: "<<num_table_hits[table_index]*1.0/total_num_aes_accesses<< std::endl;
			    }
			     output_file <<  "content of the potential groups: " <<std::endl;
			     
		    	    for(int potential_group_index=0;potential_group_index<POTENTIAL_GROUP_NUM;potential_group_index++)
				    output_file <<"beginning counter: "<< potential_groups[potential_group_index]<<" , hits number: "<<num_potential_group_hits[potential_group_index]*1.0/total_num_aes_accesses<< std::endl;
			    
			    output_file << std::endl;
				
			    output_file.close();
			    output_file.clear();
		    }

		    UINT64 smallest_index_for_potential_group=0;
		    UINT64 accumulated_hits=0;
		    for(int potential_group_index=0;potential_group_index<POTENTIAL_GROUP_NUM;potential_group_index++)
		    {
			    accumulated_hits+=num_potential_group_hits[potential_group_index];
			    if((accumulated_hits*1.0)>(1.0*HIGHEST_GROUP_MOVE_THRESHOLD*total_num_large_counter_accesses))
			    {
				    smallest_index_for_potential_group=potential_group_index;
				    break;
			    }
		    }
		    if(smallest_index_for_potential_group<(POTENTIAL_GROUP_NUM-1))
		    	table[victim_group_index]=potential_groups[smallest_index_for_potential_group];
		    else
			table[victim_group_index]=system_max;
		    sort(table, table+TABLE_SIZE); 
		    PrintOtpTable();
		    min_wc=table[0];
		    max_wc=table[TABLE_SIZE-1]+TABLE_LINE_SIZE-1;
		    for(int potential_group_index=0;potential_group_index<POTENTIAL_GROUP_NUM;potential_group_index++) 
		    {
			    if(potential_group_index<POTENTIAL_GROUP_NUM-1)	
				    potential_groups[potential_group_index]=max_wc+group_gaps[potential_group_index];
			    else
				    potential_groups[potential_group_index]=system_max;

		    }
		    otp_table_update_total++;
		    if (warmup_status == WARMUP_OVER)
			    otp_table_update_stats++;
		    if(actual_group_index_with_lowest_hits==0)
			times_of_smallest_group_with_lowest_hits++;
		    else
			times_of_smallest_group_with_lowest_hits=0;
		     if(debug_aes_table_update)
		    {
			    std::ostringstream oss;
			    //oss << KnobOutputFile.Value().c_str() <<"_morphtree_4_4_micro_baseline_debug_new_final.out";
			    oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
			    std::string out_file_name = oss.str();
			    output_file.open(out_file_name.c_str(), ios::out|ios::app);
			    //PrintTotalStats();
			    output_file << std::endl;
			    output_file <<  "after update, content of the AES table: " <<std::endl;
			    for(int table_index=0; table_index<TABLE_SIZE; table_index++)
				    output_file <<"beginning counter: "<< table[table_index]<< std::endl;
			    
			    output_file << std::endl;
				
			    output_file.close();
			    output_file.clear();
		    }	
			step_value = 0;
 	 		PrintResults();


		}

	    } 
    }
    total_num_aes_accesses=0;
    total_num_large_counter_accesses=0;
    for(int table_index=0;table_index<TABLE_SIZE;table_index++)
    {
		    num_table_hits[table_index]=0;
  		    num_table_hits_between_groups[table_index]={0};
    }
    for(int potential_group_index=0;potential_group_index<POTENTIAL_GROUP_NUM;potential_group_index++)
	    num_potential_group_hits[potential_group_index]=0;

    tick_in_current_interval=0;
  }
*/
  bool OtpTable::need_activel_relevel(UINT64 _effective_ctr)
  {
    sort(table, table+TABLE_SIZE); 
    bool hit_small=false;
    for(int table_index=0; table_index<OTP_ACTIVE_RELEVEL_SMALL_GROUP_NUMBER; table_index++)
    {
	if((_effective_ctr>=table[table_index])&&(_effective_ctr<table[table_index]+TABLE_LINE_SIZE))
	{
		hit_small=true;
	}
    }
    if(hit_small)
	return false;
    if(_effective_ctr<table[OTP_ACTIVE_RELEVEL_SMALL_GROUP_NUMBER])
    {
	num_aes_misses++;
	//printf("num_aes_misse: %lu\n", num_aes_misses);
	if(num_aes_misses==(1/POSSIBILITY_RELEVEL))
	{
		num_aes_misses=0;
		return true;
	}
	return false; 
    }
    bool found_wc=false; 
    if(_effective_ctr>max_wc)
	return false;
    for(int table_index=0; table_index<TABLE_SIZE; table_index++)
	    {
		    if((_effective_ctr>=table[table_index])&&(_effective_ctr<table[table_index]+TABLE_LINE_SIZE))
			    found_wc=true;
	    }
    if(found_wc)
	return false;
    else
    {	
	    num_aes_misses++;
	    if(num_aes_misses==(1/POSSIBILITY_RELEVEL))
		{
		    num_aes_misses=0;
		    return true;
		}
		    return false; 
    }
  }
bool same_relevel_for_small_and_big=false;
UINT64 OtpTable::get_relevel_wc(UINT64 _effective_ctr, bool default_clean, bool default_overflow)
{	relevel_reason=0;
	sort(table, table+TABLE_SIZE); 
	max_wc = table[TABLE_SIZE-1] + TABLE_LINE_SIZE - 1;
	min_wc = table[0];

	UINT64 default_new_ctr=_effective_ctr+1;
	if(default_new_ctr>max_wc)
		return default_new_ctr;
	else if  (far_relevel&&(default_new_ctr<table[OTP_ACTIVE_RELEVEL_SMALL_GROUP_NUMBER]))
	{
		if(default_new_ctr<table[0])
			relevel_reason=RELEVEL_FOR_SMALLER_VALUE_THAN_MIN;
		else
		{
			relevel_reason=RELEVEL_FOR_VALUE_IN_MEDIATE;
			for(int table_index=0; table_index<OTP_ACTIVE_RELEVEL_SMALL_GROUP_NUMBER; table_index++)
			{
				if((default_new_ctr>=table[table_index])&&(default_new_ctr<table[table_index]+TABLE_LINE_SIZE))
					relevel_reason=RELEVEL_FOR_VALUE_IN_SMALLEST;		   
			}

		}
		// printf("active releveling: default_new_ctr:%lu, final_ctr: %lu\n", default_new_ctr, table[OTP_ACTIVE_RELEVEL_BIG_GROUP_NUMBER]);
		otp_table_active_relevel_total++;
		if (warmup_status == WARMUP_OVER)
			otp_table_active_relevel_stats++;
		if(relevel_reason==RELEVEL_FOR_VALUE_IN_SMALLEST)
			return default_new_ctr;
		else
		{
		    if(!same_relevel_for_small_and_big)
		    {  
						return table[OTP_ACTIVE_RELEVEL_BIG_GROUP_NUMBER];
		        }
		    else
		    {
			    for(int table_index=0; table_index<TABLE_SIZE; table_index++)
			    {
				    if((default_new_ctr<table[table_index]))
				    {
					    relevel_reason=RELEVEL_FOR_VALUE_IN_MEDIATE;
					    otp_table_passive_relevel_total++;
					    if (warmup_status == WARMUP_OVER)
						    otp_table_passive_relevel_stats++; 
					std::ostringstream oss;
			  			
					    return table[table_index];
				    }
			    }

		    }
		    return default_new_ctr;
		}
	}
	else 
	{
		bool found_nearest_wc=false; 
		for(int table_index=0; table_index<TABLE_SIZE; table_index++)
		{
			if((default_new_ctr>=table[table_index])&&(default_new_ctr<table[table_index]+TABLE_LINE_SIZE))
				found_nearest_wc=true;
		}
		if(found_nearest_wc)
		{
			//	relevel_reason=RELEVEL_FOR_VALUE_IN_MEDIATE;
			//	printf("_effective_ctr: %lu, _new_ctr: %lu\n", _effective_ctr, default_new_ctr);
			if(default_clean)
			{
				otp_table_passive_relevel_total++;
				if (warmup_status == WARMUP_OVER)
					otp_table_passive_relevel_stats++;

			}
			return default_new_ctr;
		}
		else
		{
			for(int table_index=0; table_index<TABLE_SIZE; table_index++)
			{
				if((default_new_ctr<table[table_index]))
				{
					relevel_reason=RELEVEL_FOR_VALUE_IN_MEDIATE;
					//			    printf("passive releveling: default_new_ctr: %lu, final_ctr: %lu\n", default_new_ctr, table[table_index]);
					otp_table_passive_relevel_total++;
					if (warmup_status == WARMUP_OVER)
						otp_table_passive_relevel_stats++; 
				

					return table[table_index];
				}
			}
			return default_new_ctr;
		}
	}

}
UINT64 OtpTable::get_nearest_bottom_wc(UINT64 _effective_ctr)
{
	sort(table, table+TABLE_SIZE); 
	bool is_hit=false;
	int hit_group_index=-1;
	for(int table_index=0; table_index<TABLE_SIZE; table_index++)
	{
		if((_effective_ctr>=table[table_index])&&(_effective_ctr<table[table_index]+TABLE_LINE_SIZE))
		{	
			is_hit=true;
			hit_group_index=table_index;
		}
	}
	if(is_hit&&(hit_group_index>=0)&&(hit_group_index<(TABLE_SIZE-1))&&(table[hit_group_index+1]==(table[hit_group_index]+TABLE_LINE_SIZE)))
		//return table[hit_group_index];
		return _effective_ctr;
	for(int table_index=0; table_index<TABLE_SIZE; table_index++)
	{
		if(_effective_ctr<=table[table_index])
			return table[table_index];
	}
	return 0;
}
UINT64 OtpTable::get_nearest_bottom_group(UINT64 _effective_ctr)
{
	sort(table, table+TABLE_SIZE); 
	for(int table_index=0; table_index<TABLE_SIZE; table_index++)
	{
		if(_effective_ctr<=table[table_index])
			return table_index;
	}
	return 0;
}

/*
bool OtpTable::access(UINT64 _effective_ctr, UINT32 _wccache_hit)
{
	
	if(_effective_ctr>system_max)
	{
		if(_effective_ctr<=1000000000)
			system_max=_effective_ctr;
		else
		{
			printf("unnormal effective counter: %lu for block address: %lu\n", _effective_ctr, current_block_addr_access_aes_table);
			morph_tree.versions_level[global_counter_id].PrintNode();
		}
	}
	total_num_aes_accesses++;
	bool is_hit=false;
	if(_effective_ctr>max_wc){
		total_num_large_counter_accesses++;
		for(int potential_group_index=0; potential_group_index<POTENTIAL_GROUP_NUM; potential_group_index++)
		{
			if(_effective_ctr<potential_groups[potential_group_index]+TABLE_LINE_SIZE)
			{
				num_potential_group_hits[potential_group_index]++;
				break;
			}
			if(potential_group_index==(POTENTIAL_GROUP_NUM-1))
				num_potential_group_hits[potential_group_index]++;
		}
	}
	for(int table_index=0; table_index<TABLE_SIZE; table_index++)
	{
		if((_effective_ctr>=table[table_index])&&(_effective_ctr<table[table_index]+TABLE_LINE_SIZE))
		{
			is_hit=true;
			num_table_hits[table_index]++;
			break;
		}
	} 	
	if(is_hit)
	{
		otp_table_hit_total++;
		if(_wccache_hit<2)
			otp_table_hit_while_wc_cache_miss_total++;
		if (warmup_status == WARMUP_OVER)
		{
			otp_table_hit_stats++;	
			if(_wccache_hit<2)
				otp_table_hit_while_wc_cache_miss_stats++;
		}
	}
	else
	{
		otp_table_miss_total++;	
		if(_wccache_hit<2)
			otp_table_miss_while_wc_cache_miss_total++;

		if (warmup_status == WARMUP_OVER)
		{
			otp_table_miss_stats++;	
			if(_wccache_hit<2)
				otp_table_miss_while_wc_cache_miss_stats++;

		}
		if(_effective_ctr<min_wc)
		{
			otp_table_miss_with_small_ctr_total++;
			if (warmup_status == WARMUP_OVER)
				otp_table_miss_with_small_ctr_stats++;
		}
		else if(_effective_ctr<=max_wc)
		{
			otp_table_miss_with_medium_ctr_total++;
			if (warmup_status == WARMUP_OVER)
				otp_table_miss_with_medium_ctr_stats++;
		}
		else
		{	
			otp_table_miss_with_big_ctr_total++;
			if (warmup_status == WARMUP_OVER)
				otp_table_miss_with_big_ctr_stats++;
		}
	} 
	if((tick_in_current_interval++)>AES_OTP_INTERVAL)
		update();
	if(debug_aes_hit)
	{
		if(is_hit)
			printf("AES table hit for effective counter: %lu, data block addr:  %lu, instruction number: %lu\n", _effective_ctr, current_block_addr_access_aes_table, instruction_count_total-fast_forward_value );
		else
			printf("AES table miss for effective counter: %lu, data block addr:  %lu, instruction number: %lu\n", _effective_ctr, current_block_addr_access_aes_table, instruction_count_total-fast_forward_value );
	}
	return is_hit;	
}
*/
OtpTable otp_table;
double POSSIBILITY_WITH_OVERFLOW_RELEVEL=1;
uint64_t _dccm_counter=0;
bool possibilistic_page_level_dccm_relevel()
{
	if(POSSIBILITY_WITH_OVERFLOW_RELEVEL==0)
		return false;
	uint64_t possibility_cycle=(uint64_t)(1.0/POSSIBILITY_WITH_OVERFLOW_RELEVEL);
	_dccm_counter++;
	if(debug_dccm_counter){
		printf("dccm counter: %lu\n", _dccm_counter);
		std::ostringstream oss;
		oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
		std::string out_file_name = oss.str();
		output_file.open(out_file_name.c_str(), ios::out|ios::app);
		output_file<<"possibility counter: "<<_dccm_counter<<" , possibility cycle: "<<possibility_cycle<<std::endl;
		output_file.close();
		output_file.clear();
	}
	if((_dccm_counter>=possibility_cycle)&&(otp_table.remaining_dccm_budget_for_page_relevel()))
		return true;
	else 
		return false;
}

//xinw added for otp precalculation-end
///////////////////////////////////////////////
// Morphable Counters Integrity Tree Code Start
///////////////////////////////////////////////
static const UINT32 ZCC_FORMAT = 0;
static const UINT32 MCR_FORMAT = 1;

static const UINT32 MCR_BASE_SET_1 = 0;
static const UINT32 MCR_BASE_SET_2 = 1;

static const UINT32 ZCC_NZCTRS_FIELD_BITS = 256;

static const UINT32 ZCC_NZCTRS_NUM[6] = {16, 32, 36, 42, 51, 64};
static const UINT32 ZCC_NZCTRS_SIZES[6] = {16, 8, 7, 6, 5, 4};
static const UINT32 ZCC_16BIT_CTR = 0;
static const UINT32 ZCC_4BIT_CTR = 5;

static const UINT64 MCR_BASE_MASK = 0x7F;
static const UINT64 MCR_MAJOR_CTR_SHIFT = 7;

static const UINT64 MCR_MAX_MINOR_CTR_VALUE = 8;
static const UINT64 MCR_MAX_BASE_VALUE = 128;

static const UINT32 ZCC_MINOR_CTR_NUM = 128;
static const UINT32 MCR_SET_MINOR_CTR_NUM = 64;

static const UINT32 MINOR_CTR_POS_MASK = 127;
//xinw added for counter mode frequency stats

static UINT32 zcc_frequency_stats[4][6] = {0};
static UINT32 zcc_detailed_overflow_stats[4][6] = {0};
static UINT32 zcc_inner_switches_stats[4][6] = {0};
static UINT32 zcc_inner_switches_overflow_stats[4][6] = {0};
static UINT32 mcr_frequency_stats[4] = {0};
typedef struct OVERFLOW_NODE {
  UINT64 tree_level;
  UINT64 node_id;
  UINT64 overflow_number;
  UINT64 write_counter_value;
  OVERFLOW_NODE() : tree_level(0), node_id(0), overflow_number(0), write_counter_value(0) {}
} overflow_node;

std::deque<overflow_node> overflow_nodes_queue;
void remove_overflow_node(UINT64 _tree_level, UINT64 _morph_addr)
{
	if(!overflow_nodes_queue.size())
		return;
	UINT64 _node_id=0;
	if(_tree_level==0)
		_node_id=_morph_addr;
	else if(_tree_level==1)
		_node_id=_morph_addr-2097152;
	else if(_tree_level==2)
		_node_id=_morph_addr-2097152-16384;  
	else if(_tree_level==3)
		_node_id=_morph_addr-2097152-16384-128;
	if(debug_prioritize_overflow_nodes)
	{
		printf("before removing, the content of overflow nodes queue: \n");
		for(UINT64 overflow_nodes_queue_index=0; overflow_nodes_queue_index<overflow_nodes_queue.size(); overflow_nodes_queue_index++){
			printf("tree level: %lu , node id: %lu , overfow number: %lu\n", overflow_nodes_queue[overflow_nodes_queue_index].tree_level, overflow_nodes_queue[overflow_nodes_queue_index].node_id, overflow_nodes_queue[overflow_nodes_queue_index].overflow_number);
		}
	
	}
	bool _find_victim=false;
	for(UINT64 overflow_nodes_queue_index=0; overflow_nodes_queue_index<overflow_nodes_queue.size(); overflow_nodes_queue_index++){
		if((overflow_nodes_queue[overflow_nodes_queue_index].tree_level==_tree_level)&&(overflow_nodes_queue[overflow_nodes_queue_index].node_id==_node_id)){
			_find_victim=true;
			overflow_nodes_queue.erase(overflow_nodes_queue.begin()+overflow_nodes_queue_index);
			break;
		}
	}	
	if(debug_prioritize_overflow_nodes)
	{
		printf("after removing, the content of overflow nodes queue: \n");
		for(UINT64 overflow_nodes_queue_index=0; overflow_nodes_queue_index<overflow_nodes_queue.size(); overflow_nodes_queue_index++){
			printf("tree level: %lu , node id: %lu , overfow number: %lu\n", overflow_nodes_queue[overflow_nodes_queue_index].tree_level, overflow_nodes_queue[overflow_nodes_queue_index].node_id, overflow_nodes_queue[overflow_nodes_queue_index].overflow_number);
		}
	
	}
	if(debug_block_relevel&&_find_victim){
		std::ostringstream oss;
		oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
		std::string out_file_name = oss.str();
		output_file.open(out_file_name.c_str(), ios::out|ios::app);
		output_file<<"removing node into overflow_node_queue, the size is: "<<overflow_nodes_queue.size() <<" at instruction: "<<(instruction_count_total-fast_forward_value)<<std::endl;
		output_file.close();
		output_file.clear();
	}


}

class MorphCtrBlock {
  public:

    MorphCtrBlock();
    void InitBlock(bool is_random, uint32_t expected_level, uint32_t _node_index);
    bool CheckBlock(uint32_t expected_level);
    UINT64 GetEffectiveCounter(const UINT32& level, const UINT64& pos);
    UINT64 GetLargestEffectiveCounter();
   //xinw added for otp precomputation-begin
    UINT64 GetMinorCounter(UINT64 _effective_counter, const UINT64& pos);
    bool is_stack=false;
   //xinw added for otp precomputation-end
    void MinorCountersReset(bool mcr_base_set);

    void SwitchFormatZccToMcr();
    //xinw added one parameter: pos
    void SwitchFormatMcrToZcc(const UINT64& pos);
    void SwitchToZcc(const UINT64& new_ctr);
    bool ZccCheckForOverflows(const UINT32& minor_ctr);

    //void IncrementMinorCounter(const UINT64& pos);
    UINT32  will_overflow(const UINT64& pos, bool default_clean, bool is_relevel);
    UINT32  highest_group_to_avoid_overflow(const UINT64& pos,bool default_clean);
    bool miss_small_ctr(const UINT64& pos,bool default_clean);
    UINT32  will_overflow_ZccMinorCounter(const UINT64& pos, bool default_clean, bool is_relevel);
    UINT32 highest_group_to_avoid_overflow_ZccMinorCounter(const UINT64& pos, bool default_clean);
    UINT32  will_overflow_McrMinorCounter(const UINT64& pos, bool default_clean, bool is_relevel); 
    UINT32 highest_group_to_avoid_overflow_McrMinorCounter(const UINT64& pos, bool default_clean);
    void IncrementMinorCounter(const UINT64& pos, bool default_clean, bool should_relevel);
    void IncrementMinorCounterForSmall(const UINT64& pos, bool default_clean, bool should_relevel, UINT32 table_index);
    void IncrementZccMinorCounter(const UINT64& pos, bool default_clean, bool should_relevel);
    void IncrementZccMinorCounterForSmall(const UINT64& pos, bool default_clean, bool should_relevel, UINT32 table_index);
    bool GetMcrBaseSet(const UINT64& pos);
    void IncrementMcrMinorCounter(const UINT64& pos, bool default_clean, bool should_relevel); 
    void IncrementMcrMinorCounterForSmall(const UINT64& pos, bool default_clean, bool should_relevel, UINT32 table_index); 
    void McrDoRebasing(bool mcr_base_set, const UINT32& smallest_minor_ctr); 
    UINT32 McrGetSmallestMinorCounter(bool mcr_base_set);
    bool McrCheckForOverflows(bool mcr_base_set, const UINT32& minor_ctr);
    bool GetCounterFormat(); 
    bool CounterIsUsed(); 
    UINT32 GetCtrLevel(); 
    void SetCtrLevel(const UINT32& level);
    void SetUsed(); 
  
    //xinw added for overflow_fetch_stats-begin
    void SetSubNodeAddr(UINT64 sub_node_addr);
    void FetchForOverflow();
    void FetchForMcrOverflow(bool is_base_2);
    void SetZccMajorCounter(UINT64 _major_ctr);
    void SetMorphAddr(UINT64 _morph_addr);
    //xinw added for overflow_fetch_stats-end
    void PrintNode();
    void PrintSimpleNode();
    void RedirectPrintNode(UINT64 evict_minor_ctr_pos);
  private:
    bool ctr_format_;
    bool ctr_is_used_;
    UINT32 ctr_level_;

    // ZCC format
    UINT64 zcc_major_ctr_;
    UINT32 zcc_nzctrs_type_;
    UINT32 zcc_nzctrs_num_;
    UINT32 zcc_max_minor_ctr_;

    // MCR format
    UINT64 mcr_major_ctr_;
    UINT32 mcr_base_1_;
    UINT32 mcr_base_2_;
    UINT32 mcr_base_1_max_minor_ctr_;
    UINT32 mcr_base_2_max_minor_ctr_;
    UINT32 mcr_base_1_min_minor_ctr_;
    UINT32 mcr_base_2_min_minor_ctr_;
  public:
    UINT32 minor_ctrs_[128] = { 0 }; 
    //xinw added for overflow_fetch_stats-begin
    UINT64 sub_node_begin_addr; 
    UINT64 morph_addr;
    UINT64 current_pos;
};


MorphCtrBlock::MorphCtrBlock() {
//InitBlock(false);
/*
  zcc_major_ctr_  = 0;
  zcc_nzctrs_type_ = ZCC_16BIT_CTR;
  zcc_nzctrs_num_ = 0;
  zcc_max_minor_ctr_ = 0;
  ctr_format_ = ZCC_FORMAT; 
  ctr_is_used_ = 0;
  ctr_level_ = 0;
  
  //mcr_major_ctr_ = 1;
  mcr_major_ctr_ = 0;
  mcr_base_1_ = 0;
  mcr_base_2_ = 0; 
  mcr_base_1_max_minor_ctr_ = 0;
  mcr_base_2_max_minor_ctr_ = 0;
  mcr_base_1_min_minor_ctr_ = MCR_MAX_MINOR_CTR_VALUE;
  mcr_base_2_min_minor_ctr_ = MCR_MAX_MINOR_CTR_VALUE; 
  //xinw added for otp precalculation-begin
  if(BEGIN_WITH_BIG_COUNTER)
  {
     if(rand_func()%128<65)
     {
	    // printf("select zcc\n");
	     zcc_major_ctr_ = (rand_func()%65536) + 2048;
	     zcc_nzctrs_num_ = (rand_func()%64);
	     for(int type_index=4; type_index>=0; type_index--)
	     {
		if(zcc_nzctrs_num_>ZCC_NZCTRS_NUM[type_index])
		{
			zcc_nzctrs_type_ = type_index+1;
			break;
		}
	     }
	    // printf("zcc_nztrs_type: %u\n", zcc_nzctrs_type_);
	     UINT32 minor_index;
	    // printf("zcc non-zero counter number: %u\n", zcc_nzctrs_num_);
	     for(minor_index=0; minor_index<zcc_nzctrs_num_; minor_index++)
	     {
			minor_ctrs_[minor_index]=rand_func()&((1<<ZCC_NZCTRS_SIZES[zcc_nzctrs_type_])-1);
			if (minor_ctrs_[minor_index]==0)
				minor_ctrs_[minor_index]=1;
  	    	//	printf("minor_ctr %u\n", minor_ctrs_[minor_index]);
	     }
	     srand(rand_seed);
             rand_seed = (rand_seed+1)%100;
	     std::random_shuffle(minor_ctrs_, minor_ctrs_+128);
	     for(minor_index=0; minor_index<128; minor_index++)
	     {
		if(minor_ctrs_[minor_index]>zcc_max_minor_ctr_)
			zcc_max_minor_ctr_ = minor_ctrs_[minor_index];
	     } 
     }	
     else
     { 
	     //printf("select mcr\n");
	     zcc_major_ctr_ = (rand_func()%65536) + 2048; 
	     mcr_major_ctr_ = zcc_major_ctr_ >> MCR_MAJOR_CTR_SHIFT;
	     mcr_base_1_ = zcc_major_ctr_ & MCR_BASE_MASK;
	     mcr_base_2_ = zcc_major_ctr_ & MCR_BASE_MASK;
	     zcc_major_ctr_ = 0;
	     zcc_nzctrs_type_ = ZCC_16BIT_CTR;
	     zcc_nzctrs_num_ = 0;
	     zcc_max_minor_ctr_ = 0;
	     UINT32 minor_index;
	     for(minor_index=0;minor_index<128;minor_index++)
		minor_ctrs_[minor_index] = rand_func()%8;
	     mcr_base_1_max_minor_ctr_ = 0;
 	     mcr_base_2_max_minor_ctr_ = 0;
	     for(minor_index=0;minor_index<64;minor_index++)
	     {
		if(minor_ctrs_[minor_index]>mcr_base_1_max_minor_ctr_)
			mcr_base_1_max_minor_ctr_ = minor_ctrs_[minor_index];
	     } 
	     for(minor_index=64;minor_index<128;minor_index++)
	     {
		if(minor_ctrs_[minor_index]>mcr_base_2_max_minor_ctr_)
			mcr_base_2_max_minor_ctr_ = minor_ctrs_[minor_index];
	     }
	     ctr_format_ = MCR_FORMAT;  
     }	
  }
  //xinw added for otp precalculation-end
  */
}

void MorphCtrBlock::InitBlock(bool is_random, uint32_t expected_level, uint32_t _node_index){

	zcc_major_ctr_  = 1000;
	//zcc_major_ctr_ =(_node_index%9)*100+99;
	if(generate_tree_nodes)
		zcc_major_ctr_  = 0;
		
	zcc_nzctrs_type_ = ZCC_16BIT_CTR;
	zcc_nzctrs_num_ = 0;
	zcc_max_minor_ctr_ = 0;
	ctr_format_ = ZCC_FORMAT; 
	ctr_is_used_ = 0;
	ctr_level_ = expected_level;

	mcr_major_ctr_ = 0;
	mcr_base_1_ = 0;
	mcr_base_2_ = 0; 
	mcr_base_1_max_minor_ctr_ = 0;
	mcr_base_2_max_minor_ctr_ = 0;
	mcr_base_1_min_minor_ctr_ = MCR_MAX_MINOR_CTR_VALUE;
	mcr_base_2_min_minor_ctr_ = MCR_MAX_MINOR_CTR_VALUE; 
	return;

		zcc_major_ctr_ = 200; 
		mcr_major_ctr_ = zcc_major_ctr_ >> MCR_MAJOR_CTR_SHIFT;
		mcr_base_1_ = zcc_major_ctr_ & MCR_BASE_MASK;
		mcr_base_2_ = zcc_major_ctr_ & MCR_BASE_MASK;
		zcc_major_ctr_ = 0;
		zcc_nzctrs_type_ = ZCC_16BIT_CTR;
		zcc_nzctrs_num_ = 0;
		zcc_max_minor_ctr_ = 0;
		UINT32 minor_index;
		for(minor_index=0;minor_index<128;minor_index++)
		{
		  if(minor_index%2==0)
			minor_ctrs_[minor_index] = 0;
		    else	
			minor_ctrs_[minor_index] = 7;
		

		}
		mcr_base_1_max_minor_ctr_ = 0;
		mcr_base_2_max_minor_ctr_ = 0;
		for(minor_index=0;minor_index<64;minor_index++)
		{
			if(minor_ctrs_[minor_index]>mcr_base_1_max_minor_ctr_)
				mcr_base_1_max_minor_ctr_ = minor_ctrs_[minor_index];
		} 
		for(minor_index=64;minor_index<128;minor_index++)
		{
			if(minor_ctrs_[minor_index]>mcr_base_2_max_minor_ctr_)
				mcr_base_2_max_minor_ctr_ = minor_ctrs_[minor_index];
		}
		ctr_format_ = MCR_FORMAT;  
		ctr_is_used_ = 0;
		ctr_level_ = expected_level;
		return;

	if(expected_level)
	{
		zcc_major_ctr_  = 128;
		zcc_nzctrs_type_ = ZCC_16BIT_CTR;
		zcc_nzctrs_num_ = 0;
		zcc_max_minor_ctr_ = 0;
		ctr_format_ = ZCC_FORMAT; 
		ctr_is_used_ = 0;
		ctr_level_ = expected_level;

		//mcr_major_ctr_ = 1;
		mcr_major_ctr_ = 0;
		mcr_base_1_ = 0;
		mcr_base_2_ = 0; 
		mcr_base_1_max_minor_ctr_ = 0;
		mcr_base_2_max_minor_ctr_ = 0;
		mcr_base_1_min_minor_ctr_ = MCR_MAX_MINOR_CTR_VALUE;
		mcr_base_2_min_minor_ctr_ = MCR_MAX_MINOR_CTR_VALUE; 
	} 
	else
	{
	
		zcc_major_ctr_ = 200; 
		mcr_major_ctr_ = zcc_major_ctr_ >> MCR_MAJOR_CTR_SHIFT;
		mcr_base_1_ = zcc_major_ctr_ & MCR_BASE_MASK;
		mcr_base_2_ = 160 & MCR_BASE_MASK;
		zcc_major_ctr_ = 0;
		zcc_nzctrs_type_ = ZCC_16BIT_CTR;
		zcc_nzctrs_num_ = 0;
		zcc_max_minor_ctr_ = 0;
		UINT32 minor_index;
		for(minor_index=0;minor_index<128;minor_index++)
		{
		  if(minor_index%2==0)
			minor_ctrs_[minor_index] = 0;
		    else	
			minor_ctrs_[minor_index] = 7;
		

		}
		mcr_base_1_max_minor_ctr_ = 0;
		mcr_base_2_max_minor_ctr_ = 0;
		for(minor_index=0;minor_index<64;minor_index++)
		{
			if(minor_ctrs_[minor_index]>mcr_base_1_max_minor_ctr_)
				mcr_base_1_max_minor_ctr_ = minor_ctrs_[minor_index];
		} 
		for(minor_index=64;minor_index<128;minor_index++)
		{
			if(minor_ctrs_[minor_index]>mcr_base_2_max_minor_ctr_)
				mcr_base_2_max_minor_ctr_ = minor_ctrs_[minor_index];
		}
		ctr_format_ = MCR_FORMAT;  
		ctr_is_used_ = 0;
		ctr_level_ = expected_level;
	}

 //xinw added for otp precalculation-begin
  if(is_random)
  {
     if(rand_func()%128<65)
     {
	    // printf("select zcc\n");
	     zcc_major_ctr_ = (rand_func()%65536) + 2048;
	     zcc_nzctrs_num_ = (rand_func()%64);
	     for(int type_index=4; type_index>=0; type_index--)
	     {
		if(zcc_nzctrs_num_>ZCC_NZCTRS_NUM[type_index])
		{
			zcc_nzctrs_type_ = type_index+1;
			break;
		}
	     }
	    // printf("zcc_nztrs_type: %u\n", zcc_nzctrs_type_);
	     UINT32 minor_index;
	    // printf("zcc non-zero counter number: %u\n", zcc_nzctrs_num_);
	     for(minor_index=0; minor_index<zcc_nzctrs_num_; minor_index++)
	     {
			minor_ctrs_[minor_index]=rand_func()&((1<<ZCC_NZCTRS_SIZES[zcc_nzctrs_type_])-1);
			if (minor_ctrs_[minor_index]==0)
				minor_ctrs_[minor_index]=1;
  	    	//	printf("minor_ctr %u\n", minor_ctrs_[minor_index]);
	     }
	     //srand(rand_seed);
             //rand_seed = (rand_seed+1)%100;
	     //std::random_shuffle(minor_ctrs_, minor_ctrs_+128);
	     for(minor_index=0; minor_index<128; minor_index++)
	     {
		if(minor_ctrs_[minor_index]>zcc_max_minor_ctr_)
			zcc_max_minor_ctr_ = minor_ctrs_[minor_index];
	     } 
     }	
     else
     { 
	     //printf("select mcr\n");
	     zcc_major_ctr_ = (rand_func()%65536) + 2048; 
	     mcr_major_ctr_ = zcc_major_ctr_ >> MCR_MAJOR_CTR_SHIFT;
	     mcr_base_1_ = zcc_major_ctr_ & MCR_BASE_MASK;
	     mcr_base_2_ = zcc_major_ctr_ & MCR_BASE_MASK;
	     zcc_major_ctr_ = 0;
	     zcc_nzctrs_type_ = ZCC_16BIT_CTR;
	     zcc_nzctrs_num_ = 0;
	     zcc_max_minor_ctr_ = 0;
	     UINT32 minor_index;
	     for(minor_index=0;minor_index<128;minor_index++)
		minor_ctrs_[minor_index] = rand_func()%8;
	     mcr_base_1_max_minor_ctr_ = 0;
 	     mcr_base_2_max_minor_ctr_ = 0;
	     for(minor_index=0;minor_index<64;minor_index++)
	     {
		if(minor_ctrs_[minor_index]>mcr_base_1_max_minor_ctr_)
			mcr_base_1_max_minor_ctr_ = minor_ctrs_[minor_index];
	     } 
	     for(minor_index=64;minor_index<128;minor_index++)
	     {
		if(minor_ctrs_[minor_index]>mcr_base_2_max_minor_ctr_)
			mcr_base_2_max_minor_ctr_ = minor_ctrs_[minor_index];
	     }
	     ctr_format_ = MCR_FORMAT;  
     }	
  }
  //xinw added for otp precalculation-end
 
}
bool MorphCtrBlock::CheckBlock(uint32_t expected_level)
{
	if( (zcc_major_ctr_ != 128)||
  (zcc_nzctrs_type_ != 0)||
  (zcc_nzctrs_num_ != 0)||
  (zcc_max_minor_ctr_ != 0)||
  (ctr_format_ != 0)|| 
  (ctr_is_used_ != 0)||
  (ctr_level_ != expected_level)||
  (mcr_major_ctr_ != 0)||
  (mcr_base_1_ != 0)||
  (mcr_base_2_ != 0)|| 
  (mcr_base_1_max_minor_ctr_ != 0)||
  (mcr_base_2_max_minor_ctr_ != 0)||
  (mcr_base_1_min_minor_ctr_ != 8)||
  (mcr_base_2_min_minor_ctr_ != 8))
	return false;
 return true; 
 
}
void MorphCtrBlock::SetUsed() {
  ctr_is_used_ = 1;
}

bool MorphCtrBlock::CounterIsUsed() {
  return ctr_is_used_;
}

UINT32 MorphCtrBlock::GetCtrLevel() {
  return ctr_level_;
}

void MorphCtrBlock::SetCtrLevel(const UINT32& level) {
  ctr_level_ = level;
}

bool MorphCtrBlock::GetCounterFormat() {
  return ctr_format_;
}

UINT64 MorphCtrBlock::GetEffectiveCounter(const UINT32& level, const UINT64& pos) {
 //xinw commented-begin 
 // ctr_is_used_ = 1;
 //xinw commented-end 
  //std::cout << "getec1" << std::endl;
  UINT64 effective_ctr = 0;
  if (ctr_format_ == ZCC_FORMAT) {
    effective_ctr = zcc_major_ctr_ + minor_ctrs_[pos];
    //if(ctr_level_==0)
    // printf("effective_ctr %lu by sum of zcc_major_ctr %lu and minor_ctr %u in ZCC mode\n", effective_ctr, zcc_major_ctr_, minor_ctrs_[pos]);
  } else if (ctr_format_ == MCR_FORMAT) {
    bool mcr_base_set = GetMcrBaseSet(pos);
    if (mcr_base_set == MCR_BASE_SET_1) {
      //xinw fixed a bug here: zcc_major_ctr->mcr_major_ctr-begin
      //effective_ctr = ((zcc_major_ctr_ << MCR_MAJOR_CTR_SHIFT) | mcr_base_1_) + minor_ctrs_[pos];
      effective_ctr = ((mcr_major_ctr_ << MCR_MAJOR_CTR_SHIFT) + mcr_base_1_) + minor_ctrs_[pos];
      //xinw fixed a bug here: zcc_major_ctr->mcr_major_ctr-end
    //if(ctr_level_==0)
    //  printf("effective_ctr %lu by sum of zcc_major_ctr %lu and minor_ctr %u in MCR mode\n", effective_ctr, zcc_major_ctr_, minor_ctrs_[pos]);
    } else {
      //xinw fixed a bug here: zcc_major_ctr->mcr_major_ctr-begin
      //effective_ctr = ((zcc_major_ctr_ << MCR_MAJOR_CTR_SHIFT) | mcr_base_2_) + minor_ctrs_[pos];
      effective_ctr = ((mcr_major_ctr_ << MCR_MAJOR_CTR_SHIFT) + mcr_base_2_) + minor_ctrs_[pos];
      //xinw fixed a bug here: zcc_major_ctr->mcr_major_ctr-end
   // if(ctr_level_==0)
     //  printf("effective_ctr %lu by sum of mcr_major_ctr %lu and minor_ctr %u in MCR mode\n", effective_ctr, mcr_major_ctr_, minor_ctrs_[pos]);
    }
  }
  //std::cout << "getec2" << std::endl;
  return effective_ctr;
}

UINT64 MorphCtrBlock::GetLargestEffectiveCounter()
{
	UINT64 largest_ctr=0;
	for(UINT64 i=0;i<128;i++)
	{
		if(GetEffectiveCounter(ctr_level_, i)>largest_ctr)
			largest_ctr=GetEffectiveCounter(ctr_level_, i);
	}
	return largest_ctr;
}
//xinw added for otp precomputation-begin
UINT64 MorphCtrBlock::GetMinorCounter(UINT64 effective_ctr, const UINT64& pos)
{
  UINT64 minor_ctr;
   if (ctr_format_ == ZCC_FORMAT) {
    minor_ctr=effective_ctr - zcc_major_ctr_ ;
  } else if (ctr_format_ == MCR_FORMAT) {
    bool mcr_base_set = GetMcrBaseSet(pos);
    if (mcr_base_set == MCR_BASE_SET_1) {
      minor_ctr = effective_ctr - ((mcr_major_ctr_ << MCR_MAJOR_CTR_SHIFT) | mcr_base_1_) ;
    } else {
      minor_ctr = effective_ctr - ((mcr_major_ctr_ << MCR_MAJOR_CTR_SHIFT) | mcr_base_2_) ;
    }
  }
  return minor_ctr;
}
//xinw added for otp precomputation-end

void MorphCtrBlock::MinorCountersReset(bool mcr_base_set) {
  if (ctr_format_ == ZCC_FORMAT) {
    //UINT32 largest_minor_ctr = *std::max_element(minor_ctrs_, minor_ctrs_+ZCC_MINOR_CTR_NUM);
    UINT32 largest_minor_ctr = zcc_max_minor_ctr_; 
    //zcc_major_ctr_ += (largest_minor_ctr + 1);
    zcc_major_ctr_ += (largest_minor_ctr);
    std::fill(minor_ctrs_, minor_ctrs_+ZCC_MINOR_CTR_NUM, 0);
    zcc_max_minor_ctr_ = 0;
    zcc_nzctrs_type_ = ZCC_16BIT_CTR;
    zcc_nzctrs_num_ = 0;
    if(OTP_PRECALCULATION||OTP_L1_PRECALCULATION)
    {
    	if(otp_table.get_nearest_bottom_wc(zcc_major_ctr_))
		zcc_major_ctr_=otp_table.get_nearest_bottom_wc(zcc_major_ctr_);
    }
  
    //xinw added for updating overflow fetches-begin
    /*if (warmup_status == WARMUP_OVER) {
	    overflow_fetches_stats+=128;
    }
    overflow_fetches_total+=128;
     */
     FetchForOverflow();
    //xinw added for updating overflow fetches-end
  } else if (ctr_format_ == MCR_FORMAT) {
    if (mcr_base_set == MCR_BASE_SET_1) {
      //UINT32 largest_minor_ctr = *std::max_element(minor_ctrs_, minor_ctrs_+MCR_SET_MINOR_CTR_NUM);
      UINT32 largest_minor_ctr = mcr_base_1_max_minor_ctr_; 
      mcr_base_1_ += (largest_minor_ctr);
      std::fill(minor_ctrs_, minor_ctrs_+MCR_SET_MINOR_CTR_NUM, 0);
      mcr_base_1_max_minor_ctr_ = 0;
     /* if(OTP_PRECALCULATION)
      {
	 UINT64 new_ctr=otp_table.get_nearest_bottom_wc(GetEffectiveCounter(0,0));
	 if(new_ctr)
	 {
		mcr_major_ctr_ = new_ctr>> MCR_MAJOR_CTR_SHIFT;
  		mcr_base_1_ = new_ctr & MCR_BASE_MASK;
	 }	
      }*/

    } else {
      //UINT32 largest_minor_ctr = *std::max_element(minor_ctrs_+MCR_SET_MINOR_CTR_NUM, minor_ctrs_+(2*MCR_SET_MINOR_CTR_NUM));
      UINT32 largest_minor_ctr = mcr_base_2_max_minor_ctr_; 
      mcr_base_2_ += (largest_minor_ctr); 
      std::fill(minor_ctrs_+MCR_SET_MINOR_CTR_NUM, minor_ctrs_+(2*MCR_SET_MINOR_CTR_NUM), 0);
      mcr_base_2_max_minor_ctr_ = 0;
      /*if(OTP_PRECALCULATION)
      {
	 UINT64 new_ctr=otp_table.get_nearest_bottom_wc(GetEffectiveCounter(0,127));
	 if(new_ctr)
	 {
		mcr_major_ctr_ = new_ctr >> MCR_MAJOR_CTR_SHIFT;
  		mcr_base_2_ = new_ctr & MCR_BASE_MASK;
	 }	
      }
	*/
    }
//xinw added for updating overflow fetches-begin
 /* if (warmup_status == WARMUP_OVER) {
		overflow_fetches_stats+=64;
	}
		overflow_fetches_total+=64;
*/
  if(mcr_base_set == MCR_BASE_SET_1)  
	FetchForMcrOverflow(false);
  else
	FetchForMcrOverflow(true);
		
//xinw added for updating overflow fetches-end
 
  }
}

/*
void MorphCtrBlock::SwitchFormatZccToMcr() {
  zcc_to_mcr_switches_total[ctr_level_]++;
  if (warmup_status == WARMUP_OVER) {
    zcc_to_mcr_switches_stats[ctr_level_]++;
    wc_overflow[morph_addr*128+current_pos]++;
  }
  MinorCountersReset(0); 

  mcr_major_ctr_ = zcc_major_ctr_ >> MCR_MAJOR_CTR_SHIFT;
  mcr_base_1_ = zcc_major_ctr_ & MCR_BASE_MASK;
  mcr_base_2_ = zcc_major_ctr_ & MCR_BASE_MASK;

  zcc_major_ctr_ = 0;
  zcc_nzctrs_type_ = ZCC_16BIT_CTR;
  zcc_nzctrs_num_ = 0;
  zcc_max_minor_ctr_ = 0;

  std::fill(minor_ctrs_, minor_ctrs_+ZCC_MINOR_CTR_NUM, 0);

  ctr_format_ = MCR_FORMAT;   
}
*/
void MorphCtrBlock::SwitchFormatZccToMcr() {
	zcc_to_mcr_switches_total[ctr_level_]++;
	if (warmup_status == WARMUP_OVER) {
		zcc_to_mcr_switches_stats[ctr_level_]++;
		//wc_overflow[morph_addr*128+current_pos]++;
	}
	if(zcc_max_minor_ctr_>= MCR_MAX_MINOR_CTR_VALUE)
	{
		/*
		if(debug_actual_overflow)
		{
			std::ostringstream oss;
			oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
			std::string out_file_name = oss.str();
			output_file.open(out_file_name.c_str(), ios::out|ios::app);
			output_file<<"before switching from Zcc to mcr: "<<std::endl;
			output_file.close();
			output_file.clear();
			PrintNode();
		}*/
		MinorCountersReset(0); 

		mcr_major_ctr_ = zcc_major_ctr_ >> MCR_MAJOR_CTR_SHIFT;
		mcr_base_1_ = zcc_major_ctr_ & MCR_BASE_MASK;
		mcr_base_2_ = zcc_major_ctr_ & MCR_BASE_MASK;

		zcc_major_ctr_ = 0;
		zcc_nzctrs_type_ = ZCC_16BIT_CTR;
		zcc_nzctrs_num_ = 0;
		zcc_max_minor_ctr_ = 0;

		std::fill(minor_ctrs_, minor_ctrs_+ZCC_MINOR_CTR_NUM, 0);
		if(debug_actual_overflow)
		{
			std::ostringstream oss;
			oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
			std::string out_file_name = oss.str();
			output_file.open(out_file_name.c_str(), ios::out|ios::app);
			output_file<<"after switching from Zcc to mcr: "<<std::endl;
			output_file.close();
			output_file.clear();
			PrintNode();
		}

	}
	else
	{
		//printf("switching from zcc to mcr without overflow\n");
		mcr_major_ctr_ = zcc_major_ctr_ >> MCR_MAJOR_CTR_SHIFT;
		mcr_base_1_ = zcc_major_ctr_ & MCR_BASE_MASK;
		mcr_base_2_ = zcc_major_ctr_ & MCR_BASE_MASK;
		zcc_major_ctr_ = 0;
		zcc_nzctrs_type_ = ZCC_16BIT_CTR;
		zcc_nzctrs_num_ = 0;
		zcc_max_minor_ctr_ = 0;
	}
	ctr_format_ = MCR_FORMAT;   
}

void MorphCtrBlock::SwitchToZcc(const UINT64& new_ctr) {
	/*
	if(debug_actual_overflow)
	{
		std::ostringstream oss;
		oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
		std::string out_file_name = oss.str();
		output_file.open(out_file_name.c_str(), ios::out|ios::app);
		output_file<<"before switching to zcc: "<<std::endl;
		output_file.close();
		output_file.clear();
		PrintNode();
	}
	*/
	/*
	mcr_to_zcc_switches_total[ctr_level_]++;
	if (warmup_status == WARMUP_OVER) {
		mcr_to_zcc_switches_stats[ctr_level_]++;
		wc_overflow[morph_addr*128+current_pos]++;  
	}
	*/
	bool old_format=ctr_format_;
	reset_to_zcc_switches_stats[ctr_level_]++;
	
	FetchForOverflow();
	zcc_major_ctr_=new_ctr;
	zcc_nzctrs_type_ = ZCC_16BIT_CTR;
	zcc_nzctrs_num_ = 0;
	zcc_max_minor_ctr_ = 0;


	mcr_major_ctr_ = 0;
	mcr_base_1_ = 0; 
	mcr_base_2_ = 0;
	mcr_base_1_max_minor_ctr_ = 0;
	mcr_base_2_max_minor_ctr_ = 0;
	mcr_base_1_min_minor_ctr_ = MCR_MAX_MINOR_CTR_VALUE;
	mcr_base_2_min_minor_ctr_ = MCR_MAX_MINOR_CTR_VALUE;
	std::fill(minor_ctrs_, minor_ctrs_+ZCC_MINOR_CTR_NUM, 0);
	ctr_format_ = ZCC_FORMAT;   
	

	if(debug_actual_overflow)
	{
		std::ostringstream oss;
		oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
		std::string out_file_name = oss.str();
		output_file.open(out_file_name.c_str(), ios::out|ios::app);
		if(old_format==MCR_FORMAT)
			output_file<<"after switching from mcr to zcc: "<<std::endl;
		else
			output_file<<"after switching from zcc to zcc: "<<std::endl;
			
		output_file.close();
		output_file.clear();
		PrintNode();
	}


}
void MorphCtrBlock::SwitchFormatMcrToZcc(const UINT64& pos) {
/*	if(debug_actual_overflow)
	{
		std::ostringstream oss;
		oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
		std::string out_file_name = oss.str();
		output_file.open(out_file_name.c_str(), ios::out|ios::app);
		//output_file<<"before switching from mcr to zcc: "<<std::endl;
		output_file<<"before switching to zcc: "<<std::endl;
		output_file.close();
		output_file.clear();
		PrintNode();
	}
*/
	mcr_to_zcc_switches_total[ctr_level_]++;
	if (warmup_status == WARMUP_OVER) {
		mcr_to_zcc_switches_stats[ctr_level_]++;

		current_pos=pos;
		//wc_overflow[morph_addr*128+current_pos]++;  
	}
	FetchForOverflow();
	if(!OTP_PRECALCULATION)
	{
		if(mcr_overflow)
		{
			zcc_major_ctr_ = (mcr_major_ctr_ + 2) << MCR_MAJOR_CTR_SHIFT;
		}
		else
		{
			zcc_major_ctr_ = (mcr_major_ctr_  << MCR_MAJOR_CTR_SHIFT) + mcr_base_1_;
			if(zcc_major_ctr_ <((mcr_major_ctr_  << MCR_MAJOR_CTR_SHIFT) + mcr_base_2_))
				zcc_major_ctr_ = (mcr_major_ctr_  << MCR_MAJOR_CTR_SHIFT) + mcr_base_2_;
		}
	}
	else
	{
		bool mcr_base_set = GetMcrBaseSet(pos);
		if(mcr_overflow)
		{
			if (mcr_base_set == MCR_BASE_SET_1) 
				zcc_major_ctr_ = (mcr_major_ctr_  << MCR_MAJOR_CTR_SHIFT) + mcr_base_1_;
			else
				zcc_major_ctr_ = (mcr_major_ctr_  << MCR_MAJOR_CTR_SHIFT) + mcr_base_2_;
			//if(otp_table.get_nearest_bottom_wc(zcc_major_ctr_))
			//	zcc_major_ctr_=otp_table.get_nearest_bottom_wc(zcc_major_ctr_);
		}
		else
		{
			zcc_major_ctr_ =  GetLargestEffectiveCounter();
			/*
			zcc_major_ctr_ = (mcr_major_ctr_  << MCR_MAJOR_CTR_SHIFT) + mcr_base_1_;
			if(zcc_major_ctr_ <((mcr_major_ctr_  << MCR_MAJOR_CTR_SHIFT) + mcr_base_2_))
				zcc_major_ctr_ = (mcr_major_ctr_  << MCR_MAJOR_CTR_SHIFT) + mcr_base_2_;
			*/
		}
	}
	//zcc_major_ctr_=GetLargestEffectiveCounter()+1;
	mcr_major_ctr_ = 0;
	mcr_base_1_ = 0; 
	mcr_base_2_ = 0;
	mcr_base_1_max_minor_ctr_ = 0;
	mcr_base_2_max_minor_ctr_ = 0;
	mcr_base_1_min_minor_ctr_ = MCR_MAX_MINOR_CTR_VALUE;
	mcr_base_2_min_minor_ctr_ = MCR_MAX_MINOR_CTR_VALUE;
	std::fill(minor_ctrs_, minor_ctrs_+ZCC_MINOR_CTR_NUM, 0);
	ctr_format_ = ZCC_FORMAT;   
	if(debug_actual_overflow)
	{
		std::ostringstream oss;
		oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
		std::string out_file_name = oss.str();
		output_file.open(out_file_name.c_str(), ios::out|ios::app);
		output_file<<"after switching from mcr to zcc: "<<std::endl;
		output_file.close();
		output_file.clear();
		PrintNode();
	}



}

UINT32 MorphCtrBlock::will_overflow(const UINT64& pos, bool default_clean, bool is_relevel) {
  if (ctr_format_ == ZCC_FORMAT) {
   //return  will_overflow_ZccMinorCounter(pos, default_clean, is_relevel);
   UINT32 _ret=will_overflow_ZccMinorCounter(pos, default_clean, is_relevel);
   if(debug_overflow&&_ret){
	printf("there will be overflow in ctr_level: %u, node id: %lu, ctr_format: %d, zcc_major_ctr: %lu, zcc_nzctrs_num: %u,  zcc_max_minor_ctr: %u, mcr_major_ctr: %lu, mcr_base_1: %u, mcr_base_2: %u for minor_index: %lu, with minor counter: %u, at instruction: %lu\n,  and the detailed minor ctrs: ", ctr_level_, morph_addr, ctr_format_, zcc_major_ctr_, zcc_nzctrs_num_, zcc_max_minor_ctr_,  mcr_major_ctr_, mcr_base_1_, mcr_base_2_, pos, minor_ctrs_[pos],instruction_count_total-fast_forward_value);
	for(int i=0;i<128;i++)
		printf("index: %d, value: %u, ", i, minor_ctrs_[i]);
	printf("\n");
    }
   return _ret;

  } else if (ctr_format_ == MCR_FORMAT) {
   //return will_overflow_McrMinorCounter(pos, default_clean, is_relevel);
   UINT32 _ret=will_overflow_McrMinorCounter(pos, default_clean, is_relevel);
   if(debug_overflow&&_ret){
	printf("there will be overflow in ctr_level: %u, ctr_format: %d, zcc_major_ctr: %lu, zcc_nzctrs_num: %u,  zcc_max_minor_ctr: %u, mcr_major_ctr: %lu, mcr_base_1: %u, mcr_base_2: %u for minor_index: %lu, with minor counter: %u, at instruction: %lu\n,  and the detailed minor ctrs: ", ctr_level_, ctr_format_, zcc_major_ctr_, zcc_nzctrs_num_, zcc_max_minor_ctr_,  mcr_major_ctr_, mcr_base_1_, mcr_base_2_, pos, minor_ctrs_[pos],instruction_count_total-fast_forward_value);
	for(int i=0;i<128;i++)
		printf("index: %d, value: %u, ", i, minor_ctrs_[i]);
	printf("\n");
    }
   return _ret;

  }
  return 0;
}
UINT32 MorphCtrBlock::highest_group_to_avoid_overflow(const UINT64& pos, bool default_clean) {
  if (ctr_format_ == ZCC_FORMAT) {
   return  highest_group_to_avoid_overflow_ZccMinorCounter(pos, default_clean);
  } else if (ctr_format_ == MCR_FORMAT) {
   return highest_group_to_avoid_overflow_McrMinorCounter(pos, default_clean);
  }
  return TABLE_SIZE;
}

bool MorphCtrBlock::miss_small_ctr(const UINT64& pos, bool default_clean)
{
	UINT64 default_new_ctr=GetEffectiveCounter(ctr_level_, pos);
        if(!default_clean)
		default_new_ctr+=1;
	if(default_new_ctr>=otp_table.table[OTP_ACTIVE_RELEVEL_SMALL_GROUP_NUMBER])
		return false;	
	bool hit_in_small=false;
	for(int table_index=0; table_index<OTP_ACTIVE_RELEVEL_SMALL_GROUP_NUMBER; table_index++)
	{
				if((default_new_ctr>=otp_table.table[table_index])&&(default_new_ctr<otp_table.table[table_index]+TABLE_LINE_SIZE))
					hit_in_small=true;		   
	}
	if(hit_in_small)
		return false;
	else
  		return true;
}

void MorphCtrBlock::IncrementMinorCounter(const UINT64& pos, bool default_clean, bool should_relevel) {
  ctr_is_used_ = 1;

  if (ctr_format_ == ZCC_FORMAT) {
  /*  if((ctr_level_==0)&&(instruction_count_total>680737500))	 
      {
	 printf("place 1, for minor counter index: %lu\n", pos);
	printf("the effective counters: ");
        for(UINT64 index=0;index<128;index++)
   	 printf("index: %lu, value: %lu ", index, GetEffectiveCounter(0, index));
	printf("\n");
	}
*/	
     IncrementZccMinorCounter(pos, default_clean, should_relevel);
  /*  if((ctr_level_==0)&&(instruction_count_total>680737500))	 
      {
	 printf("after place 1, for minor counter index: %lu\n", pos);
	printf("the effective counters: ");
        for(UINT64 index=0;index<128;index++)
   	 printf("index: %lu, value: %lu ", index, GetEffectiveCounter(0, index));
	printf("\n");
	}
*/	
  } else if (ctr_format_ == MCR_FORMAT) {
    //if((ctr_level_==0)&&(instruction_count_total>680737500))	 
    //	printf("place 2\n");
    IncrementMcrMinorCounter(pos, default_clean, should_relevel);
  }	


}
void MorphCtrBlock::IncrementMinorCounterForSmall(const UINT64& pos, bool default_clean, bool should_relevel, UINT32 table_index) {
  ctr_is_used_ = 1;
  ////std::cout << "ctrformat " << ctr_format_ << std::endl;
  if (ctr_format_ == ZCC_FORMAT) {
    IncrementZccMinorCounterForSmall(pos, default_clean, should_relevel, table_index);
  } else if (ctr_format_ == MCR_FORMAT) {
    IncrementMcrMinorCounterForSmall(pos, default_clean, should_relevel, table_index);
  }
}
UINT32  MorphCtrBlock::will_overflow_ZccMinorCounter(const UINT64& pos, bool default_clean, bool is_relevel) {
	//return true;
	if (minor_ctrs_[pos] == 0) {
		zcc_nzctrs_num_ += 1; 
		if (zcc_nzctrs_num_ > ZCC_NZCTRS_NUM[zcc_nzctrs_type_]) {
			if (zcc_nzctrs_type_ == ZCC_4BIT_CTR) { 
				zcc_nzctrs_num_ -= 1;  
				if(debug_overflow){
					printf("will_overflow_ZccMinorCounter, position 1\n");
				}
				if(zcc_max_minor_ctr_>= MCR_MAX_MINOR_CTR_VALUE)
					return 128;	
				else
					return 0;
			} else { 
				if(is_relevel)
				{
					minor_ctrs_[pos]=GetMinorCounter(otp_table.get_relevel_wc(GetEffectiveCounter(ctr_level_, pos), default_clean, true), pos); 
					zcc_nzctrs_type_ += 1; 
					bool default_overflow=ZccCheckForOverflows(minor_ctrs_[pos]);
					minor_ctrs_[pos]=0;
					zcc_nzctrs_type_ -= 1; 
					zcc_nzctrs_num_ -= 1; 
					if(default_overflow)
					{
						if(debug_overflow){
							printf("will_overflow_ZccMinorCounter, position 2\n");
						}


						return 128;
					}
					else
						return 0;
				}
                          	else
				{	
					minor_ctrs_[pos] = 1;	
					zcc_nzctrs_type_ += 1; 
					bool default_overflow=ZccCheckForOverflows(minor_ctrs_[pos]);
					minor_ctrs_[pos]=0;
					zcc_nzctrs_type_ -= 1; 
					zcc_nzctrs_num_ -= 1; 
					if(default_overflow)
					{
						if(debug_overflow){
							printf("will_overflow_ZccMinorCounter, position 3\n");
						}


					        return 128;
					}
					else
						return 0;

				} 
			}      
		} else {	
				if(is_relevel)
				{
					minor_ctrs_[pos]=GetMinorCounter(otp_table.get_relevel_wc(GetEffectiveCounter(ctr_level_, pos), default_clean, true), pos); 
					bool default_overflow=ZccCheckForOverflows(minor_ctrs_[pos]);
					minor_ctrs_[pos]=0;
					zcc_nzctrs_num_ -= 1; 
					if(default_overflow)
					{	
						if(debug_overflow){
							printf("will_overflow_ZccMinorCounter, position 4\n");
						}


					        return 128;
					}
					else
						return 0;


				}
                          	else
				{	
					minor_ctrs_[pos] = 1;	
					bool default_overflow=ZccCheckForOverflows(minor_ctrs_[pos]);
					minor_ctrs_[pos]=0;
					zcc_nzctrs_num_ -= 1; 
					if(default_overflow)
					{
						if(debug_overflow){
							printf("will_overflow_ZccMinorCounter, position 5\n");
						}
					        return 128;
					}
					else
						return 0;


				}  
		
		}
	} else {
		UINT32 backup_minor_ctr=minor_ctrs_[pos];	
		if(is_relevel)
		{
			minor_ctrs_[pos]=GetMinorCounter(otp_table.get_relevel_wc(GetEffectiveCounter(ctr_level_, pos), default_clean, true), pos); 
			bool default_overflow=ZccCheckForOverflows(minor_ctrs_[pos]);
			minor_ctrs_[pos]=backup_minor_ctr;
			if(default_overflow)
		{
				if(debug_overflow){
					printf("will_overflow_ZccMinorCounter, position 6\n");
				}
				return 128;
			}
			else
				return 0;


		}
		else
		{	
			minor_ctrs_[pos] = 1;	
			bool default_overflow=ZccCheckForOverflows(minor_ctrs_[pos]);
			minor_ctrs_[pos]=backup_minor_ctr;
			if(default_overflow)
			{
				if(debug_overflow){
					printf("will_overflow_ZccMinorCounter, position 7\n");
				}
				return 128;
			}
			else
				return 0;


		} 
	}
}
UINT32  MorphCtrBlock::highest_group_to_avoid_overflow_ZccMinorCounter(const UINT64& pos, bool default_clean) {
	//return true;
        UINT64 default_new_ctr=GetEffectiveCounter(ctr_level_, pos);
        if(!default_clean)
		default_new_ctr+=1;
	if(default_new_ctr>=otp_table.table[OTP_ACTIVE_RELEVEL_SMALL_GROUP_NUMBER])
		return TABLE_SIZE;	
	bool hit_in_small=false;
	for(int table_index=0; table_index<OTP_ACTIVE_RELEVEL_SMALL_GROUP_NUMBER; table_index++)
			{
				if((default_new_ctr>=otp_table.table[table_index])&&(default_new_ctr<otp_table.table[table_index]+TABLE_LINE_SIZE))
					hit_in_small=true;		   
			}
	if(hit_in_small)
		return TABLE_SIZE;
	bool skip_most_big=false;
	if (minor_ctrs_[pos] == 0) {
		zcc_nzctrs_num_ += 1; 
		if (zcc_nzctrs_num_ > ZCC_NZCTRS_NUM[zcc_nzctrs_type_]) {
		if (zcc_nzctrs_type_ == ZCC_4BIT_CTR) {
				zcc_nzctrs_num_ -= 1; 
				UINT32 most_big=TABLE_SIZE;
				for(int i=TABLE_SIZE-1;i>=0;i--)
				{
					if(otp_table.table[i]>default_new_ctr)
					{
						minor_ctrs_[pos]=GetMinorCounter(otp_table.table[i], pos); 
						UINT32 temp_max_minor_ctr=zcc_max_minor_ctr_;
						if(minor_ctrs_[pos]>temp_max_minor_ctr)
							temp_max_minor_ctr=minor_ctrs_[pos];
						bool default_overflow=false;
						if(temp_max_minor_ctr>= MCR_MAX_MINOR_CTR_VALUE)
						 	default_overflow=true;
						minor_ctrs_[pos]=0;
						if(!default_overflow)
						{
							if(skip_most_big)
							{
								return i;
							}
							else
							{
								skip_most_big=true;
								most_big=i;
							}
						}
					}
				} 
				if(skip_most_big)
				{
					return most_big;
				}
				return TABLE_SIZE;	
			} else { 
				UINT32 most_big=TABLE_SIZE;
				zcc_nzctrs_num_ -= 1;
				for(int i=TABLE_SIZE-1;i>=0;i--)
				{
					if(otp_table.table[i]>default_new_ctr)
					{
						minor_ctrs_[pos]=GetMinorCounter(otp_table.table[i], pos); 
						zcc_nzctrs_type_ += 1; 
						bool default_overflow=ZccCheckForOverflows(minor_ctrs_[pos]);
						minor_ctrs_[pos]=0;
						zcc_nzctrs_type_ -= 1; 
						if(!default_overflow)
						{
							if(skip_most_big)
							{
								return i;
							}
							else
							{
								skip_most_big=true;
								most_big=i;
							}
						}
					}
				} 
				if(skip_most_big)
				{
					return most_big;
				}
			}      

		} else {		
			zcc_nzctrs_num_ -= 1; 
			UINT32 most_big=TABLE_SIZE;
			for(int i=TABLE_SIZE-1;i>=0;i--)
			{
				minor_ctrs_[pos]=GetMinorCounter(otp_table.table[i], pos); 
				bool default_overflow=ZccCheckForOverflows(minor_ctrs_[pos]);
				minor_ctrs_[pos]=0;
				if((otp_table.table[i]>default_new_ctr)&&(!default_overflow))
				{
					if(skip_most_big)
						return i;
					else
					{
						skip_most_big=true;
						most_big=i;
					}

				}
			}
			if(skip_most_big)
				return most_big;
		}
	} else {
		UINT32 most_big=TABLE_SIZE;
		for(int i=TABLE_SIZE-1;i>=0;i--)
		{
		UINT32 backup_minor_ctr=minor_ctrs_[pos];	
		minor_ctrs_[pos]=GetMinorCounter(otp_table.table[i], pos); 
		bool default_overflow=ZccCheckForOverflows(minor_ctrs_[pos]);
		minor_ctrs_[pos]=backup_minor_ctr;	
		if((otp_table.table[i]>default_new_ctr)&&(!default_overflow))
		{
			if(skip_most_big)
				return i;
			else
			{
				skip_most_big=true;
				most_big=i;
			}
		}
		}
		if(skip_most_big)
			return most_big;
	}
	return TABLE_SIZE;
}
//void MorphCtrBlock::IncrementZccMinorCounter(const UINT64& pos) {
void MorphCtrBlock::IncrementZccMinorCounter(const UINT64& pos, bool default_clean, bool should_relevel) {
        if(debug_overflow||debug_counter_increment)
	{
		printf("\n beginning of IncrementZccMinorCounter for L%u node id: %lu: ", ctr_level_, morph_addr);
		printf("zcc_major_ctr: %lu, zcc_nzctrs_num: %u,  zcc_max_minor_ctr: %u, mcr_major_ctr: %lu, mcr_base_1: %u, mcr_base_2: %u for minor_index: %lu, with minor counter: %u, at instruction: %lu\n,  and the detailed minor ctrs: ", zcc_major_ctr_, zcc_nzctrs_num_, zcc_max_minor_ctr_,  mcr_major_ctr_, mcr_base_1_, mcr_base_2_, pos, minor_ctrs_[pos],instruction_count_total-fast_forward_value);
		for(int i=0;i<128;i++)
			printf("index: %d, value: %u, ", i, minor_ctrs_[i]);
		printf("\n");


		uint32_t _number_of_nonzero_minor_counters=0;
		for(int _index=0;_index<128;_index++)
		{
			if(minor_ctrs_[_index])
					_number_of_nonzero_minor_counters++;
		}
		assert(_number_of_nonzero_minor_counters==zcc_nzctrs_num_);
	}
	zcc_frequency_stats[ctr_level_][zcc_nzctrs_type_]++;	
	if (minor_ctrs_[pos] == 0) {
                //xinw: fix switch from zcc to mcr
		zcc_nzctrs_num_ += 1;
		if(((OTP_PRECALCULATION&&ctr_level_==0)||(OTP_L1_PRECALCULATION&&ctr_level_==1))&&!is_stack&&should_relevel)
		{	
			minor_ctrs_[pos]+=1;
			bool default_overflow=ZccCheckForOverflows(minor_ctrs_[pos]);
			minor_ctrs_[pos]-=1;

			minor_ctrs_[pos]=GetMinorCounter(otp_table.get_relevel_wc(GetEffectiveCounter(ctr_level_, pos), default_clean, default_overflow), pos); 
		}
		else
			minor_ctrs_[pos] = 1; 
		if (minor_ctrs_[pos] > zcc_max_minor_ctr_) {
					zcc_max_minor_ctr_ = minor_ctrs_[pos];
				}

		if (zcc_nzctrs_num_ > ZCC_NZCTRS_NUM[zcc_nzctrs_type_]) {
			if (zcc_nzctrs_type_ == ZCC_4BIT_CTR) { 
				current_pos=pos;	
				SwitchFormatZccToMcr(); 
				//if(OTP_PRECALCULATION&&default_clean&&ctr_level_==0&&!is_stack)
				if(((OTP_PRECALCULATION&&ctr_level_==0)||(OTP_L1_PRECALCULATION&&ctr_level_==1))&&!is_stack&&should_relevel)
				{	
					overflow_due_to_releveling_total++; 
					if (warmup_status == WARMUP_OVER) {
						overflow_due_to_releveling_stats++; 
					}
				}			
			} else { 
				if(debug_zcc_inner_switch){
					current_pos=pos;	
					PrintNode();
				}
				zcc_nzctrs_type_ += 1; 
				zcc_inner_switches_stats[ctr_level_][zcc_nzctrs_type_]++;
				if (ZccCheckForOverflows(0)) {	
				/*	if(debug_actual_overflow)
					{
						std::ostringstream oss;
						oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
						std::string out_file_name = oss.str();
						output_file.open(out_file_name.c_str(), ios::out|ios::app);
						output_file<<"before switching within zcc: "<<std::endl;
						output_file.close();
						output_file.clear();
						PrintNode();
					}
				*/
		
					MinorCountersReset(0);
					if(debug_actual_overflow)
					{
						std::ostringstream oss;
						oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
						std::string out_file_name = oss.str();
						output_file.open(out_file_name.c_str(), ios::out|ios::app);
						output_file<<"after switching within zcc: "<<std::endl;
						output_file.close();
						output_file.clear();
						PrintNode();
					}
				}
				if(debug_zcc_inner_switch){
					PrintNode();
				}
			}      
		} /*else { 
			if(OTP_PRECALCULATION&&ctr_level_==0&&!is_stack&&should_relevel)
			{
					minor_ctrs_[pos]+=1;
					bool default_overflow=ZccCheckForOverflows(minor_ctrs_[pos]);
					minor_ctrs_[pos]-=1;
				minor_ctrs_[pos]=GetMinorCounter(otp_table.get_relevel_wc(GetEffectiveCounter(ctr_level_, pos), default_clean, default_overflow), pos); 
			}
			else 
				minor_ctrs_[pos] = 1;   
			if (minor_ctrs_[pos] > zcc_max_minor_ctr_) {
				zcc_max_minor_ctr_ = minor_ctrs_[pos];
			}
		}*/
	} else {
		//if(OTP_PRECALCULATION&&ctr_level_==0&&!is_stack&&should_relevel)
		if(((OTP_PRECALCULATION&&ctr_level_==0)||(OTP_L1_PRECALCULATION&&ctr_level_==1))&&!is_stack&&should_relevel)
		{
				minor_ctrs_[pos]+=1;
				bool default_overflow=ZccCheckForOverflows(minor_ctrs_[pos]);
				minor_ctrs_[pos]-=1;

			minor_ctrs_[pos]=GetMinorCounter(otp_table.get_relevel_wc(GetEffectiveCounter(ctr_level_, pos), default_clean, default_overflow), pos); 
			
		}
		else 
			minor_ctrs_[pos]++; 
		//xinw added for otp precomputation-end
		if (minor_ctrs_[pos] > zcc_max_minor_ctr_) {
			zcc_max_minor_ctr_ = minor_ctrs_[pos];
		}
		////std::cout << "incremented" << std::endl;
		if (ZccCheckForOverflows(minor_ctrs_[pos])) {	
		/*	if(debug_actual_overflow)
			{
				std::ostringstream oss;
				oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
				std::string out_file_name = oss.str();
				output_file.open(out_file_name.c_str(), ios::out|ios::app);
				output_file<<"before overflow within zcc: "<<std::endl;
				output_file.close();
				output_file.clear();
				PrintNode();
			}
		*/


			MinorCountersReset(0);
			if(debug_actual_overflow)
			{
				std::ostringstream oss;
				oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
				std::string out_file_name = oss.str();
				output_file.open(out_file_name.c_str(), ios::out|ios::app);
				output_file<<"after overflow within zcc: "<<std::endl;
				output_file.close();
				output_file.clear();
				PrintNode();
			}


		}
		//std::cout << "izmc2n2" << std::endl;
	}
        if(debug_overflow||debug_counter_increment)
	{
		printf("\n end of IncrementZccMinorCounter for L%u node id: %lu: ", ctr_level_, morph_addr);
		printf("zcc_major_ctr: %lu, zcc_nzctrs_num: %u,  zcc_max_minor_ctr: %u, mcr_major_ctr: %lu, mcr_base_1: %u, mcr_base_2: %u for minor_index: %lu, with minor counter: %u, at instruction: %lu\n,  and the detailed minor ctrs: ", zcc_major_ctr_, zcc_nzctrs_num_, zcc_max_minor_ctr_,  mcr_major_ctr_, mcr_base_1_, mcr_base_2_, pos, minor_ctrs_[pos],instruction_count_total-fast_forward_value);
		for(int i=0;i<128;i++)
			printf("index: %d, value: %u, ", i, minor_ctrs_[i]);
		printf("\n");


		uint32_t _number_of_nonzero_minor_counters=0;
		for(int _index=0;_index<128;_index++)
		{
			if(minor_ctrs_[_index])
					_number_of_nonzero_minor_counters++;
		}
		//assert(_number_of_nonzero_minor_counters==zcc_nzctrs_num_);
	}
}
void MorphCtrBlock::IncrementZccMinorCounterForSmall(const UINT64& pos, bool default_clean, bool should_relevel, UINT32 table_index) {
	UINT64 default_new_ctr=GetEffectiveCounter(ctr_level_, pos);
	if(!default_clean)
		default_new_ctr+=1;
	if(default_new_ctr<otp_table.table[0])
		relevel_reason=RELEVEL_FOR_SMALLER_VALUE_THAN_MIN;
	else
	{
	relevel_reason=RELEVEL_FOR_VALUE_IN_MEDIATE;
	for(int table_index=0; table_index<OTP_ACTIVE_RELEVEL_SMALL_GROUP_NUMBER; table_index++)
	{
		if((default_new_ctr>=otp_table.table[table_index])&&(default_new_ctr<otp_table.table[table_index]+TABLE_LINE_SIZE))
			relevel_reason=RELEVEL_FOR_VALUE_IN_SMALLEST;		   
	}
	}
	if (minor_ctrs_[pos] == 0) {
		zcc_nzctrs_num_ += 1; 
		if (zcc_nzctrs_num_ > ZCC_NZCTRS_NUM[zcc_nzctrs_type_]) {
			minor_ctrs_[pos]=GetMinorCounter(otp_table.table[table_index], pos); 
				if (minor_ctrs_[pos] > zcc_max_minor_ctr_) {
					zcc_max_minor_ctr_ = minor_ctrs_[pos];
				}
				
			if (zcc_nzctrs_type_ == ZCC_4BIT_CTR) { 
				current_pos=pos;
				SwitchFormatZccToMcr(); 
					
			} else { 
				zcc_nzctrs_type_ += 1; 
				if (ZccCheckForOverflows(0)) {
					MinorCountersReset(0);
				}
			}      
		} else { 
			minor_ctrs_[pos]=GetMinorCounter(otp_table.table[table_index], pos); 
			if (minor_ctrs_[pos] > zcc_max_minor_ctr_) {
				zcc_max_minor_ctr_ = minor_ctrs_[pos];
			}
		}
	} else {
		minor_ctrs_[pos]=GetMinorCounter(otp_table.table[table_index], pos); 
		if (minor_ctrs_[pos] > zcc_max_minor_ctr_) {
			zcc_max_minor_ctr_ = minor_ctrs_[pos];
		}
		if (ZccCheckForOverflows(minor_ctrs_[pos])) {
			MinorCountersReset(0);
		}
	}
}


bool MorphCtrBlock::GetMcrBaseSet(const UINT64& pos) {
  bool base_set = MCR_BASE_SET_1;
  if (pos > 63) { 
    base_set = MCR_BASE_SET_2;
  }
  
  return base_set;
}
UINT32 MorphCtrBlock::highest_group_to_avoid_overflow_McrMinorCounter(const UINT64& pos, bool default_clean) { 
  //return true; 
	UINT64 default_new_ctr=GetEffectiveCounter(ctr_level_, pos);
	if(!default_clean)
		default_new_ctr+=1;
	if(default_new_ctr>=otp_table.table[OTP_ACTIVE_RELEVEL_SMALL_GROUP_NUMBER])
		return TABLE_SIZE;	
	bool hit_in_small=false;
	for(int table_index=0; table_index<OTP_ACTIVE_RELEVEL_SMALL_GROUP_NUMBER; table_index++)
			{
				if((default_new_ctr>=otp_table.table[table_index])&&(default_new_ctr<otp_table.table[table_index]+TABLE_LINE_SIZE))
					hit_in_small=true;		   
			}
	if(hit_in_small)
		return TABLE_SIZE;
bool skip_most_big=false;
UINT32 most_big=TABLE_SIZE;
for(int i=TABLE_SIZE-1;i>=0;i--)
{
	
					
  bool base_set = GetMcrBaseSet(pos); 
  UINT32 backup_minor_ctrs_pos=minor_ctrs_[pos];
  UINT32 backup_mcr_base_1_=mcr_base_1_;
  UINT32 backup_mcr_base_2_=mcr_base_2_; 
  UINT32 backup_mcr_base_1_max_minor_ctr_=mcr_base_1_max_minor_ctr_;
  UINT32 backup_mcr_base_2_max_minor_ctr_=mcr_base_2_max_minor_ctr_;
  UINT32 minor_ctr = 0;
  bool ret=false;	
  minor_ctrs_[pos]=GetMinorCounter(otp_table.table[i], pos); 
  minor_ctr = minor_ctrs_[pos];
  if (base_set == MCR_BASE_SET_1) {
    if (minor_ctr > mcr_base_1_max_minor_ctr_)  
      mcr_base_1_max_minor_ctr_ = minor_ctr;
  }
  else
   {
    if (minor_ctr > mcr_base_2_max_minor_ctr_)  
      mcr_base_2_max_minor_ctr_ = minor_ctr;
         
   }
  if (McrCheckForOverflows(base_set, minor_ctr)) {
	  UINT32 smallest_minor_ctr = McrGetSmallestMinorCounter(base_set); 
	  if ((minor_ctr-smallest_minor_ctr)<MCR_MAX_MINOR_CTR_VALUE) {
		  if (base_set == MCR_BASE_SET_1) {
			  mcr_base_1_ += smallest_minor_ctr;
		  } else {
			  mcr_base_2_ += smallest_minor_ctr; 
		  }
		  if (mcr_base_1_ >= MCR_MAX_BASE_VALUE || mcr_base_2_ >= MCR_MAX_BASE_VALUE) {
			  ret=true;
		  } 	   
	  }
	  else
	  {
		  ret=true;	
	  }	
  } 
  minor_ctrs_[pos]=backup_minor_ctrs_pos;
  mcr_base_1_= backup_mcr_base_1_;
  mcr_base_2_= backup_mcr_base_2_; 
  mcr_base_1_max_minor_ctr_= backup_mcr_base_1_max_minor_ctr_;
  mcr_base_2_max_minor_ctr_= backup_mcr_base_2_max_minor_ctr_;
  if((otp_table.table[i]>default_new_ctr)&&(!ret))
  {
	  if(skip_most_big)
		  return i;
	  else
	  {
		  skip_most_big=true;
		  most_big=i;
	  }
  }

}
	if(skip_most_big)
		return most_big;
 	return TABLE_SIZE;
}

UINT32 MorphCtrBlock::will_overflow_McrMinorCounter(const UINT64& pos, bool default_clean, bool is_relevel) { 
  //return true;
  bool base_set = GetMcrBaseSet(pos); 
  UINT32 backup_minor_ctrs_pos=minor_ctrs_[pos];
  UINT32 backup_mcr_base_1_=mcr_base_1_;
  UINT32 backup_mcr_base_2_=mcr_base_2_; 
  UINT32 backup_mcr_base_1_max_minor_ctr_=mcr_base_1_max_minor_ctr_;
  UINT32 backup_mcr_base_2_max_minor_ctr_=mcr_base_2_max_minor_ctr_;
  UINT32 minor_ctr = 0;
  UINT32 ret=0;
  if(is_relevel)
  {
	  minor_ctrs_[pos]=GetMinorCounter(otp_table.get_relevel_wc(GetEffectiveCounter(ctr_level_, pos), default_clean, true), pos); 
  }
  else 
  {
	  minor_ctrs_[pos]++;
  }
  minor_ctr = minor_ctrs_[pos];
  if (base_set == MCR_BASE_SET_1) {
    if (minor_ctr > mcr_base_1_max_minor_ctr_)  
      mcr_base_1_max_minor_ctr_ = minor_ctr;
  }
  else
   {
    if (minor_ctr > mcr_base_2_max_minor_ctr_)  
      mcr_base_2_max_minor_ctr_ = minor_ctr;
         
   }
  if (McrCheckForOverflows(base_set, minor_ctr)) {
	  UINT32 smallest_minor_ctr = McrGetSmallestMinorCounter(base_set); 
	  if ((minor_ctr-smallest_minor_ctr)<MCR_MAX_MINOR_CTR_VALUE) {
		  if (base_set == MCR_BASE_SET_1) {
			  mcr_base_1_ += smallest_minor_ctr;
		  } else {
			  mcr_base_2_ += smallest_minor_ctr; 
		  }   
	  }
	  else
	  {
		  ret=128;
		  if (base_set == MCR_BASE_SET_1) {
			  mcr_base_1_ += minor_ctr;
		  } else {
			  mcr_base_2_ += minor_ctr; 
		  } 	
	  }	
	/*
	  if (mcr_base_1_ >= MCR_MAX_BASE_VALUE || mcr_base_2_ >= MCR_MAX_BASE_VALUE) {
			  ret+=128;
		  } 		
	*/
  } 

  if (mcr_base_1_ >= MCR_MAX_BASE_VALUE || mcr_base_2_ >= MCR_MAX_BASE_VALUE) {
	  ret=128;
  } 		


  minor_ctrs_[pos]=backup_minor_ctrs_pos;
  mcr_base_1_= backup_mcr_base_1_;
  mcr_base_2_= backup_mcr_base_2_; 
  mcr_base_1_max_minor_ctr_= backup_mcr_base_1_max_minor_ctr_;
  mcr_base_2_max_minor_ctr_= backup_mcr_base_2_max_minor_ctr_;
  
  return ret;
}

//void MorphCtrBlock::IncrementMcrMinorCounter(const UINT64& pos) { 
void MorphCtrBlock::IncrementMcrMinorCounter(const UINT64& pos, bool default_clean, bool should_relevel) { 
 	if(debug_counter_increment)
	{
		printf("\n beginning of IncrementMcrMinorCounter for L%u node id: %lu: ", ctr_level_, morph_addr);
		printf("zcc_major_ctr: %lu, zcc_nzctrs_num: %u,  zcc_max_minor_ctr: %u, mcr_major_ctr: %lu, mcr_base_1: %u, mcr_base_2: %u for minor_index: %lu, with minor counter: %u, at instruction: %lu\n,  and the detailed minor ctrs: ", zcc_major_ctr_, zcc_nzctrs_num_, zcc_max_minor_ctr_,  mcr_major_ctr_, mcr_base_1_, mcr_base_2_, pos, minor_ctrs_[pos],instruction_count_total-fast_forward_value);
		for(int i=0;i<128;i++)
			printf("index: %d, value: %u, ", i, minor_ctrs_[i]);
		printf("\n");
	}

  mcr_frequency_stats[ctr_level_]++;
  //std::cout << "icmmc1 " << pos << std::endl;
  //if(ctr_level_==2)
    // printf("increment mcr counter in L2 node, position %lu\n", pos);
  bool base_set = GetMcrBaseSet(pos);   
  UINT32 minor_ctr = 0;

  //xinw commented for otp precomputation-begin
  // minor_ctr = ++minor_ctrs_[pos]; 
  //xinw commented for otp precomputation-end
  //xinw added for otp precomputation-begin 
  //if(OTP_PRECALCULATION&&ctr_level_==0&&!is_stack&&should_relevel)
  if(((OTP_PRECALCULATION&&ctr_level_==0)||(OTP_L1_PRECALCULATION&&ctr_level_==1))&&!is_stack&&should_relevel)
	{
	  	minor_ctrs_[pos]+=1;
		//UINT32 default_new_ctr=minor_ctrs_[pos];
		bool default_overflow=McrCheckForOverflows(base_set, minor_ctrs_[pos]);
		minor_ctrs_[pos]-=1;
	        minor_ctrs_[pos]=GetMinorCounter(otp_table.get_relevel_wc(GetEffectiveCounter(ctr_level_, pos), default_clean, default_overflow), pos); 
	}
  else 
	  minor_ctrs_[pos]++;
  minor_ctr = minor_ctrs_[pos];
 // if(ctr_level_==0)
//	printf("without otp precomputation, write counter is: %lu\n", GetEffectiveCounter(ctr_level_, pos));
  //xinw added for otp precomputation-end
   
  if (base_set == MCR_BASE_SET_1) {
    if (minor_ctr > mcr_base_1_max_minor_ctr_)  
      mcr_base_1_max_minor_ctr_ = minor_ctr;
    
  }
  else
   //xinw fixed the bug here: lack of bracket which makes the logic wrong! 
   {
    if (minor_ctr > mcr_base_2_max_minor_ctr_)  
      mcr_base_2_max_minor_ctr_ = minor_ctr;
         
   }
  //std::cout << "icmmc2 " << minor_ctr << std::endl;

  if (McrCheckForOverflows(base_set, minor_ctr)) {
   if(!OTP_PRECALCULATION)
   {
    UINT32 smallest_minor_ctr = McrGetSmallestMinorCounter(base_set); 
    //std::cout << "icmmc2y " << smallest_minor_ctr << std::endl;
  //  if(ctr_level_==2)
//	printf("level 2 node minor counter is larger or equal to 8\n");
    if (smallest_minor_ctr > 0) {
	    McrDoRebasing(base_set, smallest_minor_ctr); 
    } else {

	    //xinw: deleted the overflow within mcr, directly switch from mcr to zcc
	    if(mcr_overflow)
	    {
		    MinorCountersReset(base_set); 
		    //xinw added-begin
		    mcr_counter_overflows_total[ctr_level_]++;
		    if (warmup_status == WARMUP_OVER) {
			    mcr_counter_overflows_stats[ctr_level_]++;
			    current_pos=pos;
			    //wc_overflow[morph_addr*128+current_pos]++;

		    }
	    }
	    else
	    {
		    /*
		    if (base_set == MCR_BASE_SET_1) {
			    mcr_base_1_+=minor_ctrs_[pos];
		    }
		    else{
			    mcr_base_2_+=minor_ctrs_[pos];
		    }
		    */
		    SwitchFormatMcrToZcc(pos);
		    //xinw added-end
	    }
    }
   }
   //xinw added for otp precomputation-begin
   else
   {
            UINT32 smallest_minor_ctr = McrGetSmallestMinorCounter(base_set); 
	    if ((minor_ctr-smallest_minor_ctr)<MCR_MAX_MINOR_CTR_VALUE) {
		    McrDoRebasing(base_set, smallest_minor_ctr); 
	    } else {
		if(mcr_overflow)
		{ 
		    MinorCountersReset(base_set); 
		    mcr_counter_overflows_total[ctr_level_]++;
		    if (warmup_status == WARMUP_OVER) {
			    mcr_counter_overflows_stats[ctr_level_]++;
      			    //wc_overflow[morph_addr]++;
		    }
		}
		else
		{
			/*
			if (base_set == MCR_BASE_SET_1) {
				mcr_base_1_+=minor_ctrs_[pos];
			}
			else{
				mcr_base_2_+=minor_ctrs_[pos];
			}*/
			SwitchFormatMcrToZcc(pos);

		}
	    }
	
   }
   //xinw added for otp precomputation-end
  }
    
  //std::cout << "icmmc3" << std::endl;
  if (mcr_base_1_ >= MCR_MAX_BASE_VALUE || mcr_base_2_ >= MCR_MAX_BASE_VALUE) {

    SwitchFormatMcrToZcc(pos);
  } 
	 if(debug_counter_increment)
	{
		printf("\n beginning of IncrementMcrMinorCounter for L%u node id: %lu: ", ctr_level_, morph_addr);
		printf("zcc_major_ctr: %lu, zcc_nzctrs_num: %u,  zcc_max_minor_ctr: %u, mcr_major_ctr: %lu, mcr_base_1: %u, mcr_base_2: %u for minor_index: %lu, with minor counter: %u, at instruction: %lu\n,  and the detailed minor ctrs: ", zcc_major_ctr_, zcc_nzctrs_num_, zcc_max_minor_ctr_,  mcr_major_ctr_, mcr_base_1_, mcr_base_2_, pos, minor_ctrs_[pos],instruction_count_total-fast_forward_value);
		for(int i=0;i<128;i++)
			printf("index: %d, value: %u, ", i, minor_ctrs_[i]);
		printf("\n");
	}
  
}
void MorphCtrBlock::IncrementMcrMinorCounterForSmall(const UINT64& pos, bool default_clean, bool should_relevel, UINT32 table_index) { 
	UINT64 default_new_ctr=GetEffectiveCounter(ctr_level_, pos);
	if(!default_clean)
		default_new_ctr+=1;
	if(default_new_ctr<otp_table.table[0])
		relevel_reason=RELEVEL_FOR_SMALLER_VALUE_THAN_MIN;
	else
	{
		relevel_reason=RELEVEL_FOR_VALUE_IN_MEDIATE;
		for(int table_index=0; table_index<OTP_ACTIVE_RELEVEL_SMALL_GROUP_NUMBER; table_index++)
		{
			if((default_new_ctr>=otp_table.table[table_index])&&(default_new_ctr<otp_table.table[table_index]+TABLE_LINE_SIZE))
				relevel_reason=RELEVEL_FOR_VALUE_IN_SMALLEST;		   
		}
	}

  
  bool base_set = GetMcrBaseSet(pos);   
  UINT32 minor_ctr = 0;
  minor_ctrs_[pos]=GetMinorCounter(otp_table.table[table_index], pos); 
  minor_ctr = minor_ctrs_[pos];
  if (base_set == MCR_BASE_SET_1) {
    if (minor_ctr > mcr_base_1_max_minor_ctr_)  
      mcr_base_1_max_minor_ctr_ = minor_ctr;
    
  }
  else
   {
    if (minor_ctr > mcr_base_2_max_minor_ctr_)  
      mcr_base_2_max_minor_ctr_ = minor_ctr;
   }
/*
  if (McrCheckForOverflows(base_set, minor_ctr)) {
   if(!OTP_PRECALCULATION)
   {
    UINT32 smallest_minor_ctr = McrGetSmallestMinorCounter(base_set); 
    if (smallest_minor_ctr > 0) {
      McrDoRebasing(base_set, smallest_minor_ctr); 
    } else { 
      MinorCountersReset(base_set); 
    mcr_counter_overflows_total[ctr_level_]++;
    if (warmup_status == WARMUP_OVER) {
      mcr_counter_overflows_stats[ctr_level_]++;
      //wc_overflow[morph_addr]++;
    }
    }
   }
   else
   {
            UINT32 smallest_minor_ctr = McrGetSmallestMinorCounter(base_set); 
	    if ((minor_ctr-smallest_minor_ctr)<MCR_MAX_MINOR_CTR_VALUE) {
		    McrDoRebasing(base_set, smallest_minor_ctr); 
	    } else { 
		    MinorCountersReset(base_set); 
		    mcr_counter_overflows_total[ctr_level_]++;
		    if (warmup_status == WARMUP_OVER) {
			    mcr_counter_overflows_stats[ctr_level_]++;
      			    //wc_overflow[morph_addr]++;
		    }
	    }
	
   }
  }
*/
 if (McrCheckForOverflows(base_set, minor_ctr)) {
   if(!OTP_PRECALCULATION)
   {
    UINT32 smallest_minor_ctr = McrGetSmallestMinorCounter(base_set); 
    //std::cout << "icmmc2y " << smallest_minor_ctr << std::endl;
  //  if(ctr_level_==2)
//	printf("level 2 node minor counter is larger or equal to 8\n");
    if (smallest_minor_ctr > 0) {
	    McrDoRebasing(base_set, smallest_minor_ctr); 
    } else {

	    //xinw: deleted the overflow within mcr, directly switch from mcr to zcc
	    if(mcr_overflow)
	    {
		    MinorCountersReset(base_set); 
		    //xinw added-begin
		    mcr_counter_overflows_total[ctr_level_]++;
		    if (warmup_status == WARMUP_OVER) {
			    mcr_counter_overflows_stats[ctr_level_]++;
			    current_pos=pos;
			    //wc_overflow[morph_addr*128+current_pos]++;

		    }
	    }
	    else
	    {
		    if (base_set == MCR_BASE_SET_1) {
			    mcr_base_1_+=minor_ctrs_[pos];
		    }
		    else{
			    mcr_base_2_+=minor_ctrs_[pos];
		    }

		    SwitchFormatMcrToZcc(pos);
		    //xinw added-end
	    }
    }
   }
   //xinw added for otp precomputation-begin
   else
   {
            UINT32 smallest_minor_ctr = McrGetSmallestMinorCounter(base_set); 
	    if ((minor_ctr-smallest_minor_ctr)<MCR_MAX_MINOR_CTR_VALUE) {
		    McrDoRebasing(base_set, smallest_minor_ctr); 
	    } else {
		if(mcr_overflow)
		{ 
		    MinorCountersReset(base_set); 
		    mcr_counter_overflows_total[ctr_level_]++;
		    if (warmup_status == WARMUP_OVER) {
			    mcr_counter_overflows_stats[ctr_level_]++;
      			    //wc_overflow[morph_addr]++;
		    }
		}
		else
		{
			if (base_set == MCR_BASE_SET_1) {
				mcr_base_1_+=minor_ctrs_[pos];
			}
			else{
				mcr_base_2_+=minor_ctrs_[pos];
			}
			SwitchFormatMcrToZcc(pos);

		}
	    }
	
   }
   //xinw added for otp precomputation-end
  }
  
  if (mcr_base_1_ >= MCR_MAX_BASE_VALUE || mcr_base_2_ >= MCR_MAX_BASE_VALUE) {
    SwitchFormatMcrToZcc(pos);
  } 
}


void MorphCtrBlock::McrDoRebasing(bool mcr_base_set, const UINT32& smallest_minor_ctr) { 
  NumMcrReBase[ctr_level_]++;
  if (mcr_base_set == MCR_BASE_SET_1) {
    //xinw fixed the bug here: it should be operator '+=' here, not '=+', that's much difference!
    mcr_base_1_ += smallest_minor_ctr;
    //xinw fixed the bug of updating largest minor counter
    mcr_base_1_max_minor_ctr_=0;
    //xinw fixed the bug of updating largest minor counter
    
    for (UINT32 i=0; i < MCR_SET_MINOR_CTR_NUM; ++i) {
      minor_ctrs_[i] -= smallest_minor_ctr;  
    //xinw fixed the bug of updating largest minor counter
    if(minor_ctrs_[i]>mcr_base_1_max_minor_ctr_)
	mcr_base_1_max_minor_ctr_=minor_ctrs_[i];
    //xinw fixed the bug of updating largest minor counter
    
    }
  } else {
    mcr_base_2_ += smallest_minor_ctr; 
    //xinw fixed the bug of updating largest minor counter
    mcr_base_2_max_minor_ctr_=0;
    //xinw fixed the bug of updating largest minor counter
    
    for (UINT32 i=MCR_SET_MINOR_CTR_NUM; i < (2*MCR_SET_MINOR_CTR_NUM); ++i) {
      minor_ctrs_[i] -= smallest_minor_ctr;  
    //xinw fixed the bug of updating largest minor counter
    if(minor_ctrs_[i]>mcr_base_2_max_minor_ctr_)
	mcr_base_2_max_minor_ctr_=minor_ctrs_[i];
    //xinw fixed the bug of updating largest minor counter
    
    }
  }
}


UINT32 MorphCtrBlock::McrGetSmallestMinorCounter(bool mcr_base_set) {

  UINT32 smallest_minor_ctr = 0;
  if (mcr_base_set == MCR_BASE_SET_1) { 
      smallest_minor_ctr = *std::min_element(minor_ctrs_,minor_ctrs_+MCR_SET_MINOR_CTR_NUM);
  } else { 
      smallest_minor_ctr = *std::min_element(minor_ctrs_+MCR_SET_MINOR_CTR_NUM,minor_ctrs_+(2*MCR_SET_MINOR_CTR_NUM));
  }

  return smallest_minor_ctr;
}


bool MorphCtrBlock::ZccCheckForOverflows(const UINT32& minor_ctr) {
  //UINT32 largest_minor_ctr = (minor_ctr) ? minor_ctr : *std::max_element(minor_ctrs_, minor_ctrs_+ZCC_MINOR_CTR_NUM);
  //UINT32 largest_minor_ctr = (minor_ctr) ? minor_ctr : zcc_max_minor_ctr_;
  //note: fix ZccCheckForOverflows
  UINT32 largest_minor_ctr;
  if(minor_ctr>zcc_max_minor_ctr_)
	largest_minor_ctr = minor_ctr;
  else
	largest_minor_ctr = zcc_max_minor_ctr_;
  //std::cout << "ZCCoverflow " << largest_minor_ctr << " " << (2^ZCC_NZCTRS_SIZES[zcc_nzctrs_type_]) << std::endl; 
  //xinw fixed the bug of "power()"--'^'is actually xor not power
  //if (largest_minor_ctr >= (2^ZCC_NZCTRS_SIZES[zcc_nzctrs_type_])) { 
  if (largest_minor_ctr >= ((UINT32)1<<ZCC_NZCTRS_SIZES[zcc_nzctrs_type_])) { 
    zcc_counter_overflows_total[ctr_level_]++;
    if (warmup_status == WARMUP_OVER) {
      zcc_counter_overflows_stats[ctr_level_]++;
      zcc_detailed_overflow_stats[ctr_level_][zcc_nzctrs_type_]++;
      if(minor_ctr==0)
	zcc_inner_switches_overflow_stats[ctr_level_][zcc_nzctrs_type_]++;
    //xinw added for wc_overflow
     for (UINT32 i=0; i < 128; ++i) {
     	 if(minor_ctrs_[i]==largest_minor_ctr)
		 current_pos=i;
      }   
      //wc_overflow[morph_addr*128+current_pos]++;
    }
    return TRUE;
  }
  return FALSE;
}


bool MorphCtrBlock::McrCheckForOverflows(bool mcr_base_set, const UINT32& minor_ctr) {
  UINT32 largest_minor_ctr = 0;
  if (mcr_base_set == MCR_BASE_SET_1) { 
      //largest_minor_ctr = (minor_ctr) ? minor_ctr : *std::max_element(minor_ctrs_,minor_ctrs_+MCR_SET_MINOR_CTR_NUM);
      largest_minor_ctr = (minor_ctr) ? minor_ctr : mcr_base_1_max_minor_ctr_;
  } else { 
      //largest_minor_ctr = (minor_ctr) ? minor_ctr : *std::max_element(minor_ctrs_+MCR_SET_MINOR_CTR_NUM,minor_ctrs_+(2*MCR_SET_MINOR_CTR_NUM));
      largest_minor_ctr = (minor_ctr) ? minor_ctr : mcr_base_2_max_minor_ctr_; 
  }

  if (largest_minor_ctr >= MCR_MAX_MINOR_CTR_VALUE) { 
//xinw commented-begin
/*
    mcr_counter_overflows_total[ctr_level_]++;
    if (warmup_status == WARMUP_OVER) {
      mcr_counter_overflows_stats[ctr_level_]++;
    }
*/
//xinw commented-end
    return TRUE;
  }
  return FALSE;
}

//xinw added for overflow_fetch_stats-begin
void MorphCtrBlock::SetSubNodeAddr(UINT64 sub_node_addr)
{
	sub_node_begin_addr = sub_node_addr;
}
void MorphCtrBlock::SetZccMajorCounter(UINT64 _major_ctr)
{
	zcc_major_ctr_ = _major_ctr;
}
void MorphCtrBlock::SetMorphAddr(UINT64 _morph_addr)
{
	morph_addr=_morph_addr;
}
void MorphCtrBlock::PrintNode()
{
	if(ctr_level_!=2)
		return;
	std::ostringstream oss;
	oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
	std::string out_file_name = oss.str();
	output_file.open(out_file_name.c_str(), ios::out|ios::app);
	//output_file <<"debugging on overflow:  "<<std::endl;
        //output_file << "ctr_level: "<<ctr_level_<<" node id: "<<morph_addr<<" ctr_format: "<<ctr_format_<<" zcc_major_ctr:  "<<zcc_major_ctr_<<" mcr_major_ctr: "<<mcr_major_ctr_<<" mcr_base_1: "<<mcr_base_1_ <<" mcr_base_2: "<<mcr_base_2_<<" and the minor ctrs are: "<<std::endl;
  /*	if(GetLargestEffectiveCounter()<otp_table.table[0])
		output_file<<" biggest value smaller than smallest value in table"<<std::endl;
	else
		output_file<<" biggest value not smaller than smallest value in table"<<std::endl;
*/			
       output_file << "ctr_level: "<<ctr_level_<<" node id: "<<morph_addr<<" , pos: "<<current_pos<<" ctr_format: "<<ctr_format_<<" zcc_major_ctr:  "<<zcc_major_ctr_<<" mcr_major_ctr: "<<mcr_major_ctr_<<" mcr_base_1: "<<mcr_base_1_ <<" mcr_base_2: "<<mcr_base_2_<<" at instruction  "<<(instruction_count_total-fast_forward_value)<<" with total overflow traffic: "<<overflow_fetches_stats <<" and the minor counters are: "<<std::endl;
	for(int i=0;i<128;i++)
		output_file<<" index: "<<i<<" value: "<<minor_ctrs_[i]<<std::endl;
	output_file.close();
	output_file.clear();
}
void MorphCtrBlock::PrintSimpleNode()
{
	std::ostringstream oss;
	oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
	std::string out_file_name = oss.str();
	output_file.open(out_file_name.c_str(), ios::out|ios::app);
	//output_file << "unnormal effective counter: "<<_effective_ctr<<" , for block address: "<<current_block_addr_access_aes_table<<std::endl;
        output_file << "ctr_level: "<<ctr_level_<<" node id: "<<morph_addr<<" ctr_format: "<<ctr_format_<<" zcc_major_ctr:  "<<zcc_major_ctr_<<" mcr_major_ctr: "<<mcr_major_ctr_<<" mcr_base_1: "<<mcr_base_1_ <<" mcr_base_2: "<<mcr_base_2_<<" mcr_base_1_max_minor_ctr: "<<mcr_base_1_max_minor_ctr_<<" mcr_base_2_max_minor_ctr: "<<mcr_base_2_max_minor_ctr_<<std::endl;
	output_file.close();
	output_file.clear();
}

void MorphCtrBlock::RedirectPrintNode(UINT64 pos)
{
	printf("ctr_level: %u, node id: %lu, ctr_format: %d, zcc_major_ctr: %lu, zcc_nzctrs_num: %u,  zcc_max_minor_ctr: %u, mcr_major_ctr: %lu, mcr_base_1: %u, mcr_base_2: %u for minor_index: %lu, with minor counter: %u, at instruction: %lu\n,  and the detailed minor ctrs: ", ctr_level_, morph_addr, ctr_format_, zcc_major_ctr_, zcc_nzctrs_num_, zcc_max_minor_ctr_,  mcr_major_ctr_, mcr_base_1_, mcr_base_2_, pos, minor_ctrs_[pos],instruction_count_total-fast_forward_value);
	for(int i=0;i<128;i++)
		printf("index: %d, value: %u, ", i, minor_ctrs_[i]);
	printf("\n");

}
/*
void MorphCtrBlock::FetchForOverflow()
{
    if(ctr_level_>=1)
    {
	   UINT64 begin_wc_block_addr = sub_node_begin_addr;
	   UINT64 curr_wc_block_addr;
           for(UINT64 sub_index=0; sub_index<128; sub_index++)
	   {
	    curr_wc_block_addr = begin_wc_block_addr + sub_index; 
            UINT64 curr_wccache_set = curr_wc_block_addr & WCCACHE_SET_MASK;
	    UINT curr_wccache_hit =wc_lru_cache.IsWcCacheHit(curr_wccache_set, curr_wc_block_addr, instruction_count_total); 
	    if(curr_wccache_hit!=1)
	    {
		    if (warmup_status == WARMUP_OVER) {
			    overflow_fetches_stats+=1;
		    }
		    overflow_fetches_total+=1;
	    }	
	   }
    }

}
*/
//xinw added for overflow_fetch_stats-end

// MorpTree + Encryption Level

static const UINT64 TWOM_PAGE_ADDR_FLOOR_MASK =  0xFFFFFFFFFFE00000; 
static const UINT64 FOURK_PAGE_ADDR_FLOOR_MASK = 0xFFFFFFFFFFFFF000; 
static const UINT64 EIGHTK_PAGE_ADDR_FLOOR_MASK = 0xFFFFFFFFFFFFE000; 

static const UINT64 VERSIONS_LEVEL_SIZE = 2097152;
static const UINT64 VERSIONS_LEVEL_BITS = log2(VERSIONS_LEVEL_SIZE);
static const UINT64 TREE_LEVEL1_SIZE = 16384;
static const UINT64 TREE_LEVEL1_BITS = log2(TREE_LEVEL1_BITS);
static const UINT64 TREE_LEVEL2_SIZE = 128;
static const UINT64 TREE_LEVEL2_BITS = log2(TREE_LEVEL2_BITS);

static const UINT64 PHYSICAL_PAGES_NUM = 4194304*times_of_16g;

static const UINT64 PHYSICAL_PAGE_START_ADDR = 0;
static const UINT64 VERSIONS_START_ADDR = 4194304*times_of_16g;
static const UINT64 TREE_LEVEL1_START_ADDR = 6291456*times_of_16g;
static const UINT64 TREE_LEVEL2_START_ADDR = 6307840*times_of_16g;
static const UINT64 TREE_LEVEL3_START_ADDR = 6307968*times_of_16g;

int num_version_used=0;
int num_level1_used=0;
int num_level2_used=0;
class RandomNumberCpt {
  public:
    UINT64 random_array[2097152*1000]={0};
  public:
    UINT64 get_zero_access_block_number();
    UINT64 get_variation_of_write_counter();
};
UINT64 access_number[2097152]={0};
UINT64 RandomNumberCpt::get_zero_access_block_number()
{
	//UINT64 access_number[2097152]={0};
	for(UINT64 i=0;i<2097152*1000;i++)
	{
		//printf("random_array[%lu]: %lu\n", i, random_array[i]);
		access_number[random_array[i]]++;
	}
	
	UINT64 _ret=0;
	for(UINT64 i=0;i<2097152;i++)
	{
		if (access_number[i]==0)
			_ret++;
	}
	return _ret;
} 
UINT64 RandomNumberCpt::get_variation_of_write_counter()
{	
	for(UINT64 i=0;i<2097152;i++)
	{
		access_number[i]=0;
	}

	for(UINT64 i=0;i<2097152*1000;i++)
	{
		//printf("random_array[%lu]: %lu\n", i, random_array[i]);
		access_number[random_array[i]]++;
	}
	
	UINT64 _max=0, _min=1000000000;
	for(UINT64 i=0;i<2097152;i++)
	{
		if (access_number[i]>_max)
			_max=access_number[i];
		if (access_number[i]<_min)
			_min=access_number[i];
	}
	return (_max-_min);

}
//RandomNumberCpt random_number_cpt;


class TreeNodesCpt {
  public:
    MorphCtrBlock versions_level[16384];
    MorphCtrBlock tree_level1[128];
    MorphCtrBlock tree_level2[1];
    MorphCtrBlock tree_level3;
};
  
//TreeNodesCpt tree_nodes_cpt;


class MorphTree {
  public:

    MorphTree();
    void InitTree(bool is_random);
    void CountTreeUtilization();
    bool CheckTree();
    UINT64 GetPhysicalPageAddress(const UINT64& block_addr); 
    bool NeedPageZero(const UINT64& block_addr);
    UINT64 GetCounterAddress(const UINT32& level, const UINT64& physical_page_addr);
    UINT64 GetParentCounterAddress(const UINT32& level, const UINT64& wc_block_addr); 
    UINT32 GetCurrentLevel(const UINT64& wc_block_addr);
    UINT64 GetLevelCounterId(const UINT32& level, const UINT64& wc_block_addr); 
    //void IncrementMinorCounter(const UINT64& level, const UINT64& counter_id, const UINT64& pos);
    void IncrementMinorCounter(const UINT64& level, const UINT64& counter_id, const UINT64& pos, bool default_clean, bool should_relevel);
    void IncrementMinorCounterForSmall(const UINT64& level, const UINT64& counter_id, const UINT64& pos, bool default_clean, bool should_relevel, UINT32 table_index);
    UINT32 will_overflow(const UINT64& level, const UINT64& counter_id, const UINT64& pos, bool default_clean, bool is_relevel);
    UINT32 highest_group_to_avoid_overflow(const UINT64& level, const UINT64& counter_id, const UINT64& pos, bool default_clean);
    UINT64 GetEffectiveCounter(const UINT64& level, const UINT64& counter_id, const UINT64& pos); 
    void PrintStats();
    void SetUsed(const UINT64& level, const UINT64& counter_id); 
    //xinw added for otp precomputation-begin
    UINT64 GetLargestCounter();
    UINT64 GetLargestL1Counter();
    UINT64 GetSmallestCounter();
    UINT64 GetNumberOfZero();
    //void PrintEffectiveCounters();
    //xinw added for otp precomputation-end
  public:
    /*
    MorphCtrBlock versions_level[2097152*times_of_16g];
    MorphCtrBlock tree_level1[16384*times_of_16g];
    MorphCtrBlock tree_level2[128*times_of_16g];
    MorphCtrBlock tree_level3[times_of_16g];
    */
    MorphCtrBlock* versions_level;
    MorphCtrBlock* tree_level1;
    MorphCtrBlock* tree_level2;
    MorphCtrBlock* tree_level3;




    UINT64 rand_pages_map[PHYSICAL_PAGES_NUM]; 

    UINT64 last_physical_page_addr;
   //xinw added for debugging
    UINT64 last_physical_page_addr_debug;
    std::unordered_map<UINT64, UINT64> virtual_to_physical_page_map;
};
  

MorphTree::MorphTree() {
  //xinw added for wc_overflow
  //wc_overflow=(UINT32 *)malloc(2200000*128*sizeof(UINT32));
  //wc_miss=(UINT32 *)malloc(2200000*128*sizeof(UINT32));
  ////std::cout << "MorphTree constructor" << std::endl; 
  for (UINT64 i = 0; i < PHYSICAL_PAGES_NUM; ++i) {
    rand_pages_map[i] = i; 
  }
// xinw comment this line to change the random page allocation to sequential page allocation-begin
//  srand(rand_seed);
//  rand_seed = (rand_seed+1)%100; 
//  std::random_shuffle(rand_pages_map, rand_pages_map+PHYSICAL_PAGES_NUM);
// xinw comment this line to change the random page allocation to sequential page allocation-end
  

  last_physical_page_addr = PHYSICAL_PAGE_START_ADDR;
  //xinw added for debugging
  last_physical_page_addr_debug = 262150;
  //xinw modified below for overflow_fetch_stats-begin
  /*
  tree_level3.SetCtrLevel(3);
  for (UINT32 i = 0; i < 128; ++i) {
    tree_level2[i].SetCtrLevel(2);
  }
  for (UINT32 i = 0; i < 16384; ++i) {
    tree_level1[i].SetCtrLevel(1);
  }
  for (UINT32 i = 0; i < 2097152; ++i) {
    versions_level[i].SetCtrLevel(0);
  }
  */ 
  versions_level=(MorphCtrBlock *)malloc(2097152*times_of_16g*sizeof(MorphCtrBlock));
  tree_level1=(MorphCtrBlock *)malloc(16384*times_of_16g*sizeof(MorphCtrBlock));
  tree_level2=(MorphCtrBlock *)malloc(128*times_of_16g*sizeof(MorphCtrBlock));
  tree_level3=(MorphCtrBlock *)malloc(times_of_16g*sizeof(MorphCtrBlock));
 
  for (UINT32 i = 0; i < times_of_16g; ++i) {
    tree_level3[i].SetCtrLevel(3);
    tree_level3[i].SetSubNodeAddr(TREE_LEVEL2_START_ADDR+i*128);
    tree_level3[i].SetMorphAddr(2097152*times_of_16g+16384*times_of_16g+128*times_of_16g);
  }
  for (UINT32 i = 0; i < 128*times_of_16g; ++i) {
    tree_level2[i].SetCtrLevel(2);
    tree_level2[i].SetSubNodeAddr(TREE_LEVEL1_START_ADDR + i*128);
    tree_level2[i].SetMorphAddr(2097152*times_of_16g+16384*times_of_16g+i);
  }
  for (UINT32 i = 0; i < 16384*times_of_16g; ++i) {
    tree_level1[i].SetCtrLevel(1);
    tree_level1[i].SetSubNodeAddr(VERSIONS_START_ADDR + i*128);
    tree_level1[i].SetMorphAddr(2097152*times_of_16g+i);
  }
  for (UINT32 i = 0; i < 2097152*times_of_16g; ++i) {
    versions_level[i].SetCtrLevel(0);
    versions_level[i].SetSubNodeAddr(i*128*64);
    versions_level[i].SetMorphAddr(i);
  }
  /*
  if(OTP_PRECALCULATION)
  {
	history_max_wc=GetLargestCounter()+TABLE_LINE_SIZE-1;
	last_history_max_wc=history_max_wc;
	UINT64 _min_wc=GetSmallestCounter();
	otp_table.reset(history_max_wc, _min_wc);
	printf("smallest counter in the memory: %lu\n", GetSmallestCounter());
	printf("largest counter in the memory: %lu\n", GetLargestCounter());
	otp_table.PrintOtpTable();
  }
  */
  //xinw modified below for overflow_fetch_stats-end

}

void MorphTree::InitTree(bool is_random)
{
  srand(0);
  for (UINT32 i = 0; i < 2097152*times_of_16g; ++i) {
    versions_level[i].InitBlock(is_random,0,i);
  }
  for (UINT32 i = 0; i < 16384*times_of_16g; ++i) {
    tree_level1[i].InitBlock(is_random,1,i);
  }
  for (UINT32 i = 0; i < 128*times_of_16g; ++i) {
    tree_level2[i].InitBlock(is_random,2,i);
  }
  for (UINT32 i = 0; i < times_of_16g; ++i) {
    tree_level3[i].InitBlock(is_random,3,0);
  }
  if(OTP_PRECALCULATION||OTP_L1_PRECALCULATION)
  {
	history_max_wc=127;
	//history_max_wc=147;
	//history_max_wc=15136;
	//history_max_wc=256;
	//history_max_wc=202;
	last_history_max_wc=history_max_wc;
	UINT64 _min_wc=0;
	otp_table.reset(history_max_wc, _min_wc);
	printf("smallest counter in the memory: %lu\n", GetSmallestCounter());
	printf("largest counter in the memory: %lu\n", GetLargestCounter());
	otp_table.PrintOtpTable();
  }



}
void MorphTree::CountTreeUtilization()
{
  num_version_used=0;
  num_level1_used=0;
  num_level2_used=0;
  for (UINT32 i = 0; i < 2097152*times_of_16g; ++i) {
    if(versions_level[i].CounterIsUsed())
	num_version_used++;
  }
  for (UINT32 i = 0; i < 16384*times_of_16g; ++i) {
    if(tree_level1[i].CounterIsUsed())
	num_level1_used++;
 
  }
  for (UINT32 i = 0; i < 128*times_of_16g; ++i) {
    if(tree_level2[i].CounterIsUsed())
	num_level2_used++;
  }
 }

bool MorphTree::CheckTree()
{
  for (UINT32 i = 0; i < 2097152*times_of_16g; ++i) {
    if(!(versions_level[i].CheckBlock(0)))
	return false;
  }
  for (UINT32 i = 0; i < 16384*times_of_16g; ++i) {
    if(!(tree_level1[i].CheckBlock(1)))
	return false;
  }
  for (UINT32 i = 0; i < 128*times_of_16g; ++i) {
    if(!(tree_level2[i].CheckBlock(2)))
	return false;
  }
  for (UINT32 i = 0; i < times_of_16g; ++i) {
    if(!(tree_level3[i].CheckBlock(3)))
	return false;
  }
  return true;
}
UINT64 MorphTree::GetPhysicalPageAddress(const UINT64& block_addr) {
	if(huge_page)
	{
		UINT64 virtual_page_addr = block_addr & FOURK_PAGE_ADDR_FLOOR_MASK;
		if (virtual_to_physical_page_map.find(virtual_page_addr) != virtual_to_physical_page_map.end()) {
			if(!((virtual_to_physical_page_map[virtual_page_addr])&1))
			{
				is_first_touch =1;
				virtual_to_physical_page_map[virtual_page_addr]+=1;
			}	
			return ((virtual_to_physical_page_map[virtual_page_addr])>>1); 
		} else {
			is_first_touch =1;
			UINT64 big_virtual_page_addr = virtual_page_addr & 0xFFFFFFFFFFE00000;
			UINT64 physical_page=0;
			for(UINT64 _index=0;_index<512;_index++)
			{    
				UINT64 big_physical_page = rand_pages_map[last_physical_page_addr]; 
				virtual_to_physical_page_map[big_virtual_page_addr+(_index*4096)] = (big_physical_page<<1); 
				last_physical_page_addr++; 
				if((big_virtual_page_addr+(_index*4096))==virtual_page_addr)
				{
					physical_page=big_physical_page;
					virtual_to_physical_page_map[big_virtual_page_addr+(_index*4096)]+=1;
				}
			}
			return physical_page;
		} 
	}
	else
	{
		UINT64 virtual_page_addr = block_addr & FOURK_PAGE_ADDR_FLOOR_MASK;
		if (virtual_to_physical_page_map.find(virtual_page_addr) != virtual_to_physical_page_map.end()) {
			return virtual_to_physical_page_map[virtual_page_addr]; 

		} else {
			is_first_touch =1;
			UINT64 physical_page = rand_pages_map[last_physical_page_addr]; 
			virtual_to_physical_page_map[virtual_page_addr] = physical_page; 
			last_physical_page_addr++; 
			return physical_page;
		} 
	}


}
bool MorphTree::NeedPageZero(const UINT64& block_addr) {
//return false;

	if(!use_page_zero)
		return false;
	else
	{
		UINT64 virtual_page_addr = block_addr & FOURK_PAGE_ADDR_FLOOR_MASK;

		if (virtual_to_physical_page_map.find(virtual_page_addr) != virtual_to_physical_page_map.end()) {
			return false; 

		} else {
			return true;
		}
	}

}


UINT64 MorphTree::GetLevelCounterId(const UINT32& level, const UINT64& wc_block_addr) {
  UINT64 counter_id = 0;
  if (level == 0) {
    counter_id =  (wc_block_addr - VERSIONS_START_ADDR);
  } else if (level == 1) {
    counter_id = (wc_block_addr - TREE_LEVEL1_START_ADDR); 
  } else if (level == 2) {
    counter_id = (wc_block_addr - TREE_LEVEL2_START_ADDR);
  } else if (level == 3) {
    counter_id = (wc_block_addr - TREE_LEVEL3_START_ADDR);
  }
  return counter_id;
}

UINT64 MorphTree::GetParentCounterAddress(const UINT32& level, const UINT64& wc_block_addr) {
  UINT64 parent_counter_addr = 0;
  if (level == 0) {
    parent_counter_addr =  (wc_block_addr - VERSIONS_START_ADDR)/128 + TREE_LEVEL1_START_ADDR;
  } else if (level == 1) {
    parent_counter_addr = (wc_block_addr - TREE_LEVEL1_START_ADDR)/128 + TREE_LEVEL2_START_ADDR; 
  } else if (level == 2) {
    parent_counter_addr = (wc_block_addr - TREE_LEVEL2_START_ADDR)/128 + TREE_LEVEL3_START_ADDR;
  }
   
  ////std::cout << "getparentcounteraddress " << parent_counter_addr << std::endl; 
  return parent_counter_addr;
}


UINT64 MorphTree::GetCounterAddress(const UINT32& level, const UINT64& physical_page_addr) { 
  if (level == 0) {
    return (VERSIONS_START_ADDR + physical_page_addr/2);
  } else if (level == 1) {
    return (TREE_LEVEL1_START_ADDR + physical_page_addr/256); 
  } else if (level == 2) {
    return (TREE_LEVEL2_START_ADDR + physical_page_addr/32768);
  } else if (level == 3) {
    return (TREE_LEVEL3_START_ADDR + physical_page_addr/4194304);
  }
  return 0;
}


UINT64 unnormal_dccm_releveling=0;
void MorphTree::IncrementMinorCounter(const UINT64& level, const UINT64& counter_id, const UINT64& pos, bool default_clean, bool should_relevel) {
  //std::cout << "W " << counter_id;
  if (level == 0) {
	  versions_level[counter_id].current_pos=pos;
	  bool _will_overflow=will_overflow(level, counter_id, pos, default_clean, should_relevel);
	  UINT64 ctr_before_relevel=versions_level[counter_id].GetEffectiveCounter(0,pos);
	  bool _hit_in_middle_of_group=otp_table.hit_in_table_groups_except_top_of_group(versions_level[counter_id].GetEffectiveCounter(0,pos));
	  if(debug_relevel_to_next_group)
		  {
			  std::ostringstream oss;
			  oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
			  std::string out_file_name = oss.str();
			  output_file.open(out_file_name.c_str(), ios::out|ios::app);
			  output_file <<"instruction: "<<instruction_count_total-fast_forward_value<< " , debug releveling to next group, counter before releveling: "<<versions_level[counter_id].GetEffectiveCounter(0,pos)<<" , the pos: "<<pos<<std::endl; 
			  output_file.close();
			  output_file.clear();
			  
    			 // versions_level[counter_id].PrintNode();
    			  versions_level[counter_id].PrintSimpleNode();
		  }
	  
	  if((int)KnobRelevelToNextGroup.Value()&&should_relevel&&will_overflow(level, counter_id, pos, default_clean, should_relevel)){
		  versions_level[counter_id].minor_ctrs_[pos]++;
		  UINT64 existing_max_ctr=versions_level[counter_id].GetLargestEffectiveCounter();
		  UINT64 new_ctr_dccm=otp_table.get_nearest_bottom_wc(existing_max_ctr);
		  if(new_ctr_dccm)
		 	 versions_level[counter_id].SwitchToZcc(new_ctr_dccm);
		   else{
				versions_level[counter_id].minor_ctrs_[pos]--;
		  		versions_level[counter_id].IncrementMinorCounter(pos, default_clean, should_relevel); 
		   }
	  }
	  else{
		  versions_level[counter_id].IncrementMinorCounter(pos, default_clean, should_relevel); 
	  }
	  //    versions_level[counter_id].IncrementMinorCounter(pos, default_clean, should_relevel); 
	  if(debug_relevel_to_next_group)
	  {
		  std::ostringstream oss;
		  oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
		  std::string out_file_name = oss.str();
		  output_file.open(out_file_name.c_str(), ios::out|ios::app);
		  output_file <<"instruction: "<<instruction_count_total-fast_forward_value<<  " , debug releveling to next group, counter after releveling: "<<versions_level[counter_id].GetEffectiveCounter(0,pos)<<std::endl; 
		 /* output_file <<"instruction: "<<instruction_count_total-fast_forward_value<< ", debug releveling, content of AES table: "<<std::endl;
		  for(int table_index=0; table_index<TABLE_SIZE; table_index++)
			  output_file<<otp_table.table[table_index]<<std::endl;
		*/
		  output_file.close();
		  output_file.clear();
	  }
	  UINT64 ctr_after_relevel=versions_level[counter_id].GetEffectiveCounter(0,pos);
	  if(should_relevel&&(!_will_overflow)&&(_hit_in_middle_of_group)){
			//assert(ctr_after_relevel==(ctr_before_relevel+1));
			if(ctr_after_relevel!=(ctr_before_relevel+1))
				unnormal_dccm_releveling++;
	  }
  } else if (level == 1) {
    tree_level1[counter_id].IncrementMinorCounter(pos, default_clean, should_relevel); 
  } else if (level == 2) {
    tree_level2[counter_id].IncrementMinorCounter(pos, default_clean, should_relevel); 
  } else if (level == 3) {
    tree_level3[counter_id].IncrementMinorCounter(pos, default_clean, should_relevel); 
  }
}
void MorphTree::IncrementMinorCounterForSmall(const UINT64& level, const UINT64& counter_id, const UINT64& pos, bool default_clean, bool should_relevel, UINT32 table_index) {
    versions_level[counter_id].IncrementMinorCounterForSmall(pos, default_clean, should_relevel, table_index); 
}

UINT32 MorphTree::will_overflow(const UINT64& level, const UINT64& counter_id, const UINT64& pos, bool default_clean, bool is_relevel) {
  //std::cout << "W " << counter_id;
  if (level == 0) {
    return versions_level[counter_id].will_overflow(pos, default_clean, is_relevel); 
  } else if (level == 1) {
    return tree_level1[counter_id].will_overflow(pos, default_clean, is_relevel); 
  } else if (level == 2) {
    return tree_level2[counter_id].will_overflow(pos, default_clean, is_relevel); 
  } else if (level == 3) {
    return tree_level3[counter_id].will_overflow(pos, default_clean, is_relevel); 
  }
  return 0;
}
UINT32 MorphTree::highest_group_to_avoid_overflow(const UINT64& level, const UINT64& counter_id, const UINT64& pos, bool default_clean) {
  //std::cout << "W " << counter_id;
  if (level == 0) {
    return versions_level[counter_id].highest_group_to_avoid_overflow(pos, default_clean); 
  }
  else
    return TABLE_SIZE; 
}



UINT64 MorphTree::GetEffectiveCounter(const UINT64& level, const UINT64& counter_id, const UINT64& pos) {
  if (level == 0) {
    return versions_level[counter_id].GetEffectiveCounter(0,pos); 
  } else if (level == 1) {
    return tree_level1[counter_id].GetEffectiveCounter(1,pos); 
  } else if (level == 2) {
    return tree_level2[counter_id].GetEffectiveCounter(2,pos); 
  } else if (level == 3) {
    return tree_level3[counter_id].GetEffectiveCounter(3,pos); 
  }
  return 0;
}

void MorphTree::SetUsed(const UINT64& level, const UINT64& counter_id) {
  if (level == 0) {
    versions_level[counter_id].SetUsed(); 
  } else if (level == 1) {
    tree_level1[counter_id].SetUsed(); 
  } else if (level == 2) {
    tree_level2[counter_id].SetUsed(); 
  } else if (level == 3) {
    tree_level3[counter_id].SetUsed(); 
  }
}

UINT32 MorphTree::GetCurrentLevel(const UINT64& wc_block_addr) {
  if (wc_block_addr >= VERSIONS_START_ADDR && wc_block_addr < TREE_LEVEL1_START_ADDR) {
    return 0;
  } else if (wc_block_addr >= TREE_LEVEL1_START_ADDR && wc_block_addr < TREE_LEVEL2_START_ADDR) {
    return 1;
  } else if (wc_block_addr >= TREE_LEVEL2_START_ADDR && wc_block_addr < TREE_LEVEL3_START_ADDR) {
    return 2;
  } else if (wc_block_addr >= TREE_LEVEL3_START_ADDR) {
    return 3;
  }
  std::cout << "CLvl :" << wc_block_addr << std::endl;
  return 0;
}

void MorphTree::PrintStats() {
  UINT64 version_format_distr[2] = { 0 };  
  UINT64 total_used_versions = 0;
  for (UINT64 i=0; i < 2097152; ++i) { 
    if (versions_level[i].CounterIsUsed()) {
      total_used_versions++;
      if (versions_level[i].GetCounterFormat() == ZCC_FORMAT) {
        version_format_distr[0]++;
      } else {
        version_format_distr[1]++;
      }
    } 
  }

  output_file << "LastUsedPhysicalPage: " << last_physical_page_addr << std::endl;
  output_file << "VersionsUsedTotal: " << total_used_versions << std::endl;
  output_file << "VersionsZccCtrs%: " << version_format_distr[0]/(double)total_used_versions << std::endl;
  output_file << "VersionsMcrCtrs%: " << version_format_distr[1]/(double)total_used_versions << std::endl;
  output_file << std::endl; 

  UINT64 L1_format_distr[2] = { 0 };  
  UINT64 total_used_L1ctrs = 0;
  for (UINT64 i=0; i < 16384; ++i) { 
    if (tree_level1[i].CounterIsUsed()) {
      total_used_L1ctrs++;
      if (versions_level[i].GetCounterFormat() == ZCC_FORMAT) {
        L1_format_distr[0]++;
      } else {
        L1_format_distr[1]++;
      }
    } 
  }
  output_file << "L1CtrUsedTotal: " << total_used_L1ctrs << std::endl;
  output_file << "L1CtrZccCtrs%: " << L1_format_distr[0]/(double)total_used_L1ctrs << std::endl;
  output_file << "L1CtrMcrCtrs%: " << L1_format_distr[1]/(double)total_used_L1ctrs << std::endl;
  output_file << std::endl; 

  UINT64 L2_format_distr[2] = { 0 };  
  UINT64 total_used_L2ctrs = 0;
  for (UINT64 i=0; i < 128; ++i) { 
    if (tree_level2[i].CounterIsUsed()) {
      total_used_L2ctrs++;
      if (versions_level[i].GetCounterFormat() == ZCC_FORMAT) {
        L2_format_distr[0]++;
      } else {
        L2_format_distr[1]++;
      }
    } 
  }
  output_file << "L2CtrUsedTotal: " << total_used_L2ctrs << std::endl;
  output_file << "L2CtrZccCtrs%: " << L2_format_distr[0]/(double)total_used_L2ctrs << std::endl;
  output_file << "L2CtrMcrCtrs%: " << L2_format_distr[1]/(double)total_used_L2ctrs << std::endl;
  output_file << std::endl; 
}
//xinw added for otp precomputation-begin

UINT64 MorphTree::GetLargestCounter() {
	UINT64 _max_counter=0, _counter;
	for(UINT ctr_block_index=0; ctr_block_index<2097152; ctr_block_index++)
	{
	  for(UINT _pos=0; _pos<128; _pos++)
	  {
		
    		_counter= versions_level[ctr_block_index].GetEffectiveCounter(0,_pos);
		if(_counter>_max_counter)
			_max_counter=_counter; 
	  }
	}
	return _max_counter;
}
UINT64 MorphTree::GetLargestL1Counter() {
	UINT64 _max_counter=0, _counter;
	for(UINT ctr_block_index=0; ctr_block_index<16384; ctr_block_index++)
	{
	  for(UINT _pos=0; _pos<128; _pos++)
	  {
		
    		_counter= tree_level1[ctr_block_index].GetEffectiveCounter(1,_pos);
		if(_counter>_max_counter)
			_max_counter=_counter; 
	  }
	}
	return _max_counter;
}

UINT64 MorphTree::GetNumberOfZero(){
	UINT64 _num_zero_counter=0;
	for(UINT ctr_block_index=0; ctr_block_index<2097152; ctr_block_index++)
	{
	  for(UINT _pos=0; _pos<128; _pos++)
	  {
		
    		uint64_t _counter= versions_level[ctr_block_index].GetEffectiveCounter(0,_pos);
		if(_counter==0)
			_num_zero_counter++; 
	  }
	}
	return _num_zero_counter;
}
UINT64 MorphTree::GetSmallestCounter() {
	UINT64 _min_counter=1000000000, _counter;
	for(UINT ctr_block_index=0; ctr_block_index<2097152; ctr_block_index++)
	{
	  for(UINT _pos=0; _pos<128; _pos++)
	  {
		
    		_counter= versions_level[ctr_block_index].GetEffectiveCounter(0,_pos);
		if(_counter<_min_counter)
			_min_counter=_counter; 
	  }
	}
	return _min_counter;
}
//xinw added for otp precomputation-end
MorphTree morph_tree;
//////////////////////////////////////////////
// Morphable Counters Integrity Tree Code End 
//////////////////////////////////////////////
// Struct to represent WC/metadata cache line
typedef struct WcCacheBlock {
  UINT64 wc_block_addr;
  INT64 recency_value;
  UINT64 instruction_count;
  //xinw added for more detailed metadata traffic statistics-begin
  bool wc_dirty;
  int rrpv;
  UINT64 overflow_overhead;
  //xinw added for more detailed metadata traffic statistics-end
  //xinw modified for more detailed metadata traffic statistics-begin
  //WcCacheBlock() : wc_block_addr(0), recency_value(0), instruction_count(0) {}
  WcCacheBlock() : wc_block_addr(0), recency_value(0), instruction_count(0), wc_dirty(false),rrpv(3),overflow_overhead(0) {}
  //xinw modified for more detailed metadata traffic statistics-end
} WC_CACHE_BLOCK;


static const MM_DATA_BLOCK empty_mm_block = { };
int aggressive_level=0;
class WriteCountCache {
	public:
		WriteCountCache();
		UINT64 Size(const UINT64& set) const;
		UINT64 get_victim_index(const UINT64& set);
		UINT64 InsertWcCacheBlock(const UINT64& set, const UINT64& wc_block_addr, const UINT64& new_count, bool is_dirty, const UINT64& minor_index);
		UINT64 GetInstrCount(const UINT64& set, const UINT64& wc_block_addr);
		INT32 Exists(const UINT64& set, const UINT64& wc_block_addr); 
		INT32 InvalidateDirtyVersion(const UINT64& set, const UINT64& wc_block_addr);
		INT32 CheckExists(const UINT64& set, const UINT64& wc_block_addr); 
		void PreemptiveVerification(const UINT64& wc_block_addr, const UINT64& instr_count); 
		UINT32 IsWcCacheHit(const UINT64& set, const UINT64& wc_block_addr, const UINT64& instr_count, bool is_due_to_write); 
		//xinw added
		UINT32 access_metadata_in_LLC(const UINT64& wc_block_addr, const UINT64& new_count, bool is_dirty, const UINT64& minor_index);
		UINT32 CheckWcCacheHit(const UINT64& set, const UINT64& wc_block_addr, const UINT64& instr_count); 
		UINT32 CheckWcAndLLCCacheHit(const UINT64& wc_block_addr, const UINT64& instr_count); 
		void UpdateOverflowOverhead( const UINT64& set, const UINT64& wc_block_addr, UINT64 _overflow_overhead);
		void Insert(const bool& wccache_hit, const UINT64& set, const UINT64& wc_block_addr, const UINT64& instr_count, bool is_ditry, const UINT64& minor_index ); 
		void ChainCounterIncrementFromL1(const UINT64& counter_id, const UINT64& wc_block_addr, const UINT64& minor_ctr_pos, const UINT64& instr_count, bool default_clean, bool should_relevel, UINT32 table_index);
		void ChainCounterIncrement(const UINT64& counter_id, const UINT64& wc_block_addr, const UINT64& minor_ctr_pos, const UINT64& instr_count, bool default_clean, bool should_relevel, UINT32 table_index); 
		void ChainCounterIncrementForPageRelevel(const UINT64& counter_id, const UINT64& wc_block_addr, const UINT64& minor_ctr_pos, const UINT64& instr_count, bool default_clean, bool should_relevel, UINT32 table_index); 
		void PartChainCounterIncrement(const UINT64& counter_id, const UINT64& wc_block_addr,   bool default_clean, bool should_relevel, UINT32 begin_level);
		UINT32 will_overflow(const UINT64& counter_id, const UINT64& wc_block_addr, const UINT64& minor_ctr_pos, const UINT64& instr_count, bool default_clean, bool is_relevel);
		void enqueue_overflow_nodes(const UINT64& counter_id, const UINT64& wc_block_addr, const UINT64& minor_ctr_pos, const UINT64& instr_count, bool default_clean, bool is_relevel);
		UINT32 highest_group_to_avoid_overflow(const UINT64& counter_id, const UINT64& wc_block_addr, const UINT64& minor_ctr_pos, bool default_clean, bool is_relevel);

		void WriteBack(const UINT64& L2_evict_addr, const UINT64& instruction_count); 
		void InsertL2(const UINT64& block_addr, const UINT32& ins_op,   bool _is_metadata);
	private:
		WC_CACHE_BLOCK wc_cache_[WCCACHE_SET_NUM][WCCACHE_SET_SIZE];
		INT32 wc_set_usage_[WCCACHE_SET_NUM];
	
};

WriteCountCache::WriteCountCache() {
  for (UINT32 i = 0; i < WCCACHE_SET_NUM; ++i) {
    wc_set_usage_[i] = -1;
  }

  WC_CACHE_BLOCK empty = { };
  for (UINT32 i = 0; i < WCCACHE_SET_NUM; ++i) {
    for (UINT32 j = 0; j < WCCACHE_SET_SIZE; ++j) {
      wc_cache_[i][j] = empty;
    }
  }
}
void WriteCountCache::PreemptiveVerification(const UINT64& wc_block_addr, const UINT64& instr_count) {
  //xinw added for debugging
	
  UINT32 parent_wccache_hit = 0;
  UINT32 child_tree_level = 0;
  UINT64 parent_wc_block_addr = wc_block_addr;
  //std::cout << "childtreelevel " << child_tree_level << std::endl; 
  while (parent_wccache_hit < 2 && child_tree_level < 3) {
 
    UINT64 curr_counter_id = morph_tree.GetLevelCounterId(child_tree_level, parent_wc_block_addr);
    UINT64 curr_minor_ctr_pos = curr_counter_id & MINOR_CTR_POS_MASK;
 
    morph_tree.SetUsed(child_tree_level, curr_counter_id);


    parent_wc_block_addr = morph_tree.GetParentCounterAddress(child_tree_level, parent_wc_block_addr);
    //std::cout << "parentwcblockaddr " << parent_wc_block_addr << " " << morph_tree.GetCurrentLevel(parent_wc_block_addr) << std::endl; 
    UINT64 parent_wccache_set = parent_wc_block_addr & WCCACHE_SET_MASK;
    parent_wccache_hit = IsWcCacheHit(parent_wccache_set, parent_wc_block_addr, instr_count, false); 
    //std::cout << "ishit " << parent_wccache_hit << std::endl;
    //std::cout << "parentishit " << parent_wccache_hit << std::endl; 
    child_tree_level++; 
    wc_cache_tree_access_stats[child_tree_level]++;
    wc_cache_tree_access_read_stats[child_tree_level]++;
    if(parent_wccache_hit)
    	wc_cache_tree_hit_read_stats[child_tree_level]++;
	 
   //xinw added for metadata cache debugging
    if(debug_metadata_cache){
  	printf("tree node level: %u , wc block address: %lu , hit/miss(0 means miss): %u, due to normal data read\n ",  child_tree_level, parent_wc_block_addr, parent_wccache_hit);
  }

    UINT64 parent_counter_id = morph_tree.GetLevelCounterId(child_tree_level, parent_wc_block_addr);
    morph_tree.SetUsed(child_tree_level, parent_counter_id);

    //std::cout << "L"  << child_tree_level << " " << std::hex << parent_wc_block_addr << " " << parent_counter_id  << " " << parent_wccache_set  << " " << parent_wccache_hit  << " L" << std::endl;

    //Insert(parent_wccache_hit, parent_wccache_set, parent_wc_block_addr, instr_count);
    Insert(parent_wccache_hit, parent_wccache_set, parent_wc_block_addr, instr_count, false, curr_minor_ctr_pos);

    //std::cout << "childtreelevel " << child_tree_level << std::endl; 
  } 
}

UINT32 WriteCountCache::IsWcCacheHit(const UINT64& set, const UINT64& wc_block_addr, const UINT64& instr_count, bool is_due_to_write) {
	UINT32 is_wccache_hit = 0;
	INT32 block_index = Exists(set, wc_block_addr);
	if (block_index >= 0) {
		is_wccache_hit = 2;
		UINT64 last_instr_count = wc_cache_[set][block_index].instruction_count;
		float numerator;
		numerator = (instr_count - last_instr_count) / (float)EXECUTE_WIDTH;	
		UINT64 wcblock_cache_coverage;
		wcblock_cache_coverage = numerator / (float)DECRYPT_LATENCY * 100;
		if (wcblock_cache_coverage > CRIT_OVERLAP_VAL) {
			wc_cache_hits_total++;	
			if ((warmup_status == WARMUP_OVER)) {
				wc_cache_hits_stats++;	
			}
			is_wccache_hit = 2;		
		} else {
			wccache_exist_but_miss_total++;	
			if (warmup_status == WARMUP_OVER) {
				wccache_exist_but_miss_stats++;	
			}
		}
	}
	else{
		is_wccache_hit=access_metadata_in_LLC(wc_block_addr, instr_count, is_due_to_write, 0);
	}
	wc_cache_fetches_total++;	
	if (warmup_status == WARMUP_OVER) {
		wc_cache_fetches_stats++;	
	}
	return is_wccache_hit;
}
 UINT32 WriteCountCache::CheckWcCacheHit(const UINT64& set, const UINT64& wc_block_addr, const UINT64& instr_count) {
  UINT32 is_wccache_hit = 0;
  INT32 block_index = CheckExists(set, wc_block_addr);
  if (block_index >= 0) {
      is_wccache_hit = 2;		
  }
  return is_wccache_hit;
}
 
	

void   WriteCountCache::UpdateOverflowOverhead( const UINT64& set, const UINT64& wc_block_addr, UINT64 _overflow_overhead){
   //std::cout << "iwcb0 " << set << " " << wc_block_addr << std::endl;
  //INT64 largest_recency_value = -1;
  INT32 usage_index = wc_set_usage_[set];
  for (INT32 i = 0; i <= usage_index; ++i) {
    /*
    if(wc_cache_[set][i].recency_value > largest_recency_value) {
      largest_recency_value = wc_cache_[set][i].recency_value;
      victim_index = i;
    }
    */
    if(wc_cache_[set][i].wc_block_addr == wc_block_addr) {
      if(_overflow_overhead>wc_cache_[set][i].overflow_overhead)
	wc_cache_[set][i].overflow_overhead=_overflow_overhead;
      return;
    }
  } 

}
//void WriteCountCache::Insert(const bool& wccache_hit, const UINT64& set, const UINT64& wc_block_addr, const UINT64& instr_count) {
void WriteCountCache::Insert(const bool& wccache_hit, const UINT64& set, const UINT64& wc_block_addr, const UINT64& instr_count, bool is_dirty, const UINT64& minor_index) {
 
  //std::cout << "insert1 " << set << " " << wc_block_addr << std::endl; 
  //UINT64 evict_wc_block_addr = InsertWcCacheBlock(set, wc_block_addr, instr_count);	
  UINT64 evict_wc_block_addr = InsertWcCacheBlock(set, wc_block_addr, instr_count, is_dirty, minor_index);	
  if (evict_wc_block_addr != 0) {
  //std::cout << "insert3" << std::endl;
  }
}
UINT64 WriteCountCache::get_victim_index(const UINT64& set) {
    bool found=false;
    UINT64 victim_index;
    while(!found)
    {
	    //xinw added for deprioritizing tree nodes in metadata cache
	    if(!deprioritize_tree_nodes_in_metadata_cache){
		    for (UINT64 i = 0; i <WCCACHE_SET_SIZE; ++i) {
			    if((morph_tree.GetCurrentLevel(wc_cache_[set][i].wc_block_addr)==0)&&(wc_cache_[set][i].rrpv>=3))
			    {
				    found = true;
				    victim_index=i;
			    }
		    }
		    if(!found)
		    {
			    for (UINT64 i = 0; i <WCCACHE_SET_SIZE; ++i) {
				    if((morph_tree.GetCurrentLevel(wc_cache_[set][i].wc_block_addr)==1)&&(wc_cache_[set][i].rrpv>=(3+aggressive_level)))
				    {
					    found = true;
					    victim_index =i;
					    break;
				    }
				    if((morph_tree.GetCurrentLevel(wc_cache_[set][i].wc_block_addr)>1)&&(wc_cache_[set][i].rrpv>=1000))
				    {
					    found = true;
					    victim_index =i;
					    break;
				    }
			    }
		    }
		    if(!found)
		    {
			    for (UINT64 i = 0; i <WCCACHE_SET_SIZE; ++i) {
				    wc_cache_[set][i].rrpv++;
			    }
		    }
	    }
	    else{
		    for (UINT64 i = 0; i <WCCACHE_SET_SIZE; ++i) {
			    if((morph_tree.GetCurrentLevel(wc_cache_[set][i].wc_block_addr)>0)&&(wc_cache_[set][i].rrpv>=3)){
				    found = true;
				    victim_index=i;
			    }
		    }
		    if(!found){
			    for (UINT64 i = 0; i <WCCACHE_SET_SIZE; ++i) {
				    if((morph_tree.GetCurrentLevel(wc_cache_[set][i].wc_block_addr)==0)&&(wc_cache_[set][i].rrpv>=3)){
					    found = true;
					    victim_index =i;
					    break;
				    }
			    }
		    }
		    if(!found){
			    for (UINT64 i = 0; i <WCCACHE_SET_SIZE; ++i) {
				    wc_cache_[set][i].rrpv++;
			    }
		    }

	    }

    }

    return victim_index;
}
UINT64 read_and_write_number=0;
//static const UINT64 failed_counter_increment = 0;
void WriteCountCache::WriteBack(const UINT64& L2_evict_addr, const UINT64& instruction_count){ 
        UINT64 temp_effective_version_before_increment=0; 
	UINT64 temp_effective_version_after_increment=0; 
	UINT32 evict_is_wccache_hit = 0;
	UINT64 evict_physical_page_addr = morph_tree.GetPhysicalPageAddress(L2_evict_addr);
	UINT64 evict_wc_block_addr = morph_tree.GetCounterAddress(0, evict_physical_page_addr);
	UINT64 evict_wccache_set = evict_wc_block_addr & WCCACHE_SET_MASK; 
	UINT64 evict_minor_ctr_pos = (evict_physical_page_addr%2)*64+((L2_evict_addr >> DATA_CACHE_BLOCK_BITS) & (MINOR_CTR_POS_MASK>>1));
	UINT64 evict_counter_id = morph_tree.GetLevelCounterId(0, evict_wc_block_addr);
	UINT64 old_dccm_remain_budget = dccm_remain_budget;
	read_and_write_number++;
	if(read_and_write_number>=OTP_INTERVAL){
		dccm_remain_budget=DCCM_OVERHEAD_RATIO*OTP_INTERVAL+dccm_remain_budget-dccm_overflow_traffic;
		if(debug_budget)
			printf("in the end of epoch, the remaining budget is %ld, dccm_overflow_traffic in this epoch: %ld\n", dccm_remain_budget, dccm_overflow_traffic);
		if(custom_debug)
			printf("in the end of epoch, the remaining budget is %lu, dccm_overflow_traffic in this epoch: %lu, budget in the beginning of epoch: %lu\n", dccm_remain_budget, dccm_overflow_traffic, old_dccm_remain_budget);
		read_and_write_number=0; 
		if(debug_remain_budget){
			std::ostringstream oss;
			oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
			std::string out_file_name = oss.str();
			output_file.open(out_file_name.c_str(), ios::out|ios::app);
			output_file <<"getting into new epoch with remaining budget: "<<dccm_remain_budget<<" , AccumulatedDccmOverhead: "<<accumulated_dccm_traffic_overhead<<" , PageRelevelOverhead: "<<page_releveling_overhead<<" , TotalBlockReleveling: "<<total_block_relevel_stats<<std::endl;
			output_file.close();
			output_file.clear();
		}
		dccm_overflow_traffic=0;
	}
	evict_is_wccache_hit = IsWcCacheHit(evict_wccache_set, evict_wc_block_addr, instruction_count, true); 
	wc_cache_tree_access_stats[0]++;
	wc_cache_tree_access_write_stats[0]++;	
	if(evict_is_wccache_hit)
			wc_cache_tree_hit_write_stats[0]++;	
	if(debug_metadata_cache){
		printf("tree node level: 0 , wc block address: %lu , hit/miss(0 means miss): %u, due to normal data write\n ",   evict_wc_block_addr, evict_is_wccache_hit);
	}
	Insert(evict_is_wccache_hit, evict_wccache_set, evict_wc_block_addr, instruction_count, true, evict_minor_ctr_pos);
	is_dccm_overhead=false;
	temp_effective_version_before_increment = morph_tree.GetEffectiveCounter(0, evict_counter_id, evict_minor_ctr_pos);
	if(OTP_PRECALCULATION){
		same_relevel_for_small_and_big=true;
		if(far_relevel&&(morph_tree.versions_level[evict_counter_id].miss_small_ctr(evict_minor_ctr_pos, false))&&(highest_group_to_avoid_overflow(evict_counter_id, evict_wc_block_addr, evict_minor_ctr_pos, false, true)<TABLE_SIZE)){
			UINT32 table_index=highest_group_to_avoid_overflow(evict_counter_id, evict_wc_block_addr, evict_minor_ctr_pos, false, true);
			ChainCounterIncrement(evict_counter_id, evict_wc_block_addr, evict_minor_ctr_pos, instruction_count, false, true, table_index);
		}	
		else if(will_overflow(evict_counter_id, evict_wc_block_addr, evict_minor_ctr_pos, instruction_count, true, false)>=will_overflow(evict_counter_id, evict_wc_block_addr, evict_minor_ctr_pos, instruction_count, true, true)){	
			ChainCounterIncrement(evict_counter_id, evict_wc_block_addr, evict_minor_ctr_pos, instruction_count, false,true,1000);
		}
		else{
			ChainCounterIncrement(evict_counter_id, evict_wc_block_addr, evict_minor_ctr_pos, instruction_count, false,false,1000);
		}
	}
	else{	
		ChainCounterIncrement(evict_counter_id, evict_wc_block_addr, evict_minor_ctr_pos, instruction_count, false,false,1000);
		
	}
	temp_effective_version_after_increment = morph_tree.GetEffectiveCounter(0, evict_counter_id, evict_minor_ctr_pos);

//	if(((during_page_zero&&((instruction_count_total-fast_forward_value)>726290)))&&(total_L0_overflow_events_during_page_zero>overflow_events_number))
//		printf("incurring overflow\n");
	//if(during_page_zero&&otp_table_update_stats)			
/*	if(during_page_zero&&((instruction_count_total-fast_forward_value)>726290))
	{		
		printf("after releveling, the counter value is: %lu\n", temp_effective_version_after_increment);
			morph_tree.versions_level[evict_counter_id].RedirectPrintNode(evict_minor_ctr_pos);
	}
*/
	if(temp_effective_version_after_increment==temp_effective_version_before_increment)
	{
		failed_counter_increment++;
		if((failed_counter_increment*1.0)>(0.01*memory_writebacks_stats))
		{
			printf("the counter value does not change after incrementing for data block address: %lu at instruction %lu!\n", L2_evict_addr, instruction_count);
			exit(0);
		}
	}

	if(temp_effective_version_after_increment>1000000000)
	{
	    if(debug_aes_table_update)
	    {
		std::ostringstream oss;
		oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
		std::string out_file_name = oss.str();
		output_file.open(out_file_name.c_str(), ios::out|ios::app);
		output_file<<"unnormal incrementing a counter from "<<temp_effective_version_before_increment<<" to  "<<temp_effective_version_after_increment<<" for address "<<L2_evict_addr<<std::endl;
		//output_file << "ctr_level: "<<ctr_level_<<" node id: "<<morph_addr<<" ctr_format: "<<ctr_format_<<" zcc_major_ctr:  "<<zcc_major_ctr_<<" mcr_major_ctr: "<<mcr_major_ctr_<<" mcr_base_1: "<<mcr_base_2_<<" and the minor ctrs are: "<<std::endl;
		output_file.close();
		output_file.clear();
		exit(0);
	    }
	}
	if(invalidate_dirty_version_in_metadata_cache){
		InvalidateDirtyVersion(evict_wccache_set, evict_wc_block_addr);	
		for(int threadid=0;threadid<4;threadid++){
			UINT64 L1_counter_set = L1_counter_caches[threadid].GetCacheSet(evict_wc_block_addr*64); 
			BOOL L1_counter_hit_status = L1_counter_caches[threadid].IsCacheHit(L1_counter_set, evict_wc_block_addr*64, true);
			if(L1_counter_hit_status == CACHE_HIT) {
				UINT32 block_index=L1_counter_caches[threadid].GetBlockIndex(L1_counter_set, evict_wc_block_addr*64, true);
				L1_counter_caches[threadid].InvalidateCounter(L1_counter_set, block_index, threadid);
			}
		}
	}
}
UINT64 WriteCountCache::InsertWcCacheBlock(const UINT64& set, const UINT64& wc_block_addr, const UINT64& new_count, bool is_dirty, const UINT64& minor_index) {

  INT32 block_index = -1;
  INT32 victim_index = 0;
  INT32 usage_index = wc_set_usage_[set];
  for (INT32 i = 0; i <= usage_index; ++i) {
      if(wc_cache_[set][i].wc_block_addr == wc_block_addr) {
      block_index = i; 
    }
  } 
   UINT64 evicted_wccache_block_addr = 0;
  if (block_index >= 0) {

    //xinw added for deprioritizing integrity tree nodes in private metadata cache-begin
    UINT32 _curr_tree_level = morph_tree.GetCurrentLevel(wc_cache_[set][block_index].wc_block_addr); 
    if((!deprioritize_tree_nodes_in_metadata_cache)||(_curr_tree_level==0))
    //xinw added for deprioritizing integrity tree nodes in private metadata cache-end
    	wc_cache_[set][block_index].recency_value = 0; 
    if(is_dirty)
    {
	if(dccm_block_level_relevel&&(!wc_cache_[set][block_index].wc_dirty))
	{	
		dccm_overflow_traffic++;
		accumulated_dccm_traffic_overhead++;
	}
    	wc_cache_[set][block_index].wc_dirty=true; 
    }
  } else {
	  //wc_insert[wc_block_addr-VERSIONS_START_ADDR]++;
	  //wc_miss[(wc_block_addr-VERSIONS_START_ADDR)*128+minor_index]++;
	  UINT32 curr_tree_level = morph_tree.GetCurrentLevel(wc_block_addr); 
	  if (curr_tree_level < 3) {
		  counters_fetches_total[curr_tree_level]++;
		  if (warmup_status == WARMUP_OVER) {
			  counters_fetches_stats[curr_tree_level]++;
		  }
	  }
	  if (usage_index < WCCACHE_SET_SIZE - 1) {
		  usage_index++;
		  wc_cache_[set][usage_index].wc_block_addr = wc_block_addr; 
		  wc_cache_[set][usage_index].recency_value = 0; 
		  wc_cache_[set][usage_index].rrpv = 2; 
		  wc_cache_[set][usage_index].instruction_count = new_count; 
		  wc_cache_[set][usage_index].wc_dirty = is_dirty; 
		  wc_cache_[set][usage_index].overflow_overhead = 0; 
		  if(custom_debug_wc_insertion)
			  printf("the inserted wc block index in set is : %d\n", usage_index);
	  } else {
		  victim_index=get_victim_index(set);
		  if(custom_debug_wc_insertion)
			  printf("the inserted wc block index in set is : %d\n", victim_index);
		  if(insert_metadata_into_LLC){
		  	InsertL2(64*wc_cache_[set][victim_index].wc_block_addr, wc_cache_[set][victim_index].wc_dirty, true);
		  }else{
			if(wc_cache_[set][victim_index].wc_dirty)
				num_metadata_write_backs_from_LLC++;
		  }
		  if(wc_cache_[set][victim_index].wc_dirty)
		  {
			  evicted_wccache_block_addr = wc_cache_[set][victim_index].wc_block_addr;
			  wc_cache_write_total++;
			  if (warmup_status == WARMUP_OVER) {
				  wc_cache_write_stats++;
				  UINT32 _curr_tree_level = morph_tree.GetCurrentLevel(evicted_wccache_block_addr); 
				  if((_curr_tree_level>0)&&delay_high_level_tree_node_overflow)
				  {
					  overflow_fetches_stats+=wc_cache_[set][victim_index].overflow_overhead;
					  overflow_fetches_stats_level[_curr_tree_level]+=wc_cache_[set][victim_index].overflow_overhead;
				  }
			  }
		  }    
		  wc_cache_[set][victim_index].wc_block_addr = wc_block_addr; 
		  wc_cache_[set][victim_index].recency_value = 0; 
		  //xinw added for deprioritizing integrity tree nodes in private metadata cache-begin
		  UINT32 _curr_tree_level = morph_tree.GetCurrentLevel(wc_block_addr); 
		  if((!deprioritize_tree_nodes_in_metadata_cache)||(_curr_tree_level==0))
		  //xinw added for deprioritizing integrity tree nodes in private metadata cache-end
		  	wc_cache_[set][victim_index].rrpv = 2;
		  else
		  	wc_cache_[set][victim_index].rrpv = 3;
			 
		  wc_cache_[set][victim_index].instruction_count = new_count;  
		  wc_cache_[set][victim_index].wc_dirty = is_dirty; 
		  wc_cache_[set][victim_index].overflow_overhead = 0; 
	  }
  }	
  wc_set_usage_[set] = usage_index;
  return evicted_wccache_block_addr;
}
UINT32 WriteCountCache::access_metadata_in_LLC(const UINT64& wc_block_addr, const UINT64& new_count, bool is_dirty, const UINT64& minor_index){
	UINT32 curr_tree_level = morph_tree.GetCurrentLevel(wc_block_addr); 
	if(is_dirty)
		num_write_accesses_in_LLC[curr_tree_level]++;
	else
		num_read_accesses_in_LLC[curr_tree_level]++;
	UINT64 L2_set = L2.GetCacheSet(wc_block_addr*64); 
	BOOL L2_hit_status = L2.IsCacheHit(L2_set, wc_block_addr*64, true);
	if(L2_hit_status == CACHE_HIT) {
		UINT32 L2_block_index = L2.GetBlockIndex(L2_set, wc_block_addr*64, true);
		// Invalidate L2 block
		L2.InvalidateBlock(L2_set, L2_block_index);
		UINT64 wccache_set = wc_block_addr & WCCACHE_SET_MASK;
		Insert(false, wccache_set, wc_block_addr, new_count, is_dirty, 0);
		return 2;
	}
	else{
		if(is_dirty)
			num_write_misses_in_LLC[curr_tree_level]++;
		else
			num_read_misses_in_LLC[curr_tree_level]++;
		return 0;				
	}
}

UINT64 WriteCountCache::GetInstrCount(const UINT64& set, const UINT64& wc_block_addr) {
  /*
  auto it = cache_list_[set].begin();
  for (it = cache_list_[set].begin(); it != cache_list_[set].end(); ++it) {
    if (it->first == wc_block_addr) {
      return it->second.instruction_count;
    }
  }
  */
  return 0;
}

INT32 WriteCountCache::Exists(const UINT64& set, const UINT64& wc_block_addr) {
/*
  auto it = cache_list_[set].begin();
  int i = 0;
  for (it = cache_list_[set].begin(); it != cache_list_[set].end(); ++it) {
	  if (it->first == wc_block_addr) {
	    return 1;
    }
    i++;
  }

  return 0;
*/
	INT32 block_index = -1;
	INT32 usage_index = wc_set_usage_[set];
	for (INT32 i = 0; i <= usage_index; ++i) {
		wc_cache_[set][i].recency_value++;
		if(wc_cache_[set][i].wc_block_addr == wc_block_addr) {
			wc_cache_[set][i].recency_value = 0;
			//xinw added for deprioritizing integrity tree nodes in private metadata cache-begin
			UINT32 _curr_tree_level = morph_tree.GetCurrentLevel(wc_block_addr); 
			if((!deprioritize_tree_nodes_in_metadata_cache)||(_curr_tree_level==0))
				//xinw added for deprioritizing integrity tree nodes in private metadata cache-end
				wc_cache_[set][i].rrpv = 0;
			block_index = i; 
		}
	} 
	return block_index;
}

INT32 WriteCountCache::InvalidateDirtyVersion(const UINT64& set, const UINT64& wc_block_addr){
	INT32 block_index = -1;
	INT32 usage_index = wc_set_usage_[set];
	for (INT32 i = 0; i <= usage_index; ++i) {
		if(wc_cache_[set][i].wc_block_addr == wc_block_addr) {
			block_index = i; 
		}
	} 
	if(block_index){
		if(block_index<usage_index){
			for (INT32 i = block_index+1; i <= usage_index; ++i) {
				wc_cache_[set][i-1].wc_block_addr =wc_cache_[set][i].wc_block_addr; 
				wc_cache_[set][i-1].recency_value = wc_cache_[set][i].recency_value; 
				wc_cache_[set][i-1].rrpv = wc_cache_[set][i].rrpv; 
				wc_cache_[set][i-1].instruction_count = wc_cache_[set][i].instruction_count; 
				wc_cache_[set][i-1].wc_dirty = wc_cache_[set][i].wc_dirty ; 
				wc_cache_[set][i-1].overflow_overhead = wc_cache_[set][i].overflow_overhead; 
			}  
		}
		wc_set_usage_[set]--;
		return 1;
        }
	else
		return 0;
}


INT32 WriteCountCache::CheckExists(const UINT64& set, const UINT64& wc_block_addr) {
  INT32 block_index = -1;
  INT32 usage_index = wc_set_usage_[set];
  for (INT32 i = 0; i <= usage_index; ++i) {
    if(wc_cache_[set][i].wc_block_addr == wc_block_addr) {
      block_index = i; 
    }
  } 
  return block_index;
}


//void WriteCountCache::ChainCounterIncrement(const UINT64& counter_id, const UINT64& wc_block_addr, const UINT64& minor_ctr_pos, const UINT64& instr_count) {
/*
void WriteCountCache::ChainCounterIncrement(const UINT64& counter_id, const UINT64& wc_block_addr, const UINT64& minor_ctr_pos, const UINT64& instr_count, bool default_clean, bool should_relevel) {
  relevel_reason=0;
  UINT32 curr_tree_level = 0;
  UINT64 curr_wc_block_addr = wc_block_addr; 
  UINT64 curr_counter_id = counter_id; 
  morph_tree.IncrementMinorCounter(curr_tree_level, counter_id, minor_ctr_pos, default_clean, should_relevel);
  while (curr_tree_level < 3) {
    curr_wc_block_addr = morph_tree.GetParentCounterAddress(curr_tree_level, curr_wc_block_addr);
    UINT64 curr_wccache_set = curr_wc_block_addr & WCCACHE_SET_MASK;
    UINT curr_wccache_hit = IsWcCacheHit(curr_wccache_set, curr_wc_block_addr, instr_count); 
    UINT64 curr_minor_ctr_pos = curr_counter_id & MINOR_CTR_POS_MASK;
    Insert(curr_wccache_hit, curr_wccache_set, curr_wc_block_addr, instr_count, true, curr_minor_ctr_pos);
    curr_tree_level++;
    curr_counter_id = morph_tree.GetLevelCounterId(curr_tree_level, curr_wc_block_addr);
    morph_tree.IncrementMinorCounter(curr_tree_level, curr_counter_id, curr_minor_ctr_pos, default_clean, false);
  }
} 
void WriteCountCache::ChainCounterIncrementForSmall(const UINT64& counter_id, const UINT64& wc_block_addr, const UINT64& minor_ctr_pos, const UINT64& instr_count, bool default_clean, bool should_relevel, UINT32 table_index) {
  relevel_reason=0;
  UINT32 curr_tree_level = 0;
  UINT64 curr_wc_block_addr = wc_block_addr; 
  UINT64 curr_counter_id = counter_id; 
  morph_tree.IncrementMinorCounterForSmall(curr_tree_level, counter_id, minor_ctr_pos, default_clean, should_relevel, table_index);
  while (curr_tree_level < 3) {
    curr_wc_block_addr = morph_tree.GetParentCounterAddress(curr_tree_level, curr_wc_block_addr);
    UINT64 curr_wccache_set = curr_wc_block_addr & WCCACHE_SET_MASK;
    UINT curr_wccache_hit = IsWcCacheHit(curr_wccache_set, curr_wc_block_addr, instr_count); 
    UINT64 curr_minor_ctr_pos = curr_counter_id & MINOR_CTR_POS_MASK;
    Insert(curr_wccache_hit, curr_wccache_set, curr_wc_block_addr, instr_count, true, curr_minor_ctr_pos);
    curr_tree_level++;
    curr_counter_id = morph_tree.GetLevelCounterId(curr_tree_level, curr_wc_block_addr);
    morph_tree.IncrementMinorCounter(curr_tree_level, curr_counter_id, curr_minor_ctr_pos, default_clean, false);
  }
  ////std::cout << "chaincounterincrement" << std::endl;
} 
*/
void WriteCountCache::ChainCounterIncrementFromL1(const UINT64& counter_id, const UINT64& wc_block_addr, const UINT64& minor_ctr_pos, const UINT64& instr_count, bool default_clean, bool should_relevel, UINT32 table_index) {
	relevel_reason=0;
	UINT32 curr_tree_level = 0;
	UINT64 curr_wc_block_addr = wc_block_addr; 
	UINT64 curr_counter_id = counter_id; 
	/*
	if(table_index==1000)
	{
        	if(debug_overflow||debug_counter_increment)
			printf("before L0 node calling IncrementMinorCounter\n");
		morph_tree.IncrementMinorCounter(curr_tree_level, counter_id, minor_ctr_pos, default_clean, should_relevel);
	}
	else{
        	if(debug_overflow||debug_counter_increment)
			printf("before L0 node calling IncrementMinorCounterForSmall\n");
	
		morph_tree.IncrementMinorCounterForSmall(curr_tree_level, counter_id, minor_ctr_pos, default_clean, should_relevel, table_index);
	}*/
	while (curr_tree_level < 3) {
		curr_wc_block_addr = morph_tree.GetParentCounterAddress(curr_tree_level, curr_wc_block_addr);
		UINT64 curr_wccache_set = curr_wc_block_addr & WCCACHE_SET_MASK;
		UINT curr_wccache_hit = IsWcCacheHit(curr_wccache_set, curr_wc_block_addr, instr_count, true); 
		//xinw added for metadata cache debugging
		if(debug_metadata_cache){
			printf("tree node level: %u , wc block address: %lu , hit/miss(0 means miss): %u, due to normal data write\n ", curr_tree_level+1,curr_wc_block_addr, curr_wccache_hit);
		}
		UINT64 curr_minor_ctr_pos = curr_counter_id & MINOR_CTR_POS_MASK;
		Insert(curr_wccache_hit, curr_wccache_set, curr_wc_block_addr, instr_count, true, curr_minor_ctr_pos);
		curr_tree_level++;
		wc_cache_tree_access_stats[curr_tree_level]++;
		wc_cache_tree_access_write_stats[curr_tree_level]++;
		if(curr_wccache_hit)
			wc_cache_tree_hit_write_stats[curr_tree_level]++;	
		curr_counter_id = morph_tree.GetLevelCounterId(curr_tree_level, curr_wc_block_addr);	
        	if(debug_overflow||debug_counter_increment)
			printf("before L%u node calling IncrementMinorCounter\n", curr_tree_level);
		
			//ChainCounterIncrement(evict_counter_id, evict_wc_block_addr, evict_minor_ctr_pos, instruction_count, false,true,1000);
		if((curr_tree_level==1)&&(OTP_L1_PRECALCULATION)){
			morph_tree.IncrementMinorCounter(curr_tree_level, curr_counter_id, curr_minor_ctr_pos, default_clean, true);
		}
		else
			morph_tree.IncrementMinorCounter(curr_tree_level, curr_counter_id, curr_minor_ctr_pos, default_clean, false);
	}
	////std::cout << "chaincounterincrement" << std::endl;
}

void WriteCountCache::ChainCounterIncrement(const UINT64& counter_id, const UINT64& wc_block_addr, const UINT64& minor_ctr_pos, const UINT64& instr_count, bool default_clean, bool should_relevel, UINT32 table_index) {
	relevel_reason=0;
	UINT32 curr_tree_level = 0;
	UINT64 curr_wc_block_addr = wc_block_addr; 
	UINT64 curr_counter_id = counter_id; 
	if(table_index==1000)
	{
        	if(debug_overflow||debug_counter_increment)
			printf("before L0 node calling IncrementMinorCounter\n");
		morph_tree.IncrementMinorCounter(curr_tree_level, counter_id, minor_ctr_pos, default_clean, should_relevel);
	}
	else{
        	if(debug_overflow||debug_counter_increment)
			printf("before L0 node calling IncrementMinorCounterForSmall\n");
	
		morph_tree.IncrementMinorCounterForSmall(curr_tree_level, counter_id, minor_ctr_pos, default_clean, should_relevel, table_index);
	}
	while (curr_tree_level < 3) {
		curr_wc_block_addr = morph_tree.GetParentCounterAddress(curr_tree_level, curr_wc_block_addr);
		UINT64 curr_wccache_set = curr_wc_block_addr & WCCACHE_SET_MASK;
		UINT curr_wccache_hit = IsWcCacheHit(curr_wccache_set, curr_wc_block_addr, instr_count, true); 
		//xinw added for metadata cache debugging
		if(debug_metadata_cache){
			printf("tree node level: %u , wc block address: %lu , hit/miss(0 means miss): %u, due to normal data write\n ", curr_tree_level+1,curr_wc_block_addr, curr_wccache_hit);
		}
		UINT64 curr_minor_ctr_pos = curr_counter_id & MINOR_CTR_POS_MASK;
		Insert(curr_wccache_hit, curr_wccache_set, curr_wc_block_addr, instr_count, true, curr_minor_ctr_pos);
		curr_tree_level++;
		wc_cache_tree_access_stats[curr_tree_level]++;
		wc_cache_tree_access_write_stats[curr_tree_level]++;
		if(curr_wccache_hit)
			wc_cache_tree_hit_write_stats[curr_tree_level]++;	
		curr_counter_id = morph_tree.GetLevelCounterId(curr_tree_level, curr_wc_block_addr);	
        	if(debug_overflow||debug_counter_increment)
			printf("before L%u node calling IncrementMinorCounter\n", curr_tree_level);
		
			//ChainCounterIncrement(evict_counter_id, evict_wc_block_addr, evict_minor_ctr_pos, instruction_count, false,true,1000);
		if((curr_tree_level==1)&&(OTP_L1_PRECALCULATION)){
			int overflow_under_baseline=morph_tree.tree_level1[curr_counter_id].will_overflow(curr_minor_ctr_pos, false, false);
			int overflow_under_rmcc=morph_tree.tree_level1[curr_counter_id].will_overflow(curr_minor_ctr_pos, false, true);
			if (overflow_under_rmcc<=overflow_under_baseline){
				morph_tree.IncrementMinorCounter(curr_tree_level, curr_counter_id, curr_minor_ctr_pos, default_clean, true);
			}else if (otp_table.remaining_dccm_budget_nonzero()){
					is_dccm_overhead=true;
					morph_tree.IncrementMinorCounter(curr_tree_level, curr_counter_id, curr_minor_ctr_pos, default_clean, true);
					is_dccm_overhead=false;
					
			}else{
				morph_tree.IncrementMinorCounter(curr_tree_level, curr_counter_id, curr_minor_ctr_pos, default_clean, false);
			}
		}
		else
			morph_tree.IncrementMinorCounter(curr_tree_level, curr_counter_id, curr_minor_ctr_pos, default_clean, false);
	}
	////std::cout << "chaincounterincrement" << std::endl;
}
void WriteCountCache::ChainCounterIncrementForPageRelevel(const UINT64& counter_id, const UINT64& wc_block_addr, const UINT64& minor_ctr_pos, const UINT64& instr_count, bool default_clean, bool should_relevel, UINT32 table_index) {
	relevel_reason=0;
	UINT32 curr_tree_level = 0;
	UINT64 curr_wc_block_addr = wc_block_addr; 
	UINT64 curr_counter_id = counter_id; 
	if(table_index==1000)
	{
        	if(debug_overflow||debug_counter_increment)
			printf("before L0 node calling IncrementMinorCounter\n");
		morph_tree.IncrementMinorCounter(curr_tree_level, counter_id, minor_ctr_pos, default_clean, should_relevel);
	}
	else{
        	if(debug_overflow||debug_counter_increment)
			printf("before L0 node calling IncrementMinorCounterForSmall\n");
	
		morph_tree.IncrementMinorCounterForSmall(curr_tree_level, counter_id, minor_ctr_pos, default_clean, should_relevel, table_index);
	}
	while (curr_tree_level < 3) {
		curr_wc_block_addr = morph_tree.GetParentCounterAddress(curr_tree_level, curr_wc_block_addr);
		UINT64 curr_wccache_set = curr_wc_block_addr & WCCACHE_SET_MASK;
		UINT curr_wccache_hit = IsWcCacheHit(curr_wccache_set, curr_wc_block_addr, instr_count, true); 
		//xinw added for metadata cache debugging
		if(debug_metadata_cache){
			printf("tree node level: %u , wc block address: %lu , hit/miss(0 means miss): %u, due to normal data write\n ", curr_tree_level+1,curr_wc_block_addr, curr_wccache_hit);
		}
		UINT64 curr_minor_ctr_pos = curr_counter_id & MINOR_CTR_POS_MASK;
		Insert(curr_wccache_hit, curr_wccache_set, curr_wc_block_addr, instr_count, true, curr_minor_ctr_pos);
		curr_tree_level++;
		/*wc_cache_tree_access_stats[curr_tree_level]++;
		wc_cache_tree_access_write_stats[curr_tree_level]++;
		if(curr_wccache_hit)
			wc_cache_tree_hit_write_stats[curr_tree_level]++;	
		*/
		curr_counter_id = morph_tree.GetLevelCounterId(curr_tree_level, curr_wc_block_addr);	
        	if(debug_overflow||debug_counter_increment)
			printf("before L%u node calling IncrementMinorCounter\n", curr_tree_level);
	
		morph_tree.IncrementMinorCounter(curr_tree_level, curr_counter_id, curr_minor_ctr_pos, default_clean, false);
	}
	////std::cout << "chaincounterincrement" << std::endl;
}


void WriteCountCache::PartChainCounterIncrement(const UINT64& counter_id, const UINT64& wc_block_addr,   bool default_clean, bool should_relevel, UINT32 begin_level) {
	UINT32 curr_tree_level = begin_level;
	UINT64 curr_wc_block_addr = wc_block_addr; 
	UINT64 curr_counter_id = counter_id; 
	dccm_block_level_relevel=1;
	
        if(debug_part_chain_incrementing){
		std::ostringstream oss;
		oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
		std::string out_file_name = oss.str();
		output_file.open(out_file_name.c_str(), ios::out|ios::app);
		output_file<<"start part chain coutner incrementing from level: "<<curr_tree_level<<" at instruction: "<<(instruction_count_total-fast_forward_value)<<std::endl;
		output_file.close();
		output_file.clear();
	}
	while (curr_tree_level < 3) {
		curr_wc_block_addr = morph_tree.GetParentCounterAddress(curr_tree_level, curr_wc_block_addr);
		UINT64 curr_wccache_set = curr_wc_block_addr & WCCACHE_SET_MASK;
		UINT curr_wccache_hit = IsWcCacheHit(curr_wccache_set, curr_wc_block_addr, instruction_count_total, true); 
		UINT64 curr_minor_ctr_pos = curr_counter_id & MINOR_CTR_POS_MASK;
		//Insert(curr_wccache_hit, curr_wccache_set, curr_wc_block_addr, 0, true, curr_minor_ctr_pos);
		curr_tree_level++; 
		if(debug_part_chain_incrementing){
			std::ostringstream oss;
			oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
			std::string out_file_name = oss.str();
			output_file.open(out_file_name.c_str(), ios::out|ios::app);
			output_file<<"doing part chain coutner incrementing at level: "<<curr_tree_level<<" at instruction: "<<(instruction_count_total-fast_forward_value)<<std::endl;
			output_file.close();
			output_file.clear();
		}

		//wc_cache_tree_access_stats[curr_tree_level]++;
		//wc_cache_tree_access_write_stats[curr_tree_level]++;
		//if(curr_wccache_hit)
		//	wc_cache_tree_hit_write_stats[curr_tree_level]++;	
		if(!curr_wccache_hit)
			wc_cache_tree_page_relevel_miss_stats[curr_tree_level]++;
		curr_counter_id = morph_tree.GetLevelCounterId(curr_tree_level, curr_wc_block_addr);	
		morph_tree.IncrementMinorCounter(curr_tree_level, curr_counter_id, curr_minor_ctr_pos, default_clean, false);
	}
	dccm_block_level_relevel=0;
}  
UINT32 WriteCountCache::will_overflow(const UINT64& counter_id, const UINT64& wc_block_addr, const UINT64& minor_ctr_pos, const UINT64& instr_count, bool default_clean, bool is_relevel) {
  //std::cout << "cci1 " << std::endl;
  UINT32 _overflow_traffic=0;
  UINT32 _overflow_traffic_in_current_level=0;
  UINT32 curr_tree_level = 0;
  //return morph_tree.will_overflow(curr_tree_level, counter_id, minor_ctr_pos, default_clean, is_relevel);
  _overflow_traffic+= morph_tree.will_overflow(curr_tree_level, counter_id, minor_ctr_pos, default_clean, is_relevel);
 
  //UINT32 curr_tree_level = 0;
  UINT64 curr_wc_block_addr = wc_block_addr; 
  UINT64 curr_counter_id = counter_id; 
  while (curr_tree_level < 3) {
    curr_wc_block_addr = morph_tree.GetParentCounterAddress(curr_tree_level, curr_wc_block_addr);
    UINT64 curr_minor_ctr_pos = curr_counter_id & MINOR_CTR_POS_MASK;
    curr_tree_level++;
    curr_counter_id = morph_tree.GetLevelCounterId(curr_tree_level, curr_wc_block_addr);
    _overflow_traffic_in_current_level=morph_tree.will_overflow(curr_tree_level, curr_counter_id, curr_minor_ctr_pos, default_clean, false);
    if(curr_tree_level>1)
       _overflow_traffic+=_overflow_traffic_in_current_level; 
   //_overflow_traffic+=morph_tree.will_overflow(curr_tree_level, curr_counter_id, curr_minor_ctr_pos, default_clean, false);
  }
  return _overflow_traffic;
} 
/*
void WriteCountCache::enqueue_overflow_nodes(const UINT64& counter_id, const UINT64& wc_block_addr, const UINT64& minor_ctr_pos, const UINT64& instr_count, bool default_clean, bool is_relevel) {
  UINT32 lowest_level_incurring_overflow=5;
  UINT32 curr_tree_level = 0;
  UINT64 _write_counter_value=morph_tree.GetEffectiveCounter(0, counter_id, minor_ctr_pos);
  if(morph_tree.will_overflow(curr_tree_level, counter_id, minor_ctr_pos, default_clean, is_relevel)){
	enqueue_overflow_node(curr_tree_level, counter_id, _write_counter_value);
	if(curr_tree_level<lowest_level_incurring_overflow)
		lowest_level_incurring_overflow=curr_tree_level;
  }
  UINT64 curr_wc_block_addr = wc_block_addr; 
  UINT64 curr_counter_id = counter_id; 
  while (curr_tree_level < 3) {
    curr_wc_block_addr = morph_tree.GetParentCounterAddress(curr_tree_level, curr_wc_block_addr);
    UINT64 curr_minor_ctr_pos = curr_counter_id & MINOR_CTR_POS_MASK;
    curr_tree_level++;
    curr_counter_id = morph_tree.GetLevelCounterId(curr_tree_level, curr_wc_block_addr);
    if(morph_tree.will_overflow(curr_tree_level, curr_counter_id, curr_minor_ctr_pos, default_clean, false)){
	enqueue_overflow_node(curr_tree_level, curr_counter_id, _write_counter_value);
	if(curr_tree_level<lowest_level_incurring_overflow)
		lowest_level_incurring_overflow=curr_tree_level;
    }
  }
  if(lowest_level_incurring_overflow<=3)
	number_of_preventing_block_relevel[lowest_level_incurring_overflow]++;
}
*/
UINT32 WriteCountCache::highest_group_to_avoid_overflow(const UINT64& counter_id, const UINT64& wc_block_addr, const UINT64& minor_ctr_pos, bool default_clean, bool is_relevel) {
  //std::cout << "cci1 " << std::endl;
  UINT32 curr_tree_level = 0;
  UINT32 highest_group=morph_tree.highest_group_to_avoid_overflow(curr_tree_level, counter_id, minor_ctr_pos, default_clean);
  if(highest_group<TABLE_SIZE)
	return highest_group;
  else
	return TABLE_SIZE;
	
} 

void WriteCountCache::InsertL2(const UINT64& block_addr, const UINT32& ins_op,   bool _is_metadata){
				UINT64 L2_evict_set = L2.GetCacheSet(block_addr);
				if (warmup_status == WARMUP_OVER) {
						L2_insertion_stats++;
				}
				if (L2.SetFullStatus(L2_evict_set) == SET_NOT_FULL) {
					L2.InsertNoEviction(L2_evict_set, block_addr, ins_op, 2, _is_metadata);		
				} else {
					UINT32 L2_evict_index = L2.GetEvictedIndex(L2_evict_set);		
					UINT64 L2_evict_addr = L2.GetBlockAddress(L2_evict_set, L2_evict_index);
					UINT32 L2_evict_dirty_status = L2.GetBlockDirtyStatus(L2_evict_set, L2_evict_index);
					bool meta_status=L2.GetMetaStatus(L2_evict_set, L2_evict_index);
					L2.InsertWithEviction(L2_evict_set, block_addr, L2_evict_index, ins_op, 2, _is_metadata);		
					if(meta_status){
						if(L2_evict_dirty_status == WRITE_OP)
							num_metadata_write_backs_from_LLC++;
					}
					else{
						EvictionUpdateStats(L2_evict_addr, L2_evict_index, L2_evict_dirty_status);
						if (L2_evict_dirty_status == WRITE_OP) {
							 WriteBack(L2_evict_addr, instruction_count_total);
						}
					}
				}
}






static WriteCountCache wc_lru_cache;

static std::map<UINT64, PC_BLOCK > mem_pc_table;
static std::unordered_map<UINT64, MM_DATA_BLOCK> main_memory;

static std::vector<WC_RELEVELQ_BLOCK> wc_relevelq[WR_RATIO_GROUP_NUM];
static std::list<UINT64> wc_predictq[WR_RATIO_GROUP_NUM];

static RATIO_GROUP_STATS_BLOCK ratio_group_stats[WR_RATIO_GROUP_NUM] = { };

static std::map<UINT64, UINT64> predict_stats; 

static std::vector<std::string> bad_pc;


UINT32 ExistsInMainMemory(const UINT64& block_addr) {
 
  if (main_memory.find(block_addr) != main_memory.end()) {
    return 1;
  }  
  
  return 0;
}

UINT32 ExistsInPcTable(const UINT64& prog_count) {
  if (mem_pc_table.find(prog_count) != mem_pc_table.end()) {
    return 1;
  }  
  
  return 0;
}


void InsertUpdateStats(const UINT64& block_addr, const UINT64& prog_count, const UINT64& ins_count, const UINT32& ins_op) {

  if(ins_op == WRITE_OP) {
  
    store_induced_misses_total++;
    if (warmup_status == WARMUP_OVER) {
        store_induced_misses_stats++;
    }
            
    if (ExistsInMainMemory(block_addr)) { 
      main_memory[block_addr].store_induced_misses_total++;
      main_memory[block_addr].store_instruction_count = ins_count;
    } else {
      mainmemory_assert_fails_total++;
    }
  
    if (ExistsInPcTable(prog_count)) {
      mem_pc_table[prog_count].instruction_type = STORE_INS;	
      mem_pc_table[prog_count].store_induced_misses_total++;
    } else {
      pctable_assert_fails_total++;
    }

  
  } else { 

    load_induced_misses_total++;
    if (warmup_status == WARMUP_OVER) {
      load_induced_misses_stats++;
    }

    if (ExistsInPcTable(prog_count)) {
      mem_pc_table[prog_count].instruction_type = LOAD_INS;	
    } else {
      pctable_assert_fails_total++;
    }
  }
}

// Function to determine ratio group based on the write/read ratio
UINT64 GetWrRatioGroup(const double pc_wr_ratio) {

  UINT64 ratio_group = WR_RATIO_GROUP_NUM;	

  if (pc_wr_ratio >= 0 && pc_wr_ratio <= 2) {
    ratio_group = 0;
  } else if (pc_wr_ratio > 2 && pc_wr_ratio <= 8) {
    ratio_group = 1;
  } else if (pc_wr_ratio > 8 && pc_wr_ratio <= 32) {
    ratio_group = 2;
  } else if (pc_wr_ratio > 32 && pc_wr_ratio <= 70) {
    ratio_group = 3;
  } else if (pc_wr_ratio > 70) {
    ratio_group = 4;
  } else {
    ratio_group_assert_fails_total++;
  }

  return ratio_group;
}


UINT64 MostFrequent(UINT32 group_index, UINT64 freq_val) {

  unordered_map<UINT64, UINT64> freq_hash;
  for (std::list<UINT64>::iterator it = wc_predictq[group_index].begin(); it != wc_predictq[group_index].end(); ++it) {
        freq_hash[*it]++;
  }
    
  UINT64 max_count = 0;
  UINT64 freq_wc = 0;
  for (auto it = freq_hash.begin(); it != freq_hash.end(); ++it) {
 	if ((max_count < it->second) && (it->first != freq_val)) {
            freq_wc = it->first;
            max_count = it->second;
        }
  }
 
  return freq_wc;
}

bool rq_compare_function (WC_RELEVELQ_BLOCK i, WC_RELEVELQ_BLOCK j) {
  return (i.write_count < j.write_count);
};

typedef struct PcDistrBlock {

  UINT32 instr_type; 
  UINT64 instr_addr;
  UINT32 rq_group;
  UINT32 ratio_group;
  double wr_ratio;
  double fetch_ratio;
  double predict_ratio;
  double correct_predict_ratio;
  double misprediction_ratio;
  UINT64 switches; 
  double switches_ratio; 
  std::string rtn_name;
  INT32 line_number;
  std::string file_name;

} PC_DISTR_BLOCK;


bool descorder_compare_function ( PC_DISTR_BLOCK i, PC_DISTR_BLOCK j) {
  return (i.fetch_ratio > j.fetch_ratio);
};

typedef struct RlvStatsBlock {
 
  UINT64 mem_blocks_count;
  UINT64 fetches_count;

}  RLVL_STATS_BLOCK;


VOID RecordStats() {
/*
  std::vector<UINT64> group_wcounts[4];
  std::map<UINT64, RLVL_STATS_BLOCK> rlvl_stats;

  UINT64 total_fetches = 0;
  
  for (auto it = main_memory.begin(); it != main_memory.end(); ++it) {

    total_fetches += it->second.memory_fetches_total;
 
    UINT64 rlvl_count = it->second.relevel_count_total;
    if (rlvl_count > 32) {
      rlvl_count = pow(2, floor(log((double)rlvl_count)/log(2.0)));
    }

    if (rlvl_stats.find(rlvl_count) == rlvl_stats.end()) {
      rlvl_stats[rlvl_count].fetches_count = it->second.memory_fetches_total;
      rlvl_stats[rlvl_count].mem_blocks_count = 1;
    } else {
      rlvl_stats[rlvl_count].fetches_count += it->second.memory_fetches_total;
      rlvl_stats[rlvl_count].mem_blocks_count++;
    }   
    
    if (it->second.memory_fetches_total != 0) {
      double current_pc_wr_ratio = it->second.memory_writebacks_total / (float)it->second.memory_fetches_total * 100;
      if (current_pc_wr_ratio > 100) {
        current_pc_wr_ratio = 100;
        mainmemory_error_ratio_total++;	
        if (warmup_status == WARMUP_OVER) {
          mainmemory_error_ratio_stats++;	
        }
      }	

      UINT64 current_ratio_group = GetWrRatioGroup(current_pc_wr_ratio);	

      if (current_ratio_group < 4) {
        group_wcounts[current_ratio_group].push_back(it->second.write_count);
      }

    }
  }	

  output_file << "WcountStats:" << std::endl;
  for(UINT32 i = 0; i < 4; i++) {

    UINT64 group_wcount_sum = 0;
    for (std::vector<UINT64>::iterator it = group_wcounts[i].begin(); it != group_wcounts[i].end(); ++it) {
      group_wcount_sum += *it;
    }
	
    UINT64 group_mean = group_wcount_sum/(double)group_wcounts[i].size();		

    double group_sq_diff = 0;
    for (std::vector<UINT64>::iterator it = group_wcounts[i].begin(); it != group_wcounts[i].end(); ++it) {
      group_sq_diff += (*it - group_mean) * (*it - group_mean);
    }

    UINT64 group_std_dev =  sqrt(group_sq_diff / (double)group_wcounts[i].size());
   
    if (group_wcounts[i].size() != 0) { 
      output_file << i << " " << group_wcounts[i].size() << " ";
      output_file << group_mean  << " ";
      output_file << group_std_dev << std::endl;
    } else {
      output_file << i << " ";
      output_file << group_wcounts[i].size()  << " - -" << std::endl;
    }
  }
  output_file << std::endl;

  output_file << "RlvlStats:" << std::endl;
  for (auto it = rlvl_stats.begin(); it != rlvl_stats.end(); ++it) {
    output_file << it->first << " " << it->second.fetches_count / (double)(total_fetches) << " " << it->second.mem_blocks_count / (double)(main_memory.size()) << " ";
  }
  output_file << total_fetches << std::endl;
  output_file << std::endl;
  output_file << std::endl;
*/
}
VOID PrintTotalStats() {
  output_file << "==========TOTAL_STATS==========" << std::endl << std::endl;

//xinw added statistics: begin
  output_file << "LoadInducedMPKI: " << load_induced_misses_total*1000/(double)instruction_count_total << std::endl;
  output_file << "MemActionPKI: " << (memory_writebacks_total + memory_fetches_total)*1000/(double)instruction_count_total << std::endl; 
//xinw added statistics: end
  output_file << "MemInstCount: " << memory_instruction_count_total << std::endl;
  output_file << "TotalInstCount: " << instruction_count_total << std::endl;
  output_file << std::endl; 

 output_file << "L2Hits: " << L2_hits_total << std::endl;
  output_file << std::endl; 

  output_file << "TotalMemWriteBacks: " << memory_writebacks_total << std::endl;
  output_file << "TotalMemWriteBacksReadReleveling: " << memory_writebacks_while_releveling_total << std::endl;
  output_file << "TotalOverflowDueToReleveling:" << overflow_due_to_releveling_total<<std::endl; 
  output_file << "TotalMemFetches: " << memory_fetches_total << std::endl;
  output_file << "MemBandwidthRatio(W/R)%: " << memory_writebacks_total/(double)memory_fetches_total << std::endl;
  output_file << "PredictionMemFetches: " << predictions_total << std::endl;
  if (RUN_TYPE == 0) {
  	output_file << "PredictionRate: " << predictions_total/(double)memory_fetches_total << std::endl;
  } else {
  	output_file << "PredictionRate: " << predictions_total/(double)(memory_fetches_total - prediction_store_induced_misses_total) << std::endl;
  }
  output_file << std::endl; 

  output_file << "StackMemInstCount: " << stack_memory_instruction_count_total << std::endl;
  output_file << "StackMemInstCount%: " << stack_memory_instruction_count_total/(double)memory_instruction_count_total << std::endl;
  output_file << "HeapMemInstCount: " << heap_memory_instruction_count_total << std::endl;
  output_file << "HeapMemInstCount%: " << heap_memory_instruction_count_total/(double)memory_instruction_count_total << std::endl;
  output_file << "StackMemWriteBacks: " << stack_memory_writebacks_total << std::endl;
  output_file << "StackMemWriteBacks%: " << stack_memory_writebacks_total/(double)memory_writebacks_total << std::endl;
  output_file << "HeapMemWriteBacks: " << heap_memory_writebacks_total << std::endl;
  output_file << "HeapMemWriteBacks%: " << heap_memory_writebacks_total/(double)memory_writebacks_total << std::endl;
  output_file << "StackMemFetches: " << stack_memory_fetches_total << std::endl;
  output_file << "StackMemFetches%: " << stack_memory_fetches_total/(double)memory_fetches_total << std::endl;
  output_file << "HeapMemFetches: " << heap_memory_fetches_total << std::endl;
  output_file << "HeapMemFetches%: " << heap_memory_fetches_total/(double)memory_fetches_total << std::endl;

  output_file << std::endl; 

  output_file << "LoadInducedMisses: " << load_induced_misses_total << std::endl;
  output_file << "StoreInducedMisses: " << store_induced_misses_total << std::endl;
  output_file << "StoreInducedMisses(Prediction): " << prediction_store_induced_misses_total << std::endl;
  output_file << "TotalCacheMisses: " << memory_fetches_total << std::endl;
  output_file << std::endl; 

  output_file << "TotalWcCacheHits: " << wc_cache_hits_total << std::endl;
  output_file << "WcCacheCoverage: " << wc_cache_hits_total/(double)wc_cache_fetches_total << std::endl;
  output_file << "TotalWcCacheExistButMiss: " << wccache_exist_but_miss_total << std::endl;
  output_file << "TotalWcCacheExistButMiss%: " << wccache_exist_but_miss_total/(double)wc_cache_fetches_total << std::endl;
  output_file << std::endl; 
 
  output_file << "TotalVersionsFetches: " << counters_fetches_total[0] << std::endl;
  output_file << "TotalL1CountersFetches: " << counters_fetches_total[1] << std::endl;
  output_file << "TotalL2CountersFetches: " << counters_fetches_total[2] << std::endl;
//xinw added for overflow fetches-begin
  output_file << "TotalOverflowFetches: " << overflow_fetches_total << std::endl;
  output_file << "TotalWcCacheWrites: " << wc_cache_write_total << std::endl;
//xinw added for overflow fetches-end
  output_file << std::endl; 

  output_file << "VersionsZCCtoMCRSwitches: " << zcc_to_mcr_switches_total[0] << std::endl;
  output_file << "VersionsMCRtoZCCSwitches: " << mcr_to_zcc_switches_total[0] << std::endl;
  output_file << "VersionsZCCCounterOverlows: " << zcc_counter_overflows_total[0] << std::endl;
  output_file << "VersionsMCRCounterOverlows: " << mcr_counter_overflows_total[0] << std::endl;

  output_file << "L1ZCCtoMCRSwitches: " << zcc_to_mcr_switches_total[1] << std::endl;
  output_file << "L1MCRtoZCCSwitches: " << mcr_to_zcc_switches_total[1] << std::endl;
  output_file << "L1ZCCCounterOverlows: " << zcc_counter_overflows_total[1] << std::endl;
  output_file << "L1MCRCounterOverlows: " << mcr_counter_overflows_total[1] << std::endl;

  output_file << "L2ZCCtoMCRSwitches: " << zcc_to_mcr_switches_total[2] << std::endl;
  output_file << "L2MCRtoZCCSwitches: " << mcr_to_zcc_switches_total[2] << std::endl;
  output_file << "L2ZCCCounterOverlows: " << zcc_counter_overflows_total[2] << std::endl;
  output_file << "L2MCRCounterOverlows: " << mcr_counter_overflows_total[2] << std::endl;
 //xinw added-begin 
  output_file << "L3ZCCtoMCRSwitches: " << zcc_to_mcr_switches_total[3] << std::endl;
  output_file << "L3MCRtoZCCSwitches: " << mcr_to_zcc_switches_total[3] << std::endl;
  output_file << "L3ZCCCounterOverlows: " << zcc_counter_overflows_total[3] << std::endl;
  output_file << "L3MCRCounterOverlows: " << mcr_counter_overflows_total[3] << std::endl;
  //otp precomputation stats

  output_file <<  "OTP_TABLE_Hits: "<< otp_table_hit_total<<std::endl;
  output_file <<  "OTP_TABLE_Misses: "<< otp_table_miss_total<<std::endl;
  output_file <<  "OTP_TABLE_Misses_With_Small_Ctr: "<< otp_table_miss_with_small_ctr_total<<std::endl;
  output_file <<  "OTP_TABLE_Misses_With_Medium_Ctr: "<< otp_table_miss_with_medium_ctr_total<<std::endl;
  output_file <<  "OTP_TABLE_Misses_With_Big_Ctr: "<< otp_table_miss_with_big_ctr_total<<std::endl;
  output_file <<  "OTP_TABLE_Updates: "<< otp_table_update_total<<std::endl;
  output_file <<  "OTP_TABLE_Active_Relevel: "<< otp_table_active_relevel_total<<std::endl;
  output_file <<  "OTP_TABLE_Passive_Relevel: "<< otp_table_passive_relevel_total<<std::endl;
  output_file <<  "OTP_TABLE_Hits_While_Wc_Cache_Misses: "<< otp_table_hit_while_wc_cache_miss_total <<std::endl;
  output_file <<  "OTP_TABLE_Misses_While_Wc_Cache_Misses: "<< otp_table_miss_while_wc_cache_miss_total <<std::endl;
  //xinw added-end
  output_file << std::endl; 

  morph_tree.PrintStats();
 
  output_file << "MainMemoryAssertFails: " << mainmemory_assert_fails_total << std::endl;
  output_file << "PcTableAssertFails: " << pctable_assert_fails_total << std::endl; 
  output_file << "RatioGroupAssertFails: " << ratio_group_assert_fails_total << std::endl; 
  output_file << std::endl; 

  RecordStats();
  
  output_file << "RatioError: " << mainmemory_error_ratio_total << std::endl;
  output_file << "MainMemorySize: " << main_memory.size() << std::endl;	
  output_file << std::endl; 
  output_file << "PcRatioError: " << pctable_error_ratio_total << std::endl;
  output_file << "PcTableSize: " << mem_pc_table.size() << std::endl;
  output_file << std::endl; 
  output_file << "PcStoreInsCount: " << pc_store_instructions_total << std::endl;
  output_file << "PcLoadInsCount: " << pc_load_instructions_total << std::endl;
  output_file << std::endl; 
 
  output_file << "TotalCorrectPredictions: " << correct_predictions_total << std::endl;
  if (RUN_TYPE == 0) {
  	output_file << "TotalCoverage: " << correct_predictions_total/(double)memory_fetches_total << std::endl;
  } else {
  	output_file << "TotalCoverage: " << correct_predictions_total/(double)(memory_fetches_total - prediction_store_induced_misses_total) << std::endl;
  }
  output_file << "SameWcPrediction: " << same_wc_prediction_values_total/(double)predictions_total << std::endl;
  output_file << "Mispredictions: " << (1 - correct_predictions_total/(double)predictions_total) << std::endl;
  output_file << "RlvlMispredictions: " << (1 - rlvl_correct_predictions_total/(double)rlvl_predictions_total) << std::endl;
  output_file << "StackMispredictions: " << (1 - stack_correct_predictions_total/(double)stack_predictions_total) << std::endl;
  output_file << "HeapMispredictions: " << (1 - heap_correct_predictions_total/(double)heap_predictions_total) << std::endl;
  output_file << "TotalBandwidthOverhead: " << bandwidth_overhead_total << std::endl;
  output_file << "TotalBandwidthOverhead%: " << bandwidth_overhead_total/(double)(memory_fetches_total + memory_writebacks_total) << std::endl;
  output_file << "TotalDecryptionOverhead: " << decryption_overhead_total << std::endl;
  output_file << "TotalDecryptionOverhead%: " << decryption_overhead_total/(double)(memory_fetches_total + memory_writebacks_total) << std::endl;
  output_file << "AlreadyEvictedCount: " << already_evicted_lines_total << std::endl;
  output_file << std::endl; 

  output_file << "RelevelsAttempted: " << relevels_attempt_total << std::endl;
  output_file << "RelevelsAttempted%: " << relevels_attempt_total*WC_RELEVELQ_SIZE/(double)memory_fetches_total << std::endl;
  output_file << "RelevelsSkipped: " << relevels_skipped_total << std::endl;
  output_file << "RelevelsSkipped%: " << relevels_skipped_total/(double)memory_fetches_total << std::endl;
  output_file << std::endl; 

  output_file << "Predictions: " << predictions_total << std::endl;
  output_file << "CorrPredictions: " << correct_predictions_total << std::endl;
  output_file << "RlvlPredictions: " << rlvl_predictions_total << std::endl;
  output_file << "RlvlPredictions%: " << rlvl_predictions_total/(double)predictions_total << std::endl;
  output_file << "RlvlCorrPredictions: " << rlvl_correct_predictions_total << std::endl;
  output_file << "RlvlCorrPredictions%: " << rlvl_correct_predictions_total/(double)correct_predictions_total << std::endl;
  output_file << "StackPredictions: " << stack_predictions_total << std::endl;
  output_file << "StackCorrPredictions: " << stack_correct_predictions_total << std::endl;
  output_file << "HeapPredictions: " << heap_predictions_total << std::endl;
  output_file << "HeapCorrPredictions: " << heap_correct_predictions_total << std::endl;
  output_file << "Predictions%: " << (stack_predictions_total+heap_predictions_total)/(double)predictions_total << std::endl;
  output_file << "CorrPredictions%: " << (stack_correct_predictions_total+heap_correct_predictions_total)/(double)correct_predictions_total << std::endl;
  output_file << std::endl; 

  output_file << "RatioGroupStats:" << std::endl; 
  for(UINT32 i = 0; i < WR_RATIO_GROUP_NUM; i++) {
  	if (RUN_TYPE == 0) {
		output_file << i << " " <<  ratio_group_stats[i].correct_predictions_total << " ";
		output_file << ratio_group_stats[i].predictions_total << " ";
		output_file << ratio_group_stats[i].memory_fetches_total << " ";
		output_file << ratio_group_stats[i].predictions_total/(double)memory_fetches_total <<  " ";
		output_file << ratio_group_stats[i].predictions_total/(double)predictions_total << " ";
		output_file << ratio_group_stats[i].correct_predictions_total/(double)memory_fetches_total << " ";
		output_file << ratio_group_stats[i].correct_predictions_total/(double)predictions_total << " ";
		output_file << ratio_group_stats[i].correct_predictions_total/(double)ratio_group_stats[i].predictions_total << std::endl;
    } else {
      output_file << i << " " <<  ratio_group_stats[i].correct_predictions_total << " ";
      output_file << ratio_group_stats[i].predictions_total << " ";
      output_file << ratio_group_stats[i].memory_fetches_total << " ";
      output_file << ratio_group_stats[i].predictions_total/(double)(memory_fetches_total - prediction_store_induced_misses_total) <<  " ";
      output_file << ratio_group_stats[i].correct_predictions_total/(double)(memory_fetches_total - prediction_store_induced_misses_total) << " ";
      output_file << ratio_group_stats[i].correct_predictions_total/(double)ratio_group_stats[i].predictions_total << std::endl;
    }
  }

  output_file << std::endl; 
  output_file << "PredictionDistribution:" << std::endl; 
  for(UINT32 i = 0; i < WR_RATIO_GROUP_NUM; i++) {

	output_file << i << " ";
  	for(UINT32 j = 0; j < PREDICTION_DIST_SIZE; j++) {
		output_file  << ratio_group_stats[i].predictions_distribution_total[j] << " ";
		output_file  << ratio_group_stats[i].predictions_distribution_total[j] / (double)ratio_group_stats[i].correct_predictions_total << " ";
	}
	output_file  << std::endl;
  }

  output_file << std::endl; 
  output_file << "PredictionStats" << std::endl; 
  for (auto it = predict_stats.begin(); it != predict_stats.end(); ++it) {

		output_file << it->first << " "  << it->second / (double)(predictions_total-correct_predictions_total) << " ";
	}
	output_file  << std::endl;
}

VOID PrintAfterWarmupStats() {
  output_file << "==========STATS_AFTER_WARMUP==========" << std::endl << std::endl;
  //xinw added statistics: begin
  output_file << "LoadInducedMPKI: " << load_induced_misses_stats*1000/(double)instruction_count_stats << std::endl;
  output_file << "MemActionPKI: " << (memory_writebacks_stats + memory_fetches_stats)*1000/(double)instruction_count_stats << std::endl; 
  //xinw added statistics: end
  //output_file << "MemInstCount: " << memory_instruction_count_stats << std::endl;
  //output_file << "TotalInstCount: " << instruction_count_stats << std::endl;
  output_file << "FastForwardInstCount: " << fast_forward_value << std::endl;
  output_file << "TotalInstCount: " << (get_total_inst_num()-fast_forward_value) << std::endl;
  output_file << std::endl; 
  for(int i=0;i<NUM_OF_THREADS;i++){
	  output_file << "thread_"<<i<<"_insts: "<<instruction_count_stats_thread[i*16]<<std::endl;
	  output_file << "L1_"<<i<<"_Hits: " << L1_hits_stats[i*16] << std::endl;
	  output_file << "L1_"<<i<<"_Hits%: " << L1_hits_stats[i*16]/(double)memory_instruction_count_stats[i*16] << std::endl;
	  output_file << "L1_"<<i<<"_ReadHits: " << L1_read_hits[i*16] << std::endl;
	  output_file << "L1_"<<i<<"_WriteHits: " << L1_write_hits[i*16] << std::endl;
	  output_file << "L1_"<<i<<"_ReadAccesses: " << L1_read_accesses[i*16] << std::endl;
	  output_file << "L1_"<<i<<"_WriteAccesses: " << L1_write_accesses[i*16] << std::endl;
	  output_file << "L1_"<<i<<"_Eviction: " << L1_eviction_stats[i*16] << std::endl;
	  output_file << "L1_"<<i<<"_DataAccesses: "<<L1_data_accesses[i*16]<<std::endl;
	  output_file << "L1_"<<i<<"_InstAccesses: "<<L1_inst_accesses[i*16]<<std::endl;
	  output_file << "L1_"<<i<<"_PageTableAccesses: "<<L1_page_table_accesses[i*16]<<std::endl;
	  output_file << "L1_"<<i<<"_DataHits: "<<L1_data_hits_stats[i*16]<<std::endl;
	  output_file << "L1_"<<i<<"_InstHits: "<<L1_inst_hits_stats[i*16]<<std::endl;
	  output_file << "L1_"<<i<<"_PageTableHits: "<<L1_page_table_hits_stats[i*16]<<std::endl;
	  output_file << "L1_counter_cache_"<<i<<"_num_of_total_counter_accesses: "<<num_of_total_counter_accesses[i*16]<<std::endl;
	  output_file << "L1_counter_cache_"<<i<<"_num_of_useless_counter_accesses: "<<num_of_useless_counter_accesses[i*16]<<std::endl;
	  output_file << "L1_counter_cache_"<<i<<"_num_of_invalidate_counter_accesses: "<<num_invalidate_l1_counter_cache[i*16]<<std::endl;

  }

  output_file << "L2Insertion: " << L2_insertion_stats << std::endl;
  output_file << "L2Accesses: " << L2_accesses_stats << std::endl;
  output_file << "L2Hits: " << L2_hits_stats << std::endl;
  UINT64 L1_hits_stats_total=0;
  for(int i=0;i<NUM_OF_THREADS;i++){
	L1_hits_stats_total+=L1_hits_stats[i*16];
  }
  UINT64 sum_memory_instruction_count_stats=0;
  for(int i=0;i<NUM_OF_THREADS;i++){
	sum_memory_instruction_count_stats+=memory_instruction_count_stats[i*16];
  }


  output_file << "L2Hits%: " << L2_hits_stats/(double)(sum_memory_instruction_count_stats - L1_hits_stats_total) << std::endl;
  output_file << std::endl; 
  output_file << "L2DataAccesses: "<<L2_data_accesses_stats<<std::endl;
  output_file << "L2InstAccesses: "<<L2_inst_accesses_stats<<std::endl;
  output_file << "L2PageTableAccesses: "<<L2_page_table_accesses_stats<<std::endl;
  output_file << "L2DataHits: "<<L2_data_hits_stats<<std::endl;
  output_file << "L2InstHits: "<<L2_inst_hits_stats<<std::endl;
  output_file << "L2PageTableHits: "<<L2_page_table_hits_stats<<std::endl;


  output_file << "L2DataOccupancy: " <<L2.calculate_data_block_occupancy() << std::endl;
  num_billion_insts++;
  accumulated_data_occupancy+=L2.calculate_data_block_occupancy();
  output_file << "L2AverageDataOccupancy: " <<accumulated_data_occupancy/num_billion_insts<< std::endl;
  
  output_file << "L2MetadataWritebacks: " <<num_metadata_write_backs_from_LLC << std::endl;

  //output_file << "TotalTraffic: " << (memory_fetches_stats+memory_writebacks_stats+counters_fetches_stats[0]+counters_fetches_stats[1]+counters_fetches_stats[2]+overflow_fetches_stats+wc_cache_write_stats)+wc_cache_tree_page_relevel_miss_stats[0]*2+wc_cache_tree_page_relevel_miss_stats[1]*2+wc_cache_tree_page_relevel_miss_stats[2]*2 << std::endl; 
  output_file << "TotalTraffic: " << (memory_fetches_stats+memory_writebacks_stats+wc_cache_tree_access_read_stats[0]+wc_cache_tree_access_read_stats[1]+wc_cache_tree_access_read_stats[2]+wc_cache_tree_access_read_stats[3]+wc_cache_tree_access_write_stats[0]+wc_cache_tree_access_write_stats[1]+wc_cache_tree_access_write_stats[2]+wc_cache_tree_access_write_stats[3]-wc_cache_tree_hit_read_stats[0]-wc_cache_tree_hit_read_stats[1]-wc_cache_tree_hit_read_stats[2]-wc_cache_tree_hit_read_stats[3]-wc_cache_tree_hit_write_stats[0]-wc_cache_tree_hit_write_stats[1]-wc_cache_tree_hit_write_stats[2]-wc_cache_tree_hit_write_stats[3]+overflow_fetches_stats+num_metadata_write_backs_from_LLC)+wc_cache_tree_page_relevel_miss_stats[0]*2+wc_cache_tree_page_relevel_miss_stats[1]*2+wc_cache_tree_page_relevel_miss_stats[2]*2 << std::endl; 
  output_file << "AccumulatedDccmOverhead: " << accumulated_dccm_traffic_overhead <<std::endl;
  output_file <<  "OTP_TABLE_Hit_Rate: "<< (otp_table_hit_stats*1.0/(otp_table_hit_stats+otp_table_miss_stats))<<std::endl;
  output_file << "MetadataCacheHitRate: "<< wc_cache_hits_stats/(double)wc_cache_fetches_stats << std::endl;
  output_file << "MetadataTrafficOverhead: "<<((counters_fetches_stats[0]+counters_fetches_stats[1]+counters_fetches_stats[2]+overflow_fetches_stats+wc_cache_write_stats)*1.0/(memory_fetches_stats+memory_writebacks_stats))<<std::endl;


  output_file << "TotalMemWriteBacks: " << memory_writebacks_stats << std::endl; 
  output_file << "TotalMemWriteBacksReadReleveling: " << memory_writebacks_while_releveling_stats << std::endl;
  output_file << "TotalOverflowDueToReleveling:" << overflow_due_to_releveling_stats<<std::endl; 
 
  output_file << "TotalMemFetches: " << memory_fetches_stats << std::endl;
  output_file << "MemBandwidthRatio(W/R)%: " << memory_writebacks_stats/(double)memory_fetches_stats << std::endl;
  output_file << "PredictionMemFetches: " << predictions_stats << std::endl;
  if (RUN_TYPE == 0) {
  	output_file << "PredictionRate: " << predictions_stats/(double)memory_fetches_stats << std::endl;
  } else {
  	output_file << "PredictionRate: " << predictions_stats/(double)(memory_fetches_stats - prediction_store_induced_misses_stats) << std::endl;
  }
  output_file << std::endl; 

  output_file << "LoadInducedMisses: " << load_induced_misses_stats << std::endl;
  output_file << "StoreInducedMisses: " << store_induced_misses_stats << std::endl;
  output_file << "StoreInducedMisses(Prediction): " << prediction_store_induced_misses_stats << std::endl;
  output_file << "TotalCacheMisses: " << memory_fetches_stats << std::endl;
  output_file << std::endl; 

  output_file << "TotalWcCacheHits: " << wc_cache_hits_stats << std::endl;
  output_file << "WcCacheCoverage: " << wc_cache_hits_stats/(double)wc_cache_fetches_stats << std::endl;
  output_file << "TotalWcCacheExistButMiss: " << wccache_exist_but_miss_stats << std::endl;
  output_file << "TotalWcCacheExistButMiss%: " << wccache_exist_but_miss_stats/(double)wc_cache_fetches_stats << std::endl;
  output_file << std::endl; 
  output_file << "VersionCountersAccesses: "<< wc_cache_tree_access_stats[0] << std::endl;
  output_file << "L1CountersAccesses: "<< wc_cache_tree_access_stats[1] << std::endl;
  output_file << "L2CountersAccesses: "<< wc_cache_tree_access_stats[2] << std::endl;
  output_file << "L3CountersAccesses: "<< wc_cache_tree_access_stats[3] << std::endl;

  output_file << "VersionCountersAccessesDueToRead: "<< wc_cache_tree_access_read_stats[0] << std::endl;
  output_file << "VersionCountersHitsDueToRead: "<< wc_cache_tree_hit_read_stats[0] << std::endl;
  output_file << "VersionCountersMissRateDueToRead: "<< (1.0-wc_cache_tree_hit_read_stats[0]*1.0/wc_cache_tree_access_read_stats[0]) << std::endl;
  output_file << "VersionCountersAccessesInLLCDueToRead: " <<num_read_accesses_in_LLC[0] << std::endl; 
  output_file << "VersionCountersMissesInLLCDueToRead: " <<num_read_misses_in_LLC[0] << std::endl; 

  output_file << "L1CountersAccessesDueToRead: "<< wc_cache_tree_access_read_stats[1] << std::endl;
  output_file << "L1CountersHitsDueToRead: "<< wc_cache_tree_hit_read_stats[1] << std::endl;
  output_file << "L1CountersMissRateDueToRead: "<< (1.0-wc_cache_tree_hit_read_stats[1]*1.0/wc_cache_tree_access_read_stats[1]) << std::endl;
  output_file << "L1CountersAccessesInLLCDueToRead: " <<num_read_accesses_in_LLC[1] << std::endl; 
  output_file << "L1CountersMissesInLLCDueToRead: " <<num_read_misses_in_LLC[1] << std::endl; 



  output_file << "L2CountersAccessesDueToRead: "<< wc_cache_tree_access_read_stats[2] << std::endl;
  output_file << "L2CountersHitsDueToRead: "<< wc_cache_tree_hit_read_stats[2] << std::endl;
  output_file << "L2CountersMissRateDueToRead: "<< (1.0-wc_cache_tree_hit_read_stats[2]*1.0/wc_cache_tree_access_read_stats[2]) << std::endl;
  output_file << "L2CountersAccessesInLLCDueToRead: " <<num_read_accesses_in_LLC[2] << std::endl; 
  output_file << "L2CountersMissesInLLCDueToRead: " <<num_read_misses_in_LLC[2] << std::endl; 



  output_file << "L3CountersAccessesDueToRead: "<< wc_cache_tree_access_read_stats[3] << std::endl;
  output_file << "L3CountersHitsDueToRead: "<< wc_cache_tree_hit_read_stats[3] << std::endl;
  output_file << "L3CountersMissRateDueToRead: "<< (1.0-wc_cache_tree_hit_read_stats[3]*1.0/wc_cache_tree_access_read_stats[3]) << std::endl;
  output_file << "L3CountersAccessesInLLCDueToRead: " <<num_read_accesses_in_LLC[3] << std::endl; 
  output_file << "L3CountersMissesInLLCDueToRead: " <<num_read_misses_in_LLC[3] << std::endl; 


  output_file << "VersionCountersAccessesDueToWrite: "<< wc_cache_tree_access_write_stats[0] << std::endl;
  output_file << "VersionCountersHitsDueToWrite: "<< wc_cache_tree_hit_write_stats[0] << std::endl;
  output_file << "VersionCountersMissRateDueToWrite: "<< (1.0-wc_cache_tree_hit_write_stats[0]*1.0/wc_cache_tree_access_write_stats[0]) << std::endl;
  output_file << "VersionCountersAccessesInLLCDueToWrite: " <<num_write_accesses_in_LLC[0] << std::endl; 
  output_file << "VersionCountersMissesInLLCDueToWrite: " <<num_write_misses_in_LLC[0] << std::endl; 



  output_file << "L1CountersAccessesDueToWrite: "<< wc_cache_tree_access_write_stats[1] << std::endl;
  output_file << "L1CountersHitsDueToWrite: "<< wc_cache_tree_hit_write_stats[1] << std::endl;
  output_file << "L1CountersMissRateDueToWrite: "<< (1.0-wc_cache_tree_hit_write_stats[1]*1.0/wc_cache_tree_access_write_stats[1]) << std::endl;
  output_file << "L1CountersAccessesInLLCDueToWrite: " <<num_write_accesses_in_LLC[1] << std::endl; 
  output_file << "L1CountersMissesInLLCDueToWrite: " <<num_write_misses_in_LLC[1] << std::endl; 



  output_file << "L2CountersAccessesDueToWrite: "<< wc_cache_tree_access_write_stats[2] << std::endl;
  output_file << "L2CountersHitsDueToWrite: "<< wc_cache_tree_hit_write_stats[2] << std::endl;
  output_file << "L2CountersMissRateDueToWrite: "<< (1.0-wc_cache_tree_hit_write_stats[2]*1.0/wc_cache_tree_access_write_stats[2]) << std::endl;
  output_file << "L2CountersAccessesInLLCDueToWrite: " <<num_write_accesses_in_LLC[2] << std::endl; 
  output_file << "L2CountersMissesInLLCDueToWrite: " <<num_write_misses_in_LLC[2] << std::endl; 




  output_file << "L3CountersAccessesDueToWrite: "<< wc_cache_tree_access_write_stats[3] << std::endl;
  output_file << "L3CountersHitsDueToWrite: "<< wc_cache_tree_hit_write_stats[3] << std::endl;
  output_file << "L3CountersMissRateDueToWrite: "<< (1.0-wc_cache_tree_hit_write_stats[3]*1.0/wc_cache_tree_access_write_stats[3]) << std::endl;
  output_file << "L3CountersAccessesInLLCDueToWrite: " <<num_write_accesses_in_LLC[3] << std::endl; 
  output_file << "L3CountersMissesInLLCDueToWrite: " <<num_write_misses_in_LLC[3] << std::endl; 


  output_file << "VersionCountersPageRelevelTrafic: "<< wc_cache_tree_page_relevel_miss_stats[0]*2 << std::endl;
  output_file << "NormalizedVersionCountersPageRelevelFetches: "<< wc_cache_tree_page_relevel_miss_stats[0]*1.0/memory_fetches_stats << std::endl;
  output_file << "L1CountersPageRelevelTraffic: "<< wc_cache_tree_page_relevel_miss_stats[1]*2 << std::endl;
  output_file << "NormalizedL1CountersPageRelevelFetches: "<< wc_cache_tree_page_relevel_miss_stats[1]*1.0/memory_fetches_stats << std::endl;
  output_file << "L2CountersPageRelevelTraffic: "<< wc_cache_tree_page_relevel_miss_stats[2]*2 << std::endl;
  output_file << "NormalizedL2CountersPageRelevelFetches: "<< wc_cache_tree_page_relevel_miss_stats[2]*1.0/memory_fetches_stats << std::endl;
  output_file << "L3CountersPageRelevelTraffic: "<< wc_cache_tree_page_relevel_miss_stats[3]*2 << std::endl;
  output_file << "NormalizedL3CountersPageRelevelFetches: "<< wc_cache_tree_page_relevel_miss_stats[3]*1.0/memory_fetches_stats << std::endl;

  morph_tree.CountTreeUtilization();
  output_file << "NumberVersionUsed: "<< num_version_used << std::endl;
  output_file << "NumberL1Used: "<< num_level1_used << std::endl;
  output_file << "NumberL2Used: "<< num_level2_used << std::endl;

  output_file << "TotalVersionsFetches: " << counters_fetches_stats[0] << std::endl;
  output_file << "TotalL1CountersFetches: " << counters_fetches_stats[1] << std::endl;
  output_file << "TotalL2CountersFetches: " << counters_fetches_stats[2] << std::endl;
//xinw added for overflow fetches-begin
  output_file << "FailingCounterIncrementingWithouCounterChange: " << failed_counter_increment << std::endl;
  output_file << "TotalPotentialOverflowEvents: "<< potential_overflow_events << std::endl;
  output_file << "TotalDccmOverflowEvents: "<< total_dccm_overflow_events << std::endl;
  output_file << "TotalPageLevelRelevelingOverhead: "<< page_releveling_overhead << std::endl;
  output_file << "UnnormalDccmReleveling: "<< unnormal_dccm_releveling<<std::endl;;
  output_file << "TotalBlockLevelReleveling: "<<total_block_relevel_stats << std::endl;
  output_file << "BlockLevelDccmRatio: "<<total_block_relevel_stats*1.0/memory_fetches_stats << std::endl;
  UINT64 total_number_preventing_block_releveling=0;
  for(UINT64 i=0;i<=3;i++)
	total_number_preventing_block_releveling+=number_of_preventing_block_relevel[i];
  output_file << "TimesL0PreventingBlockReleveling: " <<number_of_preventing_block_relevel[0] <<std::endl; 
  output_file << "TimesL1PreventingBlockReleveling: " <<number_of_preventing_block_relevel[1] <<std::endl; 
  output_file << "TimesL2PreventingBlockReleveling: " <<number_of_preventing_block_relevel[2] <<std::endl; 
  output_file << "TimesL3PreventingBlockReleveling: " <<number_of_preventing_block_relevel[3] <<std::endl; 
  output_file << "NormalizedTimesL0PreventingBlockReleveling: " <<number_of_preventing_block_relevel[0]*1.0/total_number_preventing_block_releveling <<std::endl; 
  output_file << "NormalizedTimesL1PreventingBlockReleveling: " <<number_of_preventing_block_relevel[1]*1.0/total_number_preventing_block_releveling <<std::endl; 
  output_file << "NormalizedTimesL2PreventingBlockReleveling: " <<number_of_preventing_block_relevel[2]*1.0/total_number_preventing_block_releveling <<std::endl; 
  output_file << "NormalizedTimesL3PreventingBlockReleveling: " <<number_of_preventing_block_relevel[3]*1.0/total_number_preventing_block_releveling <<std::endl; 
  UINT64 total_number_picked_nodes=0;
  for(UINT64 i=0;i<=3;i++)
	total_number_picked_nodes+=number_of_nodes_selected_from_different_levels[i];
  
  output_file << "L0NodePickedForPageReleveling: "<<number_of_nodes_selected_from_different_levels[0] <<std::endl;
  output_file << "L1NodePickedForPageReleveling: "<<number_of_nodes_selected_from_different_levels[1] <<std::endl;
  output_file << "L2NodePickedForPageReleveling: "<<number_of_nodes_selected_from_different_levels[2] <<std::endl;
  output_file << "L3NodePickedForPageReleveling: "<<number_of_nodes_selected_from_different_levels[3] <<std::endl;
  output_file << "NormalizedL0NodePickedForPageReleveling: "<<number_of_nodes_selected_from_different_levels[0]*1.0/total_number_picked_nodes <<std::endl;
  output_file << "NormalizedL1NodePickedForPageReleveling: "<<number_of_nodes_selected_from_different_levels[1]*1.0/total_number_picked_nodes <<std::endl;
  output_file << "NormalizedL2NodePickedForPageReleveling: "<<number_of_nodes_selected_from_different_levels[2]*1.0/total_number_picked_nodes <<std::endl;
  output_file << "NormalizedL3NodePickedForPageReleveling: "<<number_of_nodes_selected_from_different_levels[3]*1.0/total_number_picked_nodes <<std::endl;
  output_file << "TotalOverflowEvents: " <<total_overflow_events << std::endl;
  output_file << "TotalPageZeroEvents: "<< total_page_zero_events<<std::endl;
  output_file << "TotalL0OverflowEventsDuringPageZero: "<< total_L0_overflow_events_during_page_zero << std::endl;
  output_file << "TotalOverflowEventsDuringPageZero: "<< total_overflow_events_during_page_zero << std::endl;
  output_file << "TotalOverflowFetches: " << overflow_fetches_stats << std::endl; 
  output_file << "TotalOverflowFetchesLevel0: " << overflow_fetches_stats_level[0] << std::endl; 
  output_file << "TotalOverflowFetchesLevel1: " << overflow_fetches_stats_level[1] << std::endl; 
  output_file << "TotalOverflowFetchesLevel2: " << overflow_fetches_stats_level[2] << std::endl; 
  output_file << "TotalOverflowFetchesLevel3: " << overflow_fetches_stats_level[3] << std::endl; 
  output_file << "NormalizedOverflowFetchesLevel0: " << overflow_fetches_stats_level[0]*1.0/(memory_writebacks_stats-total_block_relevel_stats) << std::endl; 
  output_file << "NormalizedOverflowFetchesLevel1: " << overflow_fetches_stats_level[1]*1.0/(memory_writebacks_stats-total_block_relevel_stats) << std::endl; 
  output_file << "NormalizedOverflowFetchesLevel2: " << overflow_fetches_stats_level[2]*1.0/(memory_writebacks_stats-total_block_relevel_stats) << std::endl; 
  output_file << "NormalizedOverflowFetchesLevel3: " << overflow_fetches_stats_level[3]*1.0/(memory_writebacks_stats-total_block_relevel_stats) << std::endl; 

  output_file << "TotalOverflowSmaller: " << overflow_smaller_than_smallest_stats << std::endl;
  output_file << "TotalOverflowInSmall: " << overflow_in_small_stats << std::endl;
  output_file << "TotalOverflowInMediate: " << overflow_in_mediate_stats << std::endl;
  
  output_file << "TotalOverflowSmallerFetches: " << overflow_fetches_smaller_than_smallest_stats << std::endl;
  output_file << "TotalOverflowInSmallFetches: " << overflow_fetches_in_small_stats << std::endl;
  output_file << "TotalOverflowInMediateFetches: " << overflow_fetches_in_mediate_stats << std::endl;
  output_file << "TotalWcCacheWrites: " << wc_cache_write_stats << std::endl;
  output_file << "NormalizedTotalWcCacheWrites: " << wc_cache_write_stats*1.0/(memory_fetches_stats+memory_writebacks_stats-total_block_relevel_stats) << std::endl;
//xinw added for overflow fetches-end
  output_file << std::endl; 
  output_file << std::endl; 
  for(int i=0; i<=3;i++) 
  {
  	output_file << "Level"<<i<<"Zcc"<<"16NonZeroMinorCountersModeFrequency: " <<zcc_frequency_stats[i][0]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"32NonZeroMinorCountersModeFrequency: " <<zcc_frequency_stats[i][1]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"36NonZeroMinorCountersModeFrequency: " <<zcc_frequency_stats[i][2]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"42NonZeroMinorCountersModeFrequency: " <<zcc_frequency_stats[i][3]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"51NonZeroMinorCountersModeFrequency: " <<zcc_frequency_stats[i][4]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"64NonZeroMinorCountersModeFrequency: " <<zcc_frequency_stats[i][5]<< std::endl;
  	output_file << "Level"<<i<<"Mcr"<<"CountersModeFrequency: " <<mcr_frequency_stats[i]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"16NonZeroMinorCountersModeOverflow: " <<zcc_detailed_overflow_stats[i][0]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"32NonZeroMinorCountersModeOverflow: " <<zcc_detailed_overflow_stats[i][1]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"36NonZeroMinorCountersModeOverflow: " <<zcc_detailed_overflow_stats[i][2]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"42NonZeroMinorCountersModeOverflow: " <<zcc_detailed_overflow_stats[i][3]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"51NonZeroMinorCountersModeOverflow: " <<zcc_detailed_overflow_stats[i][4]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"64NonZeroMinorCountersModeOverflow: " <<zcc_detailed_overflow_stats[i][5]<< std::endl;
	
        output_file << "Level"<<i<<"Zcc"<<"SwitchTo16NonZeroMinorCountersModeOverflow: " <<zcc_inner_switches_overflow_stats[i][0]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"SwitchTo32NonZeroMinorCountersModeOverflow: " <<zcc_inner_switches_overflow_stats[i][1]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"SwitchTo36NonZeroMinorCountersModeOverflow: " <<zcc_inner_switches_overflow_stats[i][2]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"SwitchTo42NonZeroMinorCountersModeOverflow: " <<zcc_inner_switches_overflow_stats[i][3]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"SwitchTo51NonZeroMinorCountersModeOverflow: " <<zcc_inner_switches_overflow_stats[i][4]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"SwitchTo64NonZeroMinorCountersModeOverflow: " <<zcc_inner_switches_overflow_stats[i][5]<< std::endl;
	uint64_t total_switch_overflow=zcc_inner_switches_overflow_stats[i][0]+zcc_inner_switches_overflow_stats[i][1]+zcc_inner_switches_overflow_stats[i][2]+zcc_inner_switches_overflow_stats[i][3]+zcc_inner_switches_overflow_stats[i][4]+zcc_inner_switches_overflow_stats[i][5];
  	output_file << "Level"<<i<<"Zcc"<<"TotalSwitchOverflow: " <<total_switch_overflow<< std::endl;
		
        output_file << "Level"<<i<<"Zcc"<<"SwitchTo16NonZeroMinorCountersMode: " <<zcc_inner_switches_stats[i][0]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"SwitchTo32NonZeroMinorCountersMode: " <<zcc_inner_switches_stats[i][1]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"SwitchTo36NonZeroMinorCountersMode: " <<zcc_inner_switches_stats[i][2]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"SwitchTo42NonZeroMinorCountersMode: " <<zcc_inner_switches_stats[i][3]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"SwitchTo51NonZeroMinorCountersMode: " <<zcc_inner_switches_stats[i][4]<< std::endl;
  	output_file << "Level"<<i<<"Zcc"<<"SwitchTo64NonZeroMinorCountersMode: " <<zcc_inner_switches_stats[i][5]<< std::endl;
	uint64_t total_switch=zcc_inner_switches_stats[i][0]+zcc_inner_switches_stats[i][1]+zcc_inner_switches_stats[i][2]+zcc_inner_switches_stats[i][3]+zcc_inner_switches_stats[i][4]+zcc_inner_switches_stats[i][5];
  	output_file << "Level"<<i<<"Zcc"<<"TotalSwitch: " <<total_switch<< std::endl;
	
  }
  output_file << "VersionsZCCtoMCRSwitches: " << zcc_to_mcr_switches_stats[0] << std::endl;
  output_file << "VersionsMCRtoZCCSwitches: " << mcr_to_zcc_switches_stats[0] << std::endl;
  output_file << "VersionYDCCMResetToZccSwitches: "   << reset_to_zcc_switches_stats[0] << std::endl;
  output_file << "VersionsZCCCounterOverlows: " << zcc_counter_overflows_stats[0] << std::endl;
  output_file << "VersionsMCRCounterOverlows: " << mcr_counter_overflows_stats[0] << std::endl;
  output_file << "VersionsMCRRebase: " << NumMcrReBase[0] << std::endl;
  

  output_file << "L1ZCCtoMCRSwitches: " << zcc_to_mcr_switches_stats[1] << std::endl;
  output_file << "L1MCRtoZCCSwitches: " << mcr_to_zcc_switches_stats[1] << std::endl;
  output_file << "L1DCCMResetToZccSwitches: "   << reset_to_zcc_switches_stats[1] << std::endl;
  output_file << "L1ZCCCounterOverlows: " << zcc_counter_overflows_stats[1] << std::endl;
  output_file << "L1MCRCounterOverlows: " << mcr_counter_overflows_stats[1] << std::endl;
  output_file << "L1MCRRebase: " << NumMcrReBase[1] << std::endl;

  output_file << "L2ZCCtoMCRSwitches: " << zcc_to_mcr_switches_stats[2] << std::endl;
  output_file << "L2MCRtoZCCSwitches: " << mcr_to_zcc_switches_stats[2] << std::endl;
  output_file << "L2DCCMResetToZccSwitches: "   << reset_to_zcc_switches_stats[2] << std::endl;
  output_file << "L2ZCCCounterOverlows: " << zcc_counter_overflows_stats[2] << std::endl;
  output_file << "L2MCRCounterOverlows: " << mcr_counter_overflows_stats[2] << std::endl;
  output_file << "L2MCRRebase: " << NumMcrReBase[2] << std::endl;
   //xinw added-begin 
  output_file << "L3ZCCtoMCRSwitches: " << zcc_to_mcr_switches_stats[3] << std::endl;
  output_file << "L3MCRtoZCCSwitches: " << mcr_to_zcc_switches_stats[3] << std::endl;
  output_file << "L3DCCMResetToZccSwitches: "   << reset_to_zcc_switches_stats[3] << std::endl;
  output_file << "L3ZCCCounterOverlows: " << zcc_counter_overflows_stats[3] << std::endl;
  output_file << "L3MCRCounterOverlows: " << mcr_counter_overflows_stats[3] << std::endl;
  output_file << "L3MCRRebase: " << NumMcrReBase[3] << std::endl;
  //otp precomputation stats
  //dccm_remain_budget=DCCM_OVERHEAD_RATIO*OTP_INTERVAL+dccm_remain_budget-dccm_overflow_traffic;
  output_file <<  "DCCM_REMAIN_BUDGET: " << dccm_remain_budget<<std::endl;
  output_file <<  "OTP_AVERAGE_Distance: "<< otp_average_distance<<std::endl;
  output_file <<  "OTP_TABLE_Hits: "<< otp_table_hit_stats<<std::endl;
  output_file <<  "OTP_TABLE_Misses: "<< otp_table_miss_stats<<std::endl; 
  output_file <<  "OTP_TABLE_Misses_With_Small_Ctr: "<< otp_table_miss_with_small_ctr_stats<<std::endl;
  output_file <<  "OTP_TABLE_Misses_With_Medium_Ctr: "<< otp_table_miss_with_medium_ctr_stats<<std::endl;
  output_file <<  "OTP_TABLE_Misses_With_Big_Ctr: "<< otp_table_miss_with_big_ctr_stats<<std::endl;
  output_file <<  "Normalized_OTP_TABLE_Misses_With_Small_Ctr: "<< otp_table_miss_with_small_ctr_stats*1.0/otp_table_miss_stats<<std::endl;
  output_file <<  "Normalized_OTP_TABLE_Misses_With_Medium_Ctr: "<< otp_table_miss_with_medium_ctr_stats*1.0/otp_table_miss_stats<<std::endl;
  output_file <<  "Normalized_OTP_TABLE_Misses_With_Big_Ctr: "<< otp_table_miss_with_big_ctr_stats*1.0/otp_table_miss_stats<<std::endl;
 

  output_file <<  "OTP_TABLE_Updates: "<< otp_table_update_stats<<std::endl;
  output_file <<  "OTP_TABLE_Active_Relevel: "<< otp_table_active_relevel_stats<<std::endl;
  output_file <<  "OTP_TABLE_Passive_Relevel: "<< otp_table_passive_relevel_stats<<std::endl; 
  output_file <<  "OTP_TABLE_Hits_While_Wc_Cache_Misses: "<< otp_table_hit_while_wc_cache_miss_stats<<std::endl;
  output_file <<  "OTP_TABLE_Misses_While_Wc_Cache_Misses: "<< otp_table_miss_while_wc_cache_miss_stats<<std::endl;
  output_file <<  "content of the AES table: " <<std::endl;
  for(int table_index=0; table_index<TABLE_SIZE; table_index++)
        output_file << otp_table.table[table_index]<< std::endl;

  //xinw added-end

 
  output_file << std::endl; 
 
  output_file << "MainMemoryAssertFails: " << mainmemory_assert_fails_total << std::endl;
  output_file << "PcTableAssertFails: " << pctable_assert_fails_total << std::endl; 
  output_file << std::endl; 

 
  output_file << "RatioError: " << mainmemory_error_ratio_stats << std::endl;
  output_file << "MainMemorySize: " << main_memory.size() << std::endl;	
  output_file << std::endl; 
  output_file << "PcRatioError: " << pctable_error_ratio_stats << std::endl;
  output_file << "PcTableSize: " << mem_pc_table.size() << std::endl;
  output_file << std::endl; 
  output_file << "PcStoreInsCount: " << pc_store_instructions_total << std::endl;
  output_file << "PcLoadInsCount: " << pc_load_instructions_total << std::endl;
  output_file << std::endl; 
 
  output_file << "TotalCorrectPredictions: " << correct_predictions_stats << std::endl;
  if (RUN_TYPE == 0) {
  	output_file << "TotalCoverage: " << correct_predictions_stats/(double)memory_fetches_stats << std::endl;
  } else {
  	output_file << "TotalCoverage: " << correct_predictions_stats/(double)(memory_fetches_stats - prediction_store_induced_misses_stats) << std::endl;
  }
  output_file << "SameWcPrediction: " << same_wc_prediction_values_stats/(double)predictions_stats << std::endl;
  output_file << "Mispredictions: " << (1 - correct_predictions_stats/(double)predictions_stats) << std::endl;
  output_file << "RlvlMispredictions: " << (1 - rlvl_correct_predictions_stats/(double)rlvl_predictions_stats) << std::endl;
  output_file << "StackMispredictions: " << (1 - stack_correct_predictions_stats/(double)stack_predictions_stats) << std::endl;
  output_file << "HeapMispredictions: " << (1 - heap_correct_predictions_stats/(double)heap_predictions_stats) << std::endl;
  output_file << "TotalBandwidthOverhead: " << bandwidth_overhead_stats << std::endl;
  output_file << "TotalBandwidthOverhead%: " << bandwidth_overhead_stats/(double)(memory_fetches_stats + memory_writebacks_stats) << std::endl;
  output_file << "TotalDecryptionOverhead: " << decryption_overhead_stats << std::endl;
  output_file << "TotalDecryptionOverhead%: " << decryption_overhead_stats/(double)(memory_fetches_stats + memory_writebacks_stats) << std::endl;
  output_file << "AlreadyEvictedCount: " << already_evicted_lines_stats << std::endl;
  output_file << std::endl; 

  output_file << "RelevelsAttempted: " << relevels_attempt_stats << std::endl;
  output_file << "RelevelsAttempted%: " << relevels_attempt_stats*WC_RELEVELQ_SIZE/(double)memory_fetches_stats << std::endl;
  output_file << "RelevelsSkipped: " << relevels_skipped_stats << std::endl;
  output_file << "RelevelsSkipped%: " << relevels_skipped_stats/(double)memory_fetches_stats << std::endl;
  output_file << std::endl; 
  

  output_file << "Predictions: " << predictions_stats << std::endl;
  output_file << "CorrPredictions: " << correct_predictions_stats << std::endl;
  output_file << "RlvlPredictions: " << rlvl_predictions_stats << std::endl;
  output_file << "RlvlPredictions%: " << rlvl_predictions_stats/(double)predictions_stats << std::endl;
  output_file << "RlvlCorrPredictions: " << rlvl_correct_predictions_stats << std::endl;
  output_file << "RlvlCorrPredictions%: " << rlvl_correct_predictions_stats/(double)correct_predictions_stats << std::endl;
  output_file << "StackPredictions: " << stack_predictions_stats << std::endl;
  output_file << "StackCorrPredictions: " << stack_correct_predictions_stats << std::endl;
  output_file << "HeapPredictions: " << heap_predictions_stats << std::endl;
  output_file << "HeapCorrPredictions: " << heap_correct_predictions_stats << std::endl;
  output_file << "Predictions%: " << (stack_predictions_stats+heap_predictions_stats)/(double)predictions_stats << std::endl;
  output_file << "CorrPredictions%: " << (stack_correct_predictions_stats+heap_correct_predictions_stats)/(double)correct_predictions_stats << std::endl;
  output_file << std::endl; 

  output_file << "RatioGroupStats:" << std::endl; 
  for(UINT32 i = 0; i < WR_RATIO_GROUP_NUM; i++) {
  	if (RUN_TYPE == 0) {
		output_file << i << " " <<  ratio_group_stats[i].correct_predictions_stats << " ";
		output_file << ratio_group_stats[i].predictions_stats << " ";
		output_file << ratio_group_stats[i].memory_fetches_stats << " ";
		output_file << ratio_group_stats[i].predictions_stats/(double)memory_fetches_stats <<  " ";
		output_file << ratio_group_stats[i].predictions_stats/(double)predictions_stats << " ";
		output_file << ratio_group_stats[i].correct_predictions_stats/(double)memory_fetches_stats << " ";
		output_file << ratio_group_stats[i].correct_predictions_stats/(double)predictions_stats << " ";
		output_file << ratio_group_stats[i].correct_predictions_stats/(double)ratio_group_stats[i].predictions_stats << std::endl;
	  } else {
      output_file << i << " " <<  ratio_group_stats[i].correct_predictions_stats << " ";
      output_file << ratio_group_stats[i].predictions_stats << " ";
      output_file << ratio_group_stats[i].memory_fetches_stats << " ";
      output_file << ratio_group_stats[i].predictions_stats/(double)(memory_fetches_stats - prediction_store_induced_misses_stats) <<  " ";
      output_file << ratio_group_stats[i].correct_predictions_stats/(double)(memory_fetches_stats - prediction_store_induced_misses_stats) << " ";
      output_file << ratio_group_stats[i].correct_predictions_stats/(double)ratio_group_stats[i].predictions_stats << std::endl;
    }
  }

  output_file << std::endl; 
  output_file << "PredictionDistribution:" << std::endl; 
  for (UINT32 i = 0; i < WR_RATIO_GROUP_NUM; i++) {

	output_file << i << " ";
  	for (UINT32 j = 0; j < PREDICTION_DIST_SIZE; j++) {
		output_file  << ratio_group_stats[i].predictions_distribution_stats[j] << " ";
		output_file  << ratio_group_stats[i].predictions_distribution_stats[j] / (double)ratio_group_stats[i].correct_predictions_stats << " ";
	}
	output_file  << std::endl;
  }

	output_file  << std::endl;
  output_file << "BadPCs:" << std::endl; 
  for (UINT32 i = 0; i < bad_pc.size(); i++) {
    output_file << bad_pc[i];
  }
	output_file  << std::endl;
}

VOID PrintResults() {
  std::cout << "ToolComplete" << std::endl;
  if (DEBUG_TEST) {
    //std::cout << step_value << std::endl; 
  }
  std::ostringstream oss;
  if (step_value == 0) {
    if (RUN_TYPE == 0) {
      //oss << KnobOutputFile.Value().c_str() <<"_morphtree_4_4_micro_baseline_debug_new_final.out";
      oss<<KnobOutputDir.Value().c_str()<<"/"<<"final.out";
    } else {
      oss <<KnobOutputDir.Value().c_str() <<"/"<<"nostore_final.out";
    }
  } else {
    if (RUN_TYPE == 0) {
      oss << KnobOutputFile.Value().c_str() << "_morphtree_4_4_micro_baseline_debug_new_" << step_value << ".out";
  	} else {
      oss << KnobOutputFile.Value().c_str() << "_morphtree_4_4_micro_baseline_debug_new_nostore_" << step_value << ".out";
	  }
  }

  std::string out_file_name = oss.str();
  //output_file.open(out_file_name.c_str());
  //xinw modified open to append
  output_file.open(out_file_name.c_str(), ios::out|ios::app);
  //output_file.open(out_file_name.c_str(), ios::out);
  output_file << "BenchName: " << KnobOutputFile.Value().c_str() << std::endl; 
  output_file << "L1CacheSize: " << L1_SET_NUM * L1_SET_SIZE /16 << std::endl;
  output_file << "L2CacheSize: " << L2_SET_NUM * L2_SET_SIZE /16 << std::endl;
  output_file << "SetAssociativity: " << L2_SET_SIZE << std::endl; 
  output_file << "FastForwardValue: " << fast_forward_value << std::endl; 
  output_file << "RelevelQueueSize: " << WC_RELEVELQ_SIZE << std::endl; 
  output_file << "PredictQueueSize: " << WC_PREDICTQ_SIZE << std::endl; 
  output_file << "PredictionRange: " << PREDICTION_RANGE << std::endl; 
  output_file << "BandwidthOverheadCap: " << BANDWIDTH_OVERHEAD_SET_POINT << std::endl; 
  output_file << "DecryptionOverheadCap: " << DECRYPTION_OVERHEAD_SET_POINT << std::endl; 
  output_file << "WarmupValue: " << WARMUP_VAL << std::endl;
  output_file << "RunValue: " << RUN_VAL << std::endl;
  output_file << "StepValue: " << STEP_VAL << std::endl;
  output_file << "PcSamplingSize: " << PC_SAMPLING_SIZE << std::endl;
  output_file << std::endl; 

  if (step_value == 0) {
	  output_file << "DONE" << std::endl << std::endl;
  }
 
  //PrintTotalStats();
  output_file << std::endl << std::endl; 
  PrintAfterWarmupStats();

  output_file.close();
  output_file.clear(); 
  t2 = clock();
  float diff2 = ((float)t2 - (float)t1);
  float seconds2 = diff2 / CLOCKS_PER_SEC;
  //float seconds2 = diff2 ;
  std::ostringstream oss_simout;
  oss_simout<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
  out_file_name = oss_simout.str();
  output_file.open(out_file_name.c_str(), ios::out|ios::app);
  output_file << "pintool job running time: "<<seconds2<<" seconds."<<std::endl;
  //output_file << "hello" <<std::endl;
  output_file.close();
  output_file.clear();


}

map<UINT64, UINT64> access_history;
UINT64 access_count=0, total_first_access, total_multiple_access, period_first_access, period_multiple_access, total_workload;
UINT64 interval_index=0;
UINT64 last_period_end=0;
void update_workload_stats(const UINT64& block_addr)
{
	auto itr=access_history.find(block_addr);
	access_count++;
	if(itr==access_history.end())
	{
		access_history.insert(std::pair<UINT64, UINT64>(block_addr,(unsigned long)1));
		total_first_access++;
		period_first_access++;
		total_workload++;
	//printf("insert block addr: %lu\n", block_addr);
	}
	else
	{
		(itr->second)++;
		total_multiple_access++;
		period_multiple_access++;
	}
	
	//printf("hit block addr:                       %lu\n", block_addr);
    	if ((warmup_status == WARMUP_OVER)&&(memory_instruction_count_total>last_period_end))
	{
		std::ostringstream oss;
		//oss << KnobOutputFile.Value().c_str() << "_morphtree_baseline_2_2_randpage_microbench_final.out";
		oss << KnobOutputDir.Value().c_str() <<"/"<< "final.out";

		std::string out_file_name = oss.str();
		if(interval_index==0)
			output_file.open(out_file_name.c_str());
		else
			//output_file.open(out_file_name.c_str(),ios::out|ios::app);
			output_file.open(out_file_name.c_str(),ios::out);
		output_file << "after number of instructions "<<memory_instruction_count_total <<std::endl;
		output_file << "TotalWorkload: " << total_workload<<" , PeriodFirstAccess: "<<period_first_access<<" , PeriodMultipleAccess: "<<period_multiple_access<<" , TotalFirstAccess: " <<total_first_access<<" , TotalMultipleAccess: "<<total_multiple_access<<" , OtpUpdateTotal: "<<otp_table_update_total<< std::endl;
		output_file.close();
		output_file.clear();

		interval_index++;
	}
	if(memory_instruction_count_total>last_period_end)
	{
		period_first_access=0;
		period_multiple_access=0;
		last_period_end=(memory_instruction_count_total+1000000000);
	}
}
#define THREASHOLD_FOR_OVERFLOW_NODES_QUEUE 33
void relevel_for_overflow_nodes(){
	if(!overflow_nodes_queue.size())
		return;
	UINT64 victim_tree_level=0;
	UINT64 victim_node_id=0;
	UINT64 victim_overflow_number=0;
	for(UINT64 overflow_nodes_queue_index=0; overflow_nodes_queue_index<overflow_nodes_queue.size(); overflow_nodes_queue_index++){
		if(overflow_nodes_queue[overflow_nodes_queue_index].overflow_number>=victim_overflow_number){
			victim_overflow_number=overflow_nodes_queue[overflow_nodes_queue_index].overflow_number;
			victim_tree_level=overflow_nodes_queue[overflow_nodes_queue_index].tree_level;
			victim_node_id=overflow_nodes_queue[overflow_nodes_queue_index].node_id;
		}		
	}
	if(debug_prioritize_overflow_nodes)
	{
		printf("the content of overflow nodes queue: \n");
		for(UINT64 overflow_nodes_queue_index=0; overflow_nodes_queue_index<overflow_nodes_queue.size(); overflow_nodes_queue_index++){
			printf("tree level: %lu , node id: %lu , overfow number: %lu\n", overflow_nodes_queue[overflow_nodes_queue_index].tree_level, overflow_nodes_queue[overflow_nodes_queue_index].node_id, overflow_nodes_queue[overflow_nodes_queue_index].overflow_number);
		}
		printf("the content of victim node:\n");
		printf("tree level: %lu , node id: %lu , overfow number: %lu\n", victim_tree_level, victim_node_id, victim_overflow_number);
		std::ostringstream oss;
		oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
		std::string out_file_name = oss.str();
		output_file.open(out_file_name.c_str(), ios::out|ios::app);
		output_file<<"selecting victim at level : "<<victim_tree_level<<" , with id: "<<victim_node_id<<" at instruction: "<<(instruction_count_total-fast_forward_value)<<std::endl;
		output_file.close();
		output_file.clear();
	
	}
	if(otp_table.remaining_dccm_budget_nonzero()){
		is_dccm_overhead=true;
		UINT64 group_for_releveling=TABLE_SIZE;
		for(UINT64 overflow_nodes_queue_index=0; overflow_nodes_queue_index<overflow_nodes_queue.size(); overflow_nodes_queue_index++){
			if((overflow_nodes_queue[overflow_nodes_queue_index].tree_level==victim_tree_level)&&(overflow_nodes_queue[overflow_nodes_queue_index].node_id==victim_node_id))			{
				group_for_releveling=otp_table.get_nearest_bottom_group(overflow_nodes_queue[overflow_nodes_queue_index].write_counter_value);
				break;
			}
		}
		int64_t _dccm_traffic_before_consuming_budget=dccm_overflow_traffic;		
		number_of_nodes_selected_from_different_levels[victim_tree_level]++;
		UINT64 wc_addr=0;
		if(victim_tree_level==0)
			wc_addr=VERSIONS_START_ADDR+victim_node_id;
		if(victim_tree_level==1)
			wc_addr=TREE_LEVEL1_START_ADDR+victim_node_id;
		if(victim_tree_level==2)
			wc_addr=TREE_LEVEL2_START_ADDR+victim_node_id;
		if(victim_tree_level==3)
			wc_addr=TREE_LEVEL3_START_ADDR+victim_node_id;
	
		UINT64 page_relevel_wccache_set = wc_addr & WCCACHE_SET_MASK; 
		UINT32 page_relevel_is_wccache_hit = wc_lru_cache.IsWcCacheHit(page_relevel_wccache_set, wc_addr, instruction_count_total, true); 
		if(!page_relevel_is_wccache_hit)
			wc_cache_tree_page_relevel_miss_stats[victim_tree_level]++;
	
		if(victim_tree_level>0)
		{
			if(victim_tree_level==1){
				UINT64 existing_max_ctr=morph_tree.tree_level1[victim_node_id].GetLargestEffectiveCounter();
				morph_tree.tree_level1[victim_node_id].SwitchToZcc(existing_max_ctr+1);
			}
			if(victim_tree_level==2){
				UINT64 existing_max_ctr=morph_tree.tree_level2[victim_node_id].GetLargestEffectiveCounter();
				morph_tree.tree_level2[victim_node_id].SwitchToZcc(existing_max_ctr+1);
			}
			if(victim_tree_level==3){
				UINT64 existing_max_ctr=morph_tree.tree_level3[victim_node_id].GetLargestEffectiveCounter();
				morph_tree.tree_level3[victim_node_id].SwitchToZcc(existing_max_ctr+1);
			}
		}
		else 
		{
			UINT64 existing_max_ctr=morph_tree.versions_level[victim_node_id].GetLargestEffectiveCounter();
			UINT64 new_ctr_dccm=otp_table.get_nearest_bottom_wc(existing_max_ctr);
			if(new_ctr_dccm)
				morph_tree.versions_level[victim_node_id].SwitchToZcc(new_ctr_dccm);
		}
		wc_lru_cache.PartChainCounterIncrement(victim_node_id, wc_addr, true, false, victim_tree_level);				
		int64_t _dccm_traffic_after_consuming_budget=dccm_overflow_traffic;		
		dccm_overhead_for_groups[group_for_releveling]+=_dccm_traffic_after_consuming_budget-_dccm_traffic_before_consuming_budget;			
		dccm_page_relevel_overhead_for_groups[group_for_releveling]+=_dccm_traffic_after_consuming_budget-_dccm_traffic_before_consuming_budget;
		if(debug_block_relevel&&(group_for_releveling<TABLE_SIZE))
		{	
			std::ostringstream oss;
			oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
			std::string out_file_name = oss.str();
			output_file.open(out_file_name.c_str(), ios::out|ios::app);
			output_file<<"updating the dccm overhead below group in position: "<<group_for_releveling<<" , the added dccm overhead is: "<<(_dccm_traffic_after_consuming_budget-_dccm_traffic_before_consuming_budget)<<" at instruction: "<<(instruction_count_total-fast_forward_value)<<std::endl;
			output_file.close();
			output_file.clear();

		}

		//overflow_nodes_queue.erase(overflow_nodes_queue.begin()+overflow_nodes_queue_index);

		is_dccm_overhead=false;
	}
	_dccm_counter=0;

}
void enqueue_overflow_node(UINT64 _tree_level, UINT64 _node_id, UINT64 _write_counter_value){

	bool find_overflow_node=false;
	for(UINT64 overflow_nodes_queue_index=0; overflow_nodes_queue_index<overflow_nodes_queue.size(); overflow_nodes_queue_index++){
		if((overflow_nodes_queue[overflow_nodes_queue_index].tree_level==_tree_level)&&(overflow_nodes_queue[overflow_nodes_queue_index].node_id==_node_id)){
			find_overflow_node=true;
			overflow_nodes_queue[overflow_nodes_queue_index].overflow_number++;
			break;
		}
	}
       if(!find_overflow_node)
       {
	   overflow_node _node;
	   _node.tree_level=_tree_level;
	   _node.node_id=_node_id;
	   _node.overflow_number=1;
	   _node.write_counter_value=_write_counter_value;
	   overflow_nodes_queue.push_back(_node);
	   if(debug_block_relevel){
			   std::ostringstream oss;
			   oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
			   std::string out_file_name = oss.str();
			   output_file.open(out_file_name.c_str(), ios::out|ios::app);
			   output_file<<"pushing back node into overflow_node_queue, the size is: "<<overflow_nodes_queue.size() <<" at instruction: "<<(instruction_count_total-fast_forward_value)<<std::endl;
			   output_file.close();
			   output_file.clear();
	   }

	   if(overflow_nodes_queue.size()>=THREASHOLD_FOR_OVERFLOW_NODES_QUEUE){
		   if(debug_block_relevel){
			   std::ostringstream oss;
			   oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
			   std::string out_file_name = oss.str();
			   output_file.open(out_file_name.c_str(), ios::out|ios::app);
			   output_file<<"overflow_node_queue is full, incurs relevel in overflow_node_queue at instruction: "<<(instruction_count_total-fast_forward_value)<<std::endl;
			   output_file.close();
			   output_file.clear();
		   }
		   relevel_for_overflow_nodes();
		   overflow_nodes_queue.clear();
	   }
       }
}



UINT32 CacheCall(THREADID threadid, const UINT32& instruction_operation, const UINT64& instruction_count, const UINT64& program_counter, const UINT64& block_addr, const UINT32& stack_status, bool is_page_table_access, int access_type) {
	if(access_inst_and_page_table&&(!is_page_table_access))
		CacheCall(threadid, READ_OP, instruction_count, program_counter, get_page_table_block_address(block_addr), stack_status, true, access_page_table);
	if(instruction_operation == WRITE_OP)
		L1_write_accesses[threadid*16]++;
	else
		L1_read_accesses[threadid*16]++;
	if(access_type==access_data)
		L1_data_accesses[threadid*16]++;
	if(access_type==access_inst)
		L1_inst_accesses[threadid*16]++;
	if(access_type==access_page_table)
		L1_page_table_accesses[threadid*16]++;
	/*
	PIN_GetLock(&pinLock, threadid + 1);
	bool need_page_zero = morph_tree.NeedPageZero(block_addr);
	UINT64 debug_physical_page_addr = morph_tree.GetPhysicalPageAddress(block_addr);
	if(debug_physical_page_addr==20000000) 
		printf("\n");
	during_page_zero=0;
	if(need_page_zero)
	{
		total_page_zero_events++;
		during_page_zero=1;
		printf("page zeroing\n");
		//UINT64 page_begin_addr=block_addr&FOURK_PAGE_ADDR_FLOOR_MASK;
		UINT64 page_begin_addr=block_addr&FOURK_PAGE_ADDR_FLOOR_MASK;
		if(huge_page)
			page_begin_addr=block_addr&TWOM_PAGE_ADDR_FLOOR_MASK;
		UINT64 num_of_blocks_each_page=64;
		if(huge_page)
			num_of_blocks_each_page=64*512;
		for(UINT64 i=0;i<num_of_blocks_each_page;i++)
		{	  
			read_and_write_number++;
			if(read_and_write_number>=OTP_INTERVAL)
			{
				UINT64 old_dccm_remain_budget = dccm_remain_budget;
				dccm_remain_budget=DCCM_OVERHEAD_RATIO*OTP_INTERVAL+dccm_remain_budget-dccm_overflow_traffic;
				if(debug_budget)
					printf("in the end of epoch, the remaining budget is %ld, dccm_overflow_traffic in this epoch: %ld\n", dccm_remain_budget, dccm_overflow_traffic);
				if(custom_debug)
					printf("in the end of epoch, the remaining budget is %lu, dccm_overflow_traffic in this epoch: %lu, budget in the beginning of epoch: %lu\n", dccm_remain_budget, dccm_overflow_traffic, old_dccm_remain_budget);
				//accumulated_dccm_traffic_overhead+=dccm_overflow_traffic;     
				read_and_write_number=0;

				if(debug_remain_budget){
					std::ostringstream oss;
					oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
					std::string out_file_name = oss.str();
					output_file.open(out_file_name.c_str(), ios::out|ios::app);
			output_file <<"getting into new epoch with remaining budget: "<<dccm_remain_budget<<" , AccumulatedDccmOverhead: "<<accumulated_dccm_traffic_overhead<<" , PageRelevelOverhead: "<<page_releveling_overhead<<" , TotalBlockReleveling: "<<total_block_relevel_stats<<std::endl;
					output_file.close();
					output_file.clear();
				}

				dccm_overflow_traffic=0;
			}

			memory_fetches_total++;
			if (warmup_status == WARMUP_OVER) {
				memory_fetches_stats++;
			}
			UINT64 wc_block_addr = morph_tree.GetCounterAddress(0, morph_tree.GetPhysicalPageAddress(page_begin_addr+i*64));
			UINT64 wccache_set = wc_block_addr & WCCACHE_SET_MASK;
			UINT32 is_wccache_hit = wc_lru_cache.IsWcCacheHit(wccache_set, wc_block_addr, instruction_count, false); 

			wc_cache_tree_access_stats[0]++;
			wc_cache_tree_access_read_stats[0]++;
			if(is_wccache_hit)
				wc_cache_tree_hit_read_stats[0]++;

 
			if (is_wccache_hit < 2) {
				wc_lru_cache.PreemptiveVerification(wc_block_addr, instruction_count);
			}
			UINT64 data_addr = page_begin_addr+i*64;
			UINT64 minor_ctr_pos = (data_addr >> DATA_CACHE_BLOCK_BITS) & MINOR_CTR_POS_MASK;
			//if(debug_page_zero&&((i<100)||(i>(num_of_blocks_each_page-100))))
			wc_lru_cache.Insert(is_wccache_hit, wccache_set, wc_block_addr, instruction_count, false, minor_ctr_pos);
			memory_writebacks_total++; 
			if (warmup_status == WARMUP_OVER) {
				memory_writebacks_stats++; 
			}  
			UINT64 L2_evict_addr = page_begin_addr+i*64;
			during_write=1;
			 wc_lru_cache.WriteBack(L2_evict_addr, instruction_count);	
			//if(debug_page_zero&&((i<100)||(i>(num_of_blocks_each_page-100))))
		}  
	} 
	during_page_zero=0;
	PIN_ReleaseLock(&pinLock);
	*/
	//xinw added-end


	// Check for data cache hit
	UINT64 L1_set = L1[threadid].GetCacheSet(block_addr);
	BOOL L1_hit_status = L1[threadid].IsCacheHit(L1_set, block_addr, false);
	UINT32 L1_block_index = L1[threadid].GetBlockIndex(L1_set, block_addr, false);
	UINT64 L2_set = L2.GetCacheSet(block_addr); 
	// Check if hit in L1 cache
	if(L1_hit_status == CACHE_HIT) {
		//cout<<"cache hit in L1: "<<instruction_count_total<<endl;
		L1_hits_total[threadid*16]++;
		if (warmup_status == WARMUP_OVER) {
			L1_hits_stats[threadid*16]++;
			if(access_type==access_data)
				L1_data_hits_stats[threadid*16]++;
			if(access_type==access_inst)
				L1_inst_hits_stats[threadid*16]++;
			if(access_type==access_page_table)
				L1_page_table_hits_stats[threadid*16]++;

			if(instruction_operation == WRITE_OP)
				L1_write_hits[threadid*16]++;
			else
				L1_read_hits[threadid*16]++;
		}
		L1[threadid].UpdateBlockDirtyStatus(L1_set, block_addr, L1_block_index, instruction_operation, 1, false);
		return CACHE_HIT;
	}
	else {
		PIN_GetLock(&pinLock, threadid + 1);
		//xinw added for counter cache in L1-begin
		
		bool need_page_zero = morph_tree.NeedPageZero(block_addr);
		UINT64 debug_physical_page_addr = morph_tree.GetPhysicalPageAddress(block_addr);
		if(debug_physical_page_addr==20000000) 
			printf("\n");
		during_page_zero=0;
		if(need_page_zero){
			total_page_zero_events++;
			during_page_zero=1;
			printf("page zeroing\n");
			UINT64 page_begin_addr=block_addr&FOURK_PAGE_ADDR_FLOOR_MASK;
			if(huge_page)
				page_begin_addr=block_addr&TWOM_PAGE_ADDR_FLOOR_MASK;
			UINT64 num_of_blocks_each_page=64;
			if(huge_page)
				num_of_blocks_each_page=64*512;
			for(UINT64 i=0;i<num_of_blocks_each_page;i++){	  
				read_and_write_number++;
				if(read_and_write_number>=OTP_INTERVAL){
					dccm_remain_budget=DCCM_OVERHEAD_RATIO*OTP_INTERVAL+dccm_remain_budget-dccm_overflow_traffic;
					read_and_write_number=0;
					if(debug_remain_budget){
						std::ostringstream oss;
						oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
						std::string out_file_name = oss.str();
						output_file.open(out_file_name.c_str(), ios::out|ios::app);
						output_file <<"getting into new epoch with remaining budget: "<<dccm_remain_budget<<" , AccumulatedDccmOverhead: "<<accumulated_dccm_traffic_overhead<<" , PageRelevelOverhead: "<<page_releveling_overhead<<" , TotalBlockReleveling: "<<total_block_relevel_stats<<std::endl;
						output_file.close();
						output_file.clear();
					}
					dccm_overflow_traffic=0;
				}
				memory_fetches_total++;
				if (warmup_status == WARMUP_OVER) {
					memory_fetches_stats++;
				}
				UINT64 wc_block_addr = morph_tree.GetCounterAddress(0, morph_tree.GetPhysicalPageAddress(page_begin_addr+i*64));
				UINT64 wccache_set = wc_block_addr & WCCACHE_SET_MASK;
				UINT32 is_wccache_hit = wc_lru_cache.IsWcCacheHit(wccache_set, wc_block_addr, instruction_count, false); 
				wc_cache_tree_access_stats[0]++;
				wc_cache_tree_access_read_stats[0]++;
				if(is_wccache_hit)
					wc_cache_tree_hit_read_stats[0]++;
				if (is_wccache_hit < 2) {
					wc_lru_cache.PreemptiveVerification(wc_block_addr, instruction_count);
				}
				UINT64 data_addr = page_begin_addr+i*64;
				UINT64 minor_ctr_pos = (data_addr >> DATA_CACHE_BLOCK_BITS) & MINOR_CTR_POS_MASK;
				wc_lru_cache.Insert(is_wccache_hit, wccache_set, wc_block_addr, instruction_count, false, minor_ctr_pos);
				memory_writebacks_total++; 
				if (warmup_status == WARMUP_OVER) {
					memory_writebacks_stats++; 
				}  
				UINT64 L2_evict_addr = page_begin_addr+i*64;
				during_write=1;
				wc_lru_cache.WriteBack(L2_evict_addr, instruction_count);	
			}  
		} 
		during_page_zero=0;
		UINT64 counter_wc_block_addr = morph_tree.GetCounterAddress(0, morph_tree.GetPhysicalPageAddress(block_addr));
		UINT64 L1_counter_set = L1_counter_caches[threadid].GetCacheSet(counter_wc_block_addr*64); 
		BOOL L1_counter_hit_status = L1_counter_caches[threadid].IsCacheHit(L1_counter_set, counter_wc_block_addr*64, true);
		if(L1_counter_hit_status != CACHE_HIT) {
			if (warmup_status == WARMUP_OVER) 
				num_of_total_counter_accesses[threadid*16]++;
			if (L1_counter_caches[threadid].SetFullStatus(L1_counter_set) == SET_NOT_FULL) {
				L1_counter_caches[threadid].InsertNoEviction(L1_counter_set, counter_wc_block_addr*64, false, 1, true);		
			} else {
				UINT32 L1_counter_evict_index = L1_counter_caches[threadid].GetEvictedIndex(L1_counter_set);		
				if ((!(L1_counter_caches[threadid].GetIsUsed(L1_counter_set, L1_counter_evict_index)))&&(warmup_status == WARMUP_OVER)) {
					num_of_useless_counter_accesses[threadid*16]++;
				}
				L1_counter_caches[threadid].InsertWithEviction(L1_counter_set, counter_wc_block_addr*64, L1_counter_evict_index, false, 1, true);		
			}	
		}
		/*
		instruction_count_total=get_total_inst_num();
		cout<<"inst num: "<<instruction_count_total<<endl;
		if((instruction_count_total > fast_forward_value)&&((instruction_count_total-fast_forward_value)>(1000000000*index_of_billion))){    
			//if((instruction_count_total > fast_forward_value)&&((instruction_count_total-fast_forward_value)>(10000000*index_of_billion))){    
			time_t my_time = time(NULL);
			cout << " pintool finish " <<index_of_billion<< " billion instructions at time "<<ctime(&my_time) <<endl;
			step_value = 0;
			PrintResults();
			index_of_billion++;
			if (instruction_count_total > (fast_forward_value + RUN_INST_VAL))
			{
				exit(0);
			}

		}
		*/
		//xinw added for counter cache in L1-end
		L2_accesses_stats++;	
		if(access_type==access_data)
			L2_data_accesses_stats++;
		if(access_type==access_inst)
			L2_inst_accesses_stats++;
		if(access_type==access_page_table)
			L2_page_table_accesses_stats++;
		BOOL L2_hit_status = L2.IsCacheHit(L2_set, block_addr, false);
		if(L2_hit_status == CACHE_HIT) {
			L2_hits_total++;
			if (warmup_status == WARMUP_OVER) {
				L2_hits_stats++;
				if(access_type==access_data)
					L2_data_hits_stats++;
				if(access_type==access_inst)
					L2_inst_hits_stats++;
				if(access_type==access_page_table)
					L2_page_table_hits_stats++;
			}
			UINT32 L2_block_index = L2.GetBlockIndex(L2_set, block_addr, false);
			UINT32 L2_block_dirty_status = L2.GetBlockDirtyStatus(L2_set, L2_block_index);
			// Invalidate L2 block
			L2.InvalidateBlock(L2_set, L2_block_index);
			if (L2_block_dirty_status == WRITE_OP || instruction_operation == WRITE_OP) {
				L2_block_dirty_status = WRITE_OP;
			}
			if (L1[threadid].SetFullStatus(L1_set) == SET_NOT_FULL) {
				L1[threadid].InsertNoEviction(L1_set, block_addr, L2_block_dirty_status, 1, false);		
			} else {
				UINT32 L1_evict_index = L1[threadid].GetEvictedIndex(L1_set);		
				UINT64 L1_evict_addr = L1[threadid].GetBlockAddress(L1_set, L1_evict_index);
				UINT32 L1_evict_dirty_status = L1[threadid].GetBlockDirtyStatus(L1_set, L1_evict_index);
				L1[threadid].InsertWithEviction(L1_set, block_addr, L1_evict_index, L2_block_dirty_status, 1, false);		
				if (warmup_status == WARMUP_OVER) {
					L1_eviction_stats[threadid*16]++;
				}
				UINT64 L2_evict_set = L2.GetCacheSet(L1_evict_addr);
				UINT32 L2_evict_index = L2.GetBlockIndex(L2_evict_set, L1_evict_addr, false);
				if (L2_evict_index < OUT_OF_BOUND) {
					L2.UpdateBlockDirtyStatus(L2_evict_set, L1_evict_addr, L2_evict_index, L1_evict_dirty_status, 2, false);
					PIN_ReleaseLock(&pinLock);
					return CACHE_HIT;
				} else {
					wc_lru_cache.InsertL2(L1_evict_addr, L1_evict_dirty_status, false);
				}
			}
			PIN_ReleaseLock(&pinLock);
			return CACHE_HIT;
		}
		PIN_ReleaseLock(&pinLock);
	} 
	PIN_GetLock(&pinLock, threadid + 1);
	/*
	bool need_page_zero = morph_tree.NeedPageZero(block_addr);
	UINT64 debug_physical_page_addr = morph_tree.GetPhysicalPageAddress(block_addr);
	if(debug_physical_page_addr==20000000) 
		printf("\n");
	during_page_zero=0;
	if(need_page_zero)
	{
		total_page_zero_events++;
		during_page_zero=1;
		printf("page zeroing\n");
		//UINT64 page_begin_addr=block_addr&FOURK_PAGE_ADDR_FLOOR_MASK;
		UINT64 page_begin_addr=block_addr&FOURK_PAGE_ADDR_FLOOR_MASK;
		if(huge_page)
			page_begin_addr=block_addr&TWOM_PAGE_ADDR_FLOOR_MASK;
		UINT64 num_of_blocks_each_page=64;
		if(huge_page)
			num_of_blocks_each_page=64*512;
		for(UINT64 i=0;i<num_of_blocks_each_page;i++)
		{	  
			read_and_write_number++;
			if(read_and_write_number>=OTP_INTERVAL)
			{
				UINT64 old_dccm_remain_budget = dccm_remain_budget;
				dccm_remain_budget=DCCM_OVERHEAD_RATIO*OTP_INTERVAL+dccm_remain_budget-dccm_overflow_traffic;
				if(debug_budget)
					printf("in the end of epoch, the remaining budget is %ld, dccm_overflow_traffic in this epoch: %ld\n", dccm_remain_budget, dccm_overflow_traffic);
				if(custom_debug)
					printf("in the end of epoch, the remaining budget is %lu, dccm_overflow_traffic in this epoch: %lu, budget in the beginning of epoch: %lu\n", dccm_remain_budget, dccm_overflow_traffic, old_dccm_remain_budget);
				//accumulated_dccm_traffic_overhead+=dccm_overflow_traffic;     
				read_and_write_number=0;

				if(debug_remain_budget){
					std::ostringstream oss;
					oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
					std::string out_file_name = oss.str();
					output_file.open(out_file_name.c_str(), ios::out|ios::app);
			output_file <<"getting into new epoch with remaining budget: "<<dccm_remain_budget<<" , AccumulatedDccmOverhead: "<<accumulated_dccm_traffic_overhead<<" , PageRelevelOverhead: "<<page_releveling_overhead<<" , TotalBlockReleveling: "<<total_block_relevel_stats<<std::endl;
					output_file.close();
					output_file.clear();
				}

				dccm_overflow_traffic=0;
			}

			memory_fetches_total++;
			if (warmup_status == WARMUP_OVER) {
				memory_fetches_stats++;
			}
			UINT64 wc_block_addr = morph_tree.GetCounterAddress(0, morph_tree.GetPhysicalPageAddress(page_begin_addr+i*64));
			UINT64 wccache_set = wc_block_addr & WCCACHE_SET_MASK;
			UINT32 is_wccache_hit = wc_lru_cache.IsWcCacheHit(wccache_set, wc_block_addr, instruction_count, false); 

			wc_cache_tree_access_stats[0]++;
			wc_cache_tree_access_read_stats[0]++;
			if(is_wccache_hit)
				wc_cache_tree_hit_read_stats[0]++;

 
			if (is_wccache_hit < 2) {
				wc_lru_cache.PreemptiveVerification(wc_block_addr, instruction_count);
			}
			UINT64 data_addr = page_begin_addr+i*64;
			UINT64 minor_ctr_pos = (data_addr >> DATA_CACHE_BLOCK_BITS) & MINOR_CTR_POS_MASK;
			//if(debug_page_zero&&((i<100)||(i>(num_of_blocks_each_page-100))))
			wc_lru_cache.Insert(is_wccache_hit, wccache_set, wc_block_addr, instruction_count, false, minor_ctr_pos);
			memory_writebacks_total++; 
			if (warmup_status == WARMUP_OVER) {
				memory_writebacks_stats++; 
			}  
			UINT64 L2_evict_addr = page_begin_addr+i*64;
			during_write=1;
			 wc_lru_cache.WriteBack(L2_evict_addr, instruction_count);	
			//if(debug_page_zero&&((i<100)||(i>(num_of_blocks_each_page-100))))
		}  
	} 
	during_page_zero=0;
	*/
	instruction_count_total=get_total_inst_num();
	if((instruction_count_total > fast_forward_value)&&((instruction_count_total-fast_forward_value)>(1000000000*index_of_billion))){    
	//if((instruction_count_total > fast_forward_value)&&((instruction_count_total-fast_forward_value)>(100000000*index_of_billion))){    
		time_t my_time = time(NULL);
		cout << " pintool finish " <<index_of_billion<< " billion instructions at time "<<ctime(&my_time) <<endl;
		step_value = 0;
		PrintResults();
		index_of_billion++;
		if (instruction_count_total > (fast_forward_value + RUN_INST_VAL))
		{
			exit(0);
		}

	}
	

	read_and_write_number++;
	if(read_and_write_number>=OTP_INTERVAL)
	{
		UINT64 old_dccm_remain_budget = dccm_remain_budget;
		dccm_remain_budget=DCCM_OVERHEAD_RATIO*OTP_INTERVAL+dccm_remain_budget-dccm_overflow_traffic;
		if(debug_budget)
			printf("in the end of epoch, the remaining budget is %ld, dccm_overflow_traffic in this epoch: %ld\n", dccm_remain_budget, dccm_overflow_traffic);

		if(custom_debug)
			printf("in the end of epoch, the remaining budget is %lu, dccm_overflow_traffic in this epoch: %lu, budget in the beginning of epoch: %lu\n", dccm_remain_budget, dccm_overflow_traffic, old_dccm_remain_budget);
		read_and_write_number=0;
		if(debug_remain_budget){
			std::ostringstream oss;
			oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
			std::string out_file_name = oss.str();
			output_file.open(out_file_name.c_str(), ios::out|ios::app);
			output_file <<"getting into new epoch with remaining budget: "<<dccm_remain_budget<<" , AccumulatedDccmOverhead: "<<accumulated_dccm_traffic_overhead<<" , PageRelevelOverhead: "<<page_releveling_overhead<<" , TotalBlockReleveling: "<<total_block_relevel_stats<<std::endl;
			output_file.close();
			output_file.clear();
		}
		dccm_overflow_traffic=0;
	}
	period_counter++;
	memory_fetches_total++;
	if (warmup_status == WARMUP_OVER) {
		memory_fetches_stats++;
	}
	if (stack_status == 1) {
		stack_memory_fetches_total++;
	} else {
		heap_memory_fetches_total++;
	}
	UINT32 is_wccache_hit = 0;
	is_first_touch=0;
	UINT64 physical_page_addr = morph_tree.GetPhysicalPageAddress(block_addr);
	/*if (physical_page_addr >= 4194304) {
		std::cout << "PhysicalPage Overflow " << physical_page_addr << std::endl;
		PIN_WriteErrorMessage("PhysicalPage Overflow", 1002, PIN_ERR_FATAL, 0);
	} */
	UINT64 wc_block_addr = morph_tree.GetCounterAddress(0, physical_page_addr);
	//xinw added for L1 counter cache-begin	
	UINT64 counter_wc_block_addr = wc_block_addr;
	UINT64 L1_counter_set = L1_counter_caches[threadid].GetCacheSet(counter_wc_block_addr*64); 
	BOOL L1_counter_hit_status = L1_counter_caches[threadid].IsCacheHit(L1_counter_set, counter_wc_block_addr*64, true);
	if(L1_counter_hit_status == CACHE_HIT) {
    		L1_counter_caches[threadid].UpdateIsUsed(L1_counter_set, counter_wc_block_addr*64);
	}

	//xinw added for L1 counter cache-end


	UINT64 wccache_set = wc_block_addr & WCCACHE_SET_MASK;
	is_wccache_hit = wc_lru_cache.IsWcCacheHit(wccache_set, wc_block_addr, instruction_count, false); 
	wc_cache_tree_access_stats[0]++;
	wc_cache_tree_access_read_stats[0]++;
	if(is_wccache_hit)
		wc_cache_tree_hit_read_stats[0]++;
	if(debug_metadata_cache){
		printf("tree node level: 0 , wc block address: %lu , hit/miss(0 means miss): %u, due to normal data read\n ",   wc_block_addr, is_wccache_hit);
	}
	if (is_wccache_hit < 2) {
		wc_lru_cache.PreemptiveVerification(wc_block_addr, instruction_count);
		if(OTP_L1_PRECALCULATION){
    			UINT64 L1_wc_block_addr = morph_tree.GetParentCounterAddress(0, wc_block_addr);
			UINT64 L1_counter_id = morph_tree.GetLevelCounterId(1, L1_wc_block_addr);
    			UINT64 L1_minor_ctr_pos = L1_counter_id & MINOR_CTR_POS_MASK;
			UINT64 effective_l1_counter = morph_tree.GetEffectiveCounter(1, L1_counter_id, L1_minor_ctr_pos);
			otp_table.access(effective_l1_counter, is_wccache_hit);
        	} 
	}
	UINT64 counter_id = morph_tree.GetLevelCounterId(0, wc_block_addr);
	UINT64 minor_ctr_pos = (block_addr >> DATA_CACHE_BLOCK_BITS) & MINOR_CTR_POS_MASK;
	wc_lru_cache.Insert(is_wccache_hit, wccache_set, wc_block_addr, instruction_count, false, minor_ctr_pos);
	UINT64 effective_version = morph_tree.GetEffectiveCounter(0, counter_id, minor_ctr_pos);
	is_first_touch=0;
	global_counter_id=counter_id;
	global_minor_ctr_pos=minor_ctr_pos;
	//xinw added for otp precomputation-begin
		if(OTP_PRECALCULATION) 
	{
		dccm_block_level_relevel=0;
		current_block_addr_access_aes_table=block_addr;
		otp_table.access(effective_version, is_wccache_hit); 
		if (otp_table.need_activel_relevel(effective_version)) 
		{	
			if(debug_block_relevel){
				std::ostringstream oss;
				oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
				std::string out_file_name = oss.str();
				output_file.open(out_file_name.c_str(), ios::out|ios::app);
				output_file<<"block address: "<<block_addr<<" miss in table with counter value: "<<effective_version<<" , while remaining budeget is: "<<(DCCM_OVERHEAD_RATIO*OTP_INTERVAL+dccm_remain_budget-dccm_overflow_traffic)<<" at instruction: "<<(instruction_count_total-fast_forward_value)<<std::endl;
				output_file.close();
				output_file.clear();
			}
			during_write=0;
			same_relevel_for_small_and_big=false;
			//UINT32 read_is_wccache_hit = wc_lru_cache.IsWcCacheHit(wccache_set, wc_block_addr, instruction_count); 
			if((!wc_lru_cache.will_overflow(counter_id, wc_block_addr, minor_ctr_pos, instruction_count, true, true))&&otp_table.counter_satisfy_requirement_for_consuming_dccm_budget(effective_version) )
			{
				if(debug_block_relevel){
					std::ostringstream oss;
					oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
					std::string out_file_name = oss.str();
					output_file.open(out_file_name.c_str(), ios::out|ios::app);
					output_file<<"block address: "<<block_addr<<" with counter value: "<<effective_version<<" , doing block releveling at instruction: "<<(instruction_count_total-fast_forward_value)<<std::endl;
					output_file.close();
					output_file.clear();
				}
				int64_t _dccm_traffic_before_consuming_budget=dccm_overflow_traffic;		
				total_block_relevel_stats+=1;
				dccm_block_level_relevel=1;
				dccm_overflow_traffic+=1;
				accumulated_dccm_traffic_overhead++;
				memory_writebacks_total++; 
				memory_writebacks_while_releveling_total++; 
				if (warmup_status == WARMUP_OVER) {
					memory_writebacks_stats++; 
				}
				//UINT32 read_is_wccache_hit = wc_lru_cache.IsWcCacheHit(wccache_set, wc_block_addr, instruction_count); 
				//wc_lru_cache.Insert(read_is_wccache_hit, wccache_set, wc_block_addr, instruction_count, true, minor_ctr_pos);
				//is_dccm_overhead=true;
				wc_lru_cache.ChainCounterIncrementForPageRelevel(counter_id, wc_block_addr, minor_ctr_pos, instruction_count, true, true, 1000);
				int64_t _dccm_traffic_after_consuming_budget=dccm_overflow_traffic;		
				UINT64 group_for_releveling=otp_table.get_nearest_bottom_group(effective_version);
				dccm_overhead_for_groups[group_for_releveling]+=_dccm_traffic_after_consuming_budget-_dccm_traffic_before_consuming_budget;			
				dccm_block_relevel_overhead_for_groups[group_for_releveling]+=_dccm_traffic_after_consuming_budget-_dccm_traffic_before_consuming_budget;
				//is_dccm_overhead=false;
				_dccm_counter=0;
			}
			else{
				if((!wc_lru_cache.will_overflow(counter_id, wc_block_addr, minor_ctr_pos, instruction_count, true, true))){
					if(debug_block_relevel){
						std::ostringstream oss;
						oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
						std::string out_file_name = oss.str();
						output_file.open(out_file_name.c_str(), ios::out|ios::app);
						output_file<<"block address: "<<block_addr<<" with counter value: "<<effective_version<<" , cancel block releveling due to budget limit  at instruction: "<<(instruction_count_total-fast_forward_value)<<std::endl;
						output_file.close();
						output_file.clear();
					}
		
				}
				else if(possibilistic_page_level_dccm_relevel()&&otp_table.remaining_dccm_budget_nonzero()){
					if(debug_block_relevel){
						std::ostringstream oss;
						oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
						std::string out_file_name = oss.str();
						output_file.open(out_file_name.c_str(), ios::out|ios::app);
						output_file<<"block address: "<<block_addr<<" with counter value: "<<effective_version<<" , incurs relevel in overflow_node_queue at instruction: "<<(instruction_count_total-fast_forward_value)<<std::endl;
						output_file.close();
						output_file.clear();
					}
					_dccm_counter=0;
					relevel_for_overflow_nodes();
				}
				else{
					
			   		if(otp_table.counter_satisfy_requirement_for_epoch_period(effective_version)){
						//wc_lru_cache.enqueue_overflow_nodes(counter_id, wc_block_addr, minor_ctr_pos, instruction_count, true, true);
						if(debug_block_relevel){
							std::ostringstream oss;
							oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
							std::string out_file_name = oss.str();
							output_file.open(out_file_name.c_str(), ios::out|ios::app);
							output_file<<"block address: "<<block_addr<<" with counter value: "<<effective_version<<" , doing enqueueing to overflow_node_queue at instruction: "<<(instruction_count_total-fast_forward_value)<<std::endl;
							output_file.close();
							output_file.clear();
						}
						wc_lru_cache.enqueue_overflow_nodes(counter_id, wc_block_addr, minor_ctr_pos, instruction_count, true, true);
					}
					else{
							if(debug_block_relevel){
							std::ostringstream oss;
							oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
							std::string out_file_name = oss.str();
							output_file.open(out_file_name.c_str(), ios::out|ios::app);
							UINT64 current_small_epoch_index=(otp_table.tick_in_current_interval/(AES_OTP_INTERVAL/TABLE_SIZE));
							if(current_small_epoch_index>=TABLE_SIZE)
								current_small_epoch_index=TABLE_SIZE-1;
							output_file<<"block address: "<<block_addr<<" with counter value: "<<effective_version<<" , not doing enqueueing to overflow_node_queue at instruction: "<<(instruction_count_total-fast_forward_value)<<" becauese the largest counter available to enqueue is: "<<otp_table.table[current_small_epoch_index] <<std::endl;
							output_file.close();
							output_file.clear();
						}
					}
					//wc_lru_cache.Insert(read_is_wccache_hit, wccache_set, wc_block_addr, instruction_count, false, minor_ctr_pos);
				}
			}
			dccm_block_level_relevel=0;
		}
    } 
    else if(OTP_L1_PRECALCULATION){
		if((counter_for_relevel_L1_counter++)%100<(possibility_for_updating_L1_while_read*100)){
			if((!wc_lru_cache.will_overflow(counter_id, wc_block_addr, minor_ctr_pos, instruction_count, true, true))){
				wc_lru_cache.ChainCounterIncrementFromL1(counter_id, wc_block_addr, minor_ctr_pos, instruction_count, true, true, 1000);
			}else if (otp_table.remaining_dccm_budget_nonzero()){
					is_dccm_overhead=true;
					wc_lru_cache.ChainCounterIncrementFromL1(counter_id, wc_block_addr, minor_ctr_pos, instruction_count, true, true, 1000);
					is_dccm_overhead=false;
			}
			if(debug_memoize_l1)
				printf("possibilistic l1 relevel: yes\n");
		}
		else{
			if(debug_memoize_l1)
				printf("possibilistic l1 relevel: no\n");
		}
    }
    else if((int)KnobRecentAes.Value()){
        if(access_recent_aes(effective_version))
            otp_table_hit_stats++;	
        else{		
            otp_table_miss_stats++;	
        }
    }
	else
	{
		current_block_addr_access_aes_table=block_addr;
	}
	INT32 evict_type = MISS_NO_EVICT;
	if (L1[threadid].SetFullStatus(L1_set) == SET_NOT_FULL) {
		L1[threadid].InsertNoEviction(L1_set, block_addr, instruction_operation, 3, false);		
	} else {
		UINT32 L1_evict_index = L1[threadid].GetEvictedIndex(L1_set);		
		UINT64 L1_evict_addr = L1[threadid].GetBlockAddress(L1_set, L1_evict_index);
		UINT32 L1_evict_dirty_status = L1[threadid].GetBlockDirtyStatus(L1_set, L1_evict_index);
		L1[threadid].InsertWithEviction(L1_set, block_addr, L1_evict_index, instruction_operation, 3, false);		
		if (warmup_status == WARMUP_OVER) {
			L1_eviction_stats[threadid*16]++;
		}
		UINT64 L2_evict_set = L2.GetCacheSet(L1_evict_addr);
		UINT32 L2_evict_index = L2.GetBlockIndex(L2_evict_set, L1_evict_addr, false);
		if (L2_evict_index < OUT_OF_BOUND) {
			L2.UpdateBlockDirtyStatus(L2_evict_set, L1_evict_addr, L2_evict_index, L1_evict_dirty_status, 3, false);
			/*std::ostringstream oss_simout;
			oss_simout<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
			std::string out_file_name = oss_simout.str();
			output_file.open(out_file_name.c_str(), ios::out|ios::app);
			output_file << "assertion error: eviction from L1 incurs L2 hit"<<std::endl;
			output_file.close();
			output_file.clear();
			exit(0);*/
		} else {
			wc_lru_cache.InsertL2(L1_evict_addr, L1_evict_dirty_status, false);
		}
	}
	PIN_ReleaseLock(&pinLock);
	return evict_type;
}


VOID RecordMemRead(VOID * ip, VOID * addr, UINT32 stack_status, ADDRINT rbp, ADDRINT rsp, THREADID threadid) {

	ADDRINT * addr_ptr = (ADDRINT*)addr;
	ADDRINT value;
	PIN_SafeCopy(&value, addr_ptr, sizeof(ADDRINT));
	//uint64_t right_shift_value=value>>20;
	/*if  (is_gap&&(right_shift_value==498374593872)){
		PIN_GetLock(&pinLock, threadid + 1);
		current_iteration_of_microbenchmark++;
		fprintf(stdout, "the lowest-20-bits value while loading flag variable is %lu by PC %lu in iteration %d\n", (value-(right_shift_value<<20)), *((uint64_t *)ip), current_iteration_of_microbenchmark);
			fflush(stdout);
		if((value-(right_shift_value<<20))==7){
			fast_forward_value=get_total_inst_num();	
			printf("number of insts executed before observation window: %lu\n", instruction_count_total);
			
			warmup_status = WARMUP_OVER;
		}
		PIN_ReleaseLock(&pinLock);
	}*/
	if(is_gap){
	instruction_count_total=get_total_inst_num();
	if(instruction_count_total > fast_forward_value){
		warmup_status = WARMUP_OVER;	
	  }
	}
	//if((instruction_count_total > fast_forward_value)&&((instruction_count_total-fast_forward_value)>(1000000000*index_of_billion))){    
	
	if (warmup_status == WARMUP_OVER) {
		memory_instruction_count_stats[threadid*16]++;
		CacheCall(threadid, READ_OP, instruction_count_total, (UINT64)ip, ((UINT64)(addr)+CACHELINE_OFFSET) & DATA_BLOCK_FLOOR_ADDR_MASK, stack_status, false, access_data);		
	}
}


VOID RecordMemWrite(VOID * ip, VOID * addr, UINT32 stack_status, ADDRINT rbp, ADDRINT rsp, THREADID threadid) {
        //current_written_block_address=(UINT64)addr;
	ADDRINT * addr_ptr = (ADDRINT*)addr;
	ADDRINT value;
	PIN_SafeCopy(&value, addr_ptr, sizeof(ADDRINT));
	if (warmup_status == WARMUP_OVER) {
		memory_instruction_count_stats[threadid*16]++;
		CacheCall(threadid, WRITE_OP, instruction_count_total, (UINT64)ip, ((UINT64)(addr)+CACHELINE_OFFSET) & DATA_BLOCK_FLOOR_ADDR_MASK, stack_status, false, access_data);
	}
}

bool have_not_print_for_first_update=true;
VOID access_instruction(VOID * ip, THREADID threadid){
	if(access_inst_and_page_table){
		CacheCall(threadid, READ_OP, instruction_count_total, (UINT64)ip, get_inst_block_address((UINT64)ip), 0, false, access_inst);
		//L1_read_inst_accesses++;
	}
}
//xinw added for test-begin
bool CheckWord(char* filename, char* search){
        //unsigned int offset; 
        long unsigned offset=0;
        string line;
        ifstream Myfile;
        //Myfile.open (filename);
        //Myfile.open (KnobOutputDir.Value().c_str()  + suffix_string);
        line= KnobOutputDir.Value().c_str()+string("/end");
        std::istringstream oss_end(line);

        std::string input_file_name = oss_end.str();    
        //printf("file name: %s\n", filename);
        //printf("keyword: %s\n", search);
        string key_word(search);     
        Myfile.open (input_file_name.c_str(), ios::in);
        bool ret=false;
        if (Myfile.is_open()){
                while (!Myfile.eof()){
                        getline(Myfile,line);
     
                        //if ((offset = line.find(search, 0)) != string::npos) {
                        if ((offset = line.find(key_word, 0)) != string::npos) {
                                //Myfile.close();
                                ret=true;
                                break;
                                //return true;
                        }    
                }    
                Myfile.close();
        }    
        else{
                cout << "Unable to open this file." << endl;
                printf("file name: %s\n", filename);
                printf("keyword: %s\n", search);
        }    
     
        //return false;
        //cout <<"line: "<<line<<endl;
        //printf("keyword: %s\n", search);
        //printf("offset: %lu\n", offset);
        //printf("npos: %lu\n", string::npos);
        return ret; 
}
//xinw added for test-end

VOID docount(VOID * ip, THREADID threadid) { 
	/*
	if (warmup_status == WARMUP_OVER){ 
		if(instruction_count_stats_thread[threadid*16]%100000==0){
			PIN_GetLock(&pinLock, threadid + 1);
			global_instruction_count_stats +=100000;
			if((global_instruction_count_stats > fast_forward_value)&&((global_instruction_count_stats-fast_forward_value)>(10000000*index_of_billion))){    
				time_t my_time = time(NULL);
				cout << " pintool finish " <<index_of_billion<< " billion instructions at time "<<ctime(&my_time) <<endl;
				step_value = 0;
				PrintResults();
				index_of_billion++;
			}

			//cout<<"thread "<<threadid<<" instructions: "<<instruction_count_stats_thread[threadid*16]<<endl;

			PIN_ReleaseLock(&pinLock);
		}
	}*/
  instruction_count_stats_thread[threadid*16]++;
/*
  PIN_GetLock(&pinLock, threadid + 1);
  instruction_count_total++;
  if (warmup_status == WARMUP_OVER) {
    instruction_count_stats++;
    access_instruction(ip, threadid);
  }
  if (instruction_count_total == fast_forward_value) {
    std::cout << "FastForward Complete" << std::endl;
  }
  //xinw added for test-begin
  char key_file_name[] = "end";
  char key_word[]="end_of_initialization";
  //char key_word[]="begin_of_initialization";
  if ((instruction_count_total < fast_forward_value)&&(instruction_count_total%100000000==0)) {
          if(CheckWord(key_file_name, key_word)){
                  fast_forward_value = instruction_count_total;
                   time_t my_time = time(NULL);
                  cout << "fast forwarding for " << instruction_count_total  << "instructions at time "<<ctime(&my_time) <<endl;
          }
  } 
  if ((instruction_count_total > fast_forward_value)&&(instruction_count_total%1000000000==0)) {
  	  char key_word_loop[]="end_of_loop";
          if(CheckWord(key_file_name, key_word_loop)){
                   time_t my_time = time(NULL);
                   cout << " pintool finish loop after running "<<(instruction_count_total-fast_forward_value)<<" instructions at time "<<ctime(&my_time) <<endl;
		   step_value = 0;
 	 	   PrintResults();
        	   exit(0);
          }

  }

  //xinw added for test-end



  if((instruction_count_total > fast_forward_value)&&((instruction_count_total-fast_forward_value)>(1000000000*index_of_billion))){    
	time_t my_time = time(NULL);
        cout << " pintool finish " <<index_of_billion<< " billion instructions at time "<<ctime(&my_time) <<endl;
	have_not_print_for_first_update=false;	
	step_value = 0;
 	PrintResults();
        index_of_billion++;
  }
	PIN_ReleaseLock(&pinLock);
*/

}
// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
  //printf("execute inst with address: %lu number: %lu\n", INS_NextAddress(ins), instruction_count_total);
  // Instruments memory accesses using a predicated call, i.e.
  // the instrumentation is called iff the instruction will actually be executed.
  //
  // On the IA-32 and Intel(R) 64 architectures conditional moves and REP 
  // prefixed instructions appear as predicated instructions in Pin.
  INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_INST_PTR, IARG_THREAD_ID, IARG_END);

  UINT32 memOperands = INS_MemoryOperandCount(ins); 

  // Iterate over each memory operand of the instruction.
  for (UINT32 memOp = 0; memOp < memOperands; memOp++) {
    if (INS_MemoryOperandIsRead(ins, memOp)) {
      UINT32 is_stack = INS_IsStackRead(ins);
      INS_InsertPredicatedCall(
           ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
           IARG_INST_PTR,
           IARG_MEMORYOP_EA, memOp,
           IARG_UINT32, is_stack,
           IARG_REG_VALUE, REG_RBP,
           IARG_REG_VALUE, REG_RSP,IARG_THREAD_ID,
           IARG_END);
    }
      // Note that in some architectures a single memory operand can be 
      // both read and written (for instance incl (%eax) on IA-32)
      // In that case we instrument it once for read and once for write.
    if (INS_MemoryOperandIsWritten(ins, memOp)) {
      UINT32 is_stack =  INS_IsStackWrite(ins);
      INS_InsertPredicatedCall(
           ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
           IARG_INST_PTR,
           IARG_MEMORYOP_EA, memOp,
           IARG_UINT32, is_stack,
           IARG_REG_VALUE, REG_RBP,
           IARG_REG_VALUE, REG_RSP,IARG_THREAD_ID,
           IARG_END);
    }
  } 
}



VOID Fini(INT32 code, VOID *v)
{ 
  step_value = 0;
  PrintResults();
  //PrintOverflowResults();
  //PrintMissResults();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */
   
INT32 Usage()
{
  PIN_ERROR( "This Pintool prints a trace of memory addresses\n" 
	     + KNOB_BASE::StringKnobSummary() + "\n");
  return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
 
L1= (DataCache*)malloc(sizeof(DataCache)*max_num_of_threads);
L1_counter_caches= (DataCache*)malloc(sizeof(DataCache)*max_num_of_threads);
for(int i=0;i<max_num_of_threads;i++){
	//L1[i]=DataCache(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE);
	L1[i].Initialize(RPOLICY_LRU, L1_SET_BITS, L1_SET_NUM, L1_SET_SIZE);
	L1_counter_caches[i].Initialize(RPOLICY_LRU, L1_counter_SET_BITS, L1_counter_SET_NUM, L1_counter_SET_SIZE);
	
}

//xinw added for measuring running time of pintool job
// clock_t t1, t2;
//test();
  RandomNumberCpt *random_numbers_cpt=(RandomNumberCpt *)malloc(sizeof(RandomNumberCpt));

  TreeNodesCpt *tree_nodes_cpt_ptr=(TreeNodesCpt *)malloc(sizeof(TreeNodesCpt));
  //TreeNodesCpt tree_nodes_cpt;

  t1 = clock();
  deviation=(int)KnobDeviation.Value();
 
  memset(output_buff, '\0', sizeof(output_buff));
  setvbuf(stdout, output_buff, _IOFBF, 4);

  PIN_InitSymbols();
  
  if(PIN_Init(argc, argv))
    return Usage();
  possibility_for_updating_L1_while_read=(double)KnobPossibilityForUpdatingL1WhileRead.Value();
  time_disable_page_level_relevel_with_overhead=(double)KnobTimeDisablePageLevelRelevelWithOverhead.Value();
  insert_metadata_into_LLC=(int)KnobInsertMetadataIntoLLC.Value();
 
  deprioritize_tree_nodes_in_metadata_cache=KnobDeprioritizeTreeNodesInMetadataCache.Value();
  access_inst_and_page_table=(int)KnobAccessInstAndPageTable.Value();
  use_sampling_for_inserting_metadata_into_LLC=(int)KnobUseSamplingForInsertingMetadataIntoLLC.Value();
  AES_OTP_INTERVAL=(UINT64)KnobAesEpoch.Value();
  OTP_INTERVAL=AES_OTP_INTERVAL;
  far_relevel=(int)KnobFarRelevel.Value();
  use_page_zero=(int)KnobUsePageZero.Value();
  huge_page=(int)KnobHugePage.Value();
  //OTP_PRECALCULATION=(int)KnobPredictiveDecryption.Value();
  OTP_PRECALCULATION=0;
  OTP_L1_PRECALCULATION=(int)KnobPredictiveDecryption.Value();
  fast_forward_value = (UINT64)KnobFastForward.Value();  
  skip_gap_init = (UINT64)KnobSkipGapInit.Value();  
  RUN_INST_VAL = (UINT64)KnobRunInsts.Value();
  BEGIN_WITH_BIG_COUNTER = KnobRandomInit.Value();
  generate_tree_nodes=(int)KnobGenerateTreeNodes.Value();
  mcr_overflow=(int)KnobMcrOverflow.Value();
  invalidate_dirty_version_in_metadata_cache=(int)KnobInvalidateDirtyVersionInMetadataCache.Value();
 // use_drrip=(int)KnobUseDrrip.Value();
  if(BEGIN_WITH_BIG_COUNTER)
	morph_tree.InitTree(true);
  else
	morph_tree.InitTree(false);

  if(debug_aes_table_update)
  {
	  
  	  std::ostringstream oss;
	  oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
	  std::string out_file_name = oss.str();
	  output_file.open(out_file_name.c_str(), ios::out|ios::app);
	  output_file.close();
  	  output_file.clear();
	
  }	

  if(generate_tree_nodes)
  {
	DCCM_OVERHEAD_RATIO=KnobDccmOverhead.Value()*1.0/100;
  	DCCM_OVERHEAD_RATIO_AFTER_WARMUP=DCCM_OVERHEAD_RATIO;
  	
	std::stringstream ss_possibility;
	ss_possibility<<KnobDccmOverhead.Value()*1.0/100;
	warmup_status = WARMUP_OVER;
	ifstream file_random;
	file_random.open(KnobRandomCpt.Value().c_str(), std::ifstream::binary);

	file_random.read((char*)(random_numbers_cpt), sizeof(RandomNumberCpt));
	if (file_random)
		printf("all characters read successfully.\n");
	else
		printf("error: only  could be read.\n");
	file_random.close();
	file_random.clear();
	for(UINT64 i=0;i<100;i++)
	{
		printf("random number: %lu\n", random_numbers_cpt->random_array[i]);
	}
	printf("number of blocks with zero accesses: %lu , with variation of counter values: %lu, among 2M blocks\n", random_numbers_cpt->get_zero_access_block_number(), random_numbers_cpt->get_variation_of_write_counter());  	

	  use_page_zero=1;
	  UINT64 generate_trace_addr=0;
	  for(UINT64 index_thousand_iterations=0;index_thousand_iterations<1000;index_thousand_iterations++)
	  {
		  for(UINT64 index=0; index<1000*2097152;index++)
		  {	 
			  if((instruction_count_total > fast_forward_value)&&((instruction_count_total-fast_forward_value)>(100000000*index_of_billion)))
			  {
				  have_not_print_for_first_update=false;	
				  step_value = 0;
				  PrintResults();
				  index_of_billion++;
			  }
			  instruction_count_total++;
			  instruction_count_stats++;
			  generate_trace_addr=((random_numbers_cpt->random_array[index])+index_thousand_iterations)*64;
			  if(index%2==0)
				  CacheCall(0, WRITE_OP, instruction_count_total, 0, ((UINT64)(generate_trace_addr)) & DATA_BLOCK_FLOOR_ADDR_MASK, 0, false, access_data);		
			  else
				  CacheCall(0, READ_OP, instruction_count_total, 0, ((UINT64)(generate_trace_addr)) & DATA_BLOCK_FLOOR_ADDR_MASK, 0, false, access_data);		
			  if(index==0)
			  {
				  for(UINT64 index=0; index<16384;index++)
				  {
					  tree_nodes_cpt_ptr->versions_level[index]=morph_tree.versions_level[index];
				  }
				  for(UINT64 index=0; index<128;index++)
				  {	
					  tree_nodes_cpt_ptr->tree_level1[index]=morph_tree.tree_level1[index];
				  }
				  for(UINT64 index=0; index<1;index++)
				  {	
					  tree_nodes_cpt_ptr->tree_level2[index]=morph_tree.tree_level2[index];
				  }
				  tree_nodes_cpt_ptr->tree_level3=morph_tree.tree_level3[0];
				  ofstream file_tree_nodes;

				  std::stringstream ss;
				  std::stringstream ss_groups;
				  std::stringstream ss_index_thousand_iterations;
				  std::stringstream ss_index;
				  ss_groups << TABLE_SIZE;
				  ss_index_thousand_iterations << index_thousand_iterations;
				  ss_index << (index/2097152);
				  string file_name;
				  if(OTP_PRECALCULATION)
					  file_name = "long_part_fix_index_"+ss_index_thousand_iterations.str()+"_"+ss_index.str()+"_"+ss_possibility.str()+ "_"+"random_whole_tree_dccm_tree_nodes_"+ss_groups.str()+"_groups.cpt";
				  else
					  file_name ="long_part_fix_index_"+ss_index_thousand_iterations.str()+"_"+ss_index.str()+"_"+"random_whole_tree_baseline_tree_nodes.cpt";
				  file_tree_nodes.open(file_name.c_str(), ios::app);
				  file_tree_nodes.write((char*)(tree_nodes_cpt_ptr), sizeof(TreeNodesCpt));
				  file_tree_nodes.close();
				  if(OTP_PRECALCULATION)
					  file_name="long_part_fix_index_"+ss_index_thousand_iterations.str()+"_"+ss_index.str()+"_"+ss_possibility.str()+ "_"+"random_whole_tree_dccm_aes_table_"+ss_groups.str()+"_groups.cpt";
				  else
					  file_name="long_part_fix_index_"+ss_index_thousand_iterations.str()+"_"+ss_index.str()+"_"+"random_whole_tree_baseline_aes_table.cpt";

				  file_tree_nodes.open(file_name.c_str(), ios::app);
				  file_tree_nodes.write((char*)&otp_table, sizeof(otp_table));
				  file_tree_nodes.close();
			  }
		  }
	  }
	  
	  for(UINT64 index=0; index<16384;index++)
	  {
		  tree_nodes_cpt_ptr->versions_level[index]=morph_tree.versions_level[index];
	  }
	  for(UINT64 index=0; index<128;index++)
	  {	
		  tree_nodes_cpt_ptr->tree_level1[index]=morph_tree.tree_level1[index];
		  //tree_nodes_cpt.tree_level1[index].PrintNode();
	  }
	  for(UINT64 index=0; index<1;index++)
	  {	
		  tree_nodes_cpt_ptr->tree_level2[index]=morph_tree.tree_level2[index];
		  //tree_nodes_cpt.tree_level2[index].PrintNode();
	  }
	  tree_nodes_cpt_ptr->tree_level3=morph_tree.tree_level3[0];
	  ofstream file_tree_nodes;

	  std::stringstream ss;
	  std::stringstream ss_groups;
	  ss_groups << TABLE_SIZE;
	  string file_name;
	  if(OTP_PRECALCULATION)
		  file_name = "long_part_fix_final_random_whole_tree_dccm_tree_nodes_"+ss_possibility.str()+ "_"+ss_groups.str()+"_groups.cpt";
	  else
		  file_name = "long_part_fix_final_random_whole_tree_baseline_tree_nodes.cpt";
	  file_tree_nodes.open(file_name.c_str(), ios::app);
	  file_tree_nodes.write((char*)tree_nodes_cpt_ptr, sizeof(TreeNodesCpt));
	  file_tree_nodes.close();
	  if(OTP_PRECALCULATION)
		  file_name="long_part_fix_final_random_whole_tree_dccm_aes_table_"+ss_possibility.str()+ "_"+ss_groups.str()+"_groups.cpt";
	  else
		  file_name="long_part_fix_final_random_whole_tree_baseline_aes_table.cpt";

	  file_tree_nodes.open(file_name.c_str(), ios::app);
	  file_tree_nodes.write((char*)&otp_table, sizeof(otp_table));
	  file_tree_nodes.close();
	  exit(0);
	
  }
  else
  {
	
	ifstream file_tree_nodes;
	file_tree_nodes.open(KnobTreeCpt.Value().c_str(), std::ifstream::binary);

	file_tree_nodes.read((char*)tree_nodes_cpt_ptr, sizeof(TreeNodesCpt));
	if (file_tree_nodes)
		std::cout << "all characters read successfully.";
	else
		std::cout << "error: only " << file_tree_nodes.gcount() << " could be read";
	file_tree_nodes.close();
	file_tree_nodes.clear();
  	for(UINT64 index=0; index<16384*128*times_of_16g;index++)
	{
	    if(index<16384)
	   	 morph_tree.versions_level[index]=tree_nodes_cpt_ptr->versions_level[index];
	    else
		 morph_tree.versions_level[index]=morph_tree.versions_level[index%16384];
	    morph_tree.versions_level[index].SetCtrLevel(0);
	    morph_tree.versions_level[index].SetSubNodeAddr(index*128*64);
	    morph_tree.versions_level[index].SetMorphAddr(index);
	}
	for(UINT64 index=0; index<128*128*times_of_16g;index++)
	{
	   if(index<128)
	   	 morph_tree.tree_level1[index]=tree_nodes_cpt_ptr->tree_level1[index];
	   else
		 morph_tree.tree_level1[index]=morph_tree.tree_level1[index%128];
	    morph_tree.tree_level1[index].SetCtrLevel(1);
	    morph_tree.tree_level1[index].SetSubNodeAddr(VERSIONS_START_ADDR + index*128);
	    morph_tree.tree_level1[index].SetMorphAddr(2097152+index);
	}
	for(UINT64 index=0; index<1*128*times_of_16g;index++)
	{	
	    if(index<1)
	    	morph_tree.tree_level2[index]=tree_nodes_cpt_ptr->tree_level2[index];
	    else
		morph_tree.tree_level2[index]=morph_tree.tree_level2[0];
	    morph_tree.tree_level2[index].SetCtrLevel(2);
	    morph_tree.tree_level2[index].SetSubNodeAddr(TREE_LEVEL1_START_ADDR + index*128);
	    morph_tree.tree_level2[index].SetMorphAddr(2097152+16384+index);
	    if(debug_zcc_inner_switch)
			morph_tree.tree_level2[index].PrintNode();
	}
	printf("finished reading the tree\n");
	/*
	ifstream file_table;
	file_table.open(KnobTableCpt.Value().c_str(), std::ifstream::binary);

	file_table.read((char*)&otp_table, sizeof(otp_table));
    file_table.close();
    file_table.clear();
    printf("finished reading the table\n");
	*/
	printf("the max counter in the tree is: %lu\n", morph_tree.GetLargestCounter());
	printf("the max L1 counter in the tree is: %lu\n", morph_tree.GetLargestL1Counter());
	otp_table.reset(morph_tree.GetLargestL1Counter(), 0);
	printf("the min counter in the tree is: %lu\n", morph_tree.GetSmallestCounter());
	printf("the number of  counter 0 in the tree is: %lu\n", morph_tree.GetNumberOfZero());
    
}

  HIGH_WC_RATIO_THRESHOLD=(double)KnobHighWcRatioThreshold.Value();
  POSSIBILITY_WITH_OVERFLOW_RELEVEL=KnobPossibilityOverflowRelevel.Value();
  is_gap=(int)KnobIsGap.Value();
  if (!is_gap){
	  warmup_status = WARMUP_OVER;
	  fast_forward_value =0;
  }
  //generate_tree_nodes=(int)KnobGenerateTreeNodes.Value();
  //use_page_zero=(int)KnobUsePageZero.Value();
  delay_high_level_tree_node_overflow=(int)KnobDelayHighLevelTreeNodeOverflow.Value();
  DCCM_OVERHEAD_RATIO=KnobDccmOverhead.Value()*1.0/100;
  DCCM_OVERHEAD_RATIO_AFTER_WARMUP=DCCM_OVERHEAD_RATIO;
  CACHELINE_OFFSET=KnobCachelineOffset.Value();
  INS_AddInstrumentFunction(Instruction, 0);
  PIN_AddFiniFunction(Fini, 0);
  
  printf("before PIN_StartProgram, the aes table is correct or not: %d\n", otp_table.CheckTable(127,0));
  //morph_tree.PrintEffectiveCounters();

  PIN_StartProgram();
    
  return 0;
}
void MorphCtrBlock::FetchForOverflow()
{
	if(debug_prioritize_overflow_nodes)
	{
		printf("before removing the overflow node in the queue:\n");
		RedirectPrintNode(current_pos);	
	}

    remove_overflow_node(ctr_level_, morph_addr);


    if((ctr_level_>0)&&delay_high_level_tree_node_overflow)
    {
	    UINT64 _wc_block_addr = morph_addr+VERSIONS_START_ADDR; 
            UINT64 curr_wccache_set = _wc_block_addr & WCCACHE_SET_MASK;
	    UINT curr_wccache_hit =wc_lru_cache.CheckWcCacheHit(curr_wccache_set, _wc_block_addr, instruction_count_total); 
	    if(curr_wccache_hit||L2.CheckCacheHit(_wc_block_addr*64, true))
	    {
		wc_lru_cache.UpdateOverflowOverhead(curr_wccache_set, _wc_block_addr, 256);
		return;
	    }
    }
    total_overflow_events++;
    if(during_page_zero)
    {
	total_overflow_events_during_page_zero++;
	if(ctr_level_==0)
		total_L0_overflow_events_during_page_zero++;
    }
  	//printf("overflow\n");
	if(relevel_reason==RELEVEL_FOR_SMALLER_VALUE_THAN_MIN)
		overflow_smaller_than_smallest_stats+=1;
	if(relevel_reason==RELEVEL_FOR_VALUE_IN_SMALLEST)
		overflow_in_small_stats+=1;
	if(relevel_reason==RELEVEL_FOR_VALUE_IN_MEDIATE)
		overflow_in_mediate_stats+=1;


    if(ctr_level_>=1)
    {	
	   UINT64 begin_wc_block_addr = sub_node_begin_addr;
	   UINT64 curr_wc_block_addr;
           for(UINT64 sub_index=0; sub_index<128; sub_index++)
	   {
	    curr_wc_block_addr = begin_wc_block_addr + sub_index; 
            UINT64 curr_wccache_set = curr_wc_block_addr & WCCACHE_SET_MASK;
	    UINT curr_wccache_hit =wc_lru_cache.CheckWcCacheHit(curr_wccache_set, curr_wc_block_addr, instruction_count_total); 
	    if(!(curr_wccache_hit||L2.CheckCacheHit(curr_wc_block_addr*64, true)))
	    {
		    if (warmup_status == WARMUP_OVER) {
			    overflow_fetches_stats+=2;
			    overflow_fetches_stats_level[ctr_level_]+=2;
			    if(relevel_reason==RELEVEL_FOR_SMALLER_VALUE_THAN_MIN)
				overflow_fetches_smaller_than_smallest_stats+=2;
			    if(relevel_reason==RELEVEL_FOR_VALUE_IN_SMALLEST)
				overflow_fetches_in_small_stats+=2;
			    if(relevel_reason==RELEVEL_FOR_VALUE_IN_MEDIATE)
				overflow_fetches_in_mediate_stats+=2;
				
		    }
		    overflow_fetches_total+=2;
	            //if(is_dccm_overhead)
	            if(is_dccm_overhead)
		    {
			dccm_overflow_traffic+=2;
			accumulated_dccm_traffic_overhead+=2;
			page_releveling_overhead+=2;
		    }
	    }	
	   }
    }
   else
    {
	    //UINT64 begin_block_addr = sub_node_begin_addr;
	    //UINT64 curr_block_addr;
	    for(UINT64 sub_index=0; sub_index<128; sub_index++)
	    {
		    /*
		    curr_block_addr = begin_block_addr + sub_index*64; 
		    UINT64 L1_set = L1.GetCacheSet(curr_block_addr);
		    BOOL L1_hit_status = L1.IsCacheHit(L1_set, curr_block_addr);
		    UINT64 L2_set = L2.GetCacheSet(curr_block_addr); 
		    BOOL L2_hit_status = L2.IsCacheHit(L2_set, curr_block_addr);

		    // Check if hit in L1 cache
		    if(L1_hit_status != CACHE_HIT && L2_hit_status != CACHE_HIT) {
			    if (warmup_status == WARMUP_OVER) {
				    overflow_fetches_stats+=1;
			    }
			    overflow_fetches_total+=1;
		    } 
		    */ 
		    if (warmup_status == WARMUP_OVER) {
			    overflow_fetches_stats+=2; 
			    overflow_fetches_stats_level[ctr_level_]+=2;
		            if(relevel_reason==RELEVEL_FOR_SMALLER_VALUE_THAN_MIN)
				overflow_fetches_smaller_than_smallest_stats+=2;
			    if(relevel_reason==RELEVEL_FOR_VALUE_IN_SMALLEST)
				overflow_fetches_in_small_stats+=2;
			    if(relevel_reason==RELEVEL_FOR_VALUE_IN_MEDIATE)
				overflow_fetches_in_mediate_stats+=2;
				
		    }
		    overflow_fetches_total+=2;
		    if(is_dccm_overhead)
		    {
			 	dccm_overflow_traffic+=2;
				accumulated_dccm_traffic_overhead+=2;
				page_releveling_overhead+=2;
		    }

	    }

    }

    if(debug_overflow_for_releveling_to_next_group){
		PrintNode();
     }
}
void MorphCtrBlock::FetchForMcrOverflow(bool is_base_2)
{
    printf("this function FetchForMcrOverflow should not be called!\n");
    exit(0);
    remove_overflow_node(ctr_level_, morph_addr);
    if((ctr_level_>0)&&delay_high_level_tree_node_overflow)
    {
	    UINT64 _wc_block_addr = morph_addr+VERSIONS_START_ADDR; 
            UINT64 curr_wccache_set = _wc_block_addr & WCCACHE_SET_MASK;
	    UINT curr_wccache_hit =wc_lru_cache.CheckWcCacheHit(curr_wccache_set, _wc_block_addr, instruction_count_total); 
	    if(curr_wccache_hit||L2.CheckCacheHit(_wc_block_addr*64, true))
	    {
		wc_lru_cache.UpdateOverflowOverhead(curr_wccache_set, _wc_block_addr, 128);
		return;
	    }
    }
    if(during_page_zero)
    {
	total_overflow_events_during_page_zero++;
	if(ctr_level_==0)
		total_L0_overflow_events_during_page_zero++;
    }

	if(debug_overflow)
		printf("remaining budget: %lf,   tree level: %u, tree node id: %lu, instruction number: %lu, data block address: %lu\n", DCCM_OVERHEAD_RATIO*OTP_INTERVAL+dccm_remain_budget-dccm_overflow_traffic, ctr_level_, morph_addr, instruction_count_total,  current_written_block_address);

    	total_overflow_events++;
	//printf("overflow\n");
	if(relevel_reason==RELEVEL_FOR_SMALLER_VALUE_THAN_MIN)
		overflow_smaller_than_smallest_stats+=1;
	if(relevel_reason==RELEVEL_FOR_VALUE_IN_SMALLEST)
		overflow_in_small_stats+=1;
	if(relevel_reason==RELEVEL_FOR_VALUE_IN_MEDIATE)
		overflow_in_mediate_stats+=1;



    if(ctr_level_>=1)
    {
	   UINT64 begin_wc_block_addr = sub_node_begin_addr;
	   UINT64 curr_wc_block_addr;
	   UINT64 begin_index;
	   if(is_base_2)
		begin_index=64;
	   else
		begin_index=0;
           for(UINT64 sub_index=begin_index; sub_index<begin_index+64; sub_index++)
	   {
	    curr_wc_block_addr = begin_wc_block_addr + sub_index; 
            UINT64 curr_wccache_set = curr_wc_block_addr & WCCACHE_SET_MASK;
	    UINT curr_wccache_hit =wc_lru_cache.CheckWcCacheHit(curr_wccache_set, curr_wc_block_addr, instruction_count_total); 
	    if(!(curr_wccache_hit||L2.CheckCacheHit(curr_wc_block_addr*64, true)))
	    {
		    if (warmup_status == WARMUP_OVER) {
			    overflow_fetches_stats+=2; 
			    overflow_fetches_stats_level[ctr_level_]+=2;
			    if(relevel_reason==RELEVEL_FOR_SMALLER_VALUE_THAN_MIN)
				overflow_fetches_smaller_than_smallest_stats+=2;
			    if(relevel_reason==RELEVEL_FOR_VALUE_IN_SMALLEST)
				overflow_fetches_in_small_stats+=2;
			    if(relevel_reason==RELEVEL_FOR_VALUE_IN_MEDIATE)
				overflow_fetches_in_mediate_stats+=2;
				
		    }
		    overflow_fetches_total+=2;
		    //if(is_dccm_overhead)
	            if(is_dccm_overhead)
		    {
			dccm_overflow_traffic+=2;
			accumulated_dccm_traffic_overhead+=2;
			page_releveling_overhead+=2;
		    }

	    }	
	   }
    }
   else
    {
	    //UINT64 begin_block_addr = sub_node_begin_addr;
	    //UINT64 curr_block_addr; 
	    UINT64 begin_index;
	   if(is_base_2)
		begin_index=64;
	   else
		begin_index=0;
          
	    for(UINT64 sub_index=begin_index; sub_index<begin_index+64; sub_index++)
	    {
		    /*
		    curr_block_addr = begin_block_addr + sub_index*64; 
		    UINT64 L1_set = L1.GetCacheSet(curr_block_addr);
		    BOOL L1_hit_status = L1.IsCacheHit(L1_set, curr_block_addr);
		    UINT64 L2_set = L2.GetCacheSet(curr_block_addr); 
		    BOOL L2_hit_status = L2.IsCacheHit(L2_set, curr_block_addr);

		    // Check if hit in L1 cache
		    if(L1_hit_status != CACHE_HIT && L2_hit_status != CACHE_HIT) {
			    if (warmup_status == WARMUP_OVER) {
				    overflow_fetches_stats+=1;
			    }
			    overflow_fetches_total+=1;
		    } 
		    */
		    if (warmup_status == WARMUP_OVER) {
			    overflow_fetches_stats+=2; 
			    overflow_fetches_stats_level[ctr_level_]+=2;
			    if(relevel_reason==RELEVEL_FOR_SMALLER_VALUE_THAN_MIN)
				overflow_fetches_smaller_than_smallest_stats+=2;
			    if(relevel_reason==RELEVEL_FOR_VALUE_IN_SMALLEST)
				overflow_fetches_in_small_stats+=2;
			    if(relevel_reason==RELEVEL_FOR_VALUE_IN_MEDIATE)
				overflow_fetches_in_mediate_stats+=2;
				
		    }
		    overflow_fetches_total+=2;
		    if(is_dccm_overhead)
		   {
			dccm_overflow_traffic+=2;
			accumulated_dccm_traffic_overhead+=2;
			page_releveling_overhead+=2;
		   }


	    }

    }

    if(debug_overflow_for_releveling_to_next_group){
		PrintNode();
     }

}


	

bool OtpTable::access(UINT64 _effective_ctr, UINT32 _wccache_hit)
{
	//_dccm_counter++;
	if(_effective_ctr>system_max)
	{
		//if(_effective_ctr<=1000000000)
			system_max=_effective_ctr;
		
		if(_effective_ctr>1000000000)
		{ 
		     if(debug_aes_table_update)
		     {
			std::ostringstream oss;
			oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
			std::string out_file_name = oss.str();
			output_file.open(out_file_name.c_str(), ios::out|ios::app);
			output_file << "unnormal effective counter: "<<_effective_ctr<<" , for block address: "<<current_block_addr_access_aes_table<<std::endl;
			output_file.close();
			output_file.clear();
			morph_tree.versions_level[global_counter_id].PrintNode();
			exit(0);
		     }
		}
	}
	total_num_aes_accesses++;
	bool is_hit=false;
	if(_effective_ctr>max_wc){
		total_num_large_counter_accesses++;
		for(int potential_group_index=0; potential_group_index<POTENTIAL_GROUP_NUM; potential_group_index++)
		{
			if(_effective_ctr<potential_groups[potential_group_index]+TABLE_LINE_SIZE)
			{
				num_potential_group_hits[potential_group_index]++;
				break;
			}
			if(potential_group_index==(POTENTIAL_GROUP_NUM-1))
				num_potential_group_hits[potential_group_index]++;
		}
	}
	for(UINT64 recent_evicted_group_index=0; recent_evicted_group_index<recent_evicted_groups.size(); recent_evicted_group_index++)
		{
			if((_effective_ctr>=recent_evicted_groups[recent_evicted_group_index].beginning_counter)&&(_effective_ctr<recent_evicted_groups[recent_evicted_group_index].beginning_counter+TABLE_LINE_SIZE))
			{
				recent_evicted_groups[recent_evicted_group_index].number_of_hits++;
			}
		}
	

	for(int table_index=0; table_index<TABLE_SIZE; table_index++)
	{
		if((_effective_ctr>=table[table_index])&&(_effective_ctr<table[table_index]+TABLE_LINE_SIZE))
		{
			is_hit=true;
			num_table_hits[table_index]++;
			break;
		}
		if(_effective_ctr<table[table_index])
		{
			num_table_hits_between_groups[table_index]++;
			break;
		}
	} 	
	if(is_hit)
	{
		otp_table_hit_total++;
		if(_wccache_hit<2)
			otp_table_hit_while_wc_cache_miss_total++;
		if (warmup_status == WARMUP_OVER)
		{
			otp_table_hit_stats++;	
			if(_wccache_hit<2)
				otp_table_hit_while_wc_cache_miss_stats++;
		}
	}
	else
	{
		if(((int)KnobRecentAes.Value())&&access_recent_aes(_effective_ctr))
			otp_table_hit_stats++;	
		else{		
			otp_table_miss_total++;	
			if(_wccache_hit<2)
				otp_table_miss_while_wc_cache_miss_total++;

			if (warmup_status == WARMUP_OVER)
			{
				otp_table_miss_stats++;	
				if(_wccache_hit<2)
					otp_table_miss_while_wc_cache_miss_stats++;

			}
			if(_effective_ctr<min_wc)
			{
				otp_table_miss_with_small_ctr_total++;
				if (warmup_status == WARMUP_OVER)
					otp_table_miss_with_small_ctr_stats++;
			}
			else if(_effective_ctr<=max_wc)
			{
				otp_table_miss_with_medium_ctr_total++;
				if (warmup_status == WARMUP_OVER)
					otp_table_miss_with_medium_ctr_stats++;
			}
			else
			{	
				otp_table_miss_with_big_ctr_total++;
				if (warmup_status == WARMUP_OVER)
					otp_table_miss_with_big_ctr_stats++;
			}
		}
	} 
	if((tick_in_current_interval++)>AES_OTP_INTERVAL)
	{
		//printf("tick_in_current_interval: %lu, AES_OTP_INTERVAL: %lu", tick_in_current_interval, AES_OTP_INTERVAL);
		update();
	}
	if(debug_aes_hit)
	{
		if(is_hit)
			printf("AES table hit for effective counter: %lu, data block addr:  %lu, instruction number: %lu\n", _effective_ctr, current_block_addr_access_aes_table, instruction_count_total-fast_forward_value );
		else
			printf("AES table miss for effective counter: %lu, data block addr:  %lu, instruction number: %lu\n", _effective_ctr, current_block_addr_access_aes_table, instruction_count_total-fast_forward_value );
	}
	return is_hit;	
}


  void OtpTable::enqueue_to_recently_evicted_groups(UINT64 _beginning_counter)
  {
	recent_evicted_group _group;
	_group.beginning_counter=_beginning_counter;
	_group.number_of_hits=0;
	recent_evicted_groups.push_back(_group);
	if(recent_evicted_groups.size()>num_recent_evicted_groups)
		recent_evicted_groups.pop_front();
  }
void OtpTable::PrintTableUpdateAndResetStats()
{
	sort(table, table+TABLE_SIZE);  
	otp_table_update_total++;
	if (warmup_status == WARMUP_OVER)
		otp_table_update_stats++;
	total_num_aes_accesses=0;
	total_num_large_counter_accesses=0;
	for(int table_index=0;table_index<TABLE_SIZE;table_index++)
	{
		num_table_hits[table_index]=0;
		num_table_hits_between_groups[table_index]=0;
		dccm_overhead_for_groups[table_index]=0;
		dccm_page_relevel_overhead_for_groups[table_index]=0;
		dccm_block_relevel_overhead_for_groups[table_index]=0;

	}
	for(int potential_group_index=0;potential_group_index<POTENTIAL_GROUP_NUM;potential_group_index++)
		num_potential_group_hits[potential_group_index]=0;
	for(UINT64 recent_evicted_group_index=0; recent_evicted_group_index<recent_evicted_groups.size(); recent_evicted_group_index++)
	{
		recent_evicted_groups[recent_evicted_group_index].number_of_hits=0;
	}
	if(debug_aes_table_update){	
		std::ostringstream oss;
		//oss << KnobOutputFile.Value().c_str() <<"_morphtree_4_4_micro_baseline_debug_new_final.out";
		oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
		std::string out_file_name = oss.str();
		output_file.open(out_file_name.c_str(), ios::out|ios::app);
		//PrintTotalStats();
		output_file << std::endl;
		output_file <<  "after update, content of the AES table: " <<std::endl;
		for(int table_index=0; table_index<TABLE_SIZE; table_index++)
			output_file <<"beginning counter: "<< table[table_index]<< std::endl;

		output_file << std::endl;

		output_file.close();
		output_file.clear();
	}
	if(CheckDuplicationInTable())
	{
		printf("there's duplicated groups in AES table after running %lu instructions!\n", instruction_count_total-fast_forward_value);
		exit(0);
	}
}
  //void OtpTable::find_victim_and_update_the_table(new_group_to_insert, num_hits_to_new_group_to_insert)
  void OtpTable::find_victim_and_update_the_table(bool insertion_of_potential_group)
  { 		
	  UINT64 max_num_hits_to_evicted_groups=0;
	  for(UINT64 recent_evicted_group_index=0; recent_evicted_group_index<recent_evicted_groups.size(); recent_evicted_group_index++){
		  if(recent_evicted_groups[recent_evicted_group_index].number_of_hits>max_num_hits_to_evicted_groups){
			  max_num_hits_to_evicted_groups =recent_evicted_groups[recent_evicted_group_index].number_of_hits;
		  }
	  }
	  if (max_num_hits_to_evicted_groups*1.0<total_num_aes_accesses*1.0*((double)KnobThresholdForInsertionOfEvictedGroupOverhead.Value()))
	  {
		if(insertion_of_potential_group)
			PrintTableUpdateAndResetStats();
		  return;	
	  }
	  UINT64 min_hits_to_table=1000000000;
	  for(int table_index=0;table_index<TABLE_SIZE;table_index++){
		  if(table[table_index]!=(max_wc-TABLE_LINE_SIZE+1)){
			  if(num_table_hits[table_index]<min_hits_to_table){
				  min_hits_to_table=num_table_hits[table_index];
			  }
		  }
	  }
	  if(min_hits_to_table>=max_num_hits_to_evicted_groups){
		if(insertion_of_potential_group)
			PrintTableUpdateAndResetStats();
		  return;
	  }
	  if(debug_aes_table_update&&(!insertion_of_potential_group))
	  {
	          
		  std::ostringstream oss;
		  //oss << KnobOutputFile.Value().c_str() <<"_morphtree_4_4_micro_baseline_debug_new_final.out";
		  oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
		  std::string out_file_name = oss.str();
		  output_file.open(out_file_name.c_str(), ios::out|ios::app);
		  //PrintTotalStats();
		  output_file << "at instruction: "<<(instruction_count_total-fast_forward_value)<<" , at epoch index: "<<num_otp_interval<<std::endl;
		  output_file << "before update by inserting evicted group, AES accesses: " <<total_num_aes_accesses<<" , AES big counter accessess: " <<total_num_large_counter_accesses<<std::endl;
		  output_file <<  "system_max: "<<system_max <<std::endl;
		  UINT64 epoch_hits=0;
		  for(int table_index=0; table_index<TABLE_SIZE; table_index++)
			  epoch_hits+=num_table_hits[table_index];
		  output_file <<  "table hit rate in the epoch: "<<(epoch_hits*1.0/total_num_aes_accesses) <<std::endl;
		  output_file <<  "content of the AES table: " <<std::endl;
		  UINT64 _total_dccm_overhead=0;
		  for(int table_index=0; table_index<TABLE_SIZE; table_index++)
			  _total_dccm_overhead+=dccm_overhead_for_groups[table_index];
		  for(int table_index=0; table_index<TABLE_SIZE; table_index++)
		  {
			  if(table_index==0)
				  output_file <<"counters between 0 and "<<table[0]<<" : , misses number: "<<num_table_hits_between_groups[0]*1.0/total_num_aes_accesses <<" , with dccm overhead: "<<dccm_overhead_for_groups[table_index]*1.0/_total_dccm_overhead<< std::endl;
			  else
				  output_file <<"counters between "<<(table[table_index-1]+TABLE_LINE_SIZE)<<" and "<<table[table_index]<<" : , misses number: "<<num_table_hits_between_groups[table_index]*1.0/total_num_aes_accesses<<" , with dccm overhead: "<<dccm_overhead_for_groups[table_index]*1.0/_total_dccm_overhead<< std::endl;
			  output_file <<"beginning counter: "<< table[table_index]<<" , hits number: "<<num_table_hits[table_index]*1.0/total_num_aes_accesses<< std::endl;
		  }
		  output_file <<  "content of the potential groups: " <<std::endl;

		  for(int potential_group_index=0;potential_group_index<POTENTIAL_GROUP_NUM;potential_group_index++)
			  output_file <<"beginning counter: "<< potential_groups[potential_group_index]<<" , hits number: "<<num_potential_group_hits[potential_group_index]*1.0/total_num_aes_accesses<< std::endl;
		  output_file << "content of evicted groups: " <<std::endl;
		  for(UINT64 recent_evicted_group_index=0; recent_evicted_group_index<recent_evicted_groups.size(); recent_evicted_group_index++)
		  {
			  output_file <<"beginning counter: "<< recent_evicted_groups[recent_evicted_group_index].beginning_counter<<" , hits number: " <<recent_evicted_groups[recent_evicted_group_index].number_of_hits*1.0/total_num_aes_accesses<<std::endl;
		  }
		  output_file << std::endl;
		  output_file.close();
		  output_file.clear();
	  }
    vector<pair<UINT64, UINT64> > sorted_evicted_groups;
    for (UINT64 i = 0; i < recent_evicted_groups.size(); ++i) {
        sorted_evicted_groups.push_back(make_pair(recent_evicted_groups[i].number_of_hits, recent_evicted_groups[i].beginning_counter));
    }
    sort(sorted_evicted_groups.begin(), sorted_evicted_groups.end(), greater<pair<UINT64, UINT64>>());
    vector<pair<UINT64, UINT64> > sorted_otp_table;
    for (UINT64 i = 0; i < TABLE_SIZE; ++i) {
	//if(table[i]!=max_wc)
	if(table[i]!=(max_wc-TABLE_LINE_SIZE+1))
        	sorted_otp_table.push_back(make_pair(num_table_hits[i], table[i]));
    }
    sort(sorted_otp_table.begin(), sorted_otp_table.end());
    UINT64 min_size_of_sorted_vector=TABLE_SIZE-1;
    if(recent_evicted_groups.size()<min_size_of_sorted_vector)
	min_size_of_sorted_vector=recent_evicted_groups.size();
    UINT64 i;
    for(i=0;i<min_size_of_sorted_vector;i++)
    {
	    if((sorted_evicted_groups[i].first>sorted_otp_table[i].first)&&(sorted_evicted_groups[i].first>total_num_aes_accesses*1.0*((double)KnobThresholdForInsertionOfEvictedGroupOverhead.Value())))
	    {
		    for(UINT64 recent_evicted_group_index=0; recent_evicted_group_index<recent_evicted_groups.size(); recent_evicted_group_index++){
			    if(recent_evicted_groups[recent_evicted_group_index].beginning_counter==sorted_evicted_groups[i].second){
				    recent_evicted_groups.erase(recent_evicted_groups.begin()+recent_evicted_group_index);
				    break;
			    }
		    }
		    enqueue_to_recently_evicted_groups(sorted_otp_table[i].second);
		    //for (UINT64 table_index = 0; table_index < TABLE_SIZE-1; table_index++) {
		    for (UINT64 table_index = 0; table_index < TABLE_SIZE; table_index++) {
			if(table[table_index]==sorted_otp_table[i].second){
				table[table_index]=sorted_evicted_groups[i].second;
				break;
			}
		    }
	    }
	    else
	    {
		    break;
	    }
    }
    PrintTableUpdateAndResetStats();
  }
  bool OtpTable::hit_in_table_groups_except_top_of_group(UINT64  _effective_ctr){
	bool hit_in_table=false;
	for(int table_index=0;table_index<TABLE_SIZE;table_index++){
			if((_effective_ctr>=table[table_index])&&(_effective_ctr<(table[table_index]+TABLE_LINE_SIZE-1))){
				hit_in_table=true;
			}
	}
	return hit_in_table;
  }
  void OtpTable::update()
  {
      num_otp_interval++;
      if(debug_aes_table_update)
      {	
	  //printf("tick_in_current_interval: %lu, AES_OTP_INTERVAL: %lu", tick_in_current_interval, AES_OTP_INTERVAL);
          std::ostringstream oss;
          //oss << KnobOutputFile.Value().c_str() <<"_morphtree_4_4_micro_baseline_debug_new_final.out";
          oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
          std::string out_file_name = oss.str();
          output_file.open(out_file_name.c_str(), ios::out|ios::app);
          //PrintTotalStats();
	  output_file << "tick_in_current_interval: "<<tick_in_current_interval<<" , AES_OTP_INTERVAL: "<<AES_OTP_INTERVAL<<std::endl;
          output_file << "at instruction: "<<(instruction_count_total-fast_forward_value)<<" , at epoch index: "<<num_otp_interval<<std::endl;
          output_file << "at end of epoch, AES accesses: " <<total_num_aes_accesses<<" , AES big counter accessess: " <<total_num_large_counter_accesses<<std::endl;
          output_file <<  "system_max: "<<system_max <<std::endl;
          UINT64 epoch_hits=0;
          for(int table_index=0; table_index<TABLE_SIZE; table_index++)
              epoch_hits+=num_table_hits[table_index];
          output_file <<  "table hit rate in the epoch: "<<(epoch_hits*1.0/total_num_aes_accesses) <<std::endl;
          output_file <<  "content of the AES table: " <<std::endl;
          UINT64 _total_dccm_overhead=0;
          for(int table_index=0; table_index<TABLE_SIZE; table_index++)
              _total_dccm_overhead+=dccm_overhead_for_groups[table_index];
          for(int table_index=0; table_index<TABLE_SIZE; table_index++)
          {
              if(table_index==0)
                  output_file <<"counters between 0 and "<<table[0]<<" : , misses number: "<<num_table_hits_between_groups[0]*1.0/total_num_aes_accesses <<" , with dccm overhead: "<<dccm_overhead_for_groups[table_index]*1.0/_total_dccm_overhead<< std::endl;
              else
                  output_file <<"counters between "<<(table[table_index-1]+TABLE_LINE_SIZE)<<" and "<<table[table_index]<<" : , misses number: "<<num_table_hits_between_groups[table_index]*1.0/total_num_aes_accesses<<" , with normalized dccm overhead: "<<dccm_overhead_for_groups[table_index]*1.0/_total_dccm_overhead<<" , obsolute dccm overhead: "<<dccm_overhead_for_groups[table_index]<<" , page relevel overhead: "<<dccm_page_relevel_overhead_for_groups[table_index]<<" , block relevel overhead: "<<dccm_block_relevel_overhead_for_groups[table_index]<<" , total dccm overhead: "<<_total_dccm_overhead<< std::endl;
              output_file <<"beginning counter: "<< table[table_index]<<" , hits number: "<<num_table_hits[table_index]*1.0/total_num_aes_accesses<< std::endl;
          }
 	    output_file << "content of evicted groups: " <<std::endl;
	    for(UINT64 recent_evicted_group_index=0; recent_evicted_group_index<recent_evicted_groups.size(); recent_evicted_group_index++)
	    {
		output_file <<"beginning counter: "<< recent_evicted_groups[recent_evicted_group_index].beginning_counter<<" , hits number: " <<recent_evicted_groups[recent_evicted_group_index].number_of_hits*1.0/total_num_aes_accesses<<std::endl;
	    }



          output_file <<  "content of the potential groups: " <<std::endl;

          for(int potential_group_index=0;potential_group_index<POTENTIAL_GROUP_NUM;potential_group_index++)
              output_file <<"beginning counter: "<< potential_groups[potential_group_index]<<" , hits number: "<<num_potential_group_hits[potential_group_index]*1.0/total_num_aes_accesses<< std::endl;

          output_file << std::endl;

          output_file.close();
          output_file.clear();
      }
    //if(!OTP_PRECALCULATION)
    if((!OTP_PRECALCULATION)&&(!OTP_L1_PRECALCULATION))
	return;
    sort(table, table+TABLE_SIZE);  
    //xinw added for average distance between groups-begin
    //num_otp_interval++;
    UINT64 current_sum_distance=0;
    for(int table_index=0; table_index<TABLE_SIZE-2; table_index++)
	    current_sum_distance=current_sum_distance+table[table_index+1]-(table[table_index]+TABLE_LINE_SIZE-1);  
    total_average_distance+=current_sum_distance*1.0/(TABLE_SIZE-1);
    otp_average_distance=total_average_distance*1.0/num_otp_interval;
    //xinw added for average distance between groups-end

    double high_wc_ratio=total_num_large_counter_accesses*1.0/total_num_aes_accesses;
    if(debug_aes_table_epoch)
    {
	    std::ostringstream oss;
	    //oss << KnobOutputFile.Value().c_str() <<"_morphtree_4_4_micro_baseline_debug_new_final.out";
	    oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
	    std::string out_file_name = oss.str();
	    output_file.open(out_file_name.c_str(), ios::out|ios::app);
	    //PrintTotalStats();
	    output_file << "AES accesses: " <<total_num_aes_accesses<<" , AES big counter accessess: " <<total_num_large_counter_accesses;
	    if(high_wc_ratio>HIGH_WC_RATIO_THRESHOLD)
	    	output_file << ", exceeds the threshold" <<std::endl;
	    else
	    	output_file << ", not exceeds the threshold" <<std::endl;
	    output_file <<  "system_max: "<<system_max <<std::endl;
	    output_file <<  "content of the AES table: " <<std::endl;
            for(int table_index=0; table_index<TABLE_SIZE; table_index++)
       		 output_file << table[table_index]<< std::endl;
	    output_file <<  "content of the potential groups: " <<std::endl;

	    for(int potential_group_index=0;potential_group_index<POTENTIAL_GROUP_NUM;potential_group_index++)
		    output_file <<"beginning counter: "<< potential_groups[potential_group_index]<<" , hits number: "<<num_potential_group_hits[potential_group_index]*1.0/total_num_aes_accesses<< std::endl;
	    output_file << "content of evicted groups: " <<std::endl;
	    for(UINT64 recent_evicted_group_index=0; recent_evicted_group_index<recent_evicted_groups.size(); recent_evicted_group_index++)
	    {
		output_file <<"beginning counter: "<< recent_evicted_groups[recent_evicted_group_index].beginning_counter<<" , hits number: " <<recent_evicted_groups[recent_evicted_group_index].number_of_hits*1.0/total_num_aes_accesses<<std::endl;
	    }


	    output_file << std::endl;
    
	    output_file.close();
	    output_file.clear();
    }
    bool insertion_of_potential_group=false;
    if(high_wc_ratio>HIGH_WC_RATIO_THRESHOLD){
	    UINT64 victim_group_index=0;
	    UINT64 min_hits=1000000000;
	    for(int table_index=0;table_index<TABLE_SIZE;table_index++){
		    if(num_table_hits[table_index]<min_hits){
			    min_hits=num_table_hits[table_index];
			    victim_group_index=table_index;
		    }
	    }
	    //UINT64  actual_group_index_with_lowest_hits=victim_group_index;
	    victim_group_index=0;
	    min_hits=1000000000;
	    for(int table_index=1;table_index<TABLE_SIZE;table_index++){
		    if(num_table_hits[table_index]<min_hits){
			    min_hits=num_table_hits[table_index];
			    victim_group_index=table_index;
		    }
	    }	
	    UINT64 smallest_index_for_potential_group=1;
	    UINT64 accumulated_hits=0;
	    for(int potential_group_index=POTENTIAL_GROUP_NUM-1;potential_group_index>=0;potential_group_index--){
		    accumulated_hits+=num_potential_group_hits[potential_group_index];
		    if((accumulated_hits*1.0)<(HIGH_WC_RATIO_THRESHOLD*total_num_aes_accesses)){
			    smallest_index_for_potential_group=potential_group_index;
		    }
	    }
	    assert(smallest_index_for_potential_group);
	    smallest_index_for_potential_group--;
	    //if(num_table_hits[victim_group_index]<(1.0*HIGHEST_GROUP_MOVE_THRESHOLD*total_num_large_counter_accesses)){ 
	    UINT64 potential_hits=0;
	    for(UINT64 potential_group_index=0;potential_group_index<=smallest_index_for_potential_group;potential_group_index++){
		    potential_hits+=num_potential_group_hits[potential_group_index];
	    }
	    if(num_table_hits[victim_group_index]<potential_hits){ 
		    insertion_of_potential_group=true;
		    if(debug_aes_table_update)
		    {
			    std::ostringstream oss;
			    //oss << KnobOutputFile.Value().c_str() <<"_morphtree_4_4_micro_baseline_debug_new_final.out";
			    oss<<KnobOutputDir.Value().c_str()<<"/"<<"simout";
			    std::string out_file_name = oss.str();
			    output_file.open(out_file_name.c_str(), ios::out|ios::app);
			    //PrintTotalStats();
			    output_file << "at instruction: "<<(instruction_count_total-fast_forward_value)<<" , at epoch index: "<<num_otp_interval<<std::endl;
			    output_file << "before update, AES accesses: " <<total_num_aes_accesses<<" , AES big counter accessess: " <<total_num_large_counter_accesses<<std::endl;
			    output_file <<  "system_max: "<<system_max <<std::endl;
			    UINT64 epoch_hits=0;
			    for(int table_index=0; table_index<TABLE_SIZE; table_index++)
				    epoch_hits+=num_table_hits[table_index];
			    output_file <<  "table hit rate in the epoch: "<<(epoch_hits*1.0/total_num_aes_accesses) <<std::endl;
			    output_file <<  "content of the AES table: " <<std::endl;
			    UINT64 _total_dccm_overhead=0;
			    for(int table_index=0; table_index<TABLE_SIZE; table_index++)
				    _total_dccm_overhead+=dccm_overhead_for_groups[table_index];
			    for(int table_index=0; table_index<TABLE_SIZE; table_index++){
				    if(table_index==0)
					    output_file <<"counters between 0 and "<<table[0]<<" : , misses number: "<<num_table_hits_between_groups[0]*1.0/total_num_aes_accesses <<" , with dccm overhead: "<<dccm_overhead_for_groups[table_index]*1.0/_total_dccm_overhead<< std::endl;
				    else
					    output_file <<"counters between "<<(table[table_index-1]+TABLE_LINE_SIZE)<<" and "<<table[table_index]<<" : , misses number: "<<num_table_hits_between_groups[table_index]*1.0/total_num_aes_accesses<<" , with dccm overhead: "<<dccm_overhead_for_groups[table_index]*1.0/_total_dccm_overhead<< std::endl;
				    output_file <<"beginning counter: "<< table[table_index]<<" , hits number: "<<num_table_hits[table_index]*1.0/total_num_aes_accesses<< std::endl;
			    }
			    output_file << "content of evicted groups: " <<std::endl;
			    for(UINT64 recent_evicted_group_index=0; recent_evicted_group_index<recent_evicted_groups.size(); recent_evicted_group_index++)	    {
				    output_file <<"beginning counter: "<< recent_evicted_groups[recent_evicted_group_index].beginning_counter<<" , hits number: " <<recent_evicted_groups[recent_evicted_group_index].number_of_hits*1.0/total_num_aes_accesses<<std::endl;
			    }
			    output_file <<  "content of the potential groups: " <<std::endl;
			    for(int potential_group_index=0;potential_group_index<POTENTIAL_GROUP_NUM;potential_group_index++)
				    output_file <<"beginning counter: "<< potential_groups[potential_group_index]<<" , hits number: "<<num_potential_group_hits[potential_group_index]*1.0/total_num_aes_accesses<< std::endl;
			    output_file << std::endl;
			    output_file.close();
			    output_file.clear();
		    }
		    enqueue_to_recently_evicted_groups(table[victim_group_index]);
		    if(smallest_index_for_potential_group<(POTENTIAL_GROUP_NUM-1))
			    table[victim_group_index]=potential_groups[smallest_index_for_potential_group];
		    else
			    table[victim_group_index]=system_max;
 		   min_wc=table[0];
		   for(int table_index=0;table_index<TABLE_SIZE;table_index++){
			   if(table[table_index]<min_wc){
				min_wc=table[table_index];
			   }
		   }
		   max_wc=table[victim_group_index]+TABLE_LINE_SIZE-1;
		   for(int potential_group_index=0;potential_group_index<POTENTIAL_GROUP_NUM;potential_group_index++) {
			   if(potential_group_index<POTENTIAL_GROUP_NUM-1)	
				   potential_groups[potential_group_index]=max_wc+group_gaps[potential_group_index];
			   else
				   potential_groups[potential_group_index]=system_max;
		   }

	} 
    }
    if(recent_evicted_groups.size()){
	  	    find_victim_and_update_the_table(insertion_of_potential_group);
    }
     tick_in_current_interval=0;
    if(KnobResetTableStatsPerEpoch.Value())
    {
	    total_num_aes_accesses=0;
	    total_num_large_counter_accesses=0;
	    for(int table_index=0;table_index<TABLE_SIZE;table_index++)
	    {
		    num_table_hits[table_index]=0;
		    num_table_hits_between_groups[table_index]=0;
		    dccm_overhead_for_groups[table_index]=0;
		    dccm_page_relevel_overhead_for_groups[table_index]=0;
		    dccm_block_relevel_overhead_for_groups[table_index]=0;
	    }
	    for(int potential_group_index=0;potential_group_index<POTENTIAL_GROUP_NUM;potential_group_index++)
		    num_potential_group_hits[potential_group_index]=0;
	    for(UINT64 recent_evicted_group_index=0; recent_evicted_group_index<recent_evicted_groups.size(); recent_evicted_group_index++)
	    {
		    recent_evicted_groups[recent_evicted_group_index].number_of_hits=0;
	    }
    }

  }

bool OtpTable::remaining_dccm_budget_nonzero()
{
	if((dccm_overflow_traffic<(DCCM_OVERHEAD_RATIO*OTP_INTERVAL+dccm_remain_budget)))
		return true;
	else
		return false;
}

bool OtpTable::remaining_dccm_budget_for_page_relevel()
{
	if(((dccm_overflow_traffic<(DCCM_OVERHEAD_RATIO*OTP_INTERVAL+dccm_remain_budget)))&&((DCCM_OVERHEAD_RATIO*OTP_INTERVAL+dccm_remain_budget-dccm_overflow_traffic)>(double)KnobRemainBudgetLimitForPageLevelReleveling.Value()*AES_OTP_INTERVAL))
		return true;
	else
		return false;
}
bool OtpTable::counter_satisfy_requirement_for_consuming_dccm_budget(UINT64 _effective_ctr)
{
	UINT64 current_small_epoch_index=(tick_in_current_interval/(AES_OTP_INTERVAL/TABLE_SIZE));
	if(current_small_epoch_index>=TABLE_SIZE)
		current_small_epoch_index=TABLE_SIZE-1;
	//if(remaining_dccm_budget_nonzero()&&(_effective_ctr<table[current_small_epoch_index]))
	if(remaining_dccm_budget_nonzero())
		return true;
	else
		return false;
}

bool OtpTable::counter_satisfy_requirement_for_epoch_period(UINT64 _effective_ctr)
{
	/*
	UINT64 current_small_epoch_index=(tick_in_current_interval/(AES_OTP_INTERVAL/TABLE_SIZE));
	if(current_small_epoch_index>=TABLE_SIZE)
		current_small_epoch_index=TABLE_SIZE-1;
	if(_effective_ctr<table[current_small_epoch_index])
		return true;
	else
		return false;
	*/
	return true;
}

void WriteCountCache::enqueue_overflow_nodes(const UINT64& counter_id, const UINT64& wc_block_addr, const UINT64& minor_ctr_pos, const UINT64& instr_count, bool default_clean, bool is_relevel) {
  UINT32 lowest_level_incurring_overflow=5;
  UINT32 curr_tree_level = 0;
  UINT64 _write_counter_value=morph_tree.GetEffectiveCounter(0, counter_id, minor_ctr_pos);
  if(morph_tree.will_overflow(curr_tree_level, counter_id, minor_ctr_pos, default_clean, is_relevel)){
	enqueue_overflow_node(curr_tree_level, counter_id, _write_counter_value);
	if(curr_tree_level<lowest_level_incurring_overflow)
		lowest_level_incurring_overflow=curr_tree_level;
	if(lowest_level_incurring_overflow<=3)
		number_of_preventing_block_relevel[lowest_level_incurring_overflow]++;
	return;
  }
  UINT64 curr_wc_block_addr = wc_block_addr; 
  UINT64 curr_counter_id = counter_id; 
  while (curr_tree_level < 3) {
    curr_wc_block_addr = morph_tree.GetParentCounterAddress(curr_tree_level, curr_wc_block_addr);
    UINT64 curr_minor_ctr_pos = curr_counter_id & MINOR_CTR_POS_MASK;
    curr_tree_level++;
    curr_counter_id = morph_tree.GetLevelCounterId(curr_tree_level, curr_wc_block_addr);
    if(morph_tree.will_overflow(curr_tree_level, curr_counter_id, curr_minor_ctr_pos, default_clean, false)){
	enqueue_overflow_node(curr_tree_level, curr_counter_id, _write_counter_value);
	if(curr_tree_level<lowest_level_incurring_overflow)
		lowest_level_incurring_overflow=curr_tree_level;
	if(lowest_level_incurring_overflow<=3)
		number_of_preventing_block_relevel[lowest_level_incurring_overflow]++;
	return;
    }
  }
  //if(lowest_level_incurring_overflow<=3)
  //	number_of_preventing_block_relevel[lowest_level_incurring_overflow]++;
}

