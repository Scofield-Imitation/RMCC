#ifndef __GEM5_WRAPPER_H
#define __GEM5_WRAPPER_H

#include <string>

#include "Config.h"

using namespace std;

namespace ramulator
{

class Request;
class MemoryBase;

class Gem5Wrapper 
{
private:
    MemoryBase *mem;
// daz3: one wrapper
    long tickCount;
public:
    double tCK;
    Gem5Wrapper(const Config& configs, int cacheline);
    ~Gem5Wrapper();
    void tick();
    bool send(Request req);
    //xinw added for morphable-begin
    uint64_t get_remaining_queue_size(Request req);
    uint64_t get_overflow_queue_size(Request req);
    uint64_t prioritize(Request req);
    //xinw added for morphable-end
    //xinw added 
    void set_warmuptime(uint64_t _atomicwarmuptime, uint64_t _warmuptime);    
    void set_simulation_mode(int detailed_mode);
    void set_timeout(uint64_t _timeout);
   void finish(void);
};

} /*namespace ramulator*/

#endif /*__GEM5_WRAPPER_H*/
