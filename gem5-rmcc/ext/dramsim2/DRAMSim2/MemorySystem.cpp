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




//MemorySystem.cpp
//
//Class file for JEDEC memory system wrapper
//

#include "MemorySystem.h"
#include "IniReader.h"
#include <unistd.h>
#include "AddressMapping.h"
using namespace std;


ofstream cmd_verify_out; //used in Rank.cpp and MemoryController.cpp if VERIFICATION_OUTPUT is set

unsigned NUM_DEVICES;
unsigned NUM_RANKS;
unsigned NUM_RANKS_LOG;

namespace DRAMSim {

	powerCallBack_t MemorySystem::ReportPower = NULL;

	MemorySystem::MemorySystem(unsigned id, unsigned int megsOfMemory, CSVWriter &csvOut_, ostream &dramsim_log_) :
		dramsim_log(dramsim_log_),
		ReturnReadData(NULL),
		WriteDataDone(NULL),
		systemID(id),
		csvOut(csvOut_)
	{
		currentClockCycle = 0;

		DEBUG("===== MemorySystem "<<systemID<<" =====");


		//calculate the total storage based on the devices the user selected and the number of

		//calculate number of devices
		/************************
		  This code has always been problematic even though it's pretty simple. I'll try to explain it 
		  for my own sanity. 

		  There are two main variables here that we could let the user choose:
		  NUM_RANKS or TOTAL_STORAGE.  Since the density and width of the part is
		  fixed by the device ini file, the only variable that is really
		  controllable is the number of ranks. Users care more about choosing the
		  total amount of storage, but with a fixed device they might choose a total
		  storage that isn't possible. In that sense it's not as good to allow them
		  to choose TOTAL_STORAGE (because any NUM_RANKS value >1 will be valid).

		  However, users don't care (or know) about ranks, they care about total
		  storage, so maybe it's better to let them choose and just throw an error
		  if they choose something invalid. 

		  A bit of background: 

		  Each column contains DEVICE_WIDTH bits. A row contains NUM_COLS columns.
		  Each bank contains NUM_ROWS rows. Therefore, the total storage per DRAM device is: 
		  PER_DEVICE_STORAGE = NUM_ROWS*NUM_COLS*DEVICE_WIDTH*NUM_BANKS (in bits)

		  A rank *must* have a 64 bit output bus (JEDEC standard), so each rank must have:
		  NUM_DEVICES_PER_RANK = 64/DEVICE_WIDTH  
		  (note: if you have multiple channels ganged together, the bus width is 
		  effectively NUM_CHANS * 64/DEVICE_WIDTH)

		  If we multiply these two numbers to get the storage per rank (in bits), we get:
		  PER_RANK_STORAGE = PER_DEVICE_STORAGE*NUM_DEVICES_PER_RANK = NUM_ROWS*NUM_COLS*NUM_BANKS*64 

		  Finally, to get TOTAL_STORAGE, we need to multiply by NUM_RANKS
		  TOTAL_STORAGE = PER_RANK_STORAGE*NUM_RANKS (total storage in bits)

		  So one could compute this in reverse -- compute NUM_DEVICES,
		  PER_DEVICE_STORAGE, and PER_RANK_STORAGE first since all these parameters
		  are set by the device ini. Then, TOTAL_STORAGE/PER_RANK_STORAGE = NUM_RANKS 

		  The only way this could run into problems is if TOTAL_STORAGE < PER_RANK_STORAGE,
		  which could happen for very dense parts.
		 *********************/

		// number of bytes per rank
		unsigned long megsOfStoragePerRank = ((((long long)NUM_ROWS * (NUM_COLS * DEVICE_WIDTH) * NUM_BANKS) * ((long long)JEDEC_DATA_BUS_BITS / DEVICE_WIDTH)) / 8) >> 20;

		// If this is set, effectively override the number of ranks
		if (megsOfMemory != 0)
		{
			NUM_RANKS = megsOfMemory / megsOfStoragePerRank;
			NUM_RANKS_LOG = dramsim_log2(NUM_RANKS);
			if (NUM_RANKS == 0)
			{
				PRINT("WARNING: Cannot create memory system with "<<megsOfMemory<<"MB, defaulting to minimum size of "<<megsOfStoragePerRank<<"MB");
				NUM_RANKS=1;
			}
		}

		NUM_DEVICES = JEDEC_DATA_BUS_BITS/DEVICE_WIDTH;
		TOTAL_STORAGE = (NUM_RANKS * megsOfStoragePerRank); 

		DEBUG("CH. " <<systemID<<" TOTAL_STORAGE : "<< TOTAL_STORAGE << "MB | "<<NUM_RANKS<<" Ranks | "<< NUM_DEVICES <<" Devices per rank");


		memoryController = new MemoryController(this, csvOut, dramsim_log);

		// TODO: change to other vector constructor?
		ranks = new vector<Rank *>();

		for (size_t i=0; i<NUM_RANKS; i++)
		{
			Rank *r = new Rank(dramsim_log);
			r->setId(i);
			r->attachMemoryController(memoryController);
			ranks->push_back(r);
		}

		memoryController->attachRanks(ranks);
		
		//mc design
		//Initiate wb cache
		SetVector one_set;
		wb_cache = WriteBufferVector();
		num_sets=(WB_CACHE_TOTALCAPACITY*1024)/(64*WB_CACHE_ASSOCIATIVITY);
		for(size_t i=0; i<num_sets; i++)
		{
			one_set = SetVector();
			wb_cache.push_back(one_set);
		}
		//start : changed to single channel design
		nextSet=0;
		Write_In_Waiting_For_Each_Rank=vector<unsigned>(NUM_RANKS,0);
		read_redirect_buffer=vector<uint64_t>();
		read_redirect_buffer.reserve(1000);
		draining_rank=-1;
		drain_cap=0;
		dynamic_resume_threshold=0;
		//end
		//statistic variable
		num_force_drain=0;
		num_replace=0;
		max_min_same_rank=0;
		drain_force_redirect=0;
		reinsert_cache_full_stats=0;
		num_cache_synced_stats=0;
		num_heartbeat_violation=0;
	}



	MemorySystem::~MemorySystem()
	{
		/* the MemorySystem should exist for all time, nothing should be destroying it */  
		//	ERROR("MEMORY SYSTEM DESTRUCTOR with ID "<<systemID);
		//	abort();

		delete(memoryController);

		for (size_t i=0; i<NUM_RANKS; i++)
		{
			delete (*ranks)[i];
		}
		ranks->clear();
		delete(ranks);

		if (VERIFICATION_OUTPUT)
		{
			cmd_verify_out.flush();
			cmd_verify_out.close();
		}
	}

	bool MemorySystem::WillAcceptTransaction()
	{
		return memoryController->WillAcceptTransaction();
	}

	bool MemorySystem::addTransaction(bool isWrite, uint64_t addr) //original
	{
		//Yihan add detection if it is a data write then it is wrong
		TransactionType type = isWrite ? DATA_WRITE : DATA_READ;
		Transaction *trans = new Transaction(type,addr,NULL);

		if (memoryController->WillAcceptTransaction())
		{
			return memoryController->addTransaction(trans);
		}
		else
		{
			pendingTransactions.push_back(trans);
			return true;
		}
	}
	bool MemorySystem::addTransaction(Transaction *trans)
	{
		return memoryController->addTransaction(trans);
	}

	//prints statistics
	void MemorySystem::printStats(bool finalStats)
	{
		memoryController->printStats(finalStats);
	}


	//update the memory systems state
	void MemorySystem::update()
	{
		if(experimentScheme==Baseline)
		{
			Inspect_all_rank_for_drain_entrance_condition();
			Inspect_all_rank_for_end_drain_condition();
			if(draining_rank>-1)
			{
				wb_cache_batch_drain_rank_candidate(draining_rank);
			}
		}

		//PRINT(" ----------------- Memory System Update ------------------");

		//updates the state of each of the objects
		// NOTE - do not change order
		for (size_t i=0;i<NUM_RANKS;i++)
		{
			(*ranks)[i]->update();
		}

		//pendingTransactions will only have stuff in it if MARSS is adding stuff
		if (pendingTransactions.size() > 0 && memoryController->WillAcceptTransaction())
		{
			memoryController->addTransaction(pendingTransactions.front());
			pendingTransactions.pop_front();
		}
		memoryController->update();

		//simply increments the currentClockCycle field for each object
		for (size_t i=0;i<NUM_RANKS;i++)
		{
			(*ranks)[i]->step();
		}
		memoryController->step();
		this->step();

		//PRINT("\n"); // two new lines
	}

	void MemorySystem::RegisterCallbacks( Callback_t* readCB, Callback_t* writeCB,
			void (*reportPower)(double bgpower, double burstpower,
				double refreshpower, double actprepower))
	{
		ReturnReadData = readCB;
		WriteDataDone = writeCB;
		ReportPower = reportPower;
	}

	//Nov 30
	unsigned MemorySystem::pendingTransQueueUsedSpace(unsigned rank)
	{
		unsigned schan, srank, sbank, scol, srow;   
		unsigned used_space=0;
		for(size_t i=0; i <pendingTransactions.size(); i++)
		{
			Transaction *transaction = pendingTransactions[i];
			addressMapping(transaction->address,schan,srank,sbank,srow,scol);
			if(srank==rank)
			{
				used_space++;
			}
		}
		return used_space;
	}
	unsigned MemorySystem::pendingTransQueueUsedSpace(unsigned rank, unsigned bank)
	{
		unsigned schan, srank, sbank, scol, srow;   
		unsigned used_space=0;
		for(size_t i=0; i <pendingTransactions.size(); i++)
		{
			Transaction *transaction = pendingTransactions[i];
			addressMapping(transaction->address,schan,srank,sbank,srow,scol);
			if(srank==rank && sbank==bank)
			{
				used_space++;
			}
		}
		return used_space;
	}
	uint64_t MemorySystem::FindPendingTransQueueWrite(unsigned rank)
	{
		uint64_t ret_addr=0;
		unsigned schan, srank, sbank, scol, srow;   
		for(size_t i=0; i <pendingTransactions.size(); i++)
		{
			Transaction *transaction = pendingTransactions[i]; 
			addressMapping(transaction->address,schan,srank,sbank,srow,scol);
			BusPacketType bpType = transaction->getBusPacketType();
			if(bpType == WRITE || bpType == WRITE_P)
			{
				if(srank==rank)
				{ 
					ret_addr=transaction->address;
					pendingTransactions.erase(pendingTransactions.begin()+i);
					break;
				}
			}
		}
		return ret_addr;

	}
	bool MemorySystem::flush_stalled_read_in_pendingTrans(uint64_t addr)
	{
		for(size_t i=0; i<pendingTransactions.size(); i++)
		{
			Transaction *transaction = pendingTransactions[i];
			if(transaction->address==addr)
			{
				pendingTransactions.erase(pendingTransactions.begin()+i);
				return true;
			}
		}
		return false;
	}
	bool MemorySystem::flush_stalled_read_in_read_redirect_buffer(uint64_t addr)
	{
		for(size_t i=0; i<read_redirect_buffer.size(); i++)
		{
			if(read_redirect_buffer[i]==addr)
			{
				read_redirect_buffer.erase(read_redirect_buffer.begin()+i);
				return true;
			}
		}
		return false;
	}
	//////////////////////////////////////////////mc design start
	//check
	bool MemorySystem::can_cache_accept_drained_write(uint64_t addr)
	{
		unsigned chan, rank, bank, row, col;
		addressMapping(addr,chan,rank,bank,row,col);
		unsigned dest_set = bank % num_sets;
		vector<CpuRequest *> &current_set = getSet(dest_set);
		if(current_set.size() < WB_CACHE_ASSOCIATIVITY){    return true; }
		else{	return false;}
	}
	//check
	void MemorySystem::Inspect_all_rank_for_end_drain_condition()
	{
		if(draining_rank> -1)
		{
			if(memoryController->isRankRefreshing(draining_rank) ||	memoryController->write_complete>=dynamic_resume_threshold)
			{
				reinsert_write_packet(draining_rank);
				unpause_read_redirect_packet(draining_rank);
				drain_cap=0;
				draining_rank=-1;
				memoryController->write_complete=0;
			}
		}
	}
	//check
	unsigned MemorySystem::get_writes_to_rank_X(unsigned rank)
	{
		if(!memoryController->isRankRefreshing(rank))
		{
			if(Write_In_Waiting_For_Each_Rank[rank] >= THRESHOLD)
			{
				return Write_In_Waiting_For_Each_Rank[rank];
			}
		}
		return 0;
	}
	//check
	void MemorySystem::Inspect_all_rank_for_drain_entrance_condition()
	{
		unsigned max_write=0;
		int top_rank=-1;
		if(draining_rank==-1)
		{
			for(unsigned i=0; i<NUM_RANKS; i++)
			{
				unsigned cur_threshold = get_writes_to_rank_X(i);
				if(cur_threshold > THRESHOLD && cur_threshold > max_write)
				{
					max_write=cur_threshold;
					top_rank=i;
				}
			}
		}
		if(top_rank >-1)
		{
			draining_rank=top_rank;
			memoryController->write_complete=0;
			dynamic_resume_threshold=THRESHOLD*DRAIN_PERCENTAGE;
		}
	}
	//check
	bool MemorySystem::wb_cache_insert_candidate(uint64_t addr)
	{
		bool is_hit=false;
		bool insert_successful=false;
		unsigned chan,rank,bank,row,col;
		addressMapping(addr, chan, rank, bank, row, col);
		unsigned dest_set = bank % num_sets;
		unsigned cur_chan,cur_rank,cur_bank,cur_row,cur_col;
		vector<CpuRequest *> &current_set = getSet(dest_set);
		if(current_set.size() > WB_CACHE_ASSOCIATIVITY)
		{
			ERROR("wb cache insert full");
			exit(1);
		}
		else
		{
			for(int i=0; i<current_set.size(); i++)
			{
				if(current_set[i]->physicalAddress==addr)
				{
					is_hit=true;
					insert_successful=true;
					num_replace++;
					break;
				}
			}
			if(!is_hit)
			{
				CpuRequest *incoming_request = new CpuRequest(addr);
				vector<CpuRequest *>::iterator it;
				unsigned counter=0;
				for(it=current_set.begin(); it<current_set.end(); it++)
				{
					CpuRequest *curRequest = *it;
					addressMapping(curRequest->physicalAddress,cur_chan,cur_rank,cur_bank,cur_row,cur_col);
					if(cur_row==row && cur_rank == rank && cur_bank == bank)
					{
						counter++;
					}
				}
				if(counter==0)
				{
					current_set.push_back(incoming_request);
				}
				else
				{
					vector<CpuRequest *>::iterator mit;
					for(mit=current_set.begin(); mit<current_set.end(); mit++)
					{
						CpuRequest* travRequest = *mit;
						addressMapping(travRequest->physicalAddress,cur_chan,cur_rank,cur_bank,cur_row,cur_col);
						if(cur_row==row && cur_rank==rank && cur_bank==bank)
						{
							counter--;
						}
						if(counter==0)
						{
							current_set.insert(mit+1,incoming_request);
							break;
						}
					}
				}
				insert_successful=true;
				Write_In_Waiting_For_Each_Rank[rank]++;	
			}
		}
		return insert_successful;
	}
	//check
	void MemorySystem::wb_cache_batch_drain_rank_candidate(int draining_rank)
	{
		unsigned startingSet = nextSet;
		unsigned chan,rank,bank,row,col;
		bool found_issuable_candidate=false;
		do
		{
			vector<CpuRequest *> &current_set = getSet(nextSet);
			for(int i=0; i<current_set.size(); i++)
			{
				addressMapping(current_set[i]->physicalAddress, chan, rank, bank, row, col);
				if(draining_rank!=rank)
				{
					continue;
				}
				if(Does_Queue_Have_Space_For_Bank_X(draining_rank,bank))
				{
					found_issuable_candidate=true;
					Write_In_Waiting_For_Each_Rank[draining_rank]--;
					addTransaction(true,current_set[i]->physicalAddress);
					current_set.erase(current_set.begin()+i);
					break;
				}
			}
			if(found_issuable_candidate)
			{
				if(drain_cap>0)
				{
					drain_cap--;
					if(drain_cap==0)
					{
						getnextSet(nextSet);
					}
				}
				else if(drain_cap==0)
				{
					unsigned found_same_row=0;
					unsigned zchan,zrank,zbank,zrow,zcol;
					for(int j=0; j<current_set.size(); j++)
					{
						addressMapping(current_set[j]->physicalAddress,zchan,zrank,zbank,zrow,zcol);
						if(draining_rank==zrank && bank==zbank && row==zrow)
						{
							found_same_row++;
						}
					}
					if(found_same_row==0)
					{
						getnextSet(nextSet);
					}
					else
					{
						if(found_same_row < DRAIN_CAP_THRESHOLD)
						{
							drain_cap = found_same_row;
						}
						else
						{
							drain_cap = DRAIN_CAP_THRESHOLD;
						}
					}
				}
				break;
			}
			getnextSet(nextSet);
		}while(startingSet != nextSet);
	}
	//check
	void MemorySystem::getnextSet(unsigned &Set)
	{
		Set++;
		if(Set==num_sets)
		{
			Set=0;
		}
	}
	//should not matter with result
	vector<CpuRequest *> &MemorySystem::getSet(unsigned set)
	{
		return wb_cache[set];
	}
	//should not matter with result
	void MemorySystem::flush_stalled_read(uint64_t addr)
	{
		num_heartbeat_violation++;
		if(!flush_stalled_read_in_read_redirect_buffer(addr))
		{
			if(!flush_stalled_read_in_pendingTrans(addr))
			{
				memoryController->flush_pending_read(addr);
			}
		}
	}
	//this is essentailly addTransaction(bool isWrite, uint64_t addr) in multichanel
	//check
	bool MemorySystem::handle_cpu_request(bool isWrite, uint64_t addr)
	{
		if(isWrite)
		{
			return wb_cache_insert_candidate(addr);
		}
		else
		{
			unsigned chan,rank,bank,row,col;
			addressMapping(addr,chan,rank,bank,row,col);
			if(draining_rank==rank)
			{
				read_redirect_buffer.push_back(addr);
				return true;
			}
		}
		return addTransaction(isWrite,addr);
	}
	//check
	bool MemorySystem::wb_cache_can_accept(uint64_t addr)
	{
		unsigned chan,rank,bank,row,col;
		addressMapping(addr,chan,rank,bank,row,col);
		unsigned dest_set = bank % num_sets;
		vector<CpuRequest *> &current_set = getSet(dest_set);
		if(current_set.size() < (WB_CACHE_ASSOCIATIVITY*3/4))
		{
			return true;
		}
		else
		{
			unsigned xchan,xrank,xbank,xrow,xcol;
			vector<unsigned> num_write = vector<unsigned>(NUM_RANKS, 0);
			for(int i=0; i<current_set.size(); i++)
			{
				addressMapping(current_set[i]->physicalAddress,xchan,xrank,xbank,xrow,xcol);
				num_write[xrank]++;
			}
			int max_rank=-1;
			unsigned max_write=0;
			for(int j=0; j<num_write.size(); j++)
			{
				if(num_write[j]>max_write)
				{
					max_write=num_write[j];
					max_rank=j;
				}
			}
			if(max_rank <0)
			{
				ERROR("max write cannot be less than 0");
				exit(1);
			}
			int min_rank=max_rank;
			unsigned min_write=max_write;
			for(int k=0; k<num_write.size(); k++)
			{
				if(num_write[k]<min_write)
				{
					min_write=num_write[k];
					min_rank=k;
				}
			}
			if(min_rank <0)
			{
				ERROR("min write cannot be less than 0");
				exit(1);
			}

			if(max_rank==min_rank)
			{
				max_min_same_rank++;
			}
			if(draining_rank==-1)
			{
				nextSet=dest_set;
				drain_cap=0;
				draining_rank=max_rank;
				dynamic_resume_threshold=max_write*DRAIN_PERCENTAGE;
				if(current_set.size() > WB_CACHE_ASSOCIATIVITY)
				{
					return false;
				}else
				{
					return true;
				}
			}
			else if(draining_rank==max_rank)
			{
				nextSet=dest_set;
				drain_cap=0;
				drain_force_redirect++;
				if(current_set.size() > WB_CACHE_ASSOCIATIVITY)
				{
					return false;
				}else
				{
					return true;
				}
			}
			else
			{
				for(int l =0; l<current_set.size(); l++)
				{
					addressMapping(current_set[l]->physicalAddress,xchan,xrank,xbank,xrow,xcol);
					if(xrank==min_rank)
					{
						addTransaction(true,current_set[l]->physicalAddress);
						Write_In_Waiting_For_Each_Rank[xrank]--;
						current_set.erase(current_set.begin()+l);
						num_force_drain++;
						break;
					}
				}
				if(current_set.size() > WB_CACHE_ASSOCIATIVITY)
				{
					return false;
				}else
				{
					return true;
				}
			}
		}
		return true;
	}
	//check
	void MemorySystem::unpause_read_redirect_packet(unsigned release_rank)
	{
		unsigned chan,rank,bank,row,col;
		unsigned num_released=0;
		vector<uint64_t>::iterator it=read_redirect_buffer.begin();
		while(it!=read_redirect_buffer.end())
		{
			addressMapping(*it,chan,rank,bank,row,col);
			if(rank==release_rank)
			{
				num_released++;
				addTransaction(false,*it);
				read_redirect_buffer.erase(it);
			}
			else
			{
				++it;
			}
		}
		if(DEBUG_NON_BLOCKING_REFRESH)
		{
			cout<<"released : "<<num_released<<" packets"<<endl;
		}
	}
	//possible location
	void MemorySystem::reinsert_write_packet(int rank)
	{
		uint64_t ret_addr=0;
		do
		{
			ret_addr=memoryController->FindOutstandingWrite(rank);

			if(ret_addr==0)
			{
				ret_addr=FindPendingTransQueueWrite(rank);
			}
			if(ret_addr!=0)
			{
				if(can_cache_accept_drained_write(ret_addr))
				{
					wb_cache_insert_candidate(ret_addr);
				}
				else
				{
					addTransaction(true,ret_addr);
					reinsert_cache_full_stats++;
					break;
				}
			}
		}while(ret_addr!=0);
		sync_wb_cache();
	}
	
	//check
	void MemorySystem::sync_wb_cache()
	{
		unsigned counter=0;
		unsigned chan,rank,bank,row,col;
		for(int i=0; i<NUM_RANKS; i++)
		{
			counter=0;
			for(int j=0; j<num_sets; j++)
			{
				vector<CpuRequest *> &current_set = getSet(j);
				for(int k=0; k<current_set.size(); k++)
				{
					addressMapping(current_set[k]->physicalAddress,chan,rank,bank,row,col);
					if(i==rank)
					{
						counter++;
					}
				}
			}
			if(Write_In_Waiting_For_Each_Rank[i] != counter)
			{
				num_cache_synced_stats++;
				Write_In_Waiting_For_Each_Rank[i]=counter;
			}
		}
	}
	//check
	bool MemorySystem::Does_Queue_Have_Space_For_Bank_X(unsigned rank, unsigned bank)
	{
		if(memoryController->TransQueueUsedSpace(rank)<TRANSACTION_QUEUE_WRITE_MAX_USAGE)
		{
			if(memoryController->queueFreeSpace(rank,bank) >= ((CMD_QUEUE_DEPTH/3)+
				(memoryController->num_transqueue_drain_write_for_bank_X(rank,bank)*2) +
				(pendingTransQueueUsedSpace(rank,bank)*2)))
			{
				return true;
			}
		}
		return false;
	}
	
} /*namespace DRAMSim */



// This function can be used by autoconf AC_CHECK_LIB since
// apparently it can't detect C++ functions.
// Basically just an entry in the symbol table
extern "C"
{
	void libdramsim_is_present(void)
	{
		;
	}
}

