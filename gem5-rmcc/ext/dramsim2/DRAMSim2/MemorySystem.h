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



#ifndef MEMORYSYSTEM_H
#define MEMORYSYSTEM_H

//MemorySystem.h
//
//Header file for JEDEC memory system wrapper
//

#include "SimulatorObject.h"
#include "SystemConfiguration.h"
#include "MemoryController.h"
#include "Rank.h"
#include "Transaction.h"
#include "Callback.h"
#include "CSVWriter.h"
#include "CpuRequest.h"
#include <deque>

namespace DRAMSim
{
typedef CallbackBase<void,unsigned,uint64_t,uint64_t> Callback_t;
class MemorySystem : public SimulatorObject
{
	ostream &dramsim_log;
public:
	//functions
	MemorySystem(unsigned id, unsigned megsOfMemory, CSVWriter &csvOut_, ostream &dramsim_log_);
	virtual ~MemorySystem();
	void update();
	bool addTransaction(Transaction *trans);
	bool addTransaction(bool isWrite, uint64_t addr);
	bool flush_stalled_read_in_pendingTrans(uint64_t addr);
    unsigned pendingTransQueueUsedSpace(unsigned rank);
    unsigned pendingTransQueueUsedSpace(unsigned rank,unsigned bank);
    uint64_t FindPendingTransQueueWrite(unsigned rank);
	void printStats(bool finalStats);
	bool WillAcceptTransaction();
	void RegisterCallbacks(
	    Callback_t *readDone,
	    Callback_t *writeDone,
	    void (*reportPower)(double bgpower, double burstpower, double refreshpower, double actprepower));

	//fields
	MemoryController *memoryController;
	vector<Rank *> *ranks;
	deque<Transaction *> pendingTransactions; 


	//function pointers
	Callback_t* ReturnReadData;
	Callback_t* WriteDataDone;
	//TODO: make this a functor as well?
	static powerCallBack_t ReportPower;
	unsigned systemID;
	
	//mc design declaration
	typedef vector<CpuRequest *> SetVector;
	typedef vector<SetVector> WriteBufferVector;
	WriteBufferVector  wb_cache;
	unsigned num_sets;
	unsigned nextSet;
	vector<unsigned> Write_In_Waiting_For_Each_Rank;
	vector<uint64_t> read_redirect_buffer;
	int draining_rank;
	unsigned drain_cap;
	unsigned dynamic_resume_threshold;
	bool can_cache_accept_drained_write(uint64_t addr);
	void Inspect_all_rank_for_end_drain_condition();
	void Inspect_all_rank_for_drain_entrance_condition();
	unsigned get_writes_to_rank_X(unsigned i);
	bool wb_cache_insert_candidate(uint64_t addr);
	void wb_cache_batch_drain_rank_candidate(int draining_rank);
	void getnextSet(unsigned &Set);
	vector<CpuRequest*> &getSet(unsigned set);
	void flush_stalled_read(uint64_t addr); 
	bool flush_stalled_read_in_read_redirect_buffer(uint64_t addr);
	bool handle_cpu_request(bool isWrite, uint64_t addr);
	bool wb_cache_can_accept(uint64_t addr);
	void unpause_read_redirect_packet(unsigned rank);
	void reinsert_write_packet(int rank);
	void sync_wb_cache();
	bool Does_Queue_Have_Space_For_Bank_X(unsigned rank, unsigned bank);
	//statistic variable
	unsigned num_force_drain;
	unsigned num_replace;
	unsigned max_min_same_rank;
	unsigned drain_force_redirect;
	unsigned reinsert_cache_full_stats;
	unsigned num_cache_synced_stats;
	unsigned num_heartbeat_violation;

private:
	CSVWriter &csvOut;
};
}

#endif

