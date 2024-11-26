#ifndef __REQUEST_H
#define __REQUEST_H

#include <vector>
#include <functional>

using namespace std;

namespace ramulator
{

class Request
{
public:
    //xinw added
    bool is_counter;
    unsigned long enqueue_time;    
    long enqueue_clk;    

    bool is_first_command;
    long addr;
    // long addr_row;
    vector<int> addr_vec;
    // specify which core this request sent from, for virtual address translation
    int coreid;
	
    enum class Type
    {
        READ,
        WRITE,
        REFRESH,
        POWERDOWN,
        SELFREFRESH,
        EXTENSION,
        MAX
    } type;

    long arrive = -1;
    long wr_arrive = -1;
    long depart;
    function<void(Request&)> callback; // call back with more info
//xinw added for morphable-begin
    bool need_meta=false;
    int  meta_level=-1;
    bool meta_update=false;
    long relocated_addr=0;
    bool is_overflow=false;
    //xinw added for morphable-end 
 
   // gagan : is prefetch
    bool is_prefetch;

 Request(long addr, Type type, int coreid = 0)
      : is_first_command(true), addr(addr), coreid(coreid), type(type), callback([](Request& req){}), is_prefetch(false) {}

 Request(long addr, Type type, function<void(Request&)> callback,  bool is_prefetch, int coreid = 0)
   : is_first_command(true), addr(addr), coreid(coreid), type(type), callback(callback), is_prefetch(is_prefetch) {}

 Request(vector<int>& addr_vec, Type type, function<void(Request&)> callback, bool is_prefetch = false, int coreid = 0)
      : is_first_command(true), addr_vec(addr_vec), coreid(coreid), type(type), callback(callback) {}

    Request()
        : is_first_command(true), coreid(0) {}
 //xinw added for morphable-begin
     Request(long addr, Type type, function<void(Request&)> callback, int coreid, bool need_meta, int meta_level, bool meta_update)
        : is_first_command(true), addr(addr), coreid(coreid), type(type), callback(callback), need_meta(need_meta), meta_level(meta_level), meta_update(meta_update) {}
    //xinw added for morphable-end

    void printRead()
    {
      std::cout << std::dec <<  "\t\tcolumn: " << this->addr_vec[5] << ", chan: " << this->addr_vec[0] << ", bank: " << this->addr_vec[3] << ", bankgroup: " << this->addr_vec[2] << ", rank: " << this->addr_vec[1] << ", row: " << this->addr_vec[4] << " for addr: " << this->addr << "\n";
    }
    
    void print()
    {
      switch(type)
	{
	case Request::Type::READ:
	  std::cout << "[READ] pa[ " << std::dec << addr << " ] " << std::dec << " r[" << addr_vec[1] << "] bg[" << addr_vec[2] << "] b["
		    << addr_vec[3] << "] ch[" << addr_vec[0] << "] row[" << addr_vec[4] << "] col[" << addr_vec[5] << "]" << "\t\tcore: " << coreid << std::endl;
	  break;
	case Request::Type::WRITE:
	  //assert(is_prefetch == false);
	  std::cout << "[WRITE] pa[ " << std::dec << addr << " ] " << std::dec << " r[" << addr_vec[1] << "] bg[" << addr_vec[2] << "] b["
		    << addr_vec[3] << "] ch[" << addr_vec[0] << "] row[" << addr_vec[4] << "] col[" << addr_vec[5] << "]" << "\t\tcore: " << coreid << std::endl;
	  break;
	case Request::Type::REFRESH:
	  //assert(is_mdc == false);
	  std::cout << "[REFRESH] pa[ " << std::dec << addr << " ] " << std::dec << " r[" << addr_vec[1] << "] bg[" << addr_vec[2] << "] b["
		    << addr_vec[3] << "] ch[" << addr_vec[0] << "] row[" << addr_vec[4] << "] col[" << addr_vec[5] << "]" << "\t\tcore: " << coreid << std::endl;
	  break;
	case Request::Type::POWERDOWN:
	  //assert(is_mdc == false);
	  std::cout << "[POWERDOWN] pa[ " << std::dec << addr << " ] " << std::dec << " r[" << addr_vec[1] << "] bg[" << addr_vec[2] << "] b["
		    << addr_vec[3] << "] ch[" << addr_vec[0] << "] row[" << addr_vec[4] << "] col[" << addr_vec[5] << "]" << "\t\tcore: " << coreid << std::endl;
	  break;
	case Request::Type::SELFREFRESH:
	  //assert(is_mdc == false);
	  std::cout << "[SELFREFRESH] pa[ " << std::dec << addr << " ] " << std::dec << " r[" << addr_vec[1] << "] bg[" << addr_vec[2] << "] b["
		    << addr_vec[3] << "] ch[" << addr_vec[0] << "] row[" << addr_vec[4] << "] col[" << addr_vec[5] << "]" << "\t\tcore: " << coreid << std::endl;
	  break;
	case Request::Type::EXTENSION:
	  //assert(is_mdc == false);
	  std::cout << "[EXTENSION] pa[ " << std::dec << addr << " ] " << std::dec << " r[" << addr_vec[1] << "] bg[" << addr_vec[2] << "] b["
		    << addr_vec[3] << "] ch[" << addr_vec[0] << "] row[" << addr_vec[4] << "] col[" << addr_vec[5] << "]" << "\t\tcore: " << coreid << std::endl;
	  break;
	case Request::Type::MAX:
	  //assert(is_mdc == false);
	  std::cout << "[MAX] pa[ " << std::dec << addr << " ] " << std::dec << " r[" << addr_vec[1] << "] bg[" << addr_vec[2] << "] b["
		    << addr_vec[3] << "] ch[" << addr_vec[0] << "] row[" << addr_vec[4] << "] col[" << addr_vec[5] << "]" << "\t\tcore: " << coreid << std::endl;
	  break;
	default:
	  std::cout << "Invalid Request" << "\t\tcore: " << coreid << std::endl;
	}
    }

    int getRank()
    {
      return addr_vec[1];
    }
};

} /*namespace ramulator*/

#endif /*__REQUEST_H*/

