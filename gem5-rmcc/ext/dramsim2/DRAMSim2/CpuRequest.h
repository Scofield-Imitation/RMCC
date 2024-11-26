#ifndef CPUREQUEST_H
#define CPUREQUEST_H
#include "SystemConfiguration.h"

namespace DRAMSim
{
class CpuRequest
{
	CpuRequest();
public:
	//Fields
	uint64_t physicalAddress;
//	bool hasSendOriginalWrite;
	//Nov 14 removal unused variable
//	bool hasSendDuplicateWrite;
	CpuRequest( uint64_t physicalAddr);
};
}
#endif
