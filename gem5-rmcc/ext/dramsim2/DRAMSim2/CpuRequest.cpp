//CpuRequest.cpp
//class file for gem5 cpu write request

#include "CpuRequest.h"

using namespace DRAMSim;
using namespace std;

CpuRequest::CpuRequest(uint64_t physicalAddr):
	physicalAddress(physicalAddr)
{}
