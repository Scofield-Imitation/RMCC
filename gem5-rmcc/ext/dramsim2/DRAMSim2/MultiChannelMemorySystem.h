/*********************************************************************************
*  Copyright (c) 2010-2011, Elliott Cooper-Balis
*                             Paul Rosenfeld
*                             Bruce Jacob
*                             University of Maryland 
*                             dramninjas [at] gmail [dot] com
*  All rights reserved.
*  
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*  
*     * Redistributions of source code must retain the above copyright notice,
*        this list of conditions and the following disclaimer.
*  
*     * Redistributions in binary form must reproduce the above copyright notice,
*        this list of conditions and the following disclaimer in the documentation
*        and/or other materials provided with the distribution.
*  
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
*  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
*  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
*  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************/
#include "SimulatorObject.h"
#include "Transaction.h"
#include "SystemConfiguration.h"
#include "MemorySystem.h"
#include "IniReader.h"
#include "ClockDomain.h"
#include "CSVWriter.h"
//Yihan: add cpurequest.h
#include "CpuRequest.h"

namespace DRAMSim {

struct CacheReturn{
	uint64_t addr;
	bool duplicate;
};

class MultiChannelMemorySystem : public SimulatorObject 
{
	public: 

	MultiChannelMemorySystem(const string &dev, const string &sys, const string &pwd, const string &trc, unsigned megsOfMemory, const string *visFilename=NULL, const IniReader::OverrideMap *paramOverrides=NULL);
		virtual ~MultiChannelMemorySystem();
			bool addTransaction(Transaction *trans);
			bool addTransaction(const Transaction &trans);
			bool addTransaction(bool isWrite, uint64_t addr);
			bool willAcceptTransaction(); 
			bool willAcceptTransaction(uint64_t addr); 
			//Added by Yihan, willacceptTransaction for write
			bool willAcceptWriteTransaction(uint64_t addr);
			void update();
			void printStats(bool finalStats=false);
			ostream &getLogFile();
			void RegisterCallbacks( 
				TransactionCompleteCB *readDone,
				TransactionCompleteCB *writeDone,
				void (*reportPower)(double bgpower, double burstpower, double refreshpower, double actprepower));
			int getIniBool(const std::string &field, bool *val);
			int getIniUint(const std::string &field, unsigned int *val);
			int getIniUint64(const std::string &field, uint64_t *val);
			int getIniFloat(const std::string &field, float *val);

	void InitOutputFiles(string tracefilename);
	void setCPUClockSpeed(uint64_t cpuClkFreqHz);
	//3/13
//	void unpause_read_redirect_packet(unsigned release_rank);
//	void unpause_read_redirect_packet(unsigned chan, unsigned release_rank);
//	void unpause_read_redirect_packet();
	//output file
	std::ofstream visDataOut;
	ofstream dramsim_log; 

	//Added by Yihan WB cache typedef
//	typedef vector<CpuRequest *> SetVector;
//	typedef vector<SetVector> WriteBufferVector;
	//added by yihan to handle stalled read
 	void flush_stalled_read(uint64_t addr);	
 //	vector<uint64_t> read_redirect_buffer;
	private:
		unsigned findChannelNumber(uint64_t addr);
		void actual_update(); 
		vector<MemorySystem*> channels;
		//Yihan: add write buffer cache
//		WriteBufferVector  wb_cache;
		unsigned num_sets;
//		bool wb_cache_insert_candidate(uint64_t addr);
//		void getnextSet(unsigned &Set);
		//3/14
//		unsigned nextSet;
//		vector<unsigned> nextSet;
		//3/13 design change
//		vector<CpuRequest*> &getSet(unsigned chan, unsigned set);
//		vector<CpuRequest*> &getSet(unsigned set);
//		bool wb_cache_can_accept(uint64_t addr);
		//Yihan: new design function
//		void  wb_cache_batch_drain_rank_candidate(int pair_set);
//		void  wb_cache_batch_drain_rank_candidate(unsigned chan, int pair_set);
//		vector<unsigned> Write_In_Waiting_For_Each_Rank;
//		unsigned get_writes_to_rank_X(unsigned i);
//		unsigned get_writes_to_rank_X(unsigned chan,unsigned i);
//		void Inspect_all_rank_for_drain_entrance_condition();
//		void Inspect_all_rank_for_end_drain_condition();
//		unsigned drain_counter;
//		int draining_rank;
		//3/13 design change
//		vector<int> draining_rank;
//		unsigned transqueue_full;
//		vector<unsigned>num_force_drain;
		unsigned num_enqueue_failed;
		//3/13 
//		void reinsert_write_packet(int draining_rank);
//		void reinsert_write_packet(unsigned chan, int draining_rank);
//		unsigned reinsert_cache_full_stats;
//		void sync_wb_cache();
//		unsigned num_cache_synced_stats;
//		bool can_cache_accept_drained_write(uint64_t addr);
//		bool Does_Queue_Have_Space_For_Bank_X(unsigned rank, unsigned bank);
//		bool Does_Queue_Have_Space_For_Bank_X(unsigned chan, unsigned rank, unsigned bank);
		//3/13 design change
//		unsigned drain_cap;
//		vector<unsigned> drain_cap;
//		void print_wb_cache();
		//3/13 
//		unsigned calculate_channel_rank_location(unsigned chan, unsigned rank);
//		unsigned num_replace;
//		unsigned total_set;
		//3/27 
//		vector<unsigned> dynamic_resume_threshold;
//		unsigned number_of_writes_in_wb_for_rank_X(unsigned target_chan, unsigned search_rank);
//		vector<unsigned> drain_force_redirect;
		//end addition
		unsigned megsOfMemory; 
		string deviceIniFilename;
		string systemIniFilename;
		string traceFilename;
		string pwd;
		const string *visFilename;
		ClockDomain::ClockDomainCrosser clockDomainCrosser; 
		static void mkdirIfNotExist(string path);
		static bool fileExists(string path); 
		CSVWriter *csvOut; 
//		unsigned num_instant_write_complete;
//		vector<unsigned> num_heartbeat_violation;
	};
}
