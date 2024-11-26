#ifndef __RAMULATOR_HH__
#define __RAMULATOR_HH__

#include <deque>
#include <tuple>
#include <map>

#include "mem/abstract_mem.hh"
#include "params/Ramulator.hh"
#include "Ramulator/src/Config.h"
//xinw added for morphable-begin
// Addr (Addr)3=3;
// Addr meta_high_gb=4;
//xinw added for morphable-end
namespace ramulator{
    class Request;
    class Gem5Wrapper;
}
//xinw added for otp precalculation-begin
#include <algorithm>
#include <unordered_map>
using namespace std;
//std::deque<uint64_t> overflow_fetch_queue;
#define custom_debug 0
#define custom_writeback_debug 0
#define custom_counter_update_debug 0
#define custom_dccm_debug 0
//#define times_of_16g 2
//#define times_of_16g 1
#define times_of_16g 8
#define FALSE 0
#define TRUE 1
//bool OTP_PRECALCULATION=false;
//#define BEGIN_WITH_BIG_COUNTER 1
//#define BEGIN_WITH_BIG_COUNTER 0
#define HIGH_WC_RATIO_THRESHOLD 0.1
//#define HIGH_WC_RATIO_THRESHOLD 0.0001
#define POSSIBILITY_FILTER 0.1
//#define POSSIBILITY_RELEVEL 1
#define DCCM_OVERHEAD_RATIO_AFTER_WARMUP 0.2
#define TABLE_SIZE 64
//#define TABLE_SIZE 16
//once modified in 2_28_otp32
#define TABLE_LINE_SIZE 2
//#define TABLE_LINE_SIZE 1
#define OTP_INTERVAL 1024
//#define OTP_INTERVAL 1024
#define OTP_ACTIVE_RELEVEL_SMALL_GROUP_NUMBER 2
#define OTP_ACTIVE_RELEVEL_BIG_GROUP_NUMBER (TABLE_SIZE-2)
#define occurence_epoch 10000
#define RELEVEL_FOR_SMALLER_VALUE_THAN_MIN 1
#define RELEVEL_FOR_VALUE_IN_SMALLEST 2
#define RELEVEL_FOR_VALUE_IN_MEDIATE 3

//uint64_t history_max_wc=0;
bool access_aes_table();
bool access_metadata_cache(uint32_t _tree_level);
#define PHYSICAL_PAGES_NUM  4194304*times_of_16g
class MorphCtrBlock {
  public:
    MorphCtrBlock();
    void InitBlock(bool is_random, uint32_t expected_level, uint32_t _node_index);
    uint64_t GetEffectiveCounter(const uint32_t& level, const uint64_t& pos);
    uint32_t GetCtrLevel();
    void SetCtrLevel(const uint32_t& level);
    //xinw added for overflow_fetch_stats-begin
    void SetSubNodeAddr(uint64_t sub_node_addr);
    void SetNodeIndex(uint64_t _index);
    void FetchForOverflow();
    //xinw added for overflow_fetch_stats-end
  private:
   uint32_t ctr_level_;
   uint64_t sub_node_begin_addr;
};
//xinw added
typedef struct DELAY_RESP_ELEMENT{
	PacketPtr pkt;
	Tick resp_time;
}delay_resp_element;
class MorphTree {
  public:
    MorphTree();
    void InitTree(bool is_random);
    uint64_t GetPhysicalPageAddress(const uint64_t& block_addr);
    uint64_t GetCounterAddress(const uint32_t& level, const uint64_t& physical_page_addr);
    uint32_t GetCurrentLevel(const uint64_t& wc_block_addr);
    uint64_t GetLevelCounterId(const uint32_t& level, const uint64_t& wc_block_addr);
    uint64_t GetParentCounterAddress(const uint32_t& level, const uint64_t& wc_block_addr);
    uint32_t will_overflow(const uint64_t& level, const uint64_t& counter_id, const uint64_t& pos, bool default_clean, bool is_relevel);
    uint64_t GetEffectiveCounter(const uint64_t& level, const uint64_t& counter_id, const uint64_t& pos);
  public:
    MorphCtrBlock tree_level3[times_of_16g];
    MorphCtrBlock tree_level2[128*times_of_16g];
    MorphCtrBlock tree_level1[16384*times_of_16g];
    MorphCtrBlock versions_level[2097152*times_of_16g];
    uint64_t rand_pages_map[PHYSICAL_PAGES_NUM];
    uint64_t gem5_to_morph_pages_map[PHYSICAL_PAGES_NUM*4];
    bool gem5_to_morph_pages_set_map[PHYSICAL_PAGES_NUM*4];
    uint64_t last_physical_page_addr;
    uint64_t last_physical_page_addr_debug;
};
class Ramulator : public AbstractMemory {
private:

    class MemoryPort: public SlavePort {
    private:
        Ramulator& mem;
    public:

        MemoryPort(const std::string& _name, Ramulator& _mem): SlavePort(_name, &_mem), mem(_mem) {}
    protected:
        Tick recvAtomic(PacketPtr pkt) {
            // modified to perform a fixed latency return for atomic packets to enable fast forwarding
            // assert(false && "only accepts functional or timing packets");
            return mem.recvAtomic(pkt);
        }
        
        void recvFunctional(PacketPtr pkt) {
            mem.recvFunctional(pkt);
        }

        bool recvTimingReq(PacketPtr pkt) {
            return mem.recvTimingReq(pkt);
        }

        void recvRespRetry() {
            mem.recvRespRetry();
        }

        AddrRangeList getAddrRanges() const {
            AddrRangeList ranges;
            ranges.push_back(mem.getAddrRange());
            return ranges;
        }
    } port;

    unsigned int requestsInFlight;
   
    std::map<long, std::deque<PacketPtr> > writes;
    std::deque<PacketPtr> resp_queue;
    //xinw added
    std::deque<delay_resp_element> delay_resp_queue;

    std::deque<PacketPtr> pending_del;
    DrainManager *drain_manager;

    std::string config_file;
    ramulator::Config configs;
    ramulator::Gem5Wrapper *wrapper;
    std::function<void(ramulator::Request&)> read_cb_func;
    //xinw added for morphable-begin
    std::function<void(ramulator::Request&)> read_meta_func;
    std::function<void(ramulator::Request&)> write_overhead_func;
    //xinw added for morphable-end
    std::function<void(ramulator::Request&)> write_cb_func;
    // xinw added for using trace
    std::function<void(ramulator::Request&)> trace_cb_func;
    Tick ticks_per_clk;
    bool resp_stall;
    bool req_stall;
    //xinw
    Tick atomicwarmuptime;
    // daz3
    Tick warmuptime;

    unsigned int numOutstanding() const { return requestsInFlight + resp_queue.size(); }
    
    void sendDelayResponse();
    void sendResponse();
    void tick();
    
    EventWrapper<Ramulator, &Ramulator::sendResponse> send_resp_event;
    EventWrapper<Ramulator, &Ramulator::tick> tick_event;
    //xinw added considering different range for controllers-begin
    static uint8_t* memdata_0;
    static Addr memdata_0_start, memdata_0_end;
    static uint8_t* memdata_1;
    static Addr memdata_1_start, memdata_1_end;
    static Addr gem5_mem_size;
    //xinw added considering different range for controllers-end
public:
		//xinw added for morphable-begin
//OtpTable otp_table;

    void access_metadata_atomic(PacketPtr _PktPtr);
    bool send_requests_detailed(PacketPtr _PktPtr);
    void send_write_overhead_requests_detailed();
    Tick last_access_tick;
    uint64_t cpt_index;
    std::string checkpoint_dir;
    std::string outdir;
    uint32_t write_overhead_dequeue_interval;
    Tick simulationtime;
    uint32_t possibility_relevel;
    bool only_relevel_demand;
    bool otp_for_address;
    bool predictive_decryption;
    bool memory_encryption;
    //Tick aes_latency;
    //xinw added for caching metadata in LLC
    int offload_aes_to_llc;
    int adaptive_l2_aes;
    Tick carry_less_multiplication_latency;
    Tick llc_metadata_access_latency;
    Tick data_aes_latency;
    Tick tree_node_aes_latency;
    Tick mac_latency;
    Tick modulo_latency;
    //Tick metadata_access_latency;
    //Tick noc_latency;
    bool wait_counter;

    //xinw added for morphable-end

    typedef RamulatorParams Params;
    Ramulator(const Params *p);
    virtual void init();
    virtual void startup();
    unsigned int drain(DrainManager* dm);
    virtual BaseSlavePort& getSlavePort(const std::string& if_name, 
        PortID idx = InvalidPortID);
    ~Ramulator();
protected:
    Tick recvAtomic(PacketPtr pkt);
    void recvFunctional(PacketPtr pkt);
    bool recvTimingReq(PacketPtr pkt);
    void recvRespRetry();
    bool check_give_up_aes(Addr block_addr);
    bool check_llc_aes(Addr block_addr);
    Tick llc_aes_finish_time(Addr block_addr);
    void accessAndRespond(PacketPtr pkt, Tick otp_begin_tick, Tick _block_arrive_time);
    void accessAndRespondBaseline(PacketPtr pkt, bool is_L0_block_miss, Tick info_create_time, Tick block_arrive_time, Tick L0_arrive_time, Tick L1_arrive_time, Tick L2_arrive_time); 
    void accessAndRespondDccm(PacketPtr pkt, bool is_L0_block_miss, Tick info_create_time, Tick block_arrive_time, Tick L0_arrive_time, Tick L1_arrive_time, Tick L2_arrive_time); 
    //void accessAndRespond(PacketPtr pkt, Tick otp_begin_tick, Tick _block_arrive_time, Tick L1_arrive_time, Tick L2_arrive_time);
    void readComplete(ramulator::Request& req);
    //xinw added for morphable-begin
    void readMetaComplete(ramulator::Request& req);
    void writeOverheadComplete(ramulator::Request& req);
    //xinw added for morphable-end
    void writeComplete(ramulator::Request& req);
    //xinw added for using trace
    void traceComplete(ramulator::Request& req);
    //xinw added
    void set_req_stall();
};

#endif // __RAMULATOR_HH__
