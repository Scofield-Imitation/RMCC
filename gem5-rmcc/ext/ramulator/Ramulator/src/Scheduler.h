#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include "DRAM.h"
#include "Request.h"
#include "Controller.h"
#include <vector>
#include <map>
#include <list>
#include <functional>
#include <cassert>
#include <climits>

using namespace std;

namespace ramulator
{

template <typename T>
class Controller;

template <typename T>
class Scheduler
{
public:
    Controller<T>* ctrl;

    enum class Type {
      FCFS, FRFCFS, FRFCFS_Cap, FRFCFS_PriorHit, FRFCFS_DepPre, MAX
    } type = Type::FRFCFS;
     //type = Type::FRFCFS_PriorHit;
    //} type = Type::FCFS;

    long cap = 3;

    std::vector<bool> rankHasReadHits;

    Scheduler(Controller<T>* ctrl) : ctrl(ctrl) {}

    list<Request>::iterator get_head(list<Request>& q)
    {
      // TODO make the decision at compile time
      if (type != Type::FRFCFS_PriorHit) {
        if (!q.size())
            return q.end();

	// gagan: find which ranks have reads
	rankHasReadHits.resize(ctrl->channel->children.size(), false);
	for (auto req = q.begin(); req != q.end(); req++) {
	  if(req->type == Request::Type::READ && this->ctrl->is_row_hit(req)) { 
	      rankHasReadHits[ req->addr_vec[1] ] = true;
	  }
	}
	
        auto head = q.begin();
        for (auto itr = next(q.begin(), 1); itr != q.end(); itr++)
            head = compare[int(type)](head, itr);

	rankHasReadHits.clear();
	if(this->ctrl->enable_debug) {
	  std::cout << "Winner compare(): "; head->print();
	}

	// gagan : don't issue anything if the following conditions are met
	int winnerRank = head->addr_vec[1];
	int winnerBankgroup = head->addr_vec[2];
	int winnerBank = head->addr_vec[3];
	int winnerRow = head->addr_vec[4];
	if(this->ctrl->rowtable->get_hits(head->addr_vec) > this->cap) { // winner's row is open and exceeds the cap
	  for (auto req = q.begin(); req != q.end(); req++) {
	    if(winnerRank == req->addr_vec[1]) {
		if(!(winnerBankgroup == req->addr_vec[2] &&
		     winnerBank == req->addr_vec[3] &&
		     winnerRow == req->addr_vec[4]) &&
		   (head->type == Request::Type::WRITE && req->type == Request::Type::READ)) {
		  if(this->ctrl->is_row_hit(req)) { // There are activated reqs to winner's rank BUT not to winner's row AND req type is different
		    if(this->ctrl->enable_debug) {
		      std::cout << "Winner after rank conflict check: "; req->print();
		    }
		    return req; // old: don't do anything
		  }		  
		}
		if(winnerBankgroup == req->addr_vec[2] &&
		   winnerBank == req->addr_vec[3] &&
		   winnerRow != req->addr_vec[4] &&
		   //xinw: if head is overflow request and req is normal read request
		   //(head->type == Request::Type::WRITE && req->type == Request::Type::READ)) { // There are conflicting reqs to winner's bank
		   ((head->is_overflow||(head->type == Request::Type::WRITE)) && req->type == Request::Type::READ && (!req->is_overflow))) { // There are conflicting reqs to winner's bank
		  if(this->ctrl->enable_debug) {
		    std::cout << "Winner after bank conflict check: "; req->print();
		  }
		  return req; // old: don't do anything
		}
	    }
	  }
	}

	if(this->ctrl->enable_debug)
	  std::cout << "compare() Winner returned" << std::endl;
		
        return head;
      } else {
        if (!q.size())
            return q.end();

        auto head = q.begin();
        for (auto itr = next(q.begin(), 1); itr != q.end(); itr++) {
            head = compare[int(Type::FRFCFS_PriorHit)](head, itr);
        }

        if (this->ctrl->is_ready(head) && this->ctrl->is_row_hit(head)) {
          return head;
        }

        // prepare a list of hit request
        vector<vector<int>> hit_reqs;
        for (auto itr = q.begin() ; itr != q.end() ; ++itr) {
          if (this->ctrl->is_row_hit(itr)) {
            auto begin = itr->addr_vec.begin();
            // TODO Here it assumes all DRAM standards use PRE to close a row
            // It's better to make it more general.
            auto end = begin + int(ctrl->channel->spec->scope[int(T::Command::PRE)]) + 1;
            vector<int> rowgroup(begin, end); // bank or subarray
            hit_reqs.push_back(rowgroup);
          }
        }
        // if we can't find proper request, we need to return q.end(),
        // so that no command will be scheduled
        head = q.end();
        for (auto itr = q.begin(); itr != q.end(); itr++) {
          bool violate_hit = false;
          if ((!this->ctrl->is_row_hit(itr)) && this->ctrl->is_row_open(itr)) {
            // so the next instruction to be scheduled is PRE, might violate hit
            auto begin = itr->addr_vec.begin();
            // TODO Here it assumes all DRAM standards use PRE to close a row
            // It's better to make it more general.
            auto end = begin + int(ctrl->channel->spec->scope[int(T::Command::PRE)]) + 1;
            vector<int> rowgroup(begin, end); // bank or subarray
            for (const auto& hit_req_rowgroup : hit_reqs) {
              if (rowgroup == hit_req_rowgroup) {
                  violate_hit = true;
                  break;
              }
            }
          }
          if (violate_hit) {
            continue;
          }
          // If it comes here, that means it won't violate any hit request
          if (head == q.end()) {
            head = itr;
          } else {
            head = compare[int(Type::FRFCFS)](head, itr);
          }
        }

        return head;
      }
    }

private:
    typedef list<Request>::iterator ReqIter;
    function<ReqIter(ReqIter, ReqIter)> compare[int(Type::MAX)] = {
        // FCFS
        [this] (ReqIter req1, ReqIter req2) {
            if (req1->arrive <= req2->arrive) return req1;
            return req2;},

        // FRFCFS
        [this] (ReqIter req1, ReqIter req2) {

	  bool ready1 = this->ctrl->is_ready(req1);
	  bool ready2 = this->ctrl->is_ready(req2);

	  bool isHit1 = this->ctrl->is_row_hit(req1);
	  bool isHit2 = this->ctrl->is_row_hit(req2);
	  
	//xinw: add limit, only prioritize normal read request
	  bool isL3ReadReq1 = (!req1->is_overflow)&&(req1->type ==Request::Type::READ);
	  bool isL3ReadReq2 = (!req2->is_overflow)&&(req2->type ==Request::Type::READ);
	  //bool isL3Req1 = !req1->is_req_gen;
	  //bool isL3Req2 = !req2->is_req_gen;
	  
	  bool isReadReq1 = (req1->type == Request::Type::READ);
	  bool isReadReq2 = (req2->type == Request::Type::READ);

	  bool capNotReached1 = (this->ctrl->rowtable->get_hits(req1->addr_vec) <= this->cap);
	  bool capNotReached2 = (this->ctrl->rowtable->get_hits(req2->addr_vec) <= this->cap);

	  bool rankHasReadHits1 = rankHasReadHits[ req1->addr_vec[1] ];
	  bool rankHasReadHits2 = rankHasReadHits[ req2->addr_vec[1] ];

	  if (ready1 ^ ready2) {
	    if (ready1) return req1;
	    return req2;
	  }

	  if (isReadReq1 ^ isReadReq2) {
	    if (isReadReq1) return req1;
	    return req2;
	  }

	  if (isHit1 ^ isHit2) {
	    if (isHit1) return req1;
	    return req2;
	  }
	  
	  // if both are reads, ignore cap comparison
	  bool bothReqsAreReads = isReadReq1 && isReadReq2;
	  if(!bothReqsAreReads) {
	    if (capNotReached1 ^ capNotReached2) {
	      if (capNotReached1) return req1;
	      return req2;
	    }

	    if (rankHasReadHits1 ^ rankHasReadHits2) {
	      if (rankHasReadHits1) return req2;
	      return req1;
	    }
	  }
	  
	//xinw: add limit, only prioritize normal read request
	/*  if (isL3Req1 ^ isL3Req2) {
	    if (isL3Req1) return req1;
	    return req2;
	  }
	*/	
	  if (isL3ReadReq1 ^ isL3ReadReq2) {
	    if (isL3ReadReq1) return req1;
	    return req2;
	  }
	

	  if (req1->arrive <= req2->arrive) return req1;
	  return req2;
	},

        // FRFCFS_CAP
        [this] (ReqIter req1, ReqIter req2) {
            bool ready1 = this->ctrl->is_ready(req1);
            bool ready2 = this->ctrl->is_ready(req2);

            ready1 = ready1 && (this->ctrl->rowtable->get_hits(req1->addr_vec) <= this->cap);
            ready2 = ready2 && (this->ctrl->rowtable->get_hits(req2->addr_vec) <= this->cap);

            if (ready1 ^ ready2) {
                if (ready1) return req1;
                return req2;
            }

            if (req1->arrive <= req2->arrive) return req1;
            return req2;},
	
        // FRFCFS_PriorHit
        [this] (ReqIter req1, ReqIter req2) {


	  bool ready1 = this->ctrl->is_ready(req1) && this->ctrl->is_row_hit(req1);
	  bool ready2 = this->ctrl->is_ready(req2) && this->ctrl->is_row_hit(req2);

	  
	  if (ready1 ^ ready2) {
	    if (ready1) return req1;
	    return req2;
	  }
	    
	      if (req1->arrive <= req2->arrive)
		return req1;
	      else
		return req2;
	    
	},

	// gagan : FRFCFS_DepPre
	[this] (ReqIter req1, ReqIter req2) {
            bool ready1 = this->ctrl->is_ready(req1);
            bool ready2 = this->ctrl->is_ready(req2);

            if (ready1 ^ ready2) {
                if (ready1) return req1;
                return req2;
            }

	    if (req1->is_prefetch ^ req2->is_prefetch) {
	      if (req1->is_prefetch) return req2;
	      return req1;
	    }

            if (req1->arrive <= req2->arrive) return req1;
            return req2;},

    };
};


template <typename T>
class RowPolicy
{
public:
    Controller<T>* ctrl;

    enum class Type {
        Closed, ClosedAP, Opened, Timeout, MAX
    } type = Type::Timeout;
    // } type = Type::Closed;
    // } type = Type::Opened;

    //int timeout = 200;
    //xinw increased the timeout for debugging microbenchmarks. 
    //int timeout = 1000;
    int timeout = 5000;


    RowPolicy(Controller<T>* ctrl) : ctrl(ctrl) {}

    vector<int> get_victim(typename T::Command cmd)
    {
        return policy[int(type)](cmd);
    }

private:
    function<vector<int>(typename T::Command)> policy[int(Type::MAX)] = {
        // Closed
        [this] (typename T::Command cmd) -> vector<int> {
            for (auto& kv : this->ctrl->rowtable->table) {
                if (!this->ctrl->is_ready(cmd, kv.first))
                    continue;
                return kv.first;
            }
            return vector<int>();},

        // ClosedAP
        [this] (typename T::Command cmd) -> vector<int> {
            for (auto& kv : this->ctrl->rowtable->table) {
                if (!this->ctrl->is_ready(cmd, kv.first))
                    continue;
                return kv.first;
            }
            return vector<int>();},

        // Opened
        [this] (typename T::Command cmd) {
            return vector<int>();},

        // Timeout
        [this] (typename T::Command cmd) -> vector<int> {
            for (auto& kv : this->ctrl->rowtable->table) {
                auto& entry = kv.second;
                if (this->ctrl->clk - entry.timestamp < timeout)
                    continue;
                if (!this->ctrl->is_ready(cmd, kv.first))
                    continue;
                return kv.first;
            }
            return vector<int>();}
    };

};


template <typename T>
class RowTable
{
public:
    Controller<T>* ctrl;

    struct Entry {
        int row;
        int hits;
        long timestamp;
    };

    map<vector<int>, Entry> table;

    RowTable(Controller<T>* ctrl) : ctrl(ctrl) {}

    // gagan : rowtable print
    void print()
    {
      int ch = ctrl->channel->id;
      std::cout << "Channel: " << ch << std::endl;
      for(int ra = 0; ra < ctrl->channel->children.size(); ra++)
	{
	  std::cout << "rank " << ra << "\t\t";
	  for(int bg = 0; bg < 4; bg++)
	    {
	      std::cout << "bg " << bg << "\t";
	      for(int ba = 0; ba < 4; ba++)
		{
		  auto found = table.find({ch, ra, bg, ba});
		  if(found != table.end())
		    {
		      std::cout << "[" << found->second.row << "] ";
		    }
		  else
		    {
		      std::cout << "[Closed] ";
		    }
		}
	      std::cout << ", ";
	    }
	  std::cout << std::endl;
	}
    }

    string status(vector<int> _rowgroup)
    {
      vector<int> rowgroup = { _rowgroup[0], _rowgroup[1], _rowgroup[2], _rowgroup[3] };
      auto found = table.find(rowgroup);
      if(found != table.end())
	{
	  if(found->second.row == _rowgroup[4])
	    return "Row Hit";
	  else
	    return "Row Conflict";
	}
      return "Row Miss";
    }

    void update(typename T::Command cmd, const vector<int>& addr_vec, long clk)
    {
        auto begin = addr_vec.begin();
        auto end = begin + int(T::Level::Row);
        vector<int> rowgroup(begin, end); // bank or subarray
        int row = *end;

        T* spec = ctrl->channel->spec;

        if (spec->is_opening(cmd))
            table.insert({rowgroup, {row, 0, clk}});

        if (spec->is_accessing(cmd)) {
            // we are accessing a row -- update its entry
            auto match = table.find(rowgroup);
            assert(match != table.end());
            assert(match->second.row == row);
            match->second.hits++;
            match->second.timestamp = clk;
        } /* accessing */

        if (spec->is_closing(cmd)) {
          // we are closing one or more rows -- remove their entries
          int n_rm = 0;
          int scope;
          if (spec->is_accessing(cmd))
            scope = int(T::Level::Row) - 1; //special condition for RDA and WRA
          else
            scope = int(spec->scope[int(cmd)]);

          for (auto it = table.begin(); it != table.end();) {
            if (equal(begin, begin + scope + 1, it->first.begin())) {
              n_rm++;
              it = table.erase(it);
            }
            else
              it++;
          }

          assert(n_rm > 0);
        } /* closing */
    }

    int get_hits(const vector<int>& addr_vec, const bool to_opened_row = false)
    {
        auto begin = addr_vec.begin();
        auto end = begin + int(T::Level::Row);

        vector<int> rowgroup(begin, end);
        int row = *end;

        auto itr = table.find(rowgroup);
        if (itr == table.end())
            return 0;

        if(!to_opened_row && (itr->second.row != row))
            return 0;

        return itr->second.hits;
    }

    int get_open_row(const vector<int>& addr_vec) {
        auto begin = addr_vec.begin();
        auto end = begin + int(T::Level::Row);

        vector<int> rowgroup(begin, end);

        auto itr = table.find(rowgroup);
        if(itr == table.end())
            return -1;

        return itr->second.row;
    }
};

} /*namespace ramulator*/

#endif /*__SCHEDULER_H*/
