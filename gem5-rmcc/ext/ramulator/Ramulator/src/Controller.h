#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include <cassert>
#include <cstdio>
#include <deque>
#include <fstream>
#include <list>
#include <string>
#include <vector>
#include <queue>

#include "Config.h"
#include "DRAM.h"
#include "Refresh.h"
#include "Request.h"
#include "Scheduler.h"
#include "Statistics.h"

#include "ALDRAM.h"
#include "SALP.h"
#include "TLDRAM.h"
#include "DDR4.h"


using namespace std;

namespace ramulator
{

    extern bool warmup_complete;

template <typename T>
class Controller
{
protected:
    // For counting bandwidth
    ScalarStat read_transaction_bytes;
    ScalarStat write_transaction_bytes;

    ScalarStat row_hits;
    ScalarStat row_misses;
    ScalarStat row_conflicts;
    VectorStat read_row_hits;
    VectorStat read_row_misses;
    VectorStat read_row_conflicts;
    VectorStat write_row_hits;
    VectorStat write_row_misses;
    VectorStat write_row_conflicts;
    ScalarStat useless_activates;

    ScalarStat read_latency_avg;
    ScalarStat read_latency_sum;
    // gagan : write reqs
    ScalarStat write_latency_avg;
    ScalarStat write_latency_sum;
    // gagan : demand requests and prefetch requests
    ScalarStat demand_read_latency_avg;
    ScalarStat demand_read_latency_sum;
    ScalarStat prefetch_read_latency_avg;
    ScalarStat prefetch_read_latency_sum;

    ScalarStat req_queue_length_avg;
    ScalarStat req_queue_length_sum;
    ScalarStat read_req_queue_length_avg;
    ScalarStat read_req_queue_length_sum;
    ScalarStat write_req_queue_length_avg;
    ScalarStat write_req_queue_length_sum;

    ScalarStat last_read_at;
    ScalarStat last_write_at;


#ifndef INTEGRATED_WITH_GEM5
    VectorStat record_read_hits;
    VectorStat record_read_misses;
    VectorStat record_read_conflicts;
    VectorStat record_write_hits;
    VectorStat record_write_misses;
    VectorStat record_write_conflicts;
#endif

public:
    //xinw added
    uint64_t debug_data_read_delay_sum=0;
    ScalarStat counter_read_delay_sum, counter_read_num, counter_write_delay_sum, counter_write_num, data_read_delay_sum, data_read_num, data_write_delay_sum, data_write_num;
    ScalarStat counter_read_delay_sum_without_overflow, counter_read_num_without_overflow, counter_write_delay_sum_without_overflow, counter_write_num_without_overflow, data_read_delay_sum_without_overflow, data_read_num_without_overflow, data_write_delay_sum_without_overflow, data_write_num_without_overflow;
    
    /* Member Variables */
    long clk = 0;
    DRAM<T>* channel;

    // gagan : enable_debug
    bool enable_debug;

    // daz3
    long period_read_row_hits;
    long period_read_row_misses;
    long period_read_row_conflicts;
    long period_write_row_hits;
    long period_write_row_misses;
    long period_write_row_conflicts;
    long my_print_intelval = 200000000; //every 0.2ms

    Scheduler<T>* scheduler;  // determines the highest priority request whose commands will be issued
    RowPolicy<T>* rowpolicy;  // determines the row-policy (e.g., closed-row vs. open-row)
    RowTable<T>* rowtable;  // tracks metadata about rows (e.g., which are open and for how long)
    Refresh<T>* refresh;

    struct Queue {
        list<Request> q;
        unsigned int max = 32;
        // daz3: test tWTR
        // unsigned int max = 16;
	Controller<T> *ctrl;
        unsigned int size() { return q.size(); }
/*      void print()
      {
	//std::cout << "Printing queue: " << std::endl;
	for(auto &i : q)
	  {
	    i.print();
	  }
      }
*/
	void print()
	{
		//std::cout << "Printing queue: " << std::endl;
		for(auto i = q.begin(); i != q.end(); ++i)
		{
			i->print();
			std::cout << "\t\t" << (ctrl->is_ready(i) ? "READY" : "NOT READY") << std::endl;
		}
	}
      bool hasRequestType(Request::Type type)
      {
	for(auto &r : q)
	  {
	    if(r.type == type)
	      return true;
	  }
	return false;
      }
    };

    long long last_read_complete = 0;
    bool first_read_complete = false;
    bool first_read_enqueue = false;
    int last_rank = 0;
    Queue readq;  // queue for read requests
    Queue writeq;  // queue for write requests
    Queue actq; // read and write requests for which activate was issued are moved to 
                   // actq, which has higher priority than readq and writeq.
                   // This is an optimization
                   // for avoiding useless activations (i.e., PRECHARGE
                   // after ACTIVATE w/o READ of WRITE command)
    Queue otherq;  // queue for all "other" requests (e.g., refresh)

    deque<Request> pending;  // read requests that are about to receive data from DRAM
    bool write_mode = false;  // whether write requests should be prioritized over reads
    float wr_high_watermark = 0.8f; // threshold for switching to write mode
    float wr_low_watermark = 0.2f; // threshold for switching back to read mode
    //long refreshed = 0;  // last time refresh requests were generated

    /* Command trace for DRAMPower 3.1 */
    string cmd_trace_prefix = "cmd-trace-";
    vector<ofstream> cmd_trace_files;
    bool record_cmd_trace = false;
    /* Commands to stdout */
    bool print_cmd_trace = false;

    double tCK;
    long bw_mod;
    double max_bw;
    long w_reads;
    long w_writes;

    /* Constructor */
    Controller(const Config& configs, DRAM<T>* channel, bool enable_debug) :
        enable_debug(enable_debug),
        channel(channel),
        scheduler(new Scheduler<T>(this)),
        rowpolicy(new RowPolicy<T>(this)),
        rowtable(new RowTable<T>(this)),
        refresh(new Refresh<T>(this)),
        cmd_trace_files(channel->children.size())
    {
        // daz3: set read/write queue
        // readq.max = 128;
        readq.max = 256;
        writeq.max = 128;
        // writeq.max = 256;
        std::cout << "Controller readq " << readq.max << ", writeq " << writeq.max << std::endl;

	readq.ctrl = this;
	writeq.ctrl = this;
	otherq.ctrl = this;
	actq.ctrl = this;

        record_cmd_trace = configs.record_cmd_trace();
        print_cmd_trace = configs.print_cmd_trace();
        if (record_cmd_trace){
            if (configs["cmd_trace_prefix"] != "") {
              cmd_trace_prefix = configs["cmd_trace_prefix"];
            }
	    std::string traceDir = configs.get_tracefile_directory();
            string prefix = traceDir + cmd_trace_prefix + "chan-" + to_string(channel->id) + "-rank-";
            string suffix = ".cmdtrace";
            for (unsigned int i = 0; i < channel->children.size(); i++)
                cmd_trace_files[i].open(prefix + to_string(i) + suffix);
        }

	tCK = channel->spec->speed_entry.tCK;
	bw_mod = (int)((double)10000 / tCK);
	int *sz = channel->spec->org_entry.count;
	max_bw = channel->spec->speed_entry.rate * 1e6 * channel->spec->channel_width * sz[int(T::Level::Channel)] / 8;
	w_reads = 0;
	w_writes = 0;

	std::cout << "tCK: " << tCK << " ns." << std::endl;
	std::cout << "bw_mod: " << bw_mod << std::endl;
	std::cout << "max_bw: " << max_bw << " B/s." << std::endl;

        // daz3
        period_read_row_hits = 0;
        period_read_row_misses = 0;
        period_read_row_conflicts = 0;
        period_write_row_hits = 0;
        period_write_row_misses = 0;
        period_write_row_conflicts = 0;

        // regStats
 //xinw added
    //ScalarStat counter_read_delay_sum, counter_read_num, counter_write_delay_sum, counter_write_num, data_read_delay_sum, data_read_num, data_write_delay_sum, data_write_num;
	counter_read_delay_sum  
	    .name("counter_read_delay_sum_channel_"+to_string(channel->id) + "_core")
            .desc("Number of queuing delay for counter read")
            .precision(0)
            ;
        counter_read_num  
	    .name("counter_read_num_channel_"+to_string(channel->id) + "_core")
            .desc("Number of counter read")
            .precision(0)
            ;
        counter_write_delay_sum  
	    .name("counter_write_delay_sum_channel_"+to_string(channel->id) + "_core")
            .desc("Number of queuing delay for counter write")
            .precision(0)
            ;
	counter_write_num  
	    .name("counter_write_num_channel_"+to_string(channel->id) + "_core")
            .desc("Number of counter write")
            .precision(0)
            ;
	data_read_delay_sum  
	    .name("data_read_delay_sum_channel_"+to_string(channel->id) + "_core")
            .desc("Number of queuing delay for data read")
            .precision(0)
            ;
        data_read_num  
	    .name("data_read_num_channel_"+to_string(channel->id) + "_core")
            .desc("Number of data read")
            .precision(0)
            ;
        data_write_delay_sum  
	    .name("data_write_delay_sum_channel_"+to_string(channel->id) + "_core")
            .desc("Number of queuing delay for data write")
            .precision(0)
            ;
	data_write_num  
	    .name("data_write_num_channel_"+to_string(channel->id) + "_core")
            .desc("Number of data write")
            .precision(0)
            ;
	counter_read_delay_sum_without_overflow  
	    .name("counter_read_delay_sum_without_overflow_channel_"+to_string(channel->id) + "_core")
            .desc("Number of queuing delay for counter read")
            .precision(0)
            ;
        counter_read_num_without_overflow  
	    .name("counter_read_num_without_overflow_channel_"+to_string(channel->id) + "_core")
            .desc("Number of counter read")
            .precision(0)
            ;
        counter_write_delay_sum_without_overflow  
	    .name("counter_write_delay_sum_without_overflow_channel_"+to_string(channel->id) + "_core")
            .desc("Number of queuing delay for counter write")
            .precision(0)
            ;
	counter_write_num_without_overflow  
	    .name("counter_write_num_without_overflow_channel_"+to_string(channel->id) + "_core")
            .desc("Number of counter write")
            .precision(0)
            ;
	data_read_delay_sum_without_overflow  
	    .name("data_read_delay_sum_without_overflow_channel_"+to_string(channel->id) + "_core")
            .desc("Number of queuing delay for data read")
            .precision(0)
            ;
        data_read_num_without_overflow  
	    .name("data_read_num_without_overflow_channel_"+to_string(channel->id) + "_core")
            .desc("Number of data read")
            .precision(0)
            ;
        data_write_delay_sum_without_overflow  
	    .name("data_write_delay_sum_without_overflow_channel_"+to_string(channel->id) + "_core")
            .desc("Number of queuing delay for data write")
            .precision(0)
            ;
	data_write_num_without_overflow  
	    .name("data_write_num_without_overflow_channel_"+to_string(channel->id) + "_core")
            .desc("Number of data write")
            .precision(0)
            ;






   
        row_hits
            .name("row_hits_channel_"+to_string(channel->id) + "_core")
            .desc("Number of row hits per channel per core")
            .precision(0)
            ;
        row_misses
            .name("row_misses_channel_"+to_string(channel->id) + "_core")
            .desc("Number of row misses per channel per core")
            .precision(0)
            ;
        row_conflicts
            .name("row_conflicts_channel_"+to_string(channel->id) + "_core")
            .desc("Number of row conflicts per channel per core")
            .precision(0)
            ;

        read_row_hits
            .init(configs.get_core_num())
            .name("read_row_hits_channel_"+to_string(channel->id) + "_core")
            .desc("Number of row hits for read requests per channel per core")
            .precision(0)
            ;
        read_row_misses
            .init(configs.get_core_num())
            .name("read_row_misses_channel_"+to_string(channel->id) + "_core")
            .desc("Number of row misses for read requests per channel per core")
            .precision(0)
            ;
        read_row_conflicts
            .init(configs.get_core_num())
            .name("read_row_conflicts_channel_"+to_string(channel->id) + "_core")
            .desc("Number of row conflicts for read requests per channel per core")
            .precision(0)
            ;

        write_row_hits
            .init(configs.get_core_num())
            .name("write_row_hits_channel_"+to_string(channel->id) + "_core")
            .desc("Number of row hits for write requests per channel per core")
            .precision(0)
            ;
        write_row_misses
            .init(configs.get_core_num())
            .name("write_row_misses_channel_"+to_string(channel->id) + "_core")
            .desc("Number of row misses for write requests per channel per core")
            .precision(0)
            ;
        write_row_conflicts
            .init(configs.get_core_num())
            .name("write_row_conflicts_channel_"+to_string(channel->id) + "_core")
            .desc("Number of row conflicts for write requests per channel per core")
            .precision(0)
            ;

        useless_activates
            .name("useless_activates_"+to_string(channel->id)+ "_core")
            .desc("Number of useless activations. E.g, ACT -> PRE w/o RD or WR")
            .precision(0)
            ;

        read_transaction_bytes
            .name("read_transaction_bytes_"+to_string(channel->id))
            .desc("The total byte of read transaction per channel")
            .precision(0)
            ;
        write_transaction_bytes
            .name("write_transaction_bytes_"+to_string(channel->id))
            .desc("The total byte of write transaction per channel")
            .precision(0)
            ;

        read_latency_sum
            .name("read_latency_sum_"+to_string(channel->id))
            .desc("The memory latency cycles (in memory time domain) sum for all read requests in this channel")
            .precision(0)
            ;
        read_latency_avg
            .name("read_latency_avg_"+to_string(channel->id))
            .desc("The average memory latency cycles (in memory time domain) per request for all read requests in this channel")
            .precision(6)
            ;
	// gagan : write requests
	write_latency_sum
	  .name("write_latency_sum_"+to_string(channel->id))
            .desc("The memory latency cycles (in memory time domain) sum for all write requests in this channel")
            .precision(0)
            ;
        write_latency_avg
            .name("write_latency_avg_"+to_string(channel->id))
            .desc("The average memory latency cycles (in memory time domain) per request for all write requests in this channel")
            .precision(6)
            ;
	// gagan : demand and prefetch read requests
        demand_read_latency_sum
            .name("demand_read_latency_sum_"+to_string(channel->id))
            .desc("The memory latency cycles (in memory time domain) sum for demand read requests in this channel")
            .precision(0)
            ;
        demand_read_latency_avg
            .name("demand_read_latency_avg_"+to_string(channel->id))
            .desc("The average memory latency cycles (in memory time domain) per request for demand read requests in this channel")
            .precision(6)
            ;
	prefetch_read_latency_sum
            .name("prefetch_read_latency_sum_"+to_string(channel->id))
            .desc("The memory latency cycles (in memory time domain) sum for prefetch read requests in this channel")
            .precision(0)
            ;
        prefetch_read_latency_avg
            .name("prefetch_read_latency_avg_"+to_string(channel->id))
            .desc("The average memory latency cycles (in memory time domain) per request for prefetch read requests in this channel")
            .precision(6)
            ;
        req_queue_length_sum
            .name("req_queue_length_sum_"+to_string(channel->id))
            .desc("Sum of read and write queue length per memory cycle per channel.")
            .precision(0)
            ;
        req_queue_length_avg
            .name("req_queue_length_avg_"+to_string(channel->id))
            .desc("Average of read and write queue length per memory cycle per channel.")
            .precision(6)
            ;

        read_req_queue_length_sum
            .name("read_req_queue_length_sum_"+to_string(channel->id))
            .desc("Read queue length sum per memory cycle per channel.")
            .precision(0)
            ;
        read_req_queue_length_avg
            .name("read_req_queue_length_avg_"+to_string(channel->id))
            .desc("Read queue length average per memory cycle per channel.")
            .precision(6)
            ;

        write_req_queue_length_sum
            .name("write_req_queue_length_sum_"+to_string(channel->id))
            .desc("Write queue length sum per memory cycle per channel.")
            .precision(0)
            ;
        write_req_queue_length_avg
            .name("write_req_queue_length_avg_"+to_string(channel->id))
            .desc("Write queue length average per memory cycle per channel.")
            .precision(6)
            ;

	last_read_at
	  .name("last_read_at_"+to_string(channel->id))
	  .desc("Last read at.")
	  .precision(0)
	  ;
	last_write_at
	  .name("last_write_at_"+to_string(channel->id))
	  .desc("Last completed write at Tick.")
	  .precision(0)
	  ;


#ifndef INTEGRATED_WITH_GEM5
        record_read_hits
            .init(configs.get_core_num())
            .name("record_read_hits")
            .desc("record read hit count for this core when it reaches request limit or to the end")
            ;

        record_read_misses
            .init(configs.get_core_num())
            .name("record_read_misses")
            .desc("record_read_miss count for this core when it reaches request limit or to the end")
            ;

        record_read_conflicts
            .init(configs.get_core_num())
            .name("record_read_conflicts")
            .desc("record read conflict count for this core when it reaches request limit or to the end")
            ;

        record_write_hits
            .init(configs.get_core_num())
            .name("record_write_hits")
            .desc("record write hit count for this core when it reaches request limit or to the end")
            ;

        record_write_misses
            .init(configs.get_core_num())
            .name("record_write_misses")
            .desc("record write miss count for this core when it reaches request limit or to the end")
            ;

        record_write_conflicts
            .init(configs.get_core_num())
            .name("record_write_conflicts")
            .desc("record write conflict for this core when it reaches request limit or to the end")
            ;
#endif
    }

    ~Controller(){
        delete scheduler;
        delete rowpolicy;
        delete rowtable;
        delete channel;
        delete refresh;
        for (auto& file : cmd_trace_files)
            file.close();
        cmd_trace_files.clear();
    }
//xinw added
bool is_metadata(uint64_t physical_addr){
        if((physical_addr<((Addr)3 << 30))||((physical_addr>((Addr)4 << 30))&&(physical_addr<((Addr)124 << 30)))) 
                return false;   
	else if(physical_addr>((Addr)128 << 30)){
		return false;
	}
        else 
                return true;
}

void update_queue_delay(std::_List_iterator<ramulator::Request>& req, bool _is_first){
	if(_is_first&&(req->addr<((Addr)256 << 30))){ 
		//xinw added
	   	//std::cout << (req->type == Request::Type::READ ?  "READ " : " WRITE ") << (req->is_counter ? "Counter " : "Data ") <<  (req->is_overflow ? "Overflow req " : "L3 req ") << std::dec << clk << ", addr: " << std::hex << req->addr << std::dec << " --- \t\tcolumn: " << req->addr_vec[5] << ", chan: " << req->addr_vec[0] << ", bank: " << req->addr_vec[3] << ", bankgroup: " << req->addr_vec[2] << ", rank: " << req->addr_vec[1] << ", row: " << req->addr_vec[4] << std::endl;
		if(is_metadata(req->addr)){
			if (req->type == Request::Type::READ) {
				counter_read_num++;
				if(!req->is_overflow)
					counter_read_num_without_overflow++;
			}else if (req->type == Request::Type::WRITE) {
				counter_write_num++;
				if(!req->is_overflow)
					counter_write_num_without_overflow++;
			}	
		}
		else{	
			if (req->type == Request::Type::READ) {
				data_read_num++;
				if(!req->is_overflow)
					data_read_num_without_overflow++;
			}else if (req->type == Request::Type::WRITE) {
				data_write_num++;
				if(!req->is_overflow)
					data_write_num_without_overflow++;
			}	
		}

		if(is_metadata(req->addr)){
			if(req->type == Request::Type::READ){
				counter_read_delay_sum+=curTick()-req->enqueue_time;	
				if(!req->is_overflow)
					counter_read_delay_sum_without_overflow+=curTick()-req->enqueue_time;	
			}else if(req->type == Request::Type::WRITE){
				counter_write_delay_sum+=curTick()-req->enqueue_time;	
				if(!req->is_overflow)
					counter_write_delay_sum_without_overflow+=curTick()-req->enqueue_time;	
			}		
		}
		else{
			if(req->type == Request::Type::READ){
				data_read_delay_sum+=curTick()-req->enqueue_time;	
				debug_data_read_delay_sum+=curTick()-req->enqueue_time;
			        //std::cout << "data read delay: "<<debug_data_read_delay_sum <<" by increasing curtick "<<curTick()<<" and subtracting enqueue time "<<req->enqueue_time << std::endl;	
				if(!req->is_overflow)
					data_read_delay_sum_without_overflow+=curTick()-req->enqueue_time;	
			}else if(req->type == Request::Type::WRITE){
				data_write_delay_sum+=curTick()-req->enqueue_time;	
				if(!req->is_overflow)
					data_write_delay_sum_without_overflow+=curTick()-req->enqueue_time;	
			}	
		}
	}
} 


    void finish(long read_req, long write_req, long demand_read_req, long prefetch_read_req,long dram_cycles) {
      read_latency_avg = read_latency_sum.value() / read_req;
      req_queue_length_avg = req_queue_length_sum.value() / dram_cycles;
      read_req_queue_length_avg = read_req_queue_length_sum.value() / dram_cycles;
      write_req_queue_length_avg = write_req_queue_length_sum.value() / dram_cycles;
      // gagan :
      write_latency_avg = write_latency_sum.value() / write_req;
      // gagan : demand and prefetch reads
      demand_read_latency_avg = demand_read_latency_sum.value() / demand_read_req;
      prefetch_read_latency_avg = prefetch_read_latency_sum.value() / prefetch_read_req;
      // call finish function of each channel
      channel->finish(dram_cycles);
    }

    /* Member Functions */
    Queue& get_queue(Request::Type type)
    {
        switch (int(type)) {
            case int(Request::Type::READ): return readq;
            case int(Request::Type::WRITE): return readq; // xinw: unify all queues. also push writes into readq.
            default: return otherq;
        }
    }

    // gagan :
    bool promote(Request& req)
    {
      assert(req.type == Request::Type::READ);
      // promote : actq
      for(auto &r : actq.q)
	{
	  if(r.addr == req.addr && r.type == Request::Type::READ)
	    {
	      assert(r.is_prefetch == true);
	      r.is_prefetch = false;
	      r.arrive = clk;
	    }
	}

      for(auto &r : readq.q)
	{
	  if(r.addr == req.addr)
	    {
	      assert(r.is_prefetch == true);
	      r.arrive = clk;
	      r.is_prefetch = false;
	    }
	}

      return true;
    }
    
    bool enqueue(Request& req)
    {
	//xinw added
	req.enqueue_time=curTick();	
	req.enqueue_clk=clk;	
	/*if(req.is_counter)
		std::cout << (req.type == Request::Type::READ ?  "READ " : " WRITE ") << (req.is_overflow ? "Overflow req " : "L3 req ") << std::dec << clk << ", addr: " << std::hex << req.addr << std::dec << " --- \t\tcolumn: " << req.addr_vec[5] << ", chan: " << req.addr_vec[0] << ", bank: " << req.addr_vec[3] << ", bankgroup: " << req.addr_vec[2] << ", rank: " << req.addr_vec[1] << ", row: " << req.addr_vec[4] << std::endl;
*/
        Queue& queue = get_queue(req.type);
        if (queue.max == queue.size())
            return false;

        req.arrive = clk;
        queue.q.push_back(req);
	if(!first_read_enqueue && req.type == Request::Type::READ)
	  first_read_enqueue = true;
	//xinw added

	  //std::cout << (req.type == Request::Type::READ ?  "READ " : " WRITE ") << (req.is_counter ? "Counter " : "Data ") <<  (req.is_overflow ? "Overflow req " : "L3 req ") << std::dec << clk << ", addr: " << std::hex << req.addr << std::dec << " --- \t\tcolumn: " << req.addr_vec[5] << ", chan: " << req.addr_vec[0] << ", bank: " << req.addr_vec[3] << ", bankgroup: " << req.addr_vec[2] << ", rank: " << req.addr_vec[1] << ", row: " << req.addr_vec[4] << std::endl;
        // shortcut for read requests, if a write to same addr exists
        // necessary for coherence
        if (req.type == Request::Type::READ && find_if(writeq.q.begin(), writeq.q.end(),
                [req](Request& wreq){ return req.addr == wreq.addr;}) != writeq.q.end()){
            req.depart = clk + 1;
            pending.push_back(req);
            readq.q.pop_back();
        }
        return true;
    }
    //xinw added for morphable-begin
    uint64_t get_remaining_queue_size(Request& req)
    {
            Queue& queue = get_queue(req.type);
            return (queue.max-queue.size());
    }  
    uint64_t get_overflow_queue_size(Request& req)
    {
	    uint64_t _cnt=0;
            Queue& queue = get_queue(req.type);
 	    list<Request>::iterator it;
            for (it=queue.q.begin(); it!=queue.q.end(); ++it)
            {
                if(it->is_overflow)
                        _cnt++;
            }
	    return _cnt;
    }

    uint64_t prioritize(Request& req)
    {
            Queue& queue = get_queue(req.type);
            list<Request>::iterator it;
            for (it=queue.q.begin(); it!=queue.q.end(); ++it)
            {
                if(it->addr==req.addr)
                        break;
            }
            queue.q.splice(queue.q.begin(),queue.q,it);
            return 0;
    }

    //xinw added for morphable-end

    void tick()
    {
      /*
      if(enable_debug && first_read_enqueue)
	{
	  std::cout << "\n\n";
	  std::cout << "clk: " << clk << "\n";
	  rowtable->print();
	  std::cout << "readQ size: " << readq.q.size() << "\n";

	  map<vector<int>, vector<long> > unique;
	  for(auto &req : readq.q)
	    {
	      int ch = req.addr_vec[0];
	      int ra = req.addr_vec[1];
	      int bg = req.addr_vec[2];
	      int ba = req.addr_vec[3];
	      int row = req.addr_vec[4];

	      vector<int> rowgroup = {ch, ra, bg, ba, row};
	      auto found = unique.find(rowgroup);
	      if(found == unique.end())
		{
		  unique.insert({rowgroup, {req.addr}});
		}
	      else
		{
		  found->second.push_back(req.addr);
		}
	    }

	  for(auto &rowgroup : unique)
	    {
	      string rowStatus = rowtable->status(rowgroup.first);
	      std::cout << "For rank: " << rowgroup.first[1] << ", bankgroup: " << rowgroup.first[2] << ", bank: " << rowgroup.first[3] << ", row: " << rowgroup.first[4] << ", STATUS: " << rowStatus << "\t --- " << "number of reqs are: " << rowgroup.second.size() << "\t\t( ";
	      for(auto &r : rowgroup.second)
		std::cout << std::dec << r << ", ";
	      std::cout << ")\n";
	    }
	  std::cout << "\n\n";
	}
*/
      if(first_read_enqueue && clk % bw_mod == 0)
	{
	  std::cout << clk << ", reads: " << w_reads << ", writes: " << w_writes << std::endl;;
	  w_reads = 0;
	  w_writes = 0;
	}

      if(enable_debug && first_read_enqueue && clk % bw_mod == 0)
	{
	  double curr_time = (double)clk * tCK;
	  /*
	    std::cout << "Channel: " << channel->id << std::endl;
	    std::cout << "Time: " << (long)curr_time << " ns." << std::endl;
	  */
	  double read_bw_utilization = ((double)(64 * w_reads) / (double)(1e-5)) / (double)max_bw;
	  double write_bw_utilization = ((double)(64 * w_writes) / (double)(1e-5)) / (double)max_bw;
	  double overall_bw_utilization = read_bw_utilization + write_bw_utilization;
	  /*
	    std::cout << "read_bw_utilization: " << read_bw_utilization << std::endl;
	    std::cout << "write_bw_utilization: " << write_bw_utilization << std::endl;
	    std::cout << "overall_bw_utilization: " << read_bw_utilization + write_bw_utilization << std::endl;
	    std::cout << std::endl << std::endl;

	    std::cout << (long)curr_time << ", "
	              << channel->id << ", "
		      << read_bw_utilization << ", "
		      << write_bw_utilization << ", "
		      << overall_bw_utilization << std::endl;
	  */
	  std::cout << "Window ended. Bandwidth utilization: " << overall_bw_utilization << ", ";
	  if(overall_bw_utilization > 0.95)
	    std::cout << "greater than 0.95\n\n";
	  else
	    std::cout << "less than 0.95\n\n";
	  w_reads = 0;
	  w_writes = 0;
	}
      
        clk++;
        req_queue_length_sum += readq.size() + writeq.size() + pending.size();
        read_req_queue_length_sum += readq.size() + pending.size();
        write_req_queue_length_sum += writeq.size();

        /*** 1. Serve completed reads ***/
        if (pending.size()) {
            Request& req = pending[0];
            if (req.depart <= clk) {
                if (req.depart - req.arrive > 1) { // this request really accessed a row
                  read_latency_sum += req.depart - req.arrive;
		  // gagan : demand and prefetch reads
		  if(req.is_prefetch)
		      prefetch_read_latency_sum += req.depart - req.arrive;
		  else
		      demand_read_latency_sum += req.depart - req.arrive;
                  channel->update_serving_requests(
                      req.addr_vec.data(), -1, clk);
                }
		
                req.callback(req);
		pending.pop_front();
		last_read_at = clk;
		w_reads++;
            }
        }

        /*** 2. Refresh scheduler ***/
        refresh->tick_ref();

        /*** 3. Should we schedule writes? ***/
        if (!write_mode)
	  {
            // yes -- write queue is almost full or read queue is empty
            if (writeq.size() > int(wr_high_watermark * writeq.max) 
                    /*|| readq.size() == 0*/) // Hasan: Switching to write mode when there are just a few 
                                              // write requests, even if the read queue is empty, incurs a lot of overhead. 
                                              // Commented out the read request queue empty condition
	      write_mode = true;
        }
        else {
            // no -- write queue is almost empty and read queue is not empty
            if (writeq.size() < int(wr_low_watermark * writeq.max) && readq.size() != 0)
	      write_mode = false;
        }

      if(enable_debug && first_read_enqueue)
	  {
	    std::cout << "\n\n<------------------------------------------->" << std::endl;
	    std::cout << "Clk: " << clk << std::endl;
	    std::cout << "Channel: " << channel->id << std::endl;
	    if(actq.q.size() != 0)
	      {
		std::cout << "Activation Queue: " << std::endl;
		actq.print();
	      }
	    if(readq.q.size() != 0)
	      {
		std::cout << "Read Queue: " << std::endl;
		readq.print();
	      }
	    if(writeq.q.size() != 0)
	      {
		std::cout << "Write Queue: " << std::endl;
		writeq.print();
	      }
	    if(otherq.q.size() != 0)
	      {
		std::cout << "Other Queue: " << std::endl;
		otherq.print();
	      }
	    rowtable->print();
	    map<vector<int>, vector<pair<long, long> > > unique;
	    for(auto &req : readq.q)
	      {
		int ch = req.addr_vec[0];
		int ra = req.addr_vec[1];
		int bg = req.addr_vec[2];
		int ba = req.addr_vec[3];
		int row = req.addr_vec[4];

		vector<int> rowgroup = {ch, ra, bg, ba, row};
		auto found = unique.find(rowgroup);
		if(found == unique.end())
		  {
		    unique.insert({rowgroup, {make_pair(req.addr, req.coreid)}});
		  }
		else
		  {
		    found->second.push_back(make_pair(req.addr, req.coreid));
		  }
	      }

	    for(auto &rowgroup : unique)
	      {
		string rowStatus = rowtable->status(rowgroup.first);
		std::cout << "For rank: " << rowgroup.first[1] << ", bankgroup: " << rowgroup.first[2] << ", bank: " << rowgroup.first[3] << ", row: " << rowgroup.first[4] << ", STATUS: " << rowStatus << "\t --- " << "number of reqs are: " << rowgroup.second.size() << "\t\t( ";
		for(auto &r : rowgroup.second)
		  std::cout << std::dec << r.first << " : " << r.second << ", ";
		std::cout << ")\n";
	      }
	    std::cout << "\n\n";
	    std::cout << "<------------------------------------------->" << std::endl;
	  }
        /*** 4. Find the best command to schedule, if any ***/

        // First check the actq (which has higher priority) to see if there
        // are requests available to service in this cycle
        Queue* queue;
        if (otherq.size())
	  queue = &otherq; // "other" requests are rare, so we give them precedence over reads/writes
        else
	  queue = &readq; //xinw : unify all queues. set readq as the only queue. earlier this was actq and then queue was mapped to readq or writeq if there was nothing to enqueue.

	auto req = scheduler->get_head(queue->q);

        if (req == queue->q.end() || !is_ready(req)) {
            // we couldn't find a command to schedule -- let's try to be speculative
            auto cmd = T::Command::PRE;
            vector<int> victim = rowpolicy->get_victim(cmd);
            if (!victim.empty()){
                issue_cmd(cmd, victim);
		//xinw added
		//update_queue_delay(req, req->is_first_command);
		
		if(enable_debug && first_read_enqueue)
		  {
		    std::cout << std::dec <<  " --- \t\tcolumn: " << victim[5] << ", chan: " << victim[0] << ", bank: " << victim[3] << ", bankgroup: " << victim[2] << ", rank: " << victim[1] << ", row: " << victim[4] ;
		  }
            }
            return;  // nothing more to be done this cycle
        }
	//xinw added
	bool temp_is_first=req->is_first_command;
	if (req->is_first_command) {
            req->is_first_command = false;
            int coreid = req->coreid;
            if (req->type == Request::Type::READ || req->type == Request::Type::WRITE) {
              channel->update_serving_requests(req->addr_vec.data(), 1, clk);
            }
            int tx = (channel->spec->prefetch_size * channel->spec->channel_width / 8);
            if (req->type == Request::Type::READ) {
                if (is_row_hit(req)) {
                    ++read_row_hits[coreid];
                    ++row_hits;
                    // daz3
                    period_read_row_hits++;
                } else if (is_row_open(req)) {
                    ++read_row_conflicts[coreid];
                    ++row_conflicts;
                    // daz3
                    period_read_row_conflicts++;
                } else {
                    ++read_row_misses[coreid];
                    ++row_misses;
                    // daz3
                    period_read_row_misses++;
                }
              read_transaction_bytes += tx;
            } else if (req->type == Request::Type::WRITE) {
              if (is_row_hit(req)) {
                  ++write_row_hits[coreid];
                  ++row_hits;
                  // daz3
                  period_write_row_hits++;
              } else if (is_row_open(req)) {
                  ++write_row_conflicts[coreid];
                  ++row_conflicts;
                  // daz3
                  period_write_row_conflicts++;
              } else {
                  ++write_row_misses[coreid];
                  ++row_misses;
                  // daz3
                  period_write_row_misses++;
              }
              write_transaction_bytes += tx;
            }
        }

        // issue command on behalf of request
	//std::cout << "Issuing command on behalf of request: ";
	//req->print();	  
        auto cmd = get_first_cmd(req);
        issue_cmd(cmd, get_addr_vec(cmd, req));
		update_queue_delay(req, temp_is_first);
	if(enable_debug && first_read_enqueue)
	  {
	    if(int(cmd) == int(T::Command::RD))
	      {
		w_reads++;
		if(!first_read_complete)
		  first_read_complete = true;

		if(first_read_complete)
		  {
		    bool over4 = false;
		    long long overBy = clk - last_read_complete;
		    if(overBy > 4)
		      over4 = true;
		    last_read_complete = clk;
		    if(over4)
		      {
			std::cout << std::dec << req->addr << ", overBy: " << overBy << ", arriv: " << req->arrive << " --- \t\tcolumn: " << req->addr_vec[5] << ", chan: " << req->addr_vec[0] << ", bank: " << req->addr_vec[3] << ", bankgroup: " << req->addr_vec[2] << ", rank: " << req->addr_vec[1] << ", row: " << req->addr_vec[4] << " for addr: " << req->addr << ", \t" << "core: " << req->coreid  << ", issue - arriv: " << clk - req->arrive << "\n";
		      }
		    else
		      {
			std::cout << std::dec << req->addr << ", arriv: " << req->arrive << " --- \t\tcolumn: " << req->addr_vec[5] << ", chan: " << req->addr_vec[0] << ", bank: " << req->addr_vec[3] << ", bankgroup: " << req->addr_vec[2] << ", rank: " << req->addr_vec[1] << ", row: " << req->addr_vec[4] << " for addr: " << req->addr << ", \t" << "core: " << req->coreid << ", issue - arriv: " << clk - req->arrive << "\n";
		      }
		  }
	      }
	    else
	      {
		std::cout << std::dec << req->addr << " --- \t\tcolumn: " << req->addr_vec[5] << ", chan: " << req->addr_vec[0] << ", bank: " << req->addr_vec[3] << ", bankgroup: " << req->addr_vec[2] << ", rank: " << req->addr_vec[1] << ", row: " << req->addr_vec[4] << " for addr: " << req->addr << "\n";
	      }
	  }

        // check whether this is the last command (which finishes the request)
        //if (cmd != channel->spec->translate[int(req->type)]){
        if (!(channel->spec->is_accessing(cmd) || channel->spec->is_refreshing(cmd))) {
/* // xinw : unify all queues. don't push into actq anymore after ACT.
            if(channel->spec->is_opening(cmd)) {
                // promote the request that caused issuing activation to actq
                actq.q.push_back(*req);
                queue->q.erase(req);
            }
*/
            return;
        }

        // set a future completion time for read requests
        if (req->type == Request::Type::READ) {
	   //xinw added for debugging
	   //std::cout << (req->type == Request::Type::READ ?  "READ " : " WRITE ") << (req->is_overflow ? "Overflow req " : "L3 req ") << std::dec << clk << ", addr: " << std::hex << req->addr << std::dec << " --- \t\tcolumn: " << req->addr_vec[5] << ", chan: " << req->addr_vec[0] << ", bank: " << req->addr_vec[3] << ", bankgroup: " << req->addr_vec[2] << ", rank: " << req->addr_vec[1] << ", row: " << req->addr_vec[4] << std::endl;

            req->depart = clk + channel->spec->read_latency;
            pending.push_back(*req);
        }

        if (req->type == Request::Type::WRITE) {
   //xinw added for debugging
	   //std::cout << (req->type == Request::Type::READ ?  "READ " : " WRITE ") << (req->is_overflow ? "Overflow req " : "L3 req ") << std::dec << clk << ", addr: " << std::hex << req->addr << std::dec << " --- \t\tcolumn: " << req->addr_vec[5] << ", chan: " << req->addr_vec[0] << ", bank: " << req->addr_vec[3] << ", bankgroup: " << req->addr_vec[2] << ", rank: " << req->addr_vec[1] << ", row: " << req->addr_vec[4] << std::endl;

            channel->update_serving_requests(req->addr_vec.data(), -1, clk);
	    write_latency_sum += clk - req->wr_arrive;
	    w_writes++;
	    last_write_at = clk;
	    //xinw added for controlling trace
	    req->callback(*req);    
       }

	/*
	if(enable_debug)
	  {
	    std::cout << "Request completed: "; req->print();
	  }
	*/

	/*
	// gagan : debug
	if(req->type == Request::Type::REFRESH)
	  {
	    std::cout << "Request completed: "; req->print();
	  }
	*/
	
	if(req->type == Request::Type::READ || req->type == Request::Type::WRITE)
	  {
	    int rank = req->addr_vec[1];
	    assert(rank != get_refreshing_rank());
	  }
        // remove request from queue
        queue->q.erase(req);

        // daz3
        if(clk % my_print_intelval == 0)
        {
            std::cout << "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv" << std::endl;
            std::cout << "clk " << clk << std::endl;
            std::cout << "period_read_row_hits " << period_read_row_hits << std::endl;
            std::cout << "period_read_row_misses " << period_read_row_misses << std::endl;
            std::cout << "period_read_row_conflicts " << period_read_row_conflicts << std::endl;
            std::cout << "period_write_row_hits " << period_write_row_hits << std::endl;
            std::cout << "period_write_row_misses " << period_write_row_misses << std::endl;
            std::cout << "period_write_row_conflicts " << period_write_row_conflicts << std::endl;
            std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << std::endl;
        }
    }

    bool is_ready(list<Request>::iterator req)
    {
        typename T::Command cmd = get_first_cmd(req);
        return channel->check(cmd, req->addr_vec.data(), clk);
    }

    bool is_ready(typename T::Command cmd, const vector<int>& addr_vec)
    {
        return channel->check(cmd, addr_vec.data(), clk);
    }

    bool is_row_hit(list<Request>::iterator req)
    {
        // cmd must be decided by the request type, not the first cmd
        typename T::Command cmd = channel->spec->translate[int(req->type)];
        return channel->check_row_hit(cmd, req->addr_vec.data());
    }

    bool is_row_hit(typename T::Command cmd, const vector<int>& addr_vec)
    {
        return channel->check_row_hit(cmd, addr_vec.data());
    }

    bool is_row_open(list<Request>::iterator req)
    {
        // cmd must be decided by the request type, not the first cmd
        typename T::Command cmd = channel->spec->translate[int(req->type)];
        return channel->check_row_open(cmd, req->addr_vec.data());
    }

    bool is_row_open(typename T::Command cmd, const vector<int>& addr_vec)
    {
        return channel->check_row_open(cmd, addr_vec.data());
    }

    void update_temp(ALDRAM::Temp current_temperature)
    {
    }

    // For telling whether this channel is busying in processing read or write
    bool is_active() {
      return (channel->cur_serving_requests > 0);
    }

    // For telling whether this channel is under refresh
    /*
    bool is_refresh() {
      // gagan : for all-rank refresh
      // return clk <= channel->end_of_refreshing;
      // gagan : for staggered refresh
      bool isAnyRankRefresh = false;
      for(auto rank : channel->children)
	{
	  assert(isAnyRankRefresh == false);
	  isAnyRankRefresh = (bool)(clk <= rank->end_of_refreshing);
	}
      return isAnyRankRefresh;
    }
    */

    // gagan : returns which rank is refreshing, otherwise returns -1.
    int get_refreshing_rank()
    {
      for(auto rank : channel->children)
	{
	  if(clk <= rank->end_of_refreshing)
	    {
	      return rank->id;
	    }
	}
      return -1;
    }

    bool no_reads_to_non_refreshing_ranks(int refreshing_rank)
    {
      auto &read_queue = readq.q;
      for(auto &req : read_queue)
	{
	  int rank = req.addr_vec[1];
	  if(rank != refreshing_rank)
	    return false;
	}
      return true;
    }

    void set_high_writeq_watermark(const float watermark) {
       wr_high_watermark = watermark; 
    }

    void set_low_writeq_watermark(const float watermark) {
       wr_low_watermark = watermark;
    }

    void record_core(int coreid) {
#ifndef INTEGRATED_WITH_GEM5
      record_read_hits[coreid] = read_row_hits[coreid];
      record_read_misses[coreid] = read_row_misses[coreid];
      record_read_conflicts[coreid] = read_row_conflicts[coreid];
      record_write_hits[coreid] = write_row_hits[coreid];
      record_write_misses[coreid] = write_row_misses[coreid];
      record_write_conflicts[coreid] = write_row_conflicts[coreid];
#endif
    }

private:
    typename T::Command get_first_cmd(list<Request>::iterator req)
    {
        typename T::Command cmd = channel->spec->translate[int(req->type)];
        return channel->decode(cmd, req->addr_vec.data());
    }

    // upgrade to an autoprecharge command
    void cmd_issue_autoprecharge(typename T::Command& cmd,
                                            const vector<int>& addr_vec) {

        // currently, autoprecharge is only used with closed row policy
        if(channel->spec->is_accessing(cmd) && rowpolicy->type == RowPolicy<T>::Type::ClosedAP) {
            // check if it is the last request to the opened row
            Queue* queue = write_mode ? &writeq : &readq;

            auto begin = addr_vec.begin();
            vector<int> rowgroup(begin, begin + int(T::Level::Row) + 1);

			int num_row_hits = 0;

            for (auto itr = queue->q.begin(); itr != queue->q.end(); ++itr) {
                if (is_row_hit(itr)) { 
                    auto begin2 = itr->addr_vec.begin();
                    vector<int> rowgroup2(begin2, begin2 + int(T::Level::Row) + 1);
                    if(rowgroup == rowgroup2)
                        num_row_hits++;
                }
            }

            if(num_row_hits == 0) {
                Queue* queue = &actq;
                for (auto itr = queue->q.begin(); itr != queue->q.end(); ++itr) {
                    if (is_row_hit(itr)) {
                        auto begin2 = itr->addr_vec.begin();
                        vector<int> rowgroup2(begin2, begin2 + int(T::Level::Row) + 1);
                        if(rowgroup == rowgroup2)
                            num_row_hits++;
                    }
                }
            }

            assert(num_row_hits > 0); // The current request should be a hit, 
                                      // so there should be at least one request 
                                      // that hits in the current open row
            if(num_row_hits == 1) {
                if(cmd == T::Command::RD)
                    cmd = T::Command::RDA;
                else if (cmd == T::Command::WR)
                    cmd = T::Command::WRA;
                else
                    assert(false && "Unimplemented command type.");
            }
        }

    }

    void issue_cmd(typename T::Command cmd, const vector<int>& addr_vec)
    {
      
      if(enable_debug && first_read_enqueue)
	{
	  if(int(cmd) == int(T::Command::REF))
	    std::cout << "clk: " << clk << ", issued REF: " ;
	  if(int(cmd) == int(T::Command::PRE))
	    std::cout << "clk: " << clk << ", issued PRE: " ;
	  if(int(cmd) == int(T::Command::ACT))
	    std::cout << "clk: " << clk << ", issued ACT: " ;
	  if(int(cmd) == int(T::Command::RD))
	    std::cout << "clk: " << clk << ", issued RD: " ;
	  if(int(cmd) == int(T::Command::WR))
	    std::cout << "clk: " << clk << ", issued WR: " ;
	}
      
      //if(int(cmd) == int(T::Command::RD) || int(cmd) == int(T::Command::WR))
      //	{
      //	  last_rank = addr_vec[1];
      //	}
      
      
      	// gagan : staggered refresh
      /*
	if(channel->spec->standard_name == "DDR4" && int(cmd) == int(T::Command::REF))
	  {
	    assert(is_refresh() == false);
	  }
      */
      
        cmd_issue_autoprecharge(cmd, addr_vec);
        assert(is_ready(cmd, addr_vec));
        channel->update(cmd, addr_vec.data(), clk);

	/*
	 // gagan : debug
	if(channel->spec->standard_name == "DDR4" && int(cmd) == int(T::Command::REF))
	  {
	    int i = 0;
	    for (auto child : channel->children)
	      {
		std::cout << "end_of_refreshing_rank: " << i << ", clk: " << child->end_of_refreshing << std::endl;
		i++;
	      }
	  }
	*/

        if(cmd == T::Command::PRE){
            if(rowtable->get_hits(addr_vec, true) == 0){
                useless_activates++;
            }
        }
 
        rowtable->update(cmd, addr_vec, clk);
        if (record_cmd_trace){
            // select rank
            auto& file = cmd_trace_files[addr_vec[1]];
            string& cmd_name = channel->spec->command_name[int(cmd)];
            file<<clk<<','<<cmd_name;
            // TODO bad coding here
            if (cmd_name == "PREA" || cmd_name == "REF")
                file<<endl;
            else{
                int bank_id = addr_vec[int(T::Level::Bank)];
                if (channel->spec->standard_name == "DDR4" || channel->spec->standard_name == "GDDR5")
                    bank_id += addr_vec[int(T::Level::Bank) - 1] * channel->spec->org_entry.count[int(T::Level::Bank)];
                file<<','<<bank_id<<endl;
            }
        }
        if (print_cmd_trace){
            printf("%5s %10ld:", channel->spec->command_name[int(cmd)].c_str(), clk);
            for (int lev = 0; lev < int(T::Level::MAX); lev++)
                printf(" %5d", addr_vec[lev]);
            printf("\n");
        }
    }
    vector<int> get_addr_vec(typename T::Command cmd, list<Request>::iterator req){
        return req->addr_vec;
    }
};

template <>
vector<int> Controller<SALP>::get_addr_vec(
    SALP::Command cmd, list<Request>::iterator req);

template <>
bool Controller<SALP>::is_ready(list<Request>::iterator req);

template <>
void Controller<ALDRAM>::update_temp(ALDRAM::Temp current_temperature);
//template <>
//void Controller<TLDRAM>::update_queue_delay(Request req);


template <>
void Controller<TLDRAM>::tick();

template <>
void Controller<TLDRAM>::cmd_issue_autoprecharge(typename TLDRAM::Command& cmd,
                                                    const vector<int>& addr_vec);

} /*namespace ramulator*/

#endif /*__CONTROLLER_H*/
