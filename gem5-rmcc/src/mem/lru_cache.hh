/*
Author: Gagan
*/

#include <list>
#include <vector>
#include <unordered_map>
#include "base/types.hh"

// avoiding typedefs purposefully

class set
{
  // store block addresses, LRU kept at the end, MRU kept in  front
  std::list<Addr> q;

  // unordered map to speed up look up in q, enables very high ways (associativites)
  std::unordered_map<Addr, std::list<Addr>::iterator> map;
  unsigned ways; //ways in a set
  //unsigned occupancy; // tracks occupancy for this set

public:
  set(unsigned n)
  {
    ways = n;
    //occupancy = 0;
  }
 
  // updates the set, returns whether it was a hit or miss
  bool call(Addr x, unsigned &total_occupancy)
  {
    bool isHit;
  
    // not present in cache
    if (map.find(x) == map.end())
      {
	isHit = false;
	// cache is full
	if (q.size() == ways)
	  {
	    // delete least recently used element
	    Addr last = q.back();
	    //debug : std::cout << "------> Evicting dict " << last << std::endl;
	    // remove from the queue
	    q.pop_back();
	    // remove from the hashmap
	    map.erase(last);
	  }
	else
	  {
	    total_occupancy++;
	    //std::cout << " cold miss " << std::dec << occupancy << std::endl;
	  }
      }
    // present in cache
    else
      {
	isHit = true;
	// remove from the intermediate position in the queue because need to get it in front
	q.erase(map[x]);
      }

    // put in front, update the queue and the hashmap
    q.push_front(x);
    map[x] = q.begin();
    return isHit;
  }

  // checks the set, returns whether it will *cause* a hit or a miss
  bool check(Addr x)
  {
    // not present in cache
    if (map.find(x) == map.end())
      {
	return false;
      }
    else
      {
	return true;
      }
  }

  // display contents of set
  void display()
  {
    for (auto it = q.begin(); it != q.end(); it++)
      std::cout << std::hex << (*it) << "\t";

    std::cout << std::endl;
  }

};




// lru_cache class, contains a vector of sets
class lru_cache
{
  std::vector<set> sets; // vector of sets
  Addr size; // size of cache in bytes
  uint64_t numSets; // number of sets
  unsigned associativity; // associativity / ways
  unsigned total_occupancy;

public:
  
  // constructor
  lru_cache(uint64_t n, unsigned m)
  {
    size = n;
    associativity = m;
    numSets = size / (64 * associativity);
    total_occupancy = 0;
    //numSets = size;
    sets.resize(numSets, set(associativity));
  }

  void printInfo()
  {
    std::cout << "Size: " << (float)size / 1024 << " kilobytes" << std::endl;
    std::cout << "numSets: " << numSets << std::endl;
    std::cout << "Associativity: " << associativity << std::endl;
  }

  // retreive occupancy of each set and return the global occupancy
  float occupancy()
  {
    return (float)(total_occupancy) / (numSets * associativity);
  }

  // looks up a particular set, updates the set and returns whether it was a hit or miss
  bool call(Addr addr)
  {
    //Addr block_addr = addr * 64; // / 64;
    Addr block_addr = addr / 64;
    Addr cache_set = block_addr % numSets;
    //std::cout << "Cache call default. Addr: " << std::hex << addr << ", Block addr: " << std::hex << block_addr << ", Set: " << std::hex << cache_set << std::endl;
    return sets[cache_set].call(block_addr, total_occupancy);
  }

  // looks up a particular set, returns whether it will *cause* a hit or miss
  bool check(Addr addr)
  {
    //Addr block_addr = addr * 64; // / 64;
    //Addr page_addr = (addr >> 28) << 16;
    //page_addr = page_addr | (addr & 0xffff);
    Addr block_addr = addr / 64;
    Addr cache_set = block_addr % numSets;
    //std::cout << "Cache check default. Addr: " << std::hex << addr << ", Block addr: " << std::hex << block_addr << ", Set: " << std::hex << cache_set << std::endl;
    return sets[cache_set].check(block_addr);
  }



  void print()
  {
    /*
      for(auto it = sets.begin(); it != sets.end(); ++it)
      {
      (*it)->display;
      }
    */
  
    for(int i =  0; i < numSets; ++i)
      {
	std::cout << std::dec << i << ": ";
	sets[i].display();
      }
  
  }

};
