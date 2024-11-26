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
#include <errno.h> 
#include <sstream> //stringstream
#include <stdlib.h> // getenv()
// for directory operations 
#include <sys/stat.h>
#include <sys/types.h>

#include "MultiChannelMemorySystem.h"
#include "AddressMapping.h"
#include "IniReader.h"



using namespace DRAMSim; 


MultiChannelMemorySystem::MultiChannelMemorySystem(const string &deviceIniFilename_, const string &systemIniFilename_, const string &pwd_, const string &traceFilename_, unsigned megsOfMemory_, const string *visFilename_, const IniReader::OverrideMap *paramOverrides)
	:megsOfMemory(megsOfMemory_), deviceIniFilename(deviceIniFilename_),
	systemIniFilename(systemIniFilename_), traceFilename(traceFilename_),
	pwd(pwd_), visFilename(visFilename_),
	clockDomainCrosser(new ClockDomain::Callback<MultiChannelMemorySystem, void>(this, &MultiChannelMemorySystem::actual_update)),
	csvOut(new CSVWriter(visDataOut))
{
	currentClockCycle=0; 
	//3/14
//	nextSet=vector<unsigned>(NUM_CHANS,0);
	if (visFilename)
		printf("CC VISFILENAME=%s\n",visFilename->c_str());

	if (!isPowerOfTwo(megsOfMemory))
	{
		ERROR("Please specify a power of 2 memory size"); 
		abort(); 
	}

	if (pwd.length() > 0)
	{
		//ignore the pwd argument if the argument is an absolute path
		if (deviceIniFilename[0] != '/')
		{
			deviceIniFilename = pwd + "/" + deviceIniFilename;
		}

		if (systemIniFilename[0] != '/')
		{
			systemIniFilename = pwd + "/" + systemIniFilename;
		}
	}

	DEBUG("== Loading device model file '"<<deviceIniFilename<<"' == ");
	IniReader::ReadIniFile(deviceIniFilename, false);
	DEBUG("== Loading system model file '"<<systemIniFilename<<"' == ");
	IniReader::ReadIniFile(systemIniFilename, true);

	// If we have any overrides, set them now before creating all of the memory objects
	if (paramOverrides)
		IniReader::OverrideKeys(paramOverrides);

	IniReader::InitEnumsFromStrings();
	if (!IniReader::CheckIfAllSet())
	{
		exit(-1);
	}

	if (NUM_CHANS == 0) 
	{
		ERROR("Zero channels"); 
		abort(); 
	}
	for (size_t i=0; i<NUM_CHANS; i++)
	{
		MemorySystem *channel = new MemorySystem(i, megsOfMemory/NUM_CHANS, (*csvOut), dramsim_log);
		channels.push_back(channel);
	}

	//Yihan: reserve 64Kb writebuffercache
	num_sets=(WB_CACHE_TOTALCAPACITY*1024)/(64*WB_CACHE_ASSOCIATIVITY);
	cout<<"wb cache total capacity: "<<WB_CACHE_TOTALCAPACITY <<"Kb wb cache total associativity "<<WB_CACHE_ASSOCIATIVITY<<"\n";
	cout<<"number of sets: "<<num_sets<<"\n";
	//just check for number of channels
	//initial a set of all NULL in the beginning to all cache lines
	//UPDATE FOLLOW THE SAME STYLE as commandqueeue.cpp
//	SetVector one_set;
//	wb_cache = WriteBufferVector();
	//3/13 multi channel design now we have x*num_set total for x number of channels
	//for(size_t i =0; i<num_sets; i++){
//	total_set = num_sets * NUM_CHANS;
//	cout<<"num of channels: "<<NUM_CHANS<<endl;
//	cout<<"total set: "<<total_set<<endl;
/*	for(size_t i =0; i<total_set; i++){
		//push cache_line to each set
		one_set = SetVector();
		//cout<<"pushed a vector of size: "<<one_set.size()<<endl;
		wb_cache.push_back(one_set);
	}
	cout<<"wb cache size: "<<wb_cache.size()<<endl;
	//Yihan: Initiate all the variables we needed
	//3/13 design change number 2
	Write_In_Waiting_For_Each_Rank=vector<unsigned>(NUM_RANKS*NUM_CHANS,0);
	//	Write_In_Waiting_For_Each_Rank=vector<unsigned>(NUM_RANKS,0);
	read_redirect_buffer=vector<uint64_t>();
	//3/13 design change 
	//	read_redirect_buffer.reserve(1000);
	read_redirect_buffer.reserve(1000*NUM_CHANS);
	//3/13 design change
	//	draining_rank=-1;
	//	drain_cap=0;
	draining_rank=vector<int>(NUM_CHANS,-1);
	drain_cap=vector<unsigned>(NUM_CHANS,0);

	//3/27 percentage draining
	dynamic_resume_threshold=vector<unsigned>(NUM_CHANS,0);
	drain_force_redirect = vector<unsigned>(NUM_CHANS,0);
	//statistical parameter
	reinsert_cache_full_stats=0;
	num_force_drain=vector<unsigned>(NUM_CHANS,0);
	num_replace=0;*/
	num_enqueue_failed=0;
//	transqueue_full=0;
//	num_instant_write_complete=0;
//	num_heartbeat_violation=vector<unsigned>(NUM_CHANS,0);

	/* Initialize the ClockDomainCrosser to use the CPU speed 
	   If cpuClkFreqHz == 0, then assume a 1:1 ratio (like for TraceBasedSim)
	 */
}

//ulitiy to show the wb cache
/*void MultiChannelMemorySystem::print_wb_cache()
{
	for(int i=0; i<NUM_CHANS; i++)
	{
		cout<<"channel : "<<i<<endl;
		unsigned chan,rank,bank,row,col;
		std::map<unsigned, unsigned> unique_ranks;
		for(int s=0; s<num_sets; s++){
			//3/13
			vector<CpuRequest *> &print_set =getSet(i,s);
			cout<<"set "<<s <<" contains: "<<endl;
			std::map<unsigned, unsigned>unique_rows;
			for(int t=0; t<print_set.size(); t++){
				addressMapping(print_set[t]->physicalAddress,chan,rank,bank,row,col);
				cout<<"addr: "<< std::hex<< print_set[t]->physicalAddress<<std::dec;
				cout<<" rank: "<<rank<<" bank: "<<bank<<" row: "<<row<<endl;
				unique_rows[row]++;
				unique_ranks[rank]++;
			}
			cout<<"set: "<<s <<" have these unique row: "<<endl;
			for(std::map<unsigned, unsigned>::iterator it=unique_rows.begin(); it!=unique_rows.end(); ++it)
			{
				cout<<"row "<< it->first<<" : "<< it->second<<endl;
			}
		}
		cout<<"unique rank: "<<endl;
		for(std::map<unsigned, unsigned>::iterator rit=unique_ranks.begin(); rit!=unique_ranks.end(); ++rit)
		{
			cout<<"rank "<< rit->first<<" : "<< rit->second<<endl;
		}
	}
}
*/

void MultiChannelMemorySystem::setCPUClockSpeed(uint64_t cpuClkFreqHz)
{

	uint64_t dramsimClkFreqHz = (uint64_t)(1.0/(tCK*1e-9));
	clockDomainCrosser.clock1 = dramsimClkFreqHz; 
	clockDomainCrosser.clock2 = (cpuClkFreqHz == 0) ? dramsimClkFreqHz : cpuClkFreqHz; 
}

bool fileExists(string &path)
{
	struct stat stat_buf;
	if (stat(path.c_str(), &stat_buf) != 0) 
	{
		if (errno == ENOENT)
		{
			return false; 
		}
		ERROR("Warning: some other kind of error happened with stat(), should probably check that"); 
	}
	return true;
}
/*
bool MultiChannelMemorySystem::can_cache_accept_drained_write(uint64_t addr)
{
	unsigned schan, srank, sbank, srow, scol;
	addressMapping(addr, schan, srank, sbank, srow, scol);
	unsigned dest_set = sbank % num_sets;
	//3/13 design change
	//	vector<CpuRequest *> &current_set = getSet(dest_set);
	vector<CpuRequest *> &current_set = getSet(schan,dest_set);
	if(current_set.size() < WB_CACHE_ASSOCIATIVITY){    return true; }
	else{   return false;}
}*/
string FilenameWithNumberSuffix(const string &filename, const string &extension, unsigned maxNumber=100)
{
	string currentFilename = filename+extension;
	if (!fileExists(currentFilename))
	{
		return currentFilename;
	}

	// otherwise, add the suffixes and test them out until we find one that works
	stringstream tmpNum; 
	tmpNum<<"."<<1; 
	for (unsigned i=1; i<maxNumber; i++)
	{
		currentFilename = filename+tmpNum.str()+extension;
		if (fileExists(currentFilename))
		{
			currentFilename = filename; 
			tmpNum.seekp(0);
			tmpNum << "." << i;
		}
		else 
		{
			return currentFilename;
		}
	}
	// if we can't find one, just give up and return whatever is the current filename
	ERROR("Warning: Couldn't find a suitable suffix for "<<filename); 
	return currentFilename; 
}
/**
 * This function creates up to 3 output files: 
 * 	- The .log file if LOG_OUTPUT is set
 * 	- the .vis file where csv data for each epoch will go
 * 	- the .tmp file if verification output is enabled
 * The results directory is setup to be in PWD/TRACEFILENAME.[SIM_DESC]/DRAM_PARTNAME/PARAMS.vis
 * The environment variable SIM_DESC is also appended to output files/directories
 *
 * TODO: verification info needs to be generated per channel so it has to be
 * moved back to MemorySystem
 **/
void MultiChannelMemorySystem::InitOutputFiles(string traceFilename)
{
	size_t lastSlash;
	size_t deviceIniFilenameLength = deviceIniFilename.length();
	string sim_description_str;
	string deviceName;

	char *sim_description = getenv("SIM_DESC");
	if (sim_description)
	{
		sim_description_str = string(sim_description);
	}


	// create a properly named verification output file if need be and open it
	// as the stream 'cmd_verify_out'
	if (VERIFICATION_OUTPUT)
	{
		string basefilename = deviceIniFilename.substr(deviceIniFilename.find_last_of("/")+1);
		string verify_filename =  "sim_out_"+basefilename;
		if (sim_description != NULL)
		{
			verify_filename += "."+sim_description_str;
		}
		verify_filename += ".tmp";
		cmd_verify_out.open(verify_filename.c_str());
		if (!cmd_verify_out)
		{
			ERROR("Cannot open "<< verify_filename);
			abort(); 
		}
	}
	// This sets up the vis file output along with the creating the result
	// directory structure if it doesn't exist
	if (VIS_FILE_OUTPUT)
	{
		stringstream out,tmpNum;
		string path;
		string filename;

		if (!visFilename)
		{
			path = "results/";
			// chop off the .ini if it's there
			if (deviceIniFilename.substr(deviceIniFilenameLength-4) == ".ini")
			{
				deviceName = deviceIniFilename.substr(0,deviceIniFilenameLength-4);
				deviceIniFilenameLength -= 4;
			}

			// chop off everything past the last / (i.e. leave filename only)
			if ((lastSlash = deviceName.find_last_of("/")) != string::npos)
			{
				deviceName = deviceName.substr(lastSlash+1,deviceIniFilenameLength-lastSlash-1);
			}

			string rest;
			// working backwards, chop off the next piece of the directory
			if ((lastSlash = traceFilename.find_last_of("/")) != string::npos)
			{
				traceFilename = traceFilename.substr(lastSlash+1,traceFilename.length()-lastSlash-1);
			}
			if (sim_description != NULL)
			{
				traceFilename += "."+sim_description_str;
			}

			if (pwd.length() > 0)
			{
				path = pwd + "/" + path;
			}

			// create the directories if they don't exist 
			mkdirIfNotExist(path);
			path = path + traceFilename + "/";
			mkdirIfNotExist(path);
			path = path + deviceName + "/";
			mkdirIfNotExist(path);

			// finally, figure out the filename
			string sched = "BtR";
			string queue = "pRank";
			if (schedulingPolicy == RankThenBankRoundRobin)
			{
				sched = "RtB";
			}
			if (queuingStructure == PerRankPerBank)
			{
				queue = "pRankpBank";
			}

			/* I really don't see how "the C++ way" is better than snprintf()  */
			out << (TOTAL_STORAGE>>10) << "GB." << NUM_CHANS << "Ch." << NUM_RANKS <<"R." <<ADDRESS_MAPPING_SCHEME<<"."<<ROW_BUFFER_POLICY<<"."<< TRANS_QUEUE_DEPTH<<"TQ."<<CMD_QUEUE_DEPTH<<"CQ."<<sched<<"."<<queue;
		}
		else //visFilename given
		{
			out << *visFilename;
		}
		if (sim_description)
		{
			out << "." << sim_description;
		}

		//filename so far, without extension, see if it exists already
		filename = out.str();


		filename = FilenameWithNumberSuffix(filename, ".vis"); 
		path.append(filename);
		cerr << "writing vis file to " <<path<<endl;


		visDataOut.open(path.c_str());
		if (!visDataOut)
		{
			ERROR("Cannot open '"<<path<<"'");
			exit(-1);
		}
		//write out the ini config values for the visualizer tool
		IniReader::WriteValuesOut(visDataOut);

	}
	else
	{
		// cerr << "vis file output disabled\n";
	}
#ifdef LOG_OUTPUT
	string dramsimLogFilename("dramsim");
	if (sim_description != NULL)
	{
		dramsimLogFilename += "."+sim_description_str; 
	}

	dramsimLogFilename = FilenameWithNumberSuffix(dramsimLogFilename, ".log"); 

	dramsim_log.open(dramsimLogFilename.c_str(), ios_base::out | ios_base::trunc );

	if (!dramsim_log) 
	{
		ERROR("Cannot open "<< dramsimLogFilename);
		//	exit(-1); 
	}
#endif

}


void MultiChannelMemorySystem::mkdirIfNotExist(string path)
{
	struct stat stat_buf;
	// check if the directory exists
	if (stat(path.c_str(), &stat_buf) != 0) // nonzero return value on error, check errno
	{
		if (errno == ENOENT) 
		{
			//			DEBUG("\t directory doesn't exist, trying to create ...");

			// set permissions dwxr-xr-x on the results directories
			mode_t mode = (S_IXOTH | S_IXGRP | S_IXUSR | S_IROTH | S_IRGRP | S_IRUSR | S_IWUSR) ;
			if (mkdir(path.c_str(), mode) != 0)
			{
				perror("Error Has occurred while trying to make directory: ");
				cerr << path << endl;
				abort();
			}
		}
		else
		{
			perror("Something else when wrong: "); 
			abort();
		}
	}
	else // directory already exists
	{
		if (!S_ISDIR(stat_buf.st_mode))
		{
			ERROR(path << "is not a directory");
			abort();
		}
	}
}


MultiChannelMemorySystem::~MultiChannelMemorySystem()
{
	for (size_t i=0; i<NUM_CHANS; i++)
	{
		delete channels[i];
	}
	channels.clear(); 

	// flush our streams and close them up
#ifdef LOG_OUTPUT
	dramsim_log.flush();
	dramsim_log.close();
#endif
	if (VIS_FILE_OUTPUT) 
	{	
		visDataOut.flush();
		visDataOut.close();
	}
}
//Added by Yihan
//3/13 design change stopped at 704am
/*
void MultiChannelMemorySystem::Inspect_all_rank_for_end_drain_condition()
{
	for(unsigned i=0; i<NUM_CHANS; i++)
	{
		if(draining_rank[i] > -1)
		{
			if(channels[i]->memoryController->isRankRefreshing(draining_rank[i]) ||
					//				327 change
					//				channels[i]->memoryController->write_complete>=READ_RESUME_THRESHOLD)
				channels[i]->memoryController->write_complete>=dynamic_resume_threshold[i])
				{
					if(DEBUG_NON_BLOCKING_REFRESH)
					{
						cout<<"end drain rank :"<<draining_rank[i]<<endl;
					}
					reinsert_write_packet(i,draining_rank[i]);
					unpause_read_redirect_packet(i,draining_rank[i]);
					if(DEBUG_NON_BLOCKING_REFRESH){cout<<"u know"<<endl;}
					drain_cap[i]=0;
					draining_rank[i]=-1;
					channels[i]->memoryController->write_complete=0;
				}
		}
	}
}
*/
//Added by Yihan
//3/13 
//unsigned  MultiChannelMemorySystem::get_writes_to_rank_X(unsigned rank)
/*
unsigned  MultiChannelMemorySystem::get_writes_to_rank_X(unsigned chan, unsigned rank)
{
	if(!channels[chan]->memoryController->isRankRefreshing(rank))
	{
		if(Write_In_Waiting_For_Each_Rank[calculate_channel_rank_location(chan,rank)] >= THRESHOLD)
		{
			return Write_In_Waiting_For_Each_Rank[calculate_channel_rank_location(chan,rank)];
		}
	}

	return 0;
}
*/
/*
void MultiChannelMemorySystem::Inspect_all_rank_for_drain_entrance_condition()
{
	//3/13 multichannel design
	for(int c=0; c<NUM_CHANS;c++)
	{
		unsigned max_write=0;
		int top_rank=-1;
		//		if(draining_rank==-1){
		if(draining_rank[c]==-1)
		{
			for(unsigned i=0; i<NUM_RANKS; i++){
				//				unsigned cur_threshold=get_writes_to_rank_X(i);
				unsigned cur_threshold=get_writes_to_rank_X(c,i);
				if(cur_threshold>THRESHOLD && cur_threshold > max_write){
					max_write=cur_threshold;
					top_rank=i;
				}
			}
		}
		if(top_rank>-1)
		{
			//			draining_rank=top_rank;
			draining_rank[c]=top_rank;
			channels[c]->memoryController->write_complete=0;
			dynamic_resume_threshold[c]=THRESHOLD*DRAIN_PERCENTAGE;
			if(DEBUG_NON_BLOCKING_REFRESH){	
				cout<<"chan: "<< c <<"start draining rank: "<<draining_rank[c]<<" threshold: "<<max_write<<endl;
				//print_wb_cache();
			}
		}
	}
}*/



	void MultiChannelMemorySystem::update()
	{
		clockDomainCrosser.update(); 
	}

	void MultiChannelMemorySystem::actual_update() 
	{
		if (currentClockCycle == 0)
		{
			InitOutputFiles(traceFilename);
			DEBUG("DRAMSim2 Clock Frequency ="<<clockDomainCrosser.clock1<<"Hz, CPU Clock Frequency="<<clockDomainCrosser.clock2<<"Hz"); 
		}

		if (currentClockCycle % EPOCH_LENGTH == 0)
		{
			(*csvOut) << "ms" <<currentClockCycle * tCK * 1E-6; 
			for (size_t i=0; i<NUM_CHANS; i++)
			{
				channels[i]->printStats(false); 
			}
			csvOut->finalize();
		}

		for (size_t i=0; i<NUM_CHANS; i++)
		{
			channels[i]->update(); 
		}
/*		if(experimentScheme ==Baseline)
		{
			//break down to two if statement, starting draining, and draining in progress
			Inspect_all_rank_for_drain_entrance_condition();
			Inspect_all_rank_for_end_drain_condition();
			//unpause_read_redirect_packet();
			for(unsigned  c=0; c<NUM_CHANS; c++){
				if(draining_rank[c]>-1){
					wb_cache_batch_drain_rank_candidate(c,draining_rank[c]);
				}
			}
		}
		else
		{
			ERROR("Incorrect Scheme\n");
			exit(1);
		}*/
		currentClockCycle++;
	}
	//Yihan added wb_cache functions
	//IF BOTH REQUEST GOES TO SAME ADDRESS
	//The later one will overwrite the first one since the cache
	//will map it into same location
	/*
	bool MultiChannelMemorySystem::wb_cache_insert_candidate(uint64_t addr)
	{
		//TODO ADD INSERT
		bool is_hit=false;
		bool insert_successful=false;
		unsigned chan,rank,bank,row,col;
		unsigned cur_chan,cur_rank,cur_bank,cur_row,cur_col;
		addressMapping(addr, chan, rank, bank, row, col);
		unsigned dest_set = bank % num_sets;

		if(DEBUG_NON_BLOCKING_REFRESH){
			cout<<"incoming addr: "<<std::hex<<addr<<std::dec <<" is mapped to set: "<<dest_set<<" that set currently has a size of "<<wb_cache[dest_set].size()<<"\n";
		}
		//get the set to see if there is a hit
		//3/13
		vector<CpuRequest *> &current_set = getSet(chan,dest_set);
		if(current_set.size()>WB_CACHE_ASSOCIATIVITY){
			ERROR("I reached here\n");
			cout<<current_set.size()<<endl;;
			return false;
		}
		else{
			for(size_t i=0; i <current_set.size(); i++)
			{
				//if there is a hit, replace
				if(current_set[i]->physicalAddress==addr){
					if(DEBUG_NON_BLOCKING_REFRESH){	cout<<"found a hit on addr :"<<addr<<"\n";	}
					is_hit=true;
					insert_successful=true;
					num_replace++;
					break;
				}
			}
			if(!is_hit){
				CpuRequest *incoming_request = new CpuRequest(addr);
				vector<CpuRequest *>::iterator it;
				unsigned counter=0;
				for(it=current_set.begin(); it<current_set.end(); it++)
				{
					CpuRequest* curRequest = *it;
					addressMapping(curRequest->physicalAddress,cur_chan,cur_rank,cur_bank,cur_row,cur_col);
					if(cur_row==row && cur_rank==rank && cur_bank==bank){counter++;}
				}
				if(counter==0){current_set.push_back(incoming_request);}
				else{
					vector<CpuRequest*>::iterator mit;
					for(mit=current_set.begin(); mit<current_set.end(); mit++){
						CpuRequest* travRequest = *mit;
						addressMapping(travRequest->physicalAddress,cur_chan,cur_rank,cur_bank,cur_row,cur_col);
						if(cur_row==row && cur_rank==rank && cur_bank==bank){counter--;}
						if(counter==0){
							current_set.insert(mit+1,incoming_request);
							break;
						}
					}
				}
				insert_successful=true;
				Write_In_Waiting_For_Each_Rank[calculate_channel_rank_location(chan,rank)]++;
				if(DEBUG_NON_BLOCKING_REFRESH){
					cout<<"no hit found, insert addr: "<<std::hex<<addr<<std::dec<<" in to write buffer cache"<<endl;
				}
			}
		}
		return insert_successful;
	}*/
	// Added by Yihan cache drain for non blocking write
/*	void MultiChannelMemorySystem::wb_cache_batch_drain_rank_candidate(unsigned select_chan, int drain_rank)
	{
		unsigned startingSet = nextSet[select_chan];
		unsigned chan,rank,bank,row,col;
		bool found_issuable_candidate=false;
		if(DEBUG_NON_BLOCKING_REFRESH)
		{
			cout<<"starting set: "<<startingSet<<endl;
		}
		do
		{
			//		vector<CpuRequest *> &current_set = getSet(nextSet);
			vector<CpuRequest *> &current_set = getSet(select_chan,nextSet[select_chan]);
			for(size_t i=0; i<current_set.size(); i++)
			{
				addressMapping(current_set[i]->physicalAddress, chan, rank, bank, row, col);
				if(drain_rank!=rank){continue;}
				//			if(Does_Queue_Have_Space_For_Bank_X(drain_rank,bank))
				if(Does_Queue_Have_Space_For_Bank_X(select_chan,drain_rank,bank))
				{
					if(DEBUG_NON_BLOCKING_REFRESH)
					{
						cout<<" on channel: "<< select_chan<<endl;
						cout<<"drained a packet with address: "<< std::hex<<current_set[i]->physicalAddress<<std::dec;
						cout <<"chan: "<<chan<<" rank: "<<drain_rank<<" bank: "<<bank<<" row: "<<row<<endl;
					}
					found_issuable_candidate=true;
					//				Write_In_Waiting_For_Each_Rank[drain_rank]--;
					Write_In_Waiting_For_Each_Rank[calculate_channel_rank_location(select_chan,drain_rank)]--;
					channels[select_chan]->addTransaction(true,current_set[i]->physicalAddress);
					current_set.erase(current_set.begin()+i);
					break;
				}
			}

			if(found_issuable_candidate)
			{
				//3/13
				//			if(drain_cap>0){
				if(drain_cap[select_chan]>0){
					drain_cap[select_chan]--;
					if(DEBUG_NON_BLOCKING_REFRESH){
						cout<<"don't go to next set yet, drain_cap: "<<drain_cap[select_chan]<<endl;
					}
					if(drain_cap[select_chan]==0){
						getnextSet(nextSet[select_chan]);
						if(DEBUG_NON_BLOCKING_REFRESH){
							cout<<"move on to next set: "<<nextSet[select_chan]<<endl;
						}
					}

				}
				else if (drain_cap[select_chan]==0)
				{
					unsigned found_same_row=0;
					unsigned zchan,zrank,zbank,zrow,zcol;
					for(int j=0; j<current_set.size(); j++){
						addressMapping(current_set[j]->physicalAddress,zchan,zrank,zbank,zrow,zcol);
						if(drain_rank ==zrank && bank==zbank && row==zrow){
							found_same_row++;
						}
					}
					if(found_same_row==0){ getnextSet(nextSet[select_chan]);}
					else{
						if(found_same_row < DRAIN_CAP_THRESHOLD){	drain_cap[select_chan]= found_same_row;}
						else{	drain_cap[select_chan]=DRAIN_CAP_THRESHOLD;	}
						if(DEBUG_NON_BLOCKING_REFRESH){ cout<<"drain cap set to : "<<drain_cap[select_chan]<<endl;}
					}
				}
				break;
			}
			getnextSet(nextSet[select_chan]);
			}
			while(startingSet !=nextSet[select_chan]);
		}*/

		//Added by Yihan
		/*
		void MultiChannelMemorySystem::getnextSet(unsigned &Set)
		{
			Set++;
			if(Set==num_sets){
				Set=0;
			}
		}*/
		//Added by Yihan
		//3/13 design change 
		//vector<CpuRequest *> &MultiChannelMemorySystem::getSet(unsigned set)
		/*
		vector<CpuRequest *> &MultiChannelMemorySystem::getSet(unsigned chan, unsigned set)
		{
			unsigned actual_set =  chan*num_sets + set;
			//	return wb_cache[set];
			return wb_cache[actual_set];
		}*/
		unsigned MultiChannelMemorySystem::findChannelNumber(uint64_t addr)
		{
			// Single channel case is a trivial shortcut case 
			if (NUM_CHANS == 1)
			{
				return 0; 
			}

			if (!isPowerOfTwo(NUM_CHANS))
			{
				ERROR("We can only support power of two # of channels.\n" <<
						"I don't know what Intel was thinking, but trying to address map half a bit is a neat trick that we're not sure how to do"); 
				abort(); 
			}

			// only chan is used from this set 
			unsigned channelNumber,rank,bank,row,col;
			addressMapping(addr, channelNumber, rank, bank, row, col); 
			if (channelNumber >= NUM_CHANS)
			{
				ERROR("Got channel index "<<channelNumber<<" but only "<<NUM_CHANS<<" exist"); 
				abort();
			}
			//DEBUG("Channel idx = "<<channelNumber<<" totalbits="<<totalBits<<" channelbits="<<channelBits); 

			return channelNumber;

		}
		ostream &MultiChannelMemorySystem::getLogFile()
		{
			return dramsim_log; 
		}
		bool MultiChannelMemorySystem::addTransaction(const Transaction &trans)
		{
			// copy the transaction and send the pointer to the new transaction 
			return addTransaction(new Transaction(trans)); 
		}

		bool MultiChannelMemorySystem::addTransaction(Transaction *trans)
		{
			unsigned channelNumber = findChannelNumber(trans->address); 
			return channels[channelNumber]->addTransaction(trans); 
		}
		//added by yihan to flush stalled read
		void MultiChannelMemorySystem::flush_stalled_read(uint64_t addr )
		{
			//3/13
			unsigned channelNumber=findChannelNumber(addr);
			channels[channelNumber]->flush_stalled_read(addr);
		}

		bool MultiChannelMemorySystem::addTransaction(bool isWrite, uint64_t addr)
		{
			unsigned channelNumber = findChannelNumber(addr);
			return channels[channelNumber]->handle_cpu_request(isWrite,addr);
			//modified by Yihan to ignore ask memory system to add incoming write request
			/*if(isWrite)
			{
				if(DEBUG_NON_BLOCKING_REFRESH){
					cout<<"received a write cpu request with addr: "<<std::hex<<addr<<std::dec<<endl;
				}
				bool is_insert_successful=wb_cache_insert_candidate(addr);
				return is_insert_successful;
			}else
			{
				unsigned rank,bank,row,col,chan;
				addressMapping(addr,chan,rank,bank,row,col);
				if(draining_rank[chan] == rank)
				{
					read_redirect_buffer.push_back(addr);
					return true;
				}
			}

			return channels[channelNumber]->addTransaction(isWrite, addr);
			*/
		}

		/*
		   This function has two flavors: one with and without the address. 
		   If the simulator won't give us an address and we have multiple channels, 
		   we have to assume the worst and return false if any channel won't accept. 

		   However, if the address is given, we can just map the channel and check just
		   that memory controller
		 */


		//added by yihan to handle write request specially from cpu if non_blocking_refresh mode is enabled
		//TODO can make the reject condition more optimized, right now if there is an addr coming that are already in the set 
		//but the set not is full, we will reject the cpu request. However, in reality we should still accept and reset it's 
		//sending condition upon inserting to the wb cache
/*		bool MultiChannelMemorySystem::wb_cache_can_accept(uint64_t addr){
			unsigned chan, rank,bank,row,col; 
			addressMapping(addr, chan, rank, bank, row, col); 

			unsigned dest_set = bank % num_sets;
			//get the set to see if there is a hit
			//3/13
			vector<CpuRequest *> &current_set = getSet(chan,dest_set);
			//3/19
			//3/20
			//3/26 
			if(current_set.size()<(WB_CACHE_ASSOCIATIVITY*3/4)){
				//	if(can_cache_accept_drained_write(addr)){
				if(DEBUG_NON_BLOCKING_REFRESH){
					cout<<"write buffer at set: "<<dest_set<<" have space\n";
				}
				//		return true;
			}
				else
				{
					//3/22 update
					if(DEBUG_NON_BLOCKING_REFRESH){	cout<<"set : "<<dest_set<<" is full"<<endl;
						print_wb_cache();}
					unsigned xchan,xrank,xbank,xrow,xcol;
					//find the rank with most write in this set
					vector<unsigned> num_write = vector<unsigned>(NUM_RANKS,0);
					for(int i=0; i<current_set.size(); i++)
					{
						addressMapping(current_set[i]->physicalAddress, xchan, xrank, xbank, xrow, xcol);
						num_write[xrank]++;
					}
					int max_rank=-1;
					unsigned max_write=0;
					int min_rank=-1;
					unsigned min_write=0;
					for(int j=0; j<num_write.size(); j++)
					{
						if(num_write[j]>max_write)
						{
							max_write=num_write[j];
							max_rank=j;
						}
						if(num_write[j]<min_write)
						{
							min_write=num_write[j];
							min_rank=j;
						}
					}
					//find the max
					if(max_rank <0)
					{
						ERROR("force drain but no write is found in the set");
						exit(1);
					}
					else
					{
						if(DEBUG_NON_BLOCKING_REFRESH)
						{
							cout<<"force drain condition triggered, start draining rank: " <<max_rank<<endl;
							print_wb_cache();
						}
					}
					//check if there is no draining rank:
					if(draining_rank[chan] ==-1)
					{
						//drain one packet from this set by setting the next set to set
						nextSet[chan]=dest_set;
						drain_cap[chan]=0;
						draining_rank[chan]=max_rank;
						dynamic_resume_threshold[chan]=max_write*DRAIN_PERCENTAGE;
						if(current_set.size() > WB_CACHE_ASSOCIATIVITY){
							return false;
						}else{ return true;}
					}
					else if(draining_rank[chan]==rank)
					{
						nextSet[chan]=dest_set;
						drain_cap[chan]=0;
						drain_force_redirect[chan]++;
						if(current_set.size() > WB_CACHE_ASSOCIATIVITY){
							return false;
						}else{ return true;}
					}
					else
					{
						for(int i=0; i<current_set.size(); i++)
						{
						   addressMapping(current_set[i]->physicalAddress, xchan, xrank, xbank, xrow, xcol);
						   if(xrank==min_rank && chan==xchan)
						   {
						   		channels[xchan]->addTransaction(true,current_set[i]->physicalAddress);
						   		Write_In_Waiting_For_Each_Rank[calculate_channel_rank_location(xchan,xrank)]--;
						   		current_set.erase(current_set.begin()+i);
						   		num_force_drain[xchan]++;
						   		break;
						   }
						}

						if(current_set.size() > WB_CACHE_ASSOCIATIVITY){
							return false;
						}else{ return true;}

						//3/13
						
						   addressMapping(current_set[0]->physicalAddress, xchan, xrank, xbank, xrow, xcol);
						   channels[xchan]->addTransaction(true,current_set[0]->physicalAddress);
						   Write_In_Waiting_For_Each_Rank[calculate_channel_rank_location(xchan,xrank)]--;
						   current_set.erase(current_set.begin());
						   num_force_drain[xchan]++;
						   }else{
						   return false;
						   }
					}
			}
			return true;
		}*/

		//Added by Yihan, restrict accept for write so only to accept write when cache is not full
		bool MultiChannelMemorySystem::willAcceptWriteTransaction(uint64_t addr)
		{
			unsigned channelNumber=findChannelNumber(addr);
			if( channels[channelNumber]->wb_cache_can_accept(addr)){	return true;}
			else{ num_enqueue_failed++; return false;}
		}
		//3/13 design change
		//void MultiChannelMemorySystem::unpause_read_redirect_packet(unsigned release_rank)
/*		void MultiChannelMemorySystem::unpause_read_redirect_packet(unsigned chan, unsigned release_rank)
		{
			unsigned schan,rank,bank,row,col;
			unsigned num_released=0;
			//	vector<uint64_t>::iterator it=read_redirect_buffer.begin();
			vector<uint64_t>::iterator it=read_redirect_buffer.begin();
			//	while(it!=read_redirect_buffer.end())
			while(it!=read_redirect_buffer.end())
			{
				addressMapping(*it,schan,rank,bank,row,col);
				if(schan != chan)
				{
					++it;
				}
				else if(rank==release_rank)
				{
					num_released++;
					channels[chan]->addTransaction(false,*it);
					read_redirect_buffer.erase(it);
				}else
				{
					++it;
				}
			}
			if(DEBUG_NON_BLOCKING_REFRESH)
			{
				cout<<"released "<<num_released<<endl;
			}
		}*/
		/*
		   void MultiChannelMemorySystem::unpause_read_redirect_packet()
		   {
		   unsigned chan,rank,bank,row,col;
		   unsigned num_released=0;
		   vector<uint64_t>::iterator it=read_redirect_buffer.begin();
		   while(it!=read_redirect_buffer.end())
		   {
		   addressMapping(*it,chan,rank,bank,row,col);
		   if(rank!=draining_rank && Does_Queue_Have_Space_For_Bank_X(rank,bank))
		   {
		   num_released++;
		   channels[chan]->addTransaction(false,*it);
		   read_redirect_buffer.erase(it);
		   }else
		   {
		   ++it;
		   }
		   }
		   if(DEBUG_NON_BLOCKING_REFRESH)
		   {
		   cout<<"released "<<num_released<<endl;
		   }
		   }*/
		bool MultiChannelMemorySystem::willAcceptTransaction(uint64_t addr)
		{
			unsigned chan, rank,bank,row,col; 
			addressMapping(addr, chan, rank, bank, row, col); 
			return channels[chan]->WillAcceptTransaction(); 
		}

		bool MultiChannelMemorySystem::willAcceptTransaction()
		{
			for (size_t c=0; c<NUM_CHANS; c++) {
				if (!channels[c]->WillAcceptTransaction())
				{
					return false; 
				}
			}
			return true; 
		}
		//3/13 design change
		//void MultiChannelMemorySystem::reinsert_write_packet(int rank)
/*		void MultiChannelMemorySystem::reinsert_write_packet(unsigned chan, int rank)
		{
			if(DEBUG_NON_BLOCKING_REFRESH){
				cout<<"reinsert packet belong to chan: "<< chan<<" rank: "<<rank<<endl;
			}
			uint64_t ret_addr=0;
			do
			{
				//		ret_addr=channels[0]->memoryController->FindOutstandingWrite(rank);
				ret_addr=channels[chan]->memoryController->FindOutstandingWrite(rank);
				//		if(ret_addr==0){	ret_addr=channels[0]->FindPendingTransQueueWrite(rank);}
				if(ret_addr==0){	ret_addr=channels[chan]->FindPendingTransQueueWrite(rank);}
				if(ret_addr!=0){
					if(can_cache_accept_drained_write(ret_addr))
					{
						wb_cache_insert_candidate(ret_addr);
					}else
					{
						//(*channels[0]->WriteDataDone)(channels[0]->systemID,ret_addr,currentClockCycle);
						//channels[0]->addTransaction(true,ret_addr);
						channels[chan]->addTransaction(true,ret_addr);
						reinsert_cache_full_stats++;
						break;
					}
				}
				if(DEBUG_NON_BLOCKING_REFRESH)
				{
					cout<<"stuck here"<<endl;
				}	

			}while(ret_addr!=0);
			if(DEBUG_NON_BLOCKING_REFRESH)
			{
				cout<<"reached here"<<endl;
			}
			sync_wb_cache();
			if(DEBUG_NON_BLOCKING_REFRESH)
			{
				cout<<"passed here"<<endl;
			}

		}*/
		//3/13 design change
/*		void MultiChannelMemorySystem::sync_wb_cache()
		{
			unsigned counter_size=0;
			unsigned chan,rank,bank,row,col;
			for(int c=0; c<NUM_CHANS; c++)
			{
				for(int i=0; i<NUM_RANKS; i++)
				{
					counter_size=0;
					for(int j=0; j<num_sets; j++){
						//					vector<CpuRequest *> &current_set = getSet(j);
						vector<CpuRequest *> &current_set = getSet(c,j);
						for(size_t k=0; k<current_set.size(); k++)
						{
							addressMapping(current_set[k]->physicalAddress, chan, rank, bank, row, col);
							if(i==rank){ counter_size++; }
						}
					}
					if(Write_In_Waiting_For_Each_Rank[calculate_channel_rank_location(c,i)]!=counter_size)
					{
						if(DEBUG_NON_BLOCKING_REFRESH){
							cout<<"counter for rank: "<< i <<"  need to be updated from: "<<Write_In_Waiting_For_Each_Rank[calculate_channel_rank_location(c,i)]<<" to "<<counter_size<<endl;
							num_cache_synced_stats++;
						}
						Write_In_Waiting_For_Each_Rank[calculate_channel_rank_location(c,i)]=counter_size;
					}
				}
			}
		}*/
		void MultiChannelMemorySystem::printStats(bool finalStats) {

			(*csvOut) << "ms" <<currentClockCycle * tCK * 1E-6; 
			cout<<"number_of_wb_enqueue_failure "<<num_enqueue_failed<<endl;

			for (size_t i=0; i<NUM_CHANS; i++)
			{
				PRINT("==== Channel ["<<i<<"] ====");
				channels[i]->memoryController->num_heartbeat_violation=channels[i]->num_heartbeat_violation;
				channels[i]->memoryController->num_force_drain=channels[i]->num_force_drain;
				channels[i]->memoryController->force_drain_redirect=channels[i]->drain_force_redirect;
				channels[i]->printStats(finalStats); 
				PRINT("//// Channel ["<<i<<"] ////");
			}
			csvOut->finalize();
		}
		void MultiChannelMemorySystem::RegisterCallbacks( 
				TransactionCompleteCB *readDone,
				TransactionCompleteCB *writeDone,
				void (*reportPower)(double bgpower, double burstpower, double refreshpower, double actprepower))
		{
			for (size_t i=0; i<NUM_CHANS; i++)
			{
				channels[i]->RegisterCallbacks(readDone, writeDone, reportPower); 
			}
		}

		/*
		 * The getters below are useful to external simulators interfacing with DRAMSim
		 *
		 * Return value: 0 on success, -1 on error
		 */
		int MultiChannelMemorySystem::getIniBool(const std::string& field, bool *val)
		{
			if (!IniReader::CheckIfAllSet())
				exit(-1);
			return IniReader::getBool(field, val);
		}

		int MultiChannelMemorySystem::getIniUint(const std::string& field, unsigned int *val)
		{
			if (!IniReader::CheckIfAllSet())
				exit(-1);
			return IniReader::getUint(field, val);
		}

		int MultiChannelMemorySystem::getIniUint64(const std::string& field, uint64_t *val)
		{
			if (!IniReader::CheckIfAllSet())
				exit(-1);
			return IniReader::getUint64(field, val);
		}

		int MultiChannelMemorySystem::getIniFloat(const std::string& field, float *val)
		{
			if (!IniReader::CheckIfAllSet())
				exit(-1);
			return IniReader::getFloat(field, val);
		}
		//3/13
/*		bool MultiChannelMemorySystem::Does_Queue_Have_Space_For_Bank_X(unsigned chan, unsigned rank, unsigned bank) 
		{
			if(channels[chan]->memoryController->TransQueueUsedSpace(rank)<TRANSACTION_QUEUE_WRITE_MAX_USAGE)
			{
				if( channels[chan]->memoryController->queueFreeSpace(rank,bank)>= (	(CMD_QUEUE_DEPTH/3) +
							(channels[chan]->memoryController->num_transqueue_drain_write_for_bank_X(rank,bank)*2)+
							(channels[chan]->pendingTransQueueUsedSpace(rank,bank)*2)))
				{
					return true;
				}
			}
			return false;
		}
		//3/13
		unsigned MultiChannelMemorySystem::calculate_channel_rank_location(unsigned chan, unsigned rank)
		{
			unsigned dest=chan*NUM_RANKS+rank;
			return dest;
		}*/
/*
		unsigned MultiChannelMemorySystem::number_of_writes_in_wb_for_rank_X(unsigned target_chan, unsigned search_rank)
		{
			unsigned chan,rank,bank,row,col;
			unsigned number_of_write=0;
			for(int i=0; i<num_sets; i++)
			{
				vector<CpuRequest *> &current_set = getSet(target_chan,i);
				for(int j=0; j<current_set.size(); j++)
				{
					addressMapping(current_set[j]->physicalAddress, chan, rank, bank, row, col);
					if(rank==search_rank)
					{
						number_of_write++;
					}
				}
			}
			return number_of_write;
		}*/

		namespace DRAMSim {
			MultiChannelMemorySystem *getMemorySystemInstance(const string &dev, const string &sys, const string &pwd, const string &trc, unsigned megsOfMemory, string *visfilename) 
			{
				return new MultiChannelMemorySystem(dev, sys, pwd, trc, megsOfMemory, visfilename);
			}
		}
