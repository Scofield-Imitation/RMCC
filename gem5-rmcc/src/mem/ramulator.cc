#include "base/callback.hh"
#include "mem/ramulator.hh"
#include "Ramulator/src/Gem5Wrapper.h"
#include "Ramulator/src/Request.h"
#include "sim/system.hh"
#include "debug/Ramulator.hh"
#include "base/random.hh"
#include "mem/cache/base.hh"
//xinw added for tree's depth and ary
#define depth_of_tree 4
#define tree_ary 128
uint64_t total_num_metadata_requests=0;
uint64_t total_num_metadata_responses=0;
#define debug_verification 0
#define debug_overflow_in_flight 0
#define debug_metadata_request_and_response 0
#define debug_metadata_eviction 0
#define debug_memory_request 0
#define debug_memory_access 0
#define debug_metadata_access 0
#define debug_metadata_miss 0
#define debug_metadata_response 0
Tick noc_latency;
float threshold_for_enqueuing_overflow_requests=10000;
uint64_t timeout=1000;
string trace_name;
uint64_t control_overflow_for_use_trace=0;
uint64_t generate_trace=0;
uint64_t use_trace=0;
uint64_t num_overflow_read_in_flight=0;
// xinw added pintool stats to use-begin
uint64_t metadata_access_latency=0;
uint64_t caching_metadata_in_llc=0;
uint64_t caching_version_in_l2=0;
double 	pintool_aes_l1_hit_rate = 0; 
double 	pintool_aes_table_hit_rate = 0; 
double	pintool_metadata_miss_rate_during_page_releveling[depth_of_tree-1] ={0}; 
double    pintool_block_level_dccm_ratio = 0;
double    pintool_metadata_miss_rate_for_memory_reads[depth_of_tree-1] = {0};
double    pintool_metadata_miss_rate_for_memory_writes[depth_of_tree-1] = {0};
//xinw added for caching metedata in LLC-begin
double    pintool_llc_metadata_miss_rate_for_memory_reads[depth_of_tree-1] = {0};
double    pintool_llc_metadata_miss_rate_for_memory_writes[depth_of_tree-1] = {0};
//xinw added for caching metedata in LLC-end
//double    pintool_overflow_events_normalized_to_data_writes[depth_of_tree] = {0};
double    pintool_overflow_events_normalized_to_data_traffic[depth_of_tree] = {0};
double    pintool_evicted_metadata_traffic_normalized_to_data_traffic = 0; 
//xinw added pintool stats to use-end
//xinw added for controlling possibility set by pintool stats-begin
uint64_t 	gem5_total_data_access_counter = 0; 
uint64_t 	gem5_aes_l1_hit_num = 0; 
uint64_t 	gem5_aes_table_hit_num = 0; 
uint64_t	gem5_metadata_miss_during_page_releveling_num[depth_of_tree-1] ={0}; 
uint64_t    gem5_block_level_dccm_num = 0;
uint64_t    gem5_metadata_miss_for_memory_reads_num[depth_of_tree-1] = {0};
uint64_t    gem5_metadata_miss_for_memory_writes_num[depth_of_tree-1] = {0};
//xinw added for caching metedata in LLC-begin
uint64_t    gem5_llc_metadata_miss_for_memory_reads_num[depth_of_tree-1] = {0};
uint64_t    gem5_llc_metadata_miss_for_memory_writes_num[depth_of_tree-1] = {0};
//xinw added for caching metedata in LLC-end
uint64_t    gem5_overflow_event_num[depth_of_tree] = {0};
uint64_t    gem5_evicted_metadata_num= 0; 
uint64_t    gem5_num_of_prefetch_request_consuming_LLC_metadata_miss_budget=0;
uint64_t    gem5_num_of_prefetch_request_consuming_private_metadata_cache_miss_budget=0;
//xinw added for controlling possibility set by pintool stats-end


double current_possibility=0;
static const int DATA_CACHE_BLOCK_SIZE = 64;
static const int DATA_CACHE_BLOCK_BITS = log2(DATA_CACHE_BLOCK_SIZE);
static const uint32_t MINOR_CTR_POS_MASK = 127;
unsigned rand_seed=0;
uint64_t range_base = (uint64_t)1 << 47;//128TB, i.e., kernel space
uint64_t randomConstVal = 660225116055843;
uint64_t rand_base = 0;
uint64_t rand_func(){
        rand_base = (rand_base + randomConstVal) % 1048576;
        return rand_base;
}
uint64_t rand_metadata_addr(){
        rand_base = (rand_base + randomConstVal) % (6307968*times_of_16g-4194304*times_of_16g)+4194304*times_of_16g;
        return rand_base;
}
uint64_t rand_metadata_addr_for_specific_tree_level(uint32_t _tree_level){
	if(_tree_level==0)
	        rand_base = (rand_base + randomConstVal) % (6291456*times_of_16g-4194304*times_of_16g)+4194304*times_of_16g;
        else if(_tree_level==1)
		rand_base = (rand_base + randomConstVal) % (6307840*times_of_16g-6291456*times_of_16g)+6291456*times_of_16g;
	else if(_tree_level==2)
		rand_base = (rand_base + randomConstVal) % (6307968*times_of_16g-6307840*times_of_16g)+6307840*times_of_16g;
	else
		rand_base = 0;
	return rand_base;
}
bool mapping_for_inst_and_walker=false;
uint64_t read_number=0;
//xinw added considering different range for controllers-begin
bool lastRealWindow = false;
uint8_t* Ramulator::memdata_0 = NULL;
uint8_t* Ramulator::memdata_1 = NULL;
Addr Ramulator::memdata_0_start = 0;
Addr Ramulator::memdata_0_end = 0;
Addr Ramulator::memdata_1_start = 0;
Addr Ramulator::memdata_1_end = 0;
unsigned long num_cycles_=0;
bool ONLY_RELEVEL_DEMAND = false;
bool OTP_FOR_ADDRESS = false;
bool OTP_PRECALCULATION=false;
bool OTP_L1_PRECALCULATION=false;
uint64_t POSSIBILITY_RELEVEL;
uint64_t high_level_node_demotion_rate=0;
int current_table_index=0;
bool static_mapping=false;
int BEGIN_WITH_BIG_COUNTER=0;
Addr current_writteback_block_addr=0;
Addr current_virtual_addr=0;
//xinw added for using trace-begin
Tick last_successful_schedule_tick[3]={0};
//int current_entry_index[3]={0};
int current_entry_index[3]={10000, 20000, 30000};
uint64_t trace_addr_interval[3]={161113151, 235513979, 307737421};
struct _trace_entry{
        Tick off_tick_to_last_request;
        bool is_write_back;
        Addr block_address;
	bool is_overflow;
};
class _trace_array{
public:
	struct _trace_entry _trace[2000000];
}trace_array[3];
uint64_t total_mem_access_0=0, total_mem_access_1=0, total_mem_access_2=0, total_mem_access_3;
//xinw added for using trace-end
class read_data_blk_info{
	public:
		PacketPtr PktPtr;
		Tick begin_time_of_otp_calculation;
		bool data_block_has_arrived;
		Tick info_create_time;
		Tick block_arrive_time;
		bool is_L1_block_miss;
		bool is_L0_block_miss;
		Tick L0_node_response_time;
		Tick L1_node_response_time;
		Tick L2_node_response_time;
		bool version_ready=false;
		bool L1_ready=false;
	public:
		read_data_blk_info(PacketPtr _PktPtr, Tick _begin_time_of_otp_calculation, bool _data_block_has_arrived, bool _is_L1_block_miss, bool _is_L0_block_miss){
			PktPtr = _PktPtr;
			begin_time_of_otp_calculation = _begin_time_of_otp_calculation;
			data_block_has_arrived = _data_block_has_arrived;
			info_create_time = curTick();
			if(caching_version_in_l2)
				L0_node_response_time = curTick() - 2*noc_latency;
			else
				L0_node_response_time = curTick() + metadata_access_latency;
			L1_node_response_time = curTick() + metadata_access_latency;
			L2_node_response_time = curTick() + metadata_access_latency;
			block_arrive_time = curTick();
			is_L1_block_miss = _is_L1_block_miss;
			is_L0_block_miss = _is_L0_block_miss;
		}
};
std::map<long, std::deque<read_data_blk_info> > read_info_map;
int MAX_WRITE_OVERHEAD_QUEUE_SIZE=2048;
uint64_t overflow_fetch_per_page = 64;
typedef struct{
  bool dirty_tag=false;
  bool overflow_tag=false;
  bool insert_tag=false;
} metadata_block_in_flight_entry;
std::map<uint64_t, metadata_block_in_flight_entry*> metadata_block_in_flight_map;
uint64_t number_of_reads=0;
uint64_t version_block_unnormal_miss=0;
//xinw added for overflow fetch queue-begin
#define fetch_due_to_normal_increment 1
#define fetch_due_to_overflow 2
#define write_back_of_metadata_block 3
#define write_back_for_overflow 4
typedef struct{
   uint64_t block_address;
   int request_type;
}write_overhead_requests_entry;
std::deque<write_overhead_requests_entry*> write_overhead_requests_queue;
bool enough_space_for_enqueueing_overflow_requests(){
     int num_overflow_requests=0;
     for (int i = write_overhead_requests_queue.size() - 1; i >= 0; i--){
	     write_overhead_requests_entry request_entry = *(write_overhead_requests_queue[i]);
	     int request_type = request_entry.request_type;
	     if((request_type==fetch_due_to_overflow)||(request_type==write_back_for_overflow))
		     num_overflow_requests++;
	}
    if(num_overflow_requests<threshold_for_enqueuing_overflow_requests*MAX_WRITE_OVERHEAD_QUEUE_SIZE)
    	return true;
    else
	return false;
}

bool no_duplication_in_write_overhead_queue(uint64_t _block_address, int _request_type){
     for (int i = write_overhead_requests_queue.size() - 1; i >= 0; i--){
        write_overhead_requests_entry request_entry = *(write_overhead_requests_queue[i]);
        if ((request_entry.block_address==_block_address)&&(request_entry.request_type==_request_type))
            return false;
     }
    return true;
}
bool no_duplication_address_in_write_overhead_queue(uint64_t _block_address){
     for (int i = write_overhead_requests_queue.size() - 1; i >= 0; i--){
        write_overhead_requests_entry request_entry = *(write_overhead_requests_queue[i]);
        if (request_entry.block_address==_block_address)
            return false;
     }
    return true;
}

//xinw added for overflow fetch queue-end
// daz3
ramulator::Gem5Wrapper* wrapper1 = NULL;
bool del_wrapper = false;
Tick begin_tick = 0;
Tick atomic_begin_tick = 0;
unsigned long my_read_cnt = 0;
unsigned long my_write_cnt = 0;
unsigned long my_total_cnt = 0;
unsigned long meta_read_cnt = 0;
unsigned long meta_write_cnt = 0;
unsigned long meta_total_cnt = 0;
unsigned interval_index=1;
bool have_printed_last_access_tick=false;
//xinw added for morphable-end
bool has_write_custom_checkpoint_file=false;
bool has_read_custom_checkpoint_file=false;
int warmup_status=0;
int  WARMUP_OVER=0;
uint64_t dccm_overflow_traffic=0;
uint64_t dccm_remain_budget=0;
bool is_dccm_overhead=false;
uint64_t occurence_epoch_end=10000;
uint64_t access_number=0;
uint64_t otp_l1_hit_total=0;
uint64_t otp_l1_miss_total=0;
uint64_t otp_table_hit_total=0;
uint64_t otp_table_miss_total=0;
uint64_t otp_table_hit_stats=0;
uint64_t otp_tables_hit_stats[4]={0};
uint64_t otp_table_period_hit_stats=0;
uint64_t otp_table_miss_stats=0;
uint64_t otp_tables_miss_stats[4]={0};
uint64_t otp_table_update_stats=0;
bool access_l1_table(){
/*	if(current_possibility<pintool_aes_table_hit_rate){
		otp_table_hit_stats++;	
		return true;
	}
*/
	if(gem5_aes_l1_hit_num<pintool_aes_l1_hit_rate*gem5_total_data_access_counter){
		gem5_aes_l1_hit_num++;
		//otp_l1_hit_stats++;	
		return true;
	}
	else{
		//otp_l1_miss_stats++;
		return false;
	}
}

bool access_aes_table(){
/*	if(current_possibility<pintool_aes_table_hit_rate){
		otp_table_hit_stats++;	
		return true;
	}
*/
	if(gem5_aes_table_hit_num<pintool_aes_table_hit_rate*gem5_total_data_access_counter){
		gem5_aes_table_hit_num++;
		otp_table_hit_stats++;	
		return true;
	}
	else{
		otp_table_miss_stats++;
		return false;
	}
}
static uint64_t counters_access_stats[4] = { 0 };
static uint64_t counters_access_read_stats[4] = { 0 };
static uint64_t counters_access_write_stats[4] = { 0 };
static uint64_t memory_writebacks_while_releveling_total = 0;
//xinw added for otp precomputation-end
static const uint64_t VERSIONS_START_ADDR = 4194304*times_of_16g;
uint64_t morph_to_physical(uint64_t morph_addr){
	uint64_t physical_addr_= ((morph_addr-VERSIONS_START_ADDR)<<6)+((Addr)124 << 30);
	return physical_addr_;	
}
uint64_t physical_to_morph(uint64_t physical_addr){
	uint64_t morph_addr_=((physical_addr-((Addr)124 << 30))>>6)+VERSIONS_START_ADDR;
	return morph_addr_;
}
uint64_t is_normal_data(uint64_t physical_addr){
	if((physical_addr<((Addr)3 << 30))||((physical_addr>((Addr)4 << 30))&&(physical_addr<((Addr)124 << 30))))
		return true;
	else
		return false;
}
MorphCtrBlock::MorphCtrBlock() {
}
void MorphCtrBlock::SetCtrLevel(const uint32_t& level) {
  ctr_level_ = level;
}
void MorphCtrBlock::SetSubNodeAddr(uint64_t sub_node_addr){
        sub_node_begin_addr = sub_node_addr;
}
void MorphCtrBlock::FetchForOverflow(){
	//gem5_overflow_event_num[curr_tree_level]++;
	if(ctr_level_>=1){      
		uint64_t begin_wc_block_addr = sub_node_begin_addr;
		uint64_t curr_wc_block_addr;
		bool should_overflow=true;
		for(uint64_t sub_index=0; sub_index<overflow_fetch_per_page*2; sub_index++){
			curr_wc_block_addr = begin_wc_block_addr + sub_index; 
			std::map<uint64_t, metadata_block_in_flight_entry*>::iterator it;
			it = metadata_block_in_flight_map.find(morph_to_physical(curr_wc_block_addr));
			//if((it != metadata_block_in_flight_map.end())||!no_duplication_address_in_write_overhead_queue(morph_to_physical(curr_wc_block_addr))){
			if((it != metadata_block_in_flight_map.end())||!no_duplication_in_write_overhead_queue(morph_to_physical(curr_wc_block_addr),fetch_due_to_overflow)){
				should_overflow=false;
				break;
			}
		}
		if(!should_overflow)
			return;
		gem5_overflow_event_num[ctr_level_]++;
		for(uint64_t sub_index=0; sub_index<overflow_fetch_per_page*2; sub_index++){
			curr_wc_block_addr = begin_wc_block_addr + sub_index; 
			std::map<uint64_t, metadata_block_in_flight_entry*>::iterator it;
			it = metadata_block_in_flight_map.find(morph_to_physical(curr_wc_block_addr));
			if(it == metadata_block_in_flight_map.end()){
				write_overhead_requests_entry *new_entry=(write_overhead_requests_entry *)malloc(sizeof(write_overhead_requests_entry));
				new_entry->block_address = morph_to_physical(curr_wc_block_addr);
				new_entry->request_type = fetch_due_to_overflow;
				write_overhead_requests_queue.push_back(new_entry);
			}
			else{
				auto& in_flight_entry = *(metadata_block_in_flight_map.find(morph_to_physical(curr_wc_block_addr))->second);
				//xinw added for limiting overflow requests
				if(!in_flight_entry.overflow_tag){
					num_overflow_read_in_flight++;
				if(debug_overflow_in_flight){
					printf("debugging overflow, place 1: incrementing counter to %lu for %lu at tick %lu\n", num_overflow_read_in_flight, morph_to_physical(curr_wc_block_addr), curTick());
				}
				}
				in_flight_entry.overflow_tag=true;
			}
		}
	}
	else{       
		uint64_t begin_block_addr = sub_node_begin_addr;
		uint64_t curr_block_addr; 
		bool should_overflow=true;
		for(uint64_t sub_index=0; sub_index<overflow_fetch_per_page*2; sub_index++){       
			curr_block_addr = begin_block_addr + sub_index*64;
			std::map<uint64_t, metadata_block_in_flight_entry*>::iterator it;
			it = metadata_block_in_flight_map.find(curr_block_addr+(Addr(0x1)<<37));
			//if((it != metadata_block_in_flight_map.end())||!no_duplication_address_in_write_overhead_queue(curr_block_addr)){
			if((it != metadata_block_in_flight_map.end())||!no_duplication_in_write_overhead_queue(curr_block_addr,fetch_due_to_overflow)){
				should_overflow=false;
				break;
			}
		}
		if(!should_overflow)
			return;
		gem5_overflow_event_num[ctr_level_]++;
		for(uint64_t sub_index=0; sub_index<overflow_fetch_per_page*2; sub_index++){       
			curr_block_addr = begin_block_addr + sub_index*64;
			std::map<uint64_t, metadata_block_in_flight_entry*>::iterator it;
			it = metadata_block_in_flight_map.find(curr_block_addr+(Addr(0x1)<<37));
			if(it == metadata_block_in_flight_map.end()){
				write_overhead_requests_entry *new_entry=(write_overhead_requests_entry *)malloc(sizeof(write_overhead_requests_entry));
				new_entry->block_address = curr_block_addr + (Addr(0x1)<<37);
				new_entry->request_type = fetch_due_to_overflow;
				write_overhead_requests_queue.push_back(new_entry);
			}
			else{
				auto& in_flight_entry = *(metadata_block_in_flight_map.find(curr_block_addr+(Addr(0x1)<<37))->second);
				//xinw added for limiting overflow requests
				if(!in_flight_entry.overflow_tag){
					num_overflow_read_in_flight++;
				if(debug_overflow_in_flight){
					printf("debugging overflow, place 2: incrementing counter to %lu for %lu at tick %lu\n", num_overflow_read_in_flight, curr_block_addr+(Addr(0x1)<<37), curTick());
				}
				}
				in_flight_entry.overflow_tag=true;
				
			}
		}
	}
}
static const uint64_t FOURK_PAGE_ADDR_FLOOR_MASK = 0xFFFFFFFFFFFFF000;
static const uint64_t EIGHTK_PAGE_ADDR_FLOOR_MASK = 0xFFFFFFFFFFFFE000;
static const uint64_t PHYSICAL_PAGE_START_ADDR = 0;
static const uint64_t TREE_LEVEL1_START_ADDR = 6291456*times_of_16g;
static const uint64_t TREE_LEVEL2_START_ADDR = 6307840*times_of_16g;
static const uint64_t TREE_LEVEL3_START_ADDR = 6307968*times_of_16g;
MorphTree::MorphTree() {
  for (uint64_t i = 0; i < PHYSICAL_PAGES_NUM; ++i) {
    rand_pages_map[i] = i;
  }
 for (uint64_t i = 0; i < PHYSICAL_PAGES_NUM*4; ++i){
        gem5_to_morph_pages_set_map[i]=false;
  }
  last_physical_page_addr = PHYSICAL_PAGE_START_ADDR;
  for(uint32_t i=0;i<times_of_16g;i++){
	  tree_level3[i].SetCtrLevel(3);
	  tree_level3[i].SetSubNodeAddr(TREE_LEVEL2_START_ADDR+i*128);
  }
  for (uint32_t i = 0; i < 128*times_of_16g; ++i) {
    tree_level2[i].SetCtrLevel(2);
    tree_level2[i].SetSubNodeAddr(TREE_LEVEL1_START_ADDR + i*128);
}
  for (uint32_t i = 0; i < 16384*times_of_16g; ++i) {
    tree_level1[i].SetCtrLevel(1);
    tree_level1[i].SetSubNodeAddr(VERSIONS_START_ADDR + i*128);
 }
   for (uint64_t i = 0; i < 393216; ++i) {
    versions_level[i].SetCtrLevel(0);
    versions_level[i].SetSubNodeAddr(i*128*64);
}
  for (uint64_t i = 393216; i < 2097152*times_of_16g; ++i) {
    versions_level[i].SetCtrLevel(0);
    versions_level[i].SetSubNodeAddr(i*128*64+(Addr(1)<<30));
  }
}
uint64_t MorphTree::GetPhysicalPageAddress(const uint64_t& block_addr) {
  uint64_t physical_page = (block_addr>>12);
  if (gem5_to_morph_pages_set_map[physical_page]){
                return gem5_to_morph_pages_map[physical_page];
  }
  else{
	if(static_mapping)	{
                uint64_t morph_page;
                if(physical_page<(Addr(0x3)<<18))
                        morph_page = rand_pages_map[physical_page];
                else
                        morph_page = rand_pages_map[physical_page-(Addr(0x1)<<18)];

                gem5_to_morph_pages_map[physical_page] = morph_page;
                gem5_to_morph_pages_set_map[physical_page]=true;
                return morph_page;
	}
	else{
		//uint64_t virtual_page_addr = block_addr & FOURK_PAGE_ADDR_FLOOR_MASK;
		uint64_t virtual_page_addr = physical_page;
		uint64_t big_virtual_page_addr = virtual_page_addr & 0xFFFFFFFFFFFFFE00;
		uint64_t ret_physical_page=0;
		for(uint64_t _index=0;_index<512;_index++){    
			uint64_t big_physical_page = rand_pages_map[last_physical_page_addr]; 
			gem5_to_morph_pages_map[big_virtual_page_addr+_index] = big_physical_page; 
			last_physical_page_addr++; 
                	gem5_to_morph_pages_set_map[big_virtual_page_addr+_index]=true;
			if((big_virtual_page_addr+_index)==virtual_page_addr)
				ret_physical_page=big_physical_page;
		}
		return ret_physical_page;
		/*
                uint64_t morph_page = rand_pages_map[last_physical_page_addr];
                gem5_to_morph_pages_map[physical_page] = morph_page;
                last_physical_page_addr++;
                gem5_to_morph_pages_set_map[physical_page]=true;
                return morph_page;
		*/
		
	}
  }
}
uint64_t MorphTree::GetParentCounterAddress(const uint32_t& level, const uint64_t& wc_block_addr) {
  uint64_t parent_counter_addr = 0;
  if (level == 0) {
    parent_counter_addr =  (wc_block_addr - VERSIONS_START_ADDR)/128 + TREE_LEVEL1_START_ADDR;
  } else if (level == 1) {
    parent_counter_addr = (wc_block_addr - TREE_LEVEL1_START_ADDR)/128 + TREE_LEVEL2_START_ADDR;
  } else if (level == 2) {
    parent_counter_addr = (wc_block_addr - TREE_LEVEL2_START_ADDR)/128 + TREE_LEVEL3_START_ADDR;
  }
  return parent_counter_addr;
}
uint64_t MorphTree::GetCounterAddress(const uint32_t& level, const uint64_t& physical_page_addr) {
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
uint32_t MorphTree::GetCurrentLevel(const uint64_t& wc_block_addr) {
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
uint64_t MorphTree::GetLevelCounterId(const uint32_t& level, const uint64_t& wc_block_addr) {
  uint64_t counter_id = 0;
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
//xinw added for otp precomputation-end
MorphTree morph_tree;
/*
bool access_metadata_cache(bool is_write, uint32_t _tree_level, uint64_t curr_wc_block_addr){
	if(is_write){
		
		if(gem5_metadata_miss_for_memory_writes_num[_tree_level]<pintool_metadata_miss_rate_for_memory_writes[_tree_level]*gem5_total_data_access_counter){
			bool should_miss=true;
			std::map<uint64_t, metadata_block_in_flight_entry*>::iterator it;
			it = metadata_block_in_flight_map.find(morph_to_physical(curr_wc_block_addr));
			if((it != metadata_block_in_flight_map.end())||!no_duplication_address_in_write_overhead_queue(morph_to_physical(curr_wc_block_addr))){
				should_miss=false;
			}
			if(should_miss){
				gem5_metadata_miss_for_memory_writes_num[_tree_level]++;
				return false;
			}
			else
				return true;
		}
		else
			return true;	
	}
	else{
			if(debug_metadata_access&&_tree_level==0){
				printf("accessing node: %lu\n", curr_wc_block_addr);
			}
			std::map<uint64_t, metadata_block_in_flight_entry*>::iterator it;
			it = metadata_block_in_flight_map.find(morph_to_physical(curr_wc_block_addr));
			if(it != metadata_block_in_flight_map.end())
				return false;
			if(gem5_metadata_miss_for_memory_reads_num[_tree_level]<pintool_metadata_miss_rate_for_memory_reads[_tree_level]*gem5_total_data_access_counter){
				gem5_metadata_miss_for_memory_reads_num[_tree_level]++;
				return false;
			}
			else
				return true;

	}
}
*/
#define metadata_cache_hit 2
#define llc_hit 1
#define miss 0
int access_metadata_cache(bool is_write, uint32_t _tree_level, uint64_t curr_wc_block_addr, bool is_prefetch){
	if(is_write){
		//if(gem5_llc_metadata_miss_for_memory_writes_num[_tree_level]<pintool_llc_metadata_miss_rate_for_memory_writes[_tree_level]*gem5_total_data_access_counter){
		if(gem5_metadata_miss_for_memory_writes_num[_tree_level]<pintool_metadata_miss_rate_for_memory_writes[_tree_level]*gem5_total_data_access_counter){
			bool should_miss=true;
			std::map<uint64_t, metadata_block_in_flight_entry*>::iterator it;
			it = metadata_block_in_flight_map.find(morph_to_physical(curr_wc_block_addr));
			if((it != metadata_block_in_flight_map.end())||!no_duplication_address_in_write_overhead_queue(morph_to_physical(curr_wc_block_addr))){
				should_miss=false;
			}
			if(should_miss){
				//gem5_llc_metadata_miss_for_memory_writes_num[_tree_level]++;
				gem5_metadata_miss_for_memory_writes_num[_tree_level]++;
				return miss;
			}
			else
				return metadata_cache_hit;
		}
		else
			return metadata_cache_hit;	
	}
	else{
			if(debug_metadata_access&&_tree_level==0){
				printf("accessing node: %lu\n", curr_wc_block_addr);
			}
			std::map<uint64_t, metadata_block_in_flight_entry*>::iterator it;
			it = metadata_block_in_flight_map.find(morph_to_physical(curr_wc_block_addr));
			if(it != metadata_block_in_flight_map.end())
				return miss;
			if(gem5_metadata_miss_for_memory_reads_num[_tree_level]<pintool_metadata_miss_rate_for_memory_reads[_tree_level]*gem5_total_data_access_counter){
				gem5_metadata_miss_for_memory_reads_num[_tree_level]++;
				if(gem5_llc_metadata_miss_for_memory_reads_num[_tree_level]<pintool_llc_metadata_miss_rate_for_memory_reads[_tree_level]*gem5_total_data_access_counter){
					gem5_llc_metadata_miss_for_memory_reads_num[_tree_level]++;
					if(is_prefetch)
						gem5_num_of_prefetch_request_consuming_LLC_metadata_miss_budget++;
					return miss;
				}
				else{
					if(is_prefetch)
						gem5_num_of_prefetch_request_consuming_private_metadata_cache_miss_budget++;
					return llc_hit;
				}
			}
			else
				return metadata_cache_hit;

	}
}

Ramulator::Ramulator(const Params *p):
    AbstractMemory(p),
    port(name() + ".port", *this),
    requestsInFlight(0),
    drain_manager(NULL),
    config_file(p->config_file),
    configs(p->config_file),
    wrapper(NULL),
    read_cb_func(std::bind(&Ramulator::readComplete, this, std::placeholders::_1)),
    //xinw added for morphable-begin 
    read_meta_func(std::bind(&Ramulator::readMetaComplete, this, std::placeholders::_1)),
    write_overhead_func(std::bind(&Ramulator::writeOverheadComplete, this, std::placeholders::_1)),
    //xinw added for morphable-end 
    write_cb_func(std::bind(&Ramulator::writeComplete, this, std::placeholders::_1)),
    //xinw added for using trace
    trace_cb_func(std::bind(&Ramulator::traceComplete, this, std::placeholders::_1)),
    ticks_per_clk(0),
    resp_stall(false),
    req_stall(false),
    send_resp_event(this),
    tick_event(this),
    last_access_tick(curTick())
{
	control_overflow_for_use_trace = p->control_overflow_for_use_trace;
	caching_metadata_in_llc=p->caching_metadata_in_llc;
	threshold_for_enqueuing_overflow_requests=p->threshold_for_enqueuing_overflow_requests;
	MAX_WRITE_OVERHEAD_QUEUE_SIZE=p->max_write_overhead_queue_size;
	timeout = p->timeout; 
    // xinw added pintool stats to use-begin
	offload_aes_to_llc  = p->offload_aes_to_llc;
	carry_less_multiplication_latency = p->carry_less_multiplication_latency;
	adaptive_l2_aes=p->adaptive_l2_aes;
	caching_version_in_l2 = p->caching_version_in_l2;
     	pintool_aes_l1_hit_rate = p->pintool_aes_l1_hit_rate*1.0/1000000; 
     	pintool_aes_table_hit_rate = p->pintool_aes_table_hit_rate*1.0/1000000; 
    	pintool_metadata_miss_rate_during_page_releveling[0] = p->pintool_metadata_miss_rate_during_page_releveling_0*1.0/1000000; 
    	pintool_metadata_miss_rate_during_page_releveling[1] = p->pintool_metadata_miss_rate_during_page_releveling_1*1.0/1000000; 
    	pintool_metadata_miss_rate_during_page_releveling[2] = p->pintool_metadata_miss_rate_during_page_releveling_2*1.0/1000000; 
        pintool_block_level_dccm_ratio = p->pintool_block_level_dccm_ratio*1.0/1000000;
        pintool_metadata_miss_rate_for_memory_reads[0] = p->pintool_metadata_miss_rate_for_memory_reads_0*1.0/1000000;
        pintool_metadata_miss_rate_for_memory_reads[1] = p->pintool_metadata_miss_rate_for_memory_reads_1*1.0/1000000;
        pintool_metadata_miss_rate_for_memory_reads[2] = p->pintool_metadata_miss_rate_for_memory_reads_2*1.0/1000000;
        pintool_metadata_miss_rate_for_memory_writes[0] = p->pintool_metadata_miss_rate_for_memory_writes_0*1.0/1000000;
        pintool_metadata_miss_rate_for_memory_writes[1] = p->pintool_metadata_miss_rate_for_memory_writes_1*1.0/1000000;
        pintool_metadata_miss_rate_for_memory_writes[2] = p->pintool_metadata_miss_rate_for_memory_writes_2*1.0/1000000;

	//xinw added for caching metadata in LLC-begin
/*	pintool_llc_metadata_miss_rate_for_memory_reads[0] = p->pintool_llc_metadata_miss_rate_for_memory_reads_0*1.0/1000000;
        pintool_llc_metadata_miss_rate_for_memory_reads[1] = p->pintool_llc_metadata_miss_rate_for_memory_reads_1*1.0/1000000;
        pintool_llc_metadata_miss_rate_for_memory_reads[2] = p->pintool_llc_metadata_miss_rate_for_memory_reads_2*1.0/1000000;
        pintool_llc_metadata_miss_rate_for_memory_writes[0] = p->pintool_llc_metadata_miss_rate_for_memory_writes_0*1.0/1000000;
        pintool_llc_metadata_miss_rate_for_memory_writes[1] = p->pintool_llc_metadata_miss_rate_for_memory_writes_1*1.0/1000000;
        pintool_llc_metadata_miss_rate_for_memory_writes[2] = p->pintool_llc_metadata_miss_rate_for_memory_writes_2*1.0/1000000;
*/
	if(p->caching_metadata_in_llc){
		pintool_llc_metadata_miss_rate_for_memory_reads[0] = p->pintool_llc_metadata_miss_rate_for_memory_reads_0*1.0/1000000;
		pintool_llc_metadata_miss_rate_for_memory_reads[1] = p->pintool_llc_metadata_miss_rate_for_memory_reads_1*1.0/1000000;
		pintool_llc_metadata_miss_rate_for_memory_reads[2] = p->pintool_llc_metadata_miss_rate_for_memory_reads_2*1.0/1000000;
		pintool_llc_metadata_miss_rate_for_memory_writes[0] = p->pintool_llc_metadata_miss_rate_for_memory_writes_0*1.0/1000000;
		pintool_llc_metadata_miss_rate_for_memory_writes[1] = p->pintool_llc_metadata_miss_rate_for_memory_writes_1*1.0/1000000;
		pintool_llc_metadata_miss_rate_for_memory_writes[2] = p->pintool_llc_metadata_miss_rate_for_memory_writes_2*1.0/1000000;
	}
	else{
		pintool_llc_metadata_miss_rate_for_memory_reads[0] = 1;
		pintool_llc_metadata_miss_rate_for_memory_reads[1] = 1;
		pintool_llc_metadata_miss_rate_for_memory_reads[2] = 1;
		pintool_llc_metadata_miss_rate_for_memory_writes[0] = 1;
		pintool_llc_metadata_miss_rate_for_memory_writes[1] = 1;
		pintool_llc_metadata_miss_rate_for_memory_writes[2] = 1;
	}

	//xinw added for caching metadata in LLC-end
       
        pintool_overflow_events_normalized_to_data_traffic[0] = p->pintool_overflow_events_normalized_to_data_traffic_0*1.0/1000000;
        pintool_overflow_events_normalized_to_data_traffic[1] = p->pintool_overflow_events_normalized_to_data_traffic_1*1.0/1000000;
        pintool_overflow_events_normalized_to_data_traffic[2] = p->pintool_overflow_events_normalized_to_data_traffic_2*1.0/1000000;
        pintool_overflow_events_normalized_to_data_traffic[3] = p->pintool_overflow_events_normalized_to_data_traffic_3*1.0/1000000;
        pintool_evicted_metadata_traffic_normalized_to_data_traffic = p->pintool_evicted_metadata_traffic_normalized_to_data_traffic*1.0/1000000; 
    //xinw added pintool stats to use-end
	mapping_for_inst_and_walker=p->mapping_for_inst_and_walker;
	BEGIN_WITH_BIG_COUNTER=p->rand_init;
	static_mapping = p->static_mapping;
	atomicwarmuptime = p->atomicwarmupTime;
	warmuptime = p->warmupTime;
        //wrapper->set_warmuptime(p->atomicwarmupTime, p->warmupTime);
        configs.set_core_num(p->num_cpus);
 	//xinw added for generating trace
	configs.set_tracefile_directory(p->checkpoint_dir);
	configs.set_trace_name(p->trace_name);
	trace_name=p->trace_name;
	configs.set_generate_trace(p->generate_trace);
	use_trace=p->use_trace;
	//xinw added for morphable-begin
	cpt_index = p->cpt_index;
        checkpoint_dir = p->checkpoint_dir;
	outdir = p->outdir;
	write_overhead_dequeue_interval = p->write_overhead_dequeue_interval;
	simulationtime = p->simulationTime;
	POSSIBILITY_RELEVEL = p->possibility_relevel; 
        ONLY_RELEVEL_DEMAND = p->only_relevel_demand;
	OTP_FOR_ADDRESS	= p->otp_for_address;
	OTP_PRECALCULATION = p->predictive_decryption;
	OTP_L1_PRECALCULATION = p->predictive_decryption_l1;
	high_level_node_demotion_rate = p->high_level_tree_node_demotion_rate;
    	memory_encryption = p->memory_encryption;
	//aes_latency = p->aes_latency;	
	data_aes_latency = p->data_aes_latency;	
	//xinw added for caching metadata in LLC
	llc_metadata_access_latency = p->llc_metadata_access_latency;	
	tree_node_aes_latency = p->tree_node_aes_latency;	
	mac_latency = p->mac_latency;	
	modulo_latency = p->modulo_latency;	
	metadata_access_latency = p->metadata_access_latency;	
	noc_latency = p->noc_latency;	
	wait_counter = p->wait_counter;	
	overflow_fetch_per_page = p->overflow_fetch_per_page;
	//xinw added for morphable-end
}
Ramulator::~Ramulator(){
    // delete wrapper;
    // daz3
    if(del_wrapper == false){
        delete wrapper;
        del_wrapper = true;
    }
}
void Ramulator::init() {
    if (!port.isConnected()){ 
        fatal("Ramulator port not connected\n");
    } else { 
        port.sendRangeChange(); 
    }
//xinw added considering different range for controllers-begin
  if(memdata_0 == NULL && range.start() == 0){
	memdata_0 = pmemAddr;
	memdata_0_start = range.start();
	memdata_0_end = range.end();
      }
   if(memdata_1 == NULL && range.start() != 0){
	memdata_1 = pmemAddr;
	memdata_1_start = range.start();
	memdata_1_end = range.end();
      }
//xinw added considering different range for controllers-end
    if (wrapper1 != NULL){
        wrapper = wrapper1;
    }
    else{
        std::cout << "RAMULATOR: system cacheline size is " << system()->cacheLineSize() << std::endl;
        wrapper = new ramulator::Gem5Wrapper(configs, system()->cacheLineSize());
        wrapper1 =wrapper;
    }
    // daz3
    // wrapper = new ramulator::Gem5Wrapper(configs, system()->cacheLineSize());
    ticks_per_clk = Tick(wrapper->tCK * SimClock::Float::ns);
    //printf("ticks_per_clk: %lu\n", ticks_per_clk);
    DPRINTF(Ramulator, "Instantiated Ramulator with config file '%s' (tCK=%lf, %d ticks per clk)\n", 
        config_file.c_str(), wrapper->tCK, ticks_per_clk);
    Callback* cb = new MakeCallback<ramulator::Gem5Wrapper, &ramulator::Gem5Wrapper::finish>(wrapper);
    registerExitCallback(cb);
}
void Ramulator::startup() {
    schedule(tick_event, clockEdge());
}
unsigned int Ramulator::drain(DrainManager* dm) {
       return 0;
}
BaseSlavePort& Ramulator::getSlavePort(const std::string& if_name, PortID idx) {
    if (if_name != "port") {
        return MemObject::getSlavePort(if_name, idx);
    } else {
        return port;
    }
}
void Ramulator::sendDelayResponse() {
	assert(memory_encryption);
        assert(!resp_stall);
        assert(!delay_resp_queue.empty());
	for( int i=0; i<delay_resp_queue.size();i++){
		if(delay_resp_queue[i].resp_time<curTick()){
			if(port.sendTimingResp(delay_resp_queue[i].pkt)){
				delay_resp_queue.erase(delay_resp_queue.begin()+i);
				i--;
			}
			else{
				resp_stall = true;
				return;
			}
		}
		//else
		//	i++;
	}
}

void Ramulator::sendResponse() {
    assert(!resp_stall);
    assert(!resp_queue.empty());
    DPRINTF(Ramulator, "Attempting to send response\n");
    long addr = resp_queue.front()->getAddr();
    if(addr){/*DO NOTHING. For avoid error unused-variable*/}
    if (port.sendTimingResp(resp_queue.front())){
        DPRINTF(Ramulator, "Response to %ld sent.\n", addr);
        resp_queue.pop_front();
        if (resp_queue.size() && !send_resp_event.scheduled())
            schedule(send_resp_event, curTick());
        // check if we were asked to drain and if we are now done
        if (drain_manager && numOutstanding() == 0) {
            drain_manager->signalDrainDone();
            drain_manager = NULL;
        }
    } else 
        resp_stall = true;
}
void Ramulator::tick() { 
//xinw added for l2 aes
if(begin_tick&&(curTick()>(begin_tick + warmuptime))){ 
        std::vector<void *> l2caches = system()->l2caches;
	for(size_t i = 0; i < l2caches.size(); ++i){
		BaseCache *l2Ptr = static_cast<BaseCache *>(system()->l2caches[i]);
		l2Ptr->in_observation_window=1;
		l2Ptr->deal_with_l2_aes();
	}  
	BaseCache *l3Ptr = static_cast<BaseCache *>(system()->l3cache[0]);
	l3Ptr->in_observation_window=1;
	l3Ptr->deal_with_llc_aes();
}
//xinw added
	//xinw added for using trace-begin
	/*Tick last_successful_schedule_tick[3]={0};
	  int current_entry_index[3]={0};
	  struct _trace_entry
	  {
	  Tick off_tick_to_last_request;
	  bool is_write_back;
	  Addr block_address;
	  };
	  class _trace_array
	  {
	  public:
	  struct _trace_entry _trace[1000000];
	  }trace_array[3];
	 */
     if(memory_encryption&&(!resp_stall)&&(!delay_resp_queue.empty()))
	sendDelayResponse();
     if(use_trace&&begin_tick&&(curTick()>(begin_tick + warmuptime))){
		ramulator::Request req_0(0, ramulator::Request::Type::READ, read_cb_func, 0);	
		for (unsigned i=0;i<3;i++){
			if(((curTick()-last_successful_schedule_tick[i])>=trace_array[i]._trace[current_entry_index[i]].off_tick_to_last_request)
					&&((total_mem_access_1+total_mem_access_2+total_mem_access_3)<(3*total_mem_access_0))){
				//Addr _request_address=trace_array[i]._trace[current_entry_index[i]].block_address+((Addr)(i+1) << 36);
				Addr _request_address=trace_array[i]._trace[current_entry_index[i]].block_address+((Addr)(i+1) << 38) + ((Addr)(trace_addr_interval[i])<<6);
				if(trace_array[i]._trace[current_entry_index[i]].is_write_back){
					ramulator::Request req(_request_address, ramulator::Request::Type::WRITE, trace_cb_func, 0, false, -1, false);
					if(control_overflow_for_use_trace)
						req.is_overflow=trace_array[i]._trace[current_entry_index[i]].is_overflow;
					if((!control_overflow_for_use_trace)||(!req.is_overflow)||(wrapper->get_overflow_queue_size(req_0)<256*threshold_for_enqueuing_overflow_requests)){
					bool accepted = wrapper->send(req);
					if (accepted){
						if(i==0){
							mem_writes_1++;
							total_mem_access_1++;
						}
						if(i==1){
							mem_writes_2++;
							total_mem_access_2++;
						}
						if(i==2){
							mem_writes_3++;
							total_mem_access_3++;
						}
						last_successful_schedule_tick[i]=curTick();
						current_entry_index[i]++;
					}
					}
				}
				else{
					ramulator::Request req(_request_address, ramulator::Request::Type::READ, trace_cb_func, 0, false, -1, false);
					
					if(control_overflow_for_use_trace)
						req.is_overflow=trace_array[i]._trace[current_entry_index[i]].is_overflow;

					if((!control_overflow_for_use_trace)||(!req.is_overflow)||(wrapper->get_overflow_queue_size(req_0)<256*threshold_for_enqueuing_overflow_requests)){
					bool accepted = wrapper->send(req);
					if (accepted){
						if(i==0){
							mem_reads_1++;
							total_mem_access_1++;
						}
						if(i==1){
							mem_reads_2++;
							total_mem_access_2++;
						}
						if(i==2){
							mem_reads_3++;
							total_mem_access_3++;
						}
						last_successful_schedule_tick[i]=curTick();
						current_entry_index[i]++;
					}
					}
				}
			}
		}
	}
	//xinw added for using trace-end



















	//xinw added for statistics-begin
	num_cycles_++;
	_otp_table_hit_stats=otp_table_hit_stats;
	_otp_tables_hit_stats_0=otp_tables_hit_stats[0];
	_otp_tables_hit_stats_1=otp_tables_hit_stats[1];
	_otp_tables_hit_stats_2=otp_tables_hit_stats[2];
	_otp_tables_hit_stats_3=otp_tables_hit_stats[3];
	_otp_table_miss_stats=otp_table_miss_stats;
	_otp_tables_miss_stats_0=otp_tables_miss_stats[0];
	_otp_tables_miss_stats_1=otp_tables_miss_stats[1];
	_otp_tables_miss_stats_2=otp_tables_miss_stats[2];
	_otp_tables_miss_stats_3=otp_tables_miss_stats[3];
	//xinw added for handling data block request-begin
	//xinw added for controlling possibility set by pintool stats-begin
	total_data_access_counter=gem5_total_data_access_counter ; 
	aes_table_hit_num=gem5_aes_table_hit_num ; 
	metadata_miss_during_page_releveling_num_0=gem5_metadata_miss_during_page_releveling_num[0]; 
	metadata_miss_during_page_releveling_num_1=gem5_metadata_miss_during_page_releveling_num[1]; 
	metadata_miss_during_page_releveling_num_2=gem5_metadata_miss_during_page_releveling_num[2]; 
	block_level_dccm_num=gem5_block_level_dccm_num ;
	metadata_miss_for_memory_reads_num_0=gem5_metadata_miss_for_memory_reads_num[0];
	metadata_miss_for_memory_reads_num_1=gem5_metadata_miss_for_memory_reads_num[1];
	metadata_miss_for_memory_reads_num_2=gem5_metadata_miss_for_memory_reads_num[2];
	metadata_miss_for_memory_writes_num_0=gem5_metadata_miss_for_memory_writes_num[0];
	metadata_miss_for_memory_writes_num_1=gem5_metadata_miss_for_memory_writes_num[1];
	metadata_miss_for_memory_writes_num_2=gem5_metadata_miss_for_memory_writes_num[2];
	overflow_event_num_0=gem5_overflow_event_num[0];
	overflow_event_num_1=gem5_overflow_event_num[1];
	overflow_event_num_2=gem5_overflow_event_num[2];
	overflow_event_num_3=gem5_overflow_event_num[3];
	evicted_metadata_num=gem5_evicted_metadata_num; 
	num_of_prefetch_request_consuming_private_metadata_cache_miss_budget=gem5_num_of_prefetch_request_consuming_private_metadata_cache_miss_budget;
	num_of_prefetch_request_consuming_LLC_metadata_miss_budget=gem5_num_of_prefetch_request_consuming_LLC_metadata_miss_budget;
	//xinw added for controlling possibility set by pintool stats-end

	
	if(memory_encryption){
		std::map<long, std::deque<read_data_blk_info>>::iterator it = read_info_map.begin();
		while(it != read_info_map.end()){
			std::map<long, std::deque<read_data_blk_info>>::iterator data_blk_info = it++;
			Addr data_blk_addr=data_blk_info->first;
			if((data_blk_addr>=range.start())&&(data_blk_addr<=range.end())){
				bool data_blk_arrived = (read_info_map.find(data_blk_addr))->second.front().data_block_has_arrived; 
				bool version_arrived = (read_info_map.find(data_blk_addr))->second.front().version_ready; 
				bool L1_arrived = (read_info_map.find(data_blk_addr))->second.front().L1_ready; 
				bool L1_ready_when_request = !((read_info_map.find(data_blk_addr))->second.front().is_L1_block_miss); 
				bool L0_ready_when_request = !((read_info_map.find(data_blk_addr))->second.front().is_L0_block_miss); 
				bool version_ready=version_arrived||L0_ready_when_request;
				bool L1_ready=L1_arrived||L1_ready_when_request;
				bool counter_ready = ((!wait_counter)||(version_ready&&L1_ready));
	    			bool available_schedule= (!resp_stall && !send_resp_event.scheduled());
				//if(data_blk_arrived&&counter_ready){
				if(data_blk_arrived&&counter_ready&&available_schedule){
					auto& read_data_blk_info_queue = data_blk_info->second;
					read_data_blk_info info = read_data_blk_info_queue.front();	
					PacketPtr data_pkt = info.PktPtr;
					//Tick begin_time_of_otp_calculation=read_data_blk_info_queue.front().begin_time_of_otp_calculation;
					Tick _info_create_time = read_data_blk_info_queue.front().info_create_time;
					Tick _block_arrive_time = read_data_blk_info_queue.front().block_arrive_time;
					Tick _L0_node_response_time = read_data_blk_info_queue.front().L0_node_response_time;
					Tick _L1_node_response_time = read_data_blk_info_queue.front().L1_node_response_time;
					Tick _L2_node_response_time = read_data_blk_info_queue.front().L2_node_response_time;
					//bool _is_L1_block_miss = read_data_blk_info_queue.front().is_L1_block_miss;
					bool _is_L0_block_miss = read_data_blk_info_queue.front().is_L0_block_miss;
					read_data_blk_info_queue.pop_front();
					if (!read_data_blk_info_queue.size()){
						read_info_map.erase(data_blk_addr);
					}
					--requestsInFlight;
					//xinw added for otp precomputation-begin
					if(OTP_PRECALCULATION){
						current_table_index=data_pkt->req->contextId();  
						//current_possibility = rand()*1.0/RAND_MAX;
						bool table_hit=access_aes_table();
						if(table_hit){
							accessAndRespondDccm(data_pkt, _is_L0_block_miss, _info_create_time, _block_arrive_time, _L0_node_response_time, _L1_node_response_time, _L2_node_response_time);
						}
						else{
							accessAndRespondBaseline(data_pkt, _is_L0_block_miss, _info_create_time, _block_arrive_time, _L0_node_response_time, _L1_node_response_time, _L2_node_response_time);
						}
					}
					//xinw added for otp precomputation-end
					else{
							accessAndRespondBaseline(data_pkt, _is_L0_block_miss, _info_create_time, _block_arrive_time, _L0_node_response_time, _L1_node_response_time, _L2_node_response_time);
					}
				}
			}
		}
	}
	//xinw added for handling data block request-end
	//xinw added for overflow fetches-begin
	typedef std::map<long, std::deque<read_data_blk_info> > MyMap;
	for( MyMap::const_iterator it = read_info_map.begin(); it != read_info_map.end(); ++it ){
		auto& read_data_blk_info_queue = it->second;
		read_data_blk_info info = read_data_blk_info_queue.front();
		if((curTick()-(info.info_create_time))>5000000){ 
			stall_for_more_than_5_microseconds++;
		}
	}
	if (!(write_overhead_requests_queue.empty())){	
		send_write_overhead_requests_detailed();
	}
	//xinw added for overflow fetches-end
	//xinw added for statistics-end
	wrapper->tick();
	if (req_stall){
		req_stall = false;
		port.sendRetryReq();
	}
	schedule(tick_event, curTick() + ticks_per_clk);
}
//xinw added
Tick Ramulator::recvAtomic(PacketPtr pkt) {
    if (atomic_begin_tick == 0) {
            wrapper->set_warmuptime(atomicwarmuptime, warmuptime);
            wrapper->set_timeout(timeout);
	    atomic_begin_tick = curTick();
	    //xinw added for using trace-begin
	    ifstream file_trace;
	    file_trace.open(checkpoint_dir+trace_name, ios::app);
	    file_trace.read((char*)&trace_array[0], sizeof(trace_array[0]));
	    file_trace.close();
	    file_trace.open(checkpoint_dir+trace_name, ios::app);
	    file_trace.read((char*)&trace_array[1], sizeof(trace_array[1]));
	    file_trace.close();
	    file_trace.open(checkpoint_dir+trace_name, ios::app);
	    file_trace.read((char*)&trace_array[2], sizeof(trace_array[2]));
	    file_trace.close();
	    for(int i=0;i<=100;i++)
		    printf("the %d entry: %lu %d %lu\n",i, trace_array[0]._trace[i].off_tick_to_last_request, trace_array[0]._trace[i].is_write_back, trace_array[0]._trace[i].block_address);

	    //xinw added for using trace-end

    }
    access_metadata_atomic(pkt);
    access(pkt);
    return pkt->cacheResponding() ? 0 : 50000;
}
void Ramulator::recvFunctional(PacketPtr pkt) {
    pkt->pushLabel(name());
    functionalAccess(pkt);
    for (auto i = resp_queue.begin(); i != resp_queue.end(); ++i)
        pkt->checkFunctional(*i);
    pkt->popLabel();
}
//xinw added-begin
void Ramulator::access_metadata_atomic(PacketPtr pkt){
	    if(pkt->isWrite()){
		    uint64_t block_addr = pkt->getAddr();
		    morph_tree.GetPhysicalPageAddress(block_addr);
	    }
	    else if (pkt->isRead()){
		    uint64_t block_addr = pkt->getAddr();
		    //uint64_t physical_page_addr = morph_tree.GetPhysicalPageAddress(block_addr);
		    morph_tree.GetPhysicalPageAddress(block_addr);
	    }
}
void ChainCounterIncrement(const uint64_t& counter_id, const uint64_t& wc_block_addr, const uint64_t& minor_ctr_pos) {
	uint32_t curr_tree_level = 0;
	uint64_t curr_wc_block_addr = wc_block_addr;
	uint64_t curr_counter_id = counter_id;
	int curr_wccache_hit = access_metadata_cache(true, curr_tree_level, curr_wc_block_addr, false);
	counters_access_stats[0]++;
	counters_access_write_stats[0]++;
	if(!curr_wccache_hit){	
		std::map<uint64_t, metadata_block_in_flight_entry*>::iterator it;
		it = metadata_block_in_flight_map.find(morph_to_physical(curr_wc_block_addr));
		if(it == metadata_block_in_flight_map.end()){
			write_overhead_requests_entry *new_entry=(write_overhead_requests_entry *)malloc(sizeof(write_overhead_requests_entry));
			new_entry->block_address = morph_to_physical(curr_wc_block_addr);
			new_entry->request_type = fetch_due_to_normal_increment;
			if(no_duplication_in_write_overhead_queue(new_entry->block_address, new_entry->request_type))
				write_overhead_requests_queue.push_back(new_entry);
			else
				free(new_entry);
		}
		else{
			auto& in_flight_entry = *(metadata_block_in_flight_map.find(morph_to_physical(curr_wc_block_addr))->second);
			in_flight_entry.dirty_tag=true;
			in_flight_entry.insert_tag=true;
		}
	}
/*	if(current_possibility<(pintool_overflow_events_normalized_to_data_writes[0]/(2.0*tree_ary)))
		morph_tree.versions_level[curr_counter_id].FetchForOverflow();
*/
	if(gem5_overflow_event_num[0]<pintool_overflow_events_normalized_to_data_traffic[0]*gem5_total_data_access_counter){
		//gem5_overflow_event_num[0]++;
		morph_tree.versions_level[curr_counter_id].FetchForOverflow();
	}
	while (curr_tree_level < 3) {
		curr_wc_block_addr = morph_tree.GetParentCounterAddress(curr_tree_level, curr_wc_block_addr);
		curr_wccache_hit = access_metadata_cache(true, curr_tree_level+1, curr_wc_block_addr, false);
		if(!curr_wccache_hit){
			std::map<uint64_t, metadata_block_in_flight_entry*>::iterator it;
			it = metadata_block_in_flight_map.find(morph_to_physical(curr_wc_block_addr));
			if(it == metadata_block_in_flight_map.end()){
				write_overhead_requests_entry *new_entry=(write_overhead_requests_entry *)malloc(sizeof(write_overhead_requests_entry));
				new_entry->block_address = morph_to_physical(curr_wc_block_addr);
				new_entry->request_type = fetch_due_to_normal_increment;
				if(no_duplication_in_write_overhead_queue(new_entry->block_address, new_entry->request_type))
					write_overhead_requests_queue.push_back(new_entry);
				else
					free(new_entry);
			}
			else{
				auto& in_flight_entry = *(metadata_block_in_flight_map.find(morph_to_physical(curr_wc_block_addr))->second);
				in_flight_entry.dirty_tag=true;
				in_flight_entry.insert_tag=true;
			}
		}
		//uint64_t curr_minor_ctr_pos = curr_counter_id & MINOR_CTR_POS_MASK;
		curr_tree_level++;
		counters_access_stats[curr_tree_level]++;
		counters_access_write_stats[curr_tree_level]++;
		curr_counter_id = morph_tree.GetLevelCounterId(curr_tree_level, curr_wc_block_addr);
		//morph_tree.IncrementMinorCounter(curr_tree_level, curr_counter_id, curr_minor_ctr_pos, default_clean, false);
	/*	if(current_possibility<(pintool_overflow_events_normalized_to_data_writes[curr_tree_level]/(2.0*tree_ary))){
			if(curr_tree_level==1)
				morph_tree.tree_level1[curr_counter_id].FetchForOverflow();
			if(curr_tree_level==2)
				morph_tree.tree_level2[curr_counter_id].FetchForOverflow();
			if(curr_tree_level==3)
				morph_tree.tree_level3[curr_counter_id].FetchForOverflow();
		}*/
		if(gem5_overflow_event_num[curr_tree_level]<pintool_overflow_events_normalized_to_data_traffic[curr_tree_level]*gem5_total_data_access_counter){
			//gem5_overflow_event_num[curr_tree_level]++;
			if(curr_tree_level==1)
				morph_tree.tree_level1[curr_counter_id].FetchForOverflow();
			if(curr_tree_level==2)
				morph_tree.tree_level2[curr_counter_id].FetchForOverflow();
			if(curr_tree_level==3)
				morph_tree.tree_level3[curr_counter_id].FetchForOverflow();
		}

	
	}
}
bool Ramulator::send_requests_detailed(PacketPtr pkt){
	bool is_hard_prefetch=false;
	if(system()->getMasterName(pkt->req->masterId()).find("inst")!=std::string::npos){
		is_hard_prefetch=true;		
	}

	if(!mapping_for_inst_and_walker){
		if((system()->getMasterName(pkt->req->masterId()).find("inst")!=std::string::npos)
				||(system()->getMasterName(pkt->req->masterId()).find("walker")!=std::string::npos)){
			
			//printf("satisfy one request for inst/walker: %lu at tick: %lu\n", pkt->getAddr(), curTick());
			return true;
		}
	}
	if(write_overhead_requests_queue.size()>MAX_WRITE_OVERHEAD_QUEUE_SIZE){
	 	stall_due_to_write_overhead_queue_full++;
		set_req_stall();
		return false;
	}
	if(pkt->isWrite()){	
		uint64_t block_addr = pkt->getAddr();
		uint64_t request_physical_page_addr = morph_tree.GetPhysicalPageAddress(block_addr);
		uint64_t request_wc_block_addr = morph_tree.GetCounterAddress(0, request_physical_page_addr);
		uint64_t request_minor_ctr_pos = (request_physical_page_addr%2)*64+((block_addr >> DATA_CACHE_BLOCK_BITS) & (MINOR_CTR_POS_MASK>>1));
		uint64_t request_counter_id = morph_tree.GetLevelCounterId(0, request_wc_block_addr);
		//xinw modified for dccm-begin
		ChainCounterIncrement(request_counter_id, request_wc_block_addr, request_minor_ctr_pos);
		ramulator::Request req(pkt->getAddr(), ramulator::Request::Type::WRITE, write_cb_func, 0);
		bool accepted = wrapper->send(req);
		if (accepted){
			accessAndRespond(pkt, 0, curTick());
			DPRINTF(Ramulator, "Write to %ld accepted and served.\n", req.addr);
			// added counter to track requests in flight
			++requestsInFlight;
			// daz3
			my_write_cnt++;
			my_total_cnt++;
		} else {
			req_stall = true;
		}
		return accepted;
	}
	else if (pkt->isRead()){
	  	uint64_t block_addr = pkt->getAddr();
		//if(current_possibility<pintool_block_level_dccm_ratio){     
		if(gem5_block_level_dccm_num< pintool_block_level_dccm_ratio*gem5_total_data_access_counter){
			gem5_block_level_dccm_num++;
			ramulator::Request writeback_req(block_addr, ramulator::Request::Type::WRITE, write_cb_func, 0);
			bool accepted = wrapper->send(writeback_req);
			if (accepted){
				memory_writebacks_while_releveling_total++;
			}
			else{
				printf("fails while writting back normal data block due to releveling while read a data block\n");
				//xinw modified: instead of directly aborting, just give up writing back.
				//exit(0);
				gem5_block_level_dccm_num--;
			}
		}
		for(uint32_t victim_level=0;victim_level<3;victim_level++){
			//if(current_possibility<pintool_metadata_miss_rate_during_page_releveling[victim_level]){
			if(gem5_metadata_miss_during_page_releveling_num[victim_level]<pintool_metadata_miss_rate_during_page_releveling[victim_level]*gem5_total_data_access_counter){
					//gem5_metadata_miss_during_page_releveling_num[victim_level]++;
					uint64_t victim_wc_block_addr=rand_metadata_addr_for_specific_tree_level(victim_level);
					std::map<uint64_t, metadata_block_in_flight_entry*>::iterator it;
					it = metadata_block_in_flight_map.find(morph_to_physical(victim_wc_block_addr));
					if(it == metadata_block_in_flight_map.end()){
						metadata_block_in_flight_entry* new_entry=( metadata_block_in_flight_entry*)malloc(sizeof( metadata_block_in_flight_entry*));
						new_entry->dirty_tag=true;
						new_entry->insert_tag=false;
						//new_entry->overflow_tag=true;
						new_entry->overflow_tag=false;
						metadata_block_in_flight_map.insert(std::pair<uint64_t, metadata_block_in_flight_entry*>(morph_to_physical(victim_wc_block_addr),new_entry));
						ramulator::Request req(morph_to_physical(victim_wc_block_addr), ramulator::Request::Type::READ, read_meta_func, 0, false, -1, false);
						bool accepted = wrapper->send(req);
						
						if (accepted){
							gem5_metadata_miss_during_page_releveling_num[victim_level]++;
							DPRINTF(Ramulator, "Read to %ld accepted.\n", req.addr);
							total_num_metadata_requests++;
						 	if(debug_metadata_request_and_response){
								printf("debugging metadata request and response, request for metadata: %lu , total metadata request: %lu , total metadata response: %lu\n", req.addr, total_num_metadata_requests,total_num_metadata_responses);
							}	
						}
						else{
							//stall_due_to_ramulator_read_queue_full++;
							//req_stall = true;
							printf("failing to send the request for metadata miss during page releveling\n");
							//abort();
						}
					}
			}
		}
		int is_wccache_hit = 0;
		uint64_t physical_page_addr = morph_tree.GetPhysicalPageAddress(block_addr);
		int level=0;
		ramulator::Request req_0(pkt->getAddr(), ramulator::Request::Type::READ, read_cb_func, 0);
		if(wrapper->get_remaining_queue_size(req_0)<4){
    			stall_due_to_ramulator_read_queue_full++;
			set_req_stall();
			return false; 
		}	

		uint64_t level0_wc_block_addr = morph_tree.GetCounterAddress(0, physical_page_addr);
		ramulator::Request req_data(pkt->getAddr(), ramulator::Request::Type::READ, read_cb_func, 0);
		if((pkt->getAddr()%4096)==2048)
                        req_data.relocated_addr = morph_to_physical(level0_wc_block_addr);
		bool accepted = wrapper->send(req_data);	
		if(debug_memory_request){
			printf("sending request for data %lu at time %lu\n", pkt->getAddr(), curTick());
		}

		if (accepted){
			DPRINTF(Ramulator, "Read to %ld accepted.\n", req_data.addr);
			++requestsInFlight;
		} else {
			stall_due_to_ramulator_read_queue_full++;
			//printf("need more space for read queue, data block request stall\n");
			printf("aborting due to failing to send the request for normal data read\n");
			req_stall = true;
			abort();
		}
			
		bool _is_L0_miss=false;
		bool _is_L1_miss=false;
		int L0_ret=miss;
		int L1_ret=miss;
		while((!is_wccache_hit)&&(level<3)){
			uint64_t wc_block_addr = morph_tree.GetCounterAddress(level, physical_page_addr);
			is_wccache_hit = access_metadata_cache(false, level, wc_block_addr, is_hard_prefetch);
			if(level==0)
				L0_ret=is_wccache_hit;
			if(level==1)
				L1_ret=is_wccache_hit;


			counters_access_stats[level]++;
			counters_access_read_stats[level]++;
			if(level==1&&is_wccache_hit)
				L1_node_hit_stats++;
			if(level==1&&!is_wccache_hit)
				L1_node_miss_stats++;
			if(!is_wccache_hit){
				if(level==0)
					_is_L0_miss=true;
				if(level==1)
					_is_L1_miss=true;
				if(level>=1){
					std::map<uint64_t, metadata_block_in_flight_entry*>::iterator it;
					it = metadata_block_in_flight_map.find(morph_to_physical(wc_block_addr));
					if(it == metadata_block_in_flight_map.end()){
						metadata_block_in_flight_entry* new_entry=( metadata_block_in_flight_entry*)malloc(sizeof( metadata_block_in_flight_entry*));
						new_entry->dirty_tag=false;
						new_entry->insert_tag=false;
						new_entry->overflow_tag=false;
						metadata_block_in_flight_map.insert(std::pair<uint64_t, metadata_block_in_flight_entry*>(morph_to_physical(wc_block_addr),new_entry));
						ramulator::Request req(morph_to_physical(wc_block_addr), ramulator::Request::Type::READ, read_meta_func, 0, false, -1, false);
						 if(level==0)
                                                        req.relocated_addr = ((block_addr>>12)<<12)+2048+64*level;
						auto& in_flight_entry = *(metadata_block_in_flight_map.find(morph_to_physical(wc_block_addr))->second);
						in_flight_entry.insert_tag=true;
						bool accepted = wrapper->send(req);
						if (accepted){
							if(debug_metadata_response){
								printf("debugging metadata access: metadata %lu request for tree level: %d at tick: %lu\n", req.addr, level, curTick() );
							}
							DPRINTF(Ramulator, "Read to %ld accepted.\n", req.addr);
							total_num_metadata_requests++; 
							if(debug_metadata_request_and_response){
								printf("debugging metadata request and response for level %d, request for metadata: %lu , total metadata request: %lu , total metadata response: %lu\n", level, req.addr, total_num_metadata_requests,total_num_metadata_responses);
							}	
						}else{
							stall_due_to_ramulator_read_queue_full++;
							//printf("need more space fore read queue in ramulator, stall for level %d, pkt_addr %lu\n", level, req.addr);
							printf("aborting due to failing to send the request for metadata read  in level %d\n", level);
							req_stall = true;
							abort();
						}
					}
					else{
					auto& in_flight_entry = *(metadata_block_in_flight_map.find(morph_to_physical(wc_block_addr))->second);
					in_flight_entry.insert_tag=true;
					}
				}	
			}
			level++;
		}
		level=0;
		if(_is_L0_miss){
			uint64_t wc_block_addr = morph_tree.GetCounterAddress(level, physical_page_addr);
			std::map<uint64_t, metadata_block_in_flight_entry*>::iterator it;
			it = metadata_block_in_flight_map.find(morph_to_physical(wc_block_addr));
			if(it == metadata_block_in_flight_map.end()){
				metadata_block_in_flight_entry* new_entry=( metadata_block_in_flight_entry*)malloc(sizeof( metadata_block_in_flight_entry*));
				new_entry->dirty_tag=false;
				new_entry->insert_tag=false;
				new_entry->overflow_tag=false;
				metadata_block_in_flight_map.insert(std::pair<uint64_t, metadata_block_in_flight_entry*>(morph_to_physical(wc_block_addr),new_entry));
				ramulator::Request req(morph_to_physical(wc_block_addr), ramulator::Request::Type::READ, read_meta_func, 0, false, -1, false);
				req.relocated_addr = ((block_addr>>12)<<12)+2048+64*level;
				auto& in_flight_entry = *(metadata_block_in_flight_map.find(morph_to_physical(wc_block_addr))->second);
				in_flight_entry.insert_tag=true;
				bool accepted = wrapper->send(req);
				if(debug_memory_request){
					printf("sending request for metadata %lu at time %lu\n", morph_to_physical(wc_block_addr), curTick());
				}
				if (accepted){
					DPRINTF(Ramulator, "Read to %ld accepted.\n", req.addr);
					total_num_metadata_requests++;	
					if(debug_metadata_request_and_response){
						printf("debugging metadata request and response for level 0 read, request for metadata: %lu , total metadata request: %lu , total metadata response: %lu\n", req.addr, total_num_metadata_requests,total_num_metadata_responses);
					}		
				}
				else{
					stall_due_to_ramulator_read_queue_full++;
					req_stall = true;
					printf("aborting due to failing to send the request for metadata read  in level 0\n");
					abort();
				}
			}
			else{
				auto& in_flight_entry = *(metadata_block_in_flight_map.find(morph_to_physical(wc_block_addr))->second);
				in_flight_entry.insert_tag=true;
			}
		}
		//uint64_t level0_wc_block_addr = morph_tree.GetCounterAddress(0, physical_page_addr);
		/*ramulator::Request req_data(pkt->getAddr(), ramulator::Request::Type::READ, read_cb_func, 0);
		if((pkt->getAddr()%4096)==2048)
                        req_data.relocated_addr = morph_to_physical(level0_wc_block_addr);
		read_data_blk_info data_blk_info(pkt, curTick(), false, _is_L1_miss, _is_L0_miss);
		read_info_map[pkt->getAddr()].push_back(data_blk_info);
		bool accepted = wrapper->send(req_data);	
		if(debug_memory_request){
			printf("sending request for data %lu at time %lu\n", pkt->getAddr(), curTick());
		}

		if (accepted){
			DPRINTF(Ramulator, "Read to %ld accepted.\n", req_data.addr);
			++requestsInFlight;
		} else {
			stall_due_to_ramulator_read_queue_full++;
			//printf("need more space for read queue, data block request stall\n");
			req_stall = true;
			abort();
		}
		*/	
		read_data_blk_info data_blk_info(pkt, curTick(), false, _is_L1_miss, _is_L0_miss);
		if(L0_ret==llc_hit){
			if(caching_version_in_l2)
				data_blk_info.L0_node_response_time = curTick() -noc_latency;
			else
				data_blk_info.L0_node_response_time = curTick() + llc_metadata_access_latency;
			data_blk_info.begin_time_of_otp_calculation = curTick() + llc_metadata_access_latency;
		}	
		if(L1_ret==llc_hit){
			data_blk_info.L1_node_response_time = curTick() + llc_metadata_access_latency;
			data_blk_info.begin_time_of_otp_calculation = curTick() + llc_metadata_access_latency;
		}
		
		read_info_map[pkt->getAddr()].push_back(data_blk_info);
		//read_data_blk_info data_blk_info(pkt, curTick(), false, _is_L1_miss, _is_L0_miss);
		//read_info_map[pkt->getAddr()].push_back(data_blk_info);
		return accepted;
	}
	return true;
}
void Ramulator::send_write_overhead_requests_detailed(){
	ramulator::Request req_0(0, ramulator::Request::Type::READ, read_cb_func, 0);
	/*int queue_limit=MAX_WRITE_OVERHEAD_QUEUE_SIZE;
	if(begin_tick==0)
		queue_limit=0;	
	*/
	//if((write_overhead_requests_queue.size()>queue_limit)||(num_cycles_%write_overhead_dequeue_interval==0)){
	if(num_cycles_%write_overhead_dequeue_interval==0){
	//	do{	
			write_overhead_requests_entry request_entry = *(write_overhead_requests_queue.front());
			uint64_t addr = request_entry.block_address;
			int request_type = request_entry.request_type;
			if((request_type==write_back_of_metadata_block)||(request_type==write_back_for_overflow)){
				ramulator::Request req(addr, ramulator::Request::Type::WRITE, write_cb_func, 0);
				//if(request_type==write_back_for_overflow)
				//	req.is_overflow=true;
				if(request_type==write_back_for_overflow){
					if((request_type==write_back_for_overflow)&&(wrapper->get_overflow_queue_size(req)>=256*threshold_for_enqueuing_overflow_requests))
						return;
					req.is_overflow=true;
					num_overflow_read_in_flight++;
				}
				bool accepted = wrapper->send(req);
				if (accepted){
					metabytesWritten+=64;	
					if((request_type==write_back_of_metadata_block)){
						uint64_t written_wc_block_addr=physical_to_morph(req.addr);
						uint64_t tree_level;
						tree_level=morph_tree.GetCurrentLevel(written_wc_block_addr);
						if(tree_level==0)
							metabytesWrittenlevel0+=64;	
						if(tree_level==1)
							metabytesWrittenlevel1+=64;
						if(tree_level==2)
							metabytesWrittenlevel2+=64;
						if(tree_level==3)
							metabytesWrittenlevel3+=64;
					}
					else{
						metabytesWrittenoverflow+=64;
						if(req.addr>(Addr(0x1)<<37))
                                                        metabytesWrittenoverflowfordata+=64;
                                                else{
                                                        uint64_t written_tree_level;
                                                        uint64_t written_wc_block_addr=physical_to_morph(req.addr);
                                                        written_tree_level=morph_tree.GetCurrentLevel(written_wc_block_addr);
                                                        if(written_tree_level==0)
                                                                metabytesWrittenoverflowformeta_0+=64;
                                                }
					}
					free(write_overhead_requests_queue.front());
					write_overhead_requests_queue.pop_front();
				}
				else{
					printf("write overhead request denied for write back of metadata block\n");
					//break;
					return;
					//exit(0);
				}
			}
			else{
				//if((request_type==fetch_due_to_overflow)&&(num_overflow_read_in_flight>=256*threshold_for_enqueuing_overflow_requests))
				 if((request_type==fetch_due_to_overflow)&&(wrapper->get_overflow_queue_size(req_0)>=256*threshold_for_enqueuing_overflow_requests))
					return;
				std::map<uint64_t, metadata_block_in_flight_entry*>::iterator it;
				it = metadata_block_in_flight_map.find(addr);
				if(it == metadata_block_in_flight_map.end()){	
					ramulator::Request req(addr, ramulator::Request::Type::READ, read_meta_func, 0, false, -1, true);
					metadata_block_in_flight_entry* new_entry=(metadata_block_in_flight_entry*)malloc(sizeof(metadata_block_in_flight_entry));
					new_entry->dirty_tag=false;
					new_entry->insert_tag=false;
					new_entry->overflow_tag=false;
					metadata_block_in_flight_map.insert(std::pair<uint64_t, metadata_block_in_flight_entry*>(addr,new_entry));
					auto& in_flight_entry = *(metadata_block_in_flight_map.find(addr)->second);
					if(request_type==fetch_due_to_normal_increment){
						in_flight_entry.dirty_tag=true;
						in_flight_entry.insert_tag=true;
					}
					else if(request_type==fetch_due_to_overflow){
						in_flight_entry.overflow_tag=true;
						req.is_overflow=true;
					}
					bool accepted = wrapper->send(req);	
					if(accepted){
						total_num_metadata_requests++;
						if(debug_metadata_request_and_response){
							//printf("debugging metadata request and response, request for metadata: %lu , total metadata request: %lu , total metadata response: %lu\n", req.addr, total_num_metadata_requests,total_num_metadata_responses);
						}	
						if(in_flight_entry.overflow_tag==true){
							num_overflow_read_in_flight++;
							if(debug_overflow_in_flight){
								printf("debugging overflow, place 3: incrementing counter to %lu for %lu at tick %lu\n", num_overflow_read_in_flight, req.addr, curTick());
							}

						}
						/*	if(debug_metadata_response){
							uint64_t tree_level;
							tree_level=morph_tree.GetCurrentLevel(physical_to_morph(req.addr));
							printf("debugging metadata access: metadata %lu request for tree level: %lu at tick: %lu\n", req.addr, tree_level, curTick() );
						}
						*/

						/*
						metadata_block_in_flight_entry* new_entry=(metadata_block_in_flight_entry*)malloc(sizeof(metadata_block_in_flight_entry));
						new_entry->dirty_tag=false;
						new_entry->insert_tag=false;
						new_entry->overflow_tag=false;
						metadata_block_in_flight_map.insert(std::pair<uint64_t, metadata_block_in_flight_entry*>(addr,new_entry));
						*/
					}
					else{
						metadata_block_in_flight_map.erase(addr);
						printf("write overhead request denied for requst type%d\n", request_type);
						//break;
						return;
						//exit(0);
					}
				}
				else{
					auto& in_flight_entry = *(metadata_block_in_flight_map.find(addr)->second);
					if(request_type==fetch_due_to_normal_increment){
						in_flight_entry.dirty_tag=true;
						in_flight_entry.insert_tag=true;
					}
					else if(request_type==fetch_due_to_overflow){
						if(!in_flight_entry.overflow_tag){
						
							num_overflow_read_in_flight++;
							if(debug_overflow_in_flight){
								printf("debugging overflow, place 4: incrementing counter to %lu for %lu at tick %lu\n", num_overflow_read_in_flight, addr, curTick());
							}

						}
						in_flight_entry.overflow_tag=true;
					}
				}
				free(write_overhead_requests_queue.front());
				write_overhead_requests_queue.pop_front();
			}
			//	write_overhead_requests_queue.pop_front();
	//	}
	//	while(write_overhead_requests_queue.size()>queue_limit);
	}
	//while((write_overhead_requests_queue.size()>0)&&(wrapper->get_remaining_queue_size(req_0)>256));
}
//xinw added-end
bool Ramulator::recvTimingReq(PacketPtr pkt) {
    assert(!req_stall);
    for (PacketPtr pendPkt: pending_del)
        delete pendPkt;
    pending_del.clear(); 
    if (begin_tick == 0) {
        wrapper->set_simulation_mode(1);

        begin_tick = curTick();
    }
   if (pkt->cacheResponding()) {
        // snooper will supply based on copy of packet
        // still target's responsibility to delete packet
        pending_del.push_back(pkt);
        return true;
    }
    bool accepted = true; 
    // daz3
    if (curTick()<=(begin_tick + warmuptime)) {
        accessAndRespond(pkt, 0, curTick());
	//xinw added for morphable-begin
	if(memory_encryption){
		access_metadata_atomic(pkt);
	}
	//xinw added for morphable-end
          //xinw added for prefetchers-begin
        if(!lastRealWindow && curTick() >= (begin_tick + 0.90f * (float)warmuptime)){
            lastRealWindow = true;
            std::vector<void *> dcaches = system()->dcaches;
            for(size_t i = 0; i < dcaches.size(); ++i)
              {
                double unusedPrefetches = system()->unusedPrefetches[dcaches[i]];
                double sentPrefetches = system()->sentPrefetches[dcaches[i]];
                double prefetcherAccuracy = (double)1 - ((double)unusedPrefetches / sentPrefetches);
                std::cout << "dcache " << i << " prefetcherAccuracy: " << prefetcherAccuracy;
                if(prefetcherAccuracy < 0.94f){
                    BaseCache *cachePtr = static_cast<BaseCache *>(dcaches[i]);
                    cachePtr->turnOffSecondPrefetcher();
                    std::cout << ", TURNING OFF.";
                  }
                std::cout << std::endl;
              }
            std::vector<void *> l2caches = system()->l2caches;
            for(size_t i = 0; i < l2caches.size(); ++i){
                double unusedPrefetches = system()->unusedPrefetches[l2caches[i]];
                double sentPrefetches = system()->sentPrefetches[l2caches[i]];
                double prefetcherAccuracy = (double)1 - ((double)unusedPrefetches / sentPrefetches);
                std::cout << "L2 " << i << " prefetcherAccuracy: " << prefetcherAccuracy;
                if(prefetcherAccuracy < 0.94f){
                    BaseCache *cachePtr = static_cast<BaseCache *>(l2caches[i]);
                    cachePtr->turnOffSecondPrefetcher();
                    std::cout << ", TURNING OFF.";
                  }
                std::cout << std::endl;
              }
          }
        //xinw added for prefetchers-end
        return accepted;
    }
    if((memory_encryption)&&((pkt->isRead())||(pkt->isWrite()))){
	//current_possibility = rand()*1.0/RAND_MAX;
	if(debug_memory_access){
		if(pkt->isRead())
			printf("memory access, read for block: %lu\n", pkt->getAddr());
		else
			printf("memory access, write for block: %lu\n", pkt->getAddr());
			
	}
	//gem5_total_data_access_counter++;
	if(debug_metadata_eviction){
		if(pkt->isRead())
			printf("memory read, total data access: %lu , block address: %lu , at tick: %lu\n", gem5_total_data_access_counter, pkt->getAddr(), curTick());
		else
			printf("memory write, total data access: %lu , block address: %lu , at tick: %lu\n", gem5_total_data_access_counter, pkt->getAddr(), curTick());
			
	}

//	if(current_possibility<pintool_evicted_metadata_traffic_normalized_to_data_traffic){
	if(gem5_evicted_metadata_num<pintool_evicted_metadata_traffic_normalized_to_data_traffic*gem5_total_data_access_counter){
			if(debug_metadata_eviction){
				printf("metadata eviction: actual evicted metadata: %lu , total data access: %lu , at tick: %lu\n", gem5_evicted_metadata_num, gem5_total_data_access_counter, curTick());
			}
			gem5_evicted_metadata_num++;
			uint64_t evict_addr=rand_metadata_addr();
			ramulator::Request req_evict(morph_to_physical(evict_addr), ramulator::Request::Type::WRITE, write_cb_func, 0);
			bool accepted = wrapper->send(req_evict);	
			if (accepted){
				metabytesWritten+=64;	
				if(req_evict.addr<(Addr(0x1)<<32)){
					uint64_t written_wc_block_addr=physical_to_morph(req_evict.addr);
					uint64_t tree_level;
					tree_level=morph_tree.GetCurrentLevel(written_wc_block_addr);
					if(tree_level==0)
						metabytesWrittenlevel0+=64;	
					if(tree_level==1)
						metabytesWrittenlevel1+=64;
					if(tree_level==2)
						metabytesWrittenlevel2+=64;
					if(tree_level==3)
						metabytesWrittenlevel3+=64;
				}
				DPRINTF(Ramulator, "Write to %ld accepted and served.\n", req_evict.addr);
			} else {
				write_overhead_requests_entry *new_entry=(write_overhead_requests_entry *)malloc(sizeof(write_overhead_requests_entry));
				//new_entry->block_address = morph_to_physical(morph_to_physical(evict_addr));
				new_entry->block_address = morph_to_physical(evict_addr);
				new_entry->request_type = write_back_of_metadata_block;
				if(no_duplication_in_write_overhead_queue(new_entry->block_address, new_entry->request_type))
					write_overhead_requests_queue.push_back(new_entry);
				else
					free(new_entry);
			}
	}
    }
    if (pkt->isRead()) {
	    //xinw added-begin
	    //printf("receive one request: %lu at tick: %lu\n", pkt->getAddr(), curTick());
	    if(memory_encryption)
		    return send_requests_detailed(pkt);
	    else{
		    ramulator::Request req(pkt->getAddr(), ramulator::Request::Type::READ, read_cb_func, 0);
		    accepted = wrapper->send(req);
		    if (accepted){
			    //xinw added for morphable-begin
			    DPRINTF(Ramulator, "Read to %ld accepted.\n", req.addr);
			    // added counter to track requests in flight
			    ++requestsInFlight;
			    // daz3
			    my_read_cnt++;
			    my_total_cnt++;	 
			

			    read_data_blk_info data_blk_info(pkt, 0, false, false, false);
			    read_info_map[req.addr].push_back(data_blk_info);


		    } else {
			    printf("data block request stall\n");
			    req_stall = true;
		    }
		     //xinw moved this above: only create data_blk_info when request is sent successfully.
		    /*
		    read_data_blk_info data_blk_info(pkt, 0, false, false, false);
		    read_info_map[req.addr].push_back(data_blk_info);
		    */
	    }
	    //xinw added-end		
    } else if (pkt->isWrite()) {
       	//xinw added for morphable-begin
	if(memory_encryption){
	     return send_requests_detailed(pkt);
	}
	else{
        	ramulator::Request req(pkt->getAddr(), ramulator::Request::Type::WRITE, write_cb_func, 0);
		accepted = wrapper->send(req);
		if (accepted){
			accessAndRespond(pkt, 0, curTick());
			DPRINTF(Ramulator, "Write to %ld accepted and served.\n", req.addr);
			// added counter to track requests in flight
			++requestsInFlight;
			// daz3
			my_write_cnt++;
			my_total_cnt++;
		} else {
			req_stall = true;
		}
	}
	//xinw added for morphable-end
    } else {
        accessAndRespond(pkt, 0, curTick());
        my_total_cnt++;
    }
    return accepted;
}
void Ramulator::recvRespRetry() {
    DPRINTF(Ramulator, "Retrying\n");
    assert(resp_stall);
    resp_stall = false;
    if(memory_encryption){
	sendDelayResponse();
    }
    else
    	sendResponse();
}
bool Ramulator::check_give_up_aes(Addr block_addr){
   std::vector<void *> l2caches = system()->l2caches;
   for(size_t i = 0; i < l2caches.size(); ++i){
   	BaseCache *cachePtr = static_cast<BaseCache *>(l2caches[i]);
   	if(cachePtr->check_give_up_aes(block_addr))
		return true;                
   }
   return false;
}
bool Ramulator::check_llc_aes(Addr block_addr){
   std::vector<void *> l3cache = system()->l3cache;
   BaseCache *cachePtr = static_cast<BaseCache *>(l3cache[0]);
   return cachePtr->check_llc_aes(block_addr);                 
}
Tick Ramulator::llc_aes_finish_time(Addr block_addr){
   std::vector<void *> l3cache = system()->l3cache;
   BaseCache *cachePtr = static_cast<BaseCache *>(l3cache[0]);
   return cachePtr->llc_aes_finish_time(block_addr);                 
}

 

void Ramulator::accessAndRespond(PacketPtr pkt, Tick otp_begin_tick, Tick block_arrive_time) {
    //xinw added for morphable-begin
    last_access_tick = curTick()-begin_tick;
    _last_access_tick = last_access_tick;
    //xinw added for morphable-end
    //xinw added
    if(!begin_tick)
	return;
    bool need_resp = pkt->needsResponse();
    access(pkt);
    if (need_resp) {
        assert(pkt->isResponse());
        pkt->headerDelay = pkt->payloadDelay = 0;

        DPRINTF(Ramulator, "Queuing response for address %lld\n",
                pkt->getAddr());
	
	if(memory_encryption){
		delay_resp_element _resp;
		_resp.pkt=pkt;
		
		Tick schedule_time=curTick();
		if(pkt->isRead()){
			//if((block_arrive_time+mac_latency)>schedule_time)
			//	schedule_time=block_arrive_time+mac_latency;
		}
		_resp.resp_time=schedule_time;
		delay_resp_queue.push_back(_resp);
		if(debug_verification)
			printf("curTick: %lu , otp_begin_tick: %lu , block_arrive_time: %lu , scheduled_response_time: %lu\n", curTick(), otp_begin_tick, block_arrive_time, schedule_time);
	}
	else{
		resp_queue.push_back(pkt);
		if (!resp_stall && !send_resp_event.scheduled()){
			schedule(send_resp_event, curTick());
		}
	}
  } else 
        pending_del.push_back(pkt);
}


void Ramulator::accessAndRespondBaseline(PacketPtr pkt, bool is_L0_block_miss, Tick info_create_time, Tick block_arrive_time, Tick L0_arrive_time, Tick L1_arrive_time, Tick L2_arrive_time){ 
  last_access_tick = curTick()-begin_tick;
    _last_access_tick = last_access_tick;
    if(!begin_tick)
	return;
    bool need_resp = pkt->needsResponse();
    access(pkt);
    if (need_resp) {
        assert(pkt->isResponse());
        pkt->headerDelay = pkt->payloadDelay = 0;
        DPRINTF(Ramulator, "Queuing response for address %lld\n",
                pkt->getAddr());
	if(memory_encryption){
		if(check_give_up_aes(pkt->getAddr()))
		   num_given_up_aes_in_l2++;
		  if(is_L0_block_miss){
			
			delay_resp_element _resp;
			_resp.pkt=pkt;
			_resp.resp_time=curTick();
			if((block_arrive_time+mac_latency)>_resp.resp_time)	
				_resp.resp_time=block_arrive_time+mac_latency;
			if(OTP_PRECALCULATION){
				if((L0_arrive_time+data_aes_latency-carry_less_multiplication_latency)>_resp.resp_time)
					_resp.resp_time=L0_arrive_time+data_aes_latency-carry_less_multiplication_latency;
			}
			else{
				if((L0_arrive_time+data_aes_latency)>_resp.resp_time)
					_resp.resp_time=L0_arrive_time+data_aes_latency;
			}
			if((L0_arrive_time+mac_latency)>_resp.resp_time)
				_resp.resp_time=L0_arrive_time+mac_latency;
			if((L1_arrive_time+tree_node_aes_latency)>_resp.resp_time)
				_resp.resp_time=L1_arrive_time+tree_node_aes_latency;
			if((L2_arrive_time+tree_node_aes_latency)>_resp.resp_time)
				_resp.resp_time=L2_arrive_time+tree_node_aes_latency;
			if(OTP_PRECALCULATION)
				_resp.resp_time+=carry_less_multiplication_latency;
	
			if((check_give_up_aes(pkt->getAddr()))&&caching_version_in_l2&&adaptive_l2_aes){
				if(check_llc_aes(pkt->getAddr())){
					if((llc_aes_finish_time(pkt->getAddr()))>_resp.resp_time)
						_resp.resp_time=llc_aes_finish_time(pkt->getAddr());
				}
				else{
					if(info_create_time+data_aes_latency>_resp.resp_time)
						_resp.resp_time=info_create_time+data_aes_latency;
				}
			}
		delay_resp_queue.push_back(_resp);
		}
		else{
			delay_resp_element _resp;
			_resp.pkt=pkt;
			_resp.resp_time=curTick();
			if((block_arrive_time+mac_latency)>_resp.resp_time)
				_resp.resp_time=block_arrive_time+mac_latency;
			//xinw changed for caching metadata in LLC.
			if((L0_arrive_time+data_aes_latency)>_resp.resp_time)
				_resp.resp_time=L0_arrive_time+data_aes_latency;	
			if((check_give_up_aes(pkt->getAddr()))&&caching_version_in_l2&&adaptive_l2_aes){
				if(check_llc_aes(pkt->getAddr())){
					if((llc_aes_finish_time(pkt->getAddr()))>_resp.resp_time)
						_resp.resp_time=llc_aes_finish_time(pkt->getAddr())+data_aes_latency;
				}
				else{
					if(info_create_time+data_aes_latency>_resp.resp_time)
						_resp.resp_time=info_create_time+data_aes_latency;
				}
			}
			delay_resp_queue.push_back(_resp);
		}
	}
	else{
		resp_queue.push_back(pkt);
		if (!resp_stall && !send_resp_event.scheduled()){
		
				schedule(send_resp_event, curTick());
		}  
	}
  } else 
        pending_del.push_back(pkt);
}
void Ramulator::accessAndRespondDccm(PacketPtr pkt, bool is_L0_block_miss, Tick info_create_time, Tick block_arrive_time, Tick L0_arrive_time, Tick L1_arrive_time, Tick L2_arrive_time){ 
  last_access_tick = curTick()-begin_tick;
    _last_access_tick = last_access_tick;
    if(!begin_tick)
	return;
    bool need_resp = pkt->needsResponse();
    access(pkt);
    if (need_resp) {
        assert(pkt->isResponse());
        pkt->headerDelay = pkt->payloadDelay = 0;
        DPRINTF(Ramulator, "Queuing response for address %lld\n",
                pkt->getAddr());
	if(memory_encryption){
		  if(check_give_up_aes(pkt->getAddr()))
		     num_given_up_aes_in_l2++;
		 
		  if(is_L0_block_miss){
			
			delay_resp_element _resp;
			_resp.pkt=pkt;
			_resp.resp_time=curTick();
			if((block_arrive_time+mac_latency)>_resp.resp_time)	
				_resp.resp_time=block_arrive_time+mac_latency;
			if((L0_arrive_time+mac_latency)>_resp.resp_time)
				_resp.resp_time=L0_arrive_time+mac_latency;
			if(OTP_L1_PRECALCULATION&&access_l1_table()){
				if((L1_arrive_time)>_resp.resp_time)
					_resp.resp_time=L1_arrive_time;
			}	
			else{
				if((L1_arrive_time+tree_node_aes_latency)>_resp.resp_time)
					_resp.resp_time=L1_arrive_time+tree_node_aes_latency;
			}
			if((L2_arrive_time+tree_node_aes_latency)>_resp.resp_time)
				_resp.resp_time=L2_arrive_time+tree_node_aes_latency;
			if(OTP_PRECALCULATION)
				_resp.resp_time+=carry_less_multiplication_latency;
			/*if((check_give_up_aes(pkt->getAddr()))&&caching_version_in_l2&&adaptive_l2_aes){
				if(check_llc_aes(pkt->getAddr())){
					if((llc_aes_finish_time(pkt->getAddr()))>_resp.resp_time)
						_resp.resp_time=llc_aes_finish_time(pkt->getAddr())+data_aes_latency;
				}
				else{
					if(info_create_time+data_aes_latency>_resp.resp_time)
						_resp.resp_time=info_create_time+data_aes_latency;
				}
			}*/	
			if(info_create_time+data_aes_latency>_resp.resp_time)
				_resp.resp_time=info_create_time+data_aes_latency;
				

			delay_resp_queue.push_back(_resp);
		}
		else{
			delay_resp_element _resp;
			_resp.pkt=pkt;
			_resp.resp_time=curTick();
			if((block_arrive_time+mac_latency)>_resp.resp_time)
				_resp.resp_time=block_arrive_time+mac_latency;
			if((L0_arrive_time)>_resp.resp_time)
				_resp.resp_time=L0_arrive_time;
			/* 
			if((check_give_up_aes(pkt->getAddr()))&&caching_version_in_l2&&adaptive_l2_aes){
				if(check_llc_aes(pkt->getAddr())){
					if((llc_aes_finish_time(pkt->getAddr()))>_resp.resp_time)
						_resp.resp_time=llc_aes_finish_time(pkt->getAddr())+data_aes_latency;
				}
				else{
					if(info_create_time+data_aes_latency>_resp.resp_time)
						_resp.resp_time=info_create_time+data_aes_latency;
				}
			}
			*/		
			if(info_create_time+data_aes_latency>_resp.resp_time)
				_resp.resp_time=info_create_time+data_aes_latency;
			

			delay_resp_queue.push_back(_resp);
		}
	}
	else{
		resp_queue.push_back(pkt);
		if (!resp_stall && !send_resp_event.scheduled()){
		
				schedule(send_resp_event, curTick());
		}  
	}
  } else 
        pending_del.push_back(pkt);
}
//xinw modified for morphable-begin
void Ramulator::readComplete(ramulator::Request& req){
    //xinw added for using trace
    mem_reads_0++;
    total_mem_access_0++;
    if(is_normal_data(req.addr))
	gem5_total_data_access_counter++;
    DPRINTF(Ramulator, "Read to %ld completed.\n", req.addr);
    if(memory_encryption){ 
	    if(debug_memory_request){
		    printf("receiving response for data %lu at time %lu\n", req.addr, curTick());
	    }



	    uint64_t physical_page_addr = morph_tree.GetPhysicalPageAddress(req.addr);
	    uint64_t wc_block_addr = morph_tree.GetCounterAddress(0, physical_page_addr);
	    uint64_t level1_wc_block_addr = morph_tree.GetCounterAddress(1, physical_page_addr);
	    auto& _read_data_blk_info_queue = read_info_map.find(req.addr)->second;	
	    read_data_blk_info _info = _read_data_blk_info_queue.front();	
    	    dram_latency_for_data+=curTick()- _info.info_create_time;
	    
	    dram_read_for_data++;
	    bool _version_ready=((!_info.is_L0_block_miss)||(_info.version_ready));
	    bool _L1_ready=((!_info.is_L1_block_miss)||(_info.L1_ready));
	    bool counter_ready = ((!wait_counter)||(_version_ready&&_L1_ready));
	    bool available_schedule= (!resp_stall && !send_resp_event.scheduled());
	    //if(counter_ready){	
	    if(counter_ready&&available_schedule){	
		    auto& read_data_blk_info_queue = read_info_map.find(req.addr)->second;	
		    read_data_blk_info info = read_data_blk_info_queue.front();	
		    info.block_arrive_time=curTick();
		    PacketPtr data_pkt = info.PktPtr;
		    //Tick otp_begin_tick = info.begin_time_of_otp_calculation;
		    Tick _info_create_time = info.info_create_time;
		    Tick _block_arrive_time=curTick();
		    //bool _is_L1_block_miss = info.is_L1_block_miss;
		    bool _is_L0_block_miss = info.is_L0_block_miss;
		    Tick _L0_node_response_time = info.L0_node_response_time;
		    Tick _L1_node_response_time = info.L1_node_response_time;
		    Tick _L2_node_response_time = info.L2_node_response_time;
		    read_data_blk_info_queue.pop_front();
		    if (!read_data_blk_info_queue.size())
			    read_info_map.erase(req.addr);
		    // added counter to track requests in flight
		    --requestsInFlight;
			if(OTP_PRECALCULATION){
			    	current_table_index=data_pkt->req->contextId();  
				//current_possibility = rand()*1.0/RAND_MAX;
				bool table_hit=access_aes_table();
				if(table_hit){
					accessAndRespondDccm(data_pkt, _is_L0_block_miss, _info_create_time, _block_arrive_time, _L0_node_response_time, _L1_node_response_time, _L2_node_response_time);
				}
				else{
					accessAndRespondBaseline(data_pkt, _is_L0_block_miss, _info_create_time, _block_arrive_time, _L0_node_response_time, _L1_node_response_time, _L2_node_response_time);
				}
				}
			//xinw added for otp precomputation-end
			else{
					accessAndRespondBaseline(data_pkt, _is_L0_block_miss, _info_create_time, _block_arrive_time, _L0_node_response_time, _L1_node_response_time, _L2_node_response_time);
			}
	    }
	    else{
		   //if(!is_wccache_hit)
		   if(!_version_ready){
		    std::map<uint64_t,metadata_block_in_flight_entry*>::iterator it;
		    it = metadata_block_in_flight_map.find(morph_to_physical(wc_block_addr));
		    if(it == metadata_block_in_flight_map.end()){
			    version_block_unnormal_miss++;
			    version_block_miss_not_expected = version_block_unnormal_miss*1.0/number_of_reads;
			    metadata_block_in_flight_entry* new_entry=(metadata_block_in_flight_entry*)malloc(sizeof(metadata_block_in_flight_entry));
			    new_entry->insert_tag=false;
			    new_entry->overflow_tag=false;
			    new_entry->dirty_tag=false;
			    metadata_block_in_flight_map.insert(std::pair<uint64_t, metadata_block_in_flight_entry*>(morph_to_physical(wc_block_addr),new_entry));
			    auto& in_flight_entry = *(metadata_block_in_flight_map.find(morph_to_physical(wc_block_addr))->second);
			    in_flight_entry.insert_tag=true;
			    ramulator::Request req(morph_to_physical(wc_block_addr), ramulator::Request::Type::READ, read_meta_func, 0, false, 0, false);
			    bool accepted = wrapper->send(req);
			    if (accepted){
				    DPRINTF(Ramulator, "Read to %ld accepted.\n", req.addr);
				    total_num_metadata_requests++;
				    if(debug_metadata_request_and_response){
					    printf("debugging metadata request and response for level 0, request for metadata: %lu , total metadata request: %lu , total metadata response: %lu\n", req.addr, total_num_metadata_requests,total_num_metadata_responses);
				    }	
			    }
			    else{
    				    stall_due_to_ramulator_read_queue_full++;
				    req_stall = true;
				    printf("aborting due to failing to send the request for retrying metadata read  in level 0\n");
				    abort();
			    }
		    }
		    }
		   //if(!_version_ready){
		   if(!_L1_ready){
			   std::map<uint64_t,metadata_block_in_flight_entry*>::iterator it;
			   it = metadata_block_in_flight_map.find(morph_to_physical(level1_wc_block_addr));
			   if(it == metadata_block_in_flight_map.end()){
				   metadata_block_in_flight_entry* new_entry=(metadata_block_in_flight_entry*)malloc(sizeof(metadata_block_in_flight_entry));
				   new_entry->insert_tag=false;
				   new_entry->overflow_tag=false;
				   new_entry->dirty_tag=false;
				   metadata_block_in_flight_map.insert(std::pair<uint64_t, metadata_block_in_flight_entry*>(morph_to_physical(level1_wc_block_addr),new_entry));
				   auto& in_flight_entry = *(metadata_block_in_flight_map.find(morph_to_physical(level1_wc_block_addr))->second);
				   in_flight_entry.insert_tag=true;

				   ramulator::Request req(morph_to_physical(level1_wc_block_addr), ramulator::Request::Type::READ, read_meta_func, 0, false, 0, false);
				   bool accepted = wrapper->send(req);
				   if (accepted){
					   DPRINTF(Ramulator, "Read to %ld accepted.\n", req.addr);
					   total_num_metadata_requests++;
					   if(debug_metadata_request_and_response){
						   printf("debugging metadata request and response for level 1, request for metadata: %lu , total metadata request: %lu , total metadata response: %lu\n", req.addr, total_num_metadata_requests,total_num_metadata_responses);
					   }	
				   }
				   else{
					   stall_due_to_ramulator_read_queue_full++;
					   req_stall = true;
				    	   printf("aborting due to failing to send the request for retrying metadata read  in level 1\n");
					   abort();
				   }
			   }
		   }
		    auto& read_info= read_info_map.find(req.addr)->second;
		    read_info.front().data_block_has_arrived=true;
		    read_info.front().block_arrive_time=curTick();
	    }
    }
    else{
		auto& read_data_blk_info_queue = read_info_map.find(req.addr)->second;	
		    read_data_blk_info info = read_data_blk_info_queue.front();	
		    PacketPtr data_pkt = info.PktPtr;
		    read_data_blk_info_queue.pop_front();
		    if (!read_data_blk_info_queue.size())
			    read_info_map.erase(req.addr);
		    --requestsInFlight;
		    accessAndRespond(data_pkt, 0, curTick());
    }
}
//xinw modified for morphable-end
//xinw added for morphable-begin
void Ramulator::readMetaComplete(ramulator::Request& req){
	total_num_metadata_responses++;
	if(debug_metadata_request_and_response){
		//printf("debugging metadata request and response, response for metadata: %lu , total metadata request: %lu , total metadata response: %lu\n", req.addr, total_num_metadata_requests,total_num_metadata_responses);
	}	
	if(debug_memory_request){
		printf("receiving response for metadata %lu at time %lu\n", req.addr, curTick());
	}


	//xinw added for using trace
	mem_reads_0++;
	total_mem_access_0++;
	metabytesRead+=64;
	uint64_t wc_block_addr = physical_to_morph(req.addr);
	std::map<uint64_t,metadata_block_in_flight_entry*>::iterator it;
	it = metadata_block_in_flight_map.find(req.addr);
	if(it != metadata_block_in_flight_map.end()){
		auto& in_flight_entry = *(metadata_block_in_flight_map.find(req.addr)->second);
		bool overflow_tag=in_flight_entry.overflow_tag;
		bool insert_tag=in_flight_entry.insert_tag;
		if(overflow_tag){
			num_overflow_read_in_flight--;
			if(debug_overflow_in_flight){
				printf("debugging overflow, place 5: decrementing counter to %lu for %lu at tick %lu\n", num_overflow_read_in_flight, req.addr, curTick());
			}

		}
		if(overflow_tag&&(!insert_tag)){
			metabytesReadoverflow+=64;
			if(req.addr>(Addr(0x1)<<37))
				metabytesReadoverflowfordata+=64;
			else{
				uint64_t read_tree_level;
				read_tree_level=morph_tree.GetCurrentLevel(wc_block_addr);
				if(read_tree_level==0)
					metabytesReadoverflowformeta_0+=64;
			}
		}
		else{	

			uint64_t read_tree_level;
			read_tree_level=morph_tree.GetCurrentLevel(wc_block_addr);
			if(debug_metadata_response){
				printf("debugging metadata access: metadata %lu response for tree level: %lu at tick: %lu\n", req.addr, read_tree_level, curTick() );
				if(req.addr==3355453504){
					printf("arriving at breakpoint.\n");
				}
			}


			if(read_tree_level==0)
				metabytesReadlevel0+=64;	
			if(read_tree_level==1)
				metabytesReadlevel1+=64;
			if(read_tree_level==2)
				metabytesReadlevel2+=64;
			if(read_tree_level==3)
				metabytesReadlevel3+=64;
		}
		free(metadata_block_in_flight_map.find(req.addr)->second);
		metadata_block_in_flight_map.erase(req.addr);	
		if(overflow_tag&&!insert_tag){
			Addr overflow_addr=req.addr;
			/*ramulator::Request req_for_overflow(overflow_addr, ramulator::Request::Type::WRITE, write_cb_func, 0);
			req_for_overflow.is_overflow=true;
			bool accepted = wrapper->send(req_for_overflow);	
			if (accepted){
				metabytesWritten+=64;
				metabytesWrittenoverflow+=64;

				if(overflow_addr>(Addr(0x1)<<37))
					metabytesWrittenoverflowfordata+=64;
				else{
					uint64_t written_tree_level;
					written_tree_level=morph_tree.GetCurrentLevel(wc_block_addr);
					if(written_tree_level==0)
						metabytesWrittenoverflowformeta_0+=64;
				}
				DPRINTF(Ramulator, "Write to %ld accepted and served.\n", req.addr);
			} else {*/
				write_overhead_requests_entry *new_entry=(write_overhead_requests_entry *)malloc(sizeof(write_overhead_requests_entry));
				new_entry->block_address = overflow_addr;
				new_entry->request_type = write_back_for_overflow;
				if(no_duplication_in_write_overhead_queue(new_entry->block_address, new_entry->request_type))
					write_overhead_requests_queue.push_back(new_entry);
				else
					free(new_entry);
			//}
		}
		else{
		}
		std::map<long, std::deque<read_data_blk_info>>::iterator it = read_info_map.begin();
		while(it != read_info_map.end()){
			std::map<long, std::deque<read_data_blk_info>>::iterator data_blk_info = it++;
			Addr data_blk_addr=data_blk_info->first;
			uint64_t physical_page_addr = morph_tree.GetPhysicalPageAddress(data_blk_addr);
			uint64_t level0_wc_block_addr_for_data = morph_tree.GetCounterAddress(0, physical_page_addr);
			if(level0_wc_block_addr_for_data==wc_block_addr){
				(data_blk_info->second).front().begin_time_of_otp_calculation = curTick() +  llc_metadata_access_latency;		
				if(caching_version_in_l2)
					(data_blk_info->second).front().L0_node_response_time = curTick();
				else{
					if(caching_metadata_in_llc)
						(data_blk_info->second).front().L0_node_response_time = curTick() + llc_metadata_access_latency;
					else
						(data_blk_info->second).front().L0_node_response_time = curTick() + metadata_access_latency;
				}
				(data_blk_info->second).front().version_ready = true;		
    				dram_latency_for_version+=curTick()- (data_blk_info->second).front().info_create_time;
				//if((curTick()- (data_blk_info->second).front().info_create_time+mac_latency)>aes_latency)
				//	exceeded_dram_verfication_latency_for_version_longer_than_aes_latency+=(curTick()- (data_blk_info->second).front().info_create_time+mac_latency-aes_latency);
	    			dram_read_for_version++;
			}
			uint64_t level1_wc_block_addr_for_data = morph_tree.GetCounterAddress(1, physical_page_addr);
			if(level1_wc_block_addr_for_data==wc_block_addr){
				(data_blk_info->second).front().begin_time_of_otp_calculation = curTick() + llc_metadata_access_latency;
				if(caching_metadata_in_llc)
					(data_blk_info->second).front().L1_node_response_time = curTick() +  llc_metadata_access_latency;
				else
					(data_blk_info->second).front().L1_node_response_time = curTick() +  metadata_access_latency;
				(data_blk_info->second).front().L1_ready = true;		
			}	
			uint64_t level2_wc_block_addr_for_data = morph_tree.GetCounterAddress(2, physical_page_addr);
			if(level2_wc_block_addr_for_data==wc_block_addr){
				(data_blk_info->second).front().begin_time_of_otp_calculation = curTick()+ llc_metadata_access_latency;
				if(caching_metadata_in_llc)
					(data_blk_info->second).front().L2_node_response_time = curTick() +  llc_metadata_access_latency;
				else
					(data_blk_info->second).front().L2_node_response_time = curTick() +  metadata_access_latency;
			}

		}
	}
	else{
		printf("error, there should be an element in metadata_block_in_flight_map\n");
		abort();
	}
}
void Ramulator::writeOverheadComplete(ramulator::Request& req){
}
//xinw added for morphable-end
void Ramulator::writeComplete(ramulator::Request& req){
    //xinw added for using trace
    mem_writes_0++;
    total_mem_access_0++;
    if(is_normal_data(req.addr))
	gem5_total_data_access_counter++;
 
    DPRINTF(Ramulator, "Write to %ld completed.\n", req.addr);
    // added counter to track requests in flight
    --requestsInFlight;
    // check if we were asked to drain and if we are now done
    if (drain_manager && numOutstanding() == 0) {
        drain_manager->signalDrainDone();
        drain_manager = NULL;
    }
}
void Ramulator::traceComplete(ramulator::Request& req){
}
void Ramulator::set_req_stall(){
	if(curTick()>(atomic_begin_tick+atomicwarmuptime))
		req_stall=true;
}
Ramulator *RamulatorParams::create(){
    return new Ramulator(this);
}
