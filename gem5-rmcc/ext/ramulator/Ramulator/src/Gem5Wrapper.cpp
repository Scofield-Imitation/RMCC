#include <map>

#include "Gem5Wrapper.h"
#include "Config.h"
#include "Request.h"
#include "MemoryFactory.h"
#include "Memory.h"
#include "DDR3.h"
#include "DDR4.h"
#include "LPDDR3.h"
#include "LPDDR4.h"
#include "GDDR5.h"
#include "WideIO.h"
#include "WideIO2.h"
#include "HBM.h"
#include "SALP.h"

using namespace ramulator;

static map<string, function<MemoryBase *(const Config&, int, bool)> > name_to_func = {
    {"DDR3", &MemoryFactory<DDR3>::create},
    {"DDR4", &MemoryFactory<DDR4>::create},
    {"LPDDR3", &MemoryFactory<LPDDR3>::create},
    {"LPDDR4", &MemoryFactory<LPDDR4>::create},
    {"GDDR5", &MemoryFactory<GDDR5>::create}, 
    {"WideIO", &MemoryFactory<WideIO>::create},
    {"WideIO2", &MemoryFactory<WideIO2>::create},
    {"HBM", &MemoryFactory<HBM>::create},
    {"SALP-1", &MemoryFactory<SALP>::create},
    {"SALP-2", &MemoryFactory<SALP>::create},
    {"SALP-MASA", &MemoryFactory<SALP>::create},
};


Gem5Wrapper::Gem5Wrapper(const Config& configs, int cacheline)
{
    const string& std_name = configs["standard"];
    assert(name_to_func.find(std_name) != name_to_func.end() && "unrecognized standard name");
    mem = name_to_func[std_name](configs, cacheline, false);
    tCK = mem->clk_ns();
    // daz3:  one wrapper
    tickCount = 0;
}


Gem5Wrapper::~Gem5Wrapper() {
    delete mem;
}

void Gem5Wrapper::tick()
{
    // mem->tick();
    // daz3
    if(tickCount % 2 == 0)
    {
        mem->tick();
    }
    tickCount++;
}

bool Gem5Wrapper::send(Request req)
{
  return mem->send(req);
}
//xinw added for morphable-begin
uint64_t Gem5Wrapper::get_remaining_queue_size(Request req)
{
    return mem->get_remaining_queue_size(req);
}
uint64_t Gem5Wrapper::get_overflow_queue_size(Request req)
{
    return mem->get_overflow_queue_size(req);
}

void Gem5Wrapper::set_warmuptime(uint64_t _atomicwarmuptime, uint64_t _warmuptime)    
{
    mem->set_warmuptime(_atomicwarmuptime, _warmuptime);
}
void Gem5Wrapper::set_simulation_mode(int detailed_mode)
{
    mem->set_simulation_mode(detailed_mode);
}
uint64_t Gem5Wrapper::prioritize(Request req)
{
    return mem->prioritize(req);
}

void Gem5Wrapper::set_timeout(uint64_t _timeout)
{
    mem->set_timeout(_timeout);
}
//xinw added for morphable-end

void Gem5Wrapper::finish(void) {
    mem->finish();
}
