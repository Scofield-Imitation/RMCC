#ifndef __DDR4_H
#define __DDR4_H

#include "DRAM.h"
#include "Request.h"
#include <vector>
#include <functional>

using namespace std;

namespace ramulator
{

class DDR4
{
public:
    static string standard_name;
    enum class Org;
    enum class Speed;
    DDR4(Org org, Speed speed);
    DDR4(const string& org_str, const string& speed_str);
    
    static map<string, enum Org> org_map;
    static map<string, enum Speed> speed_map;
    /* Level */
    enum class Level : int
    { 
        Channel, Rank, BankGroup, Bank, Row, Column, MAX
    };
    
    static std::string level_str [int(Level::MAX)];

    /* Command */
    enum class Command : int
    { 
        ACT, PRE, PREA, 
        RD,  WR,  RDA,  WRA, 
        REF, PDE, PDX,  SRE, SRX, 
        MAX
    };

    string command_name[int(Command::MAX)] = {
        "ACT", "PRE", "PREA", 
        "RD",  "WR",  "RDA",  "WRA", 
        "REF", "PDE", "PDX",  "SRE", "SRX"
    };

    Level scope[int(Command::MAX)] = {
        Level::Row,    Level::Bank,   Level::Rank,   
        Level::Column, Level::Column, Level::Column, Level::Column,
        Level::Rank,   Level::Rank,   Level::Rank,   Level::Rank,   Level::Rank
    };

    bool is_opening(Command cmd) 
    {
        switch(int(cmd)) {
            case int(Command::ACT):
                return true;
            default:
                return false;
        }
    }

    bool is_accessing(Command cmd) 
    {
        switch(int(cmd)) {
            case int(Command::RD):
            case int(Command::WR):
            case int(Command::RDA):
            case int(Command::WRA):
                return true;
            default:
                return false;
        }
    }

    bool is_closing(Command cmd) 
    {
        switch(int(cmd)) {
            case int(Command::RDA):
            case int(Command::WRA):
            case int(Command::PRE):
            case int(Command::PREA):
                return true;
            default:
                return false;
        }
    }

    bool is_refreshing(Command cmd) 
    {
        switch(int(cmd)) {
            case int(Command::REF):
                return true;
            default:
                return false;
        }
    }

    /* State */
    enum class State : int
    {
        Opened, Closed, PowerUp, ActPowerDown, PrePowerDown, SelfRefresh, MAX
    } start[int(Level::MAX)] = {
        State::MAX, State::PowerUp, State::MAX, State::Closed, State::Closed, State::MAX
    };

    /* Translate */
    Command translate[int(Request::Type::MAX)] = {
        Command::RD,  Command::WR,
        Command::REF, Command::PDE, Command::SRE
    };

    /* Prereq */
    function<Command(DRAM<DDR4>*, Command cmd, int)> prereq[int(Level::MAX)][int(Command::MAX)];

    // SAUGATA: added function object container for row hit status
    /* Row hit */
    function<bool(DRAM<DDR4>*, Command cmd, int)> rowhit[int(Level::MAX)][int(Command::MAX)];
    function<bool(DRAM<DDR4>*, Command cmd, int)> rowopen[int(Level::MAX)][int(Command::MAX)];

    /* Timing */
    struct TimingEntry
    {
        Command cmd;
        int dist;
        int val;
        bool sibling;
    }; 
    vector<TimingEntry> timing[int(Level::MAX)][int(Command::MAX)];

    /* Lambda */
    function<void(DRAM<DDR4>*, int)> lambda[int(Level::MAX)][int(Command::MAX)];

    /* Organization */
    enum class Org : int
    {
        DDR4_2Gb_x4,   DDR4_2Gb_x8,   DDR4_2Gb_x16,
        DDR4_4Gb_x4,   DDR4_4Gb_x8,   DDR4_4Gb_x16,
        DDR4_8Gb_x4,   DDR4_8Gb_x8,   DDR4_8Gb_x16,
        DDR4_4Gb_x8_w16, DDR4_4Gb_x8_w32, DDR4_4Gb_x8_w64,
        DDR4_4Gb_x8_w8,
        DDR4_4Gb_x8_4xBank,
        DDR4_4Gb_x8_2xBank,
        MAX
    };

    struct OrgEntry {
        int size;
        int dq;
        int count[int(Level::MAX)];
    } org_table[int(Org::MAX)] = {
        {2<<10,  4, {0, 0, 4, 4, 1<<15, 1<<10}}, {2<<10,  8, {0, 0, 4, 4, 1<<14, 1<<10}}, {2<<10, 16, {0, 0, 2, 4, 1<<14, 1<<10}},
        //{4<<10,  4, {0, 0, 4, 4, 1<<16, 1<<10}}, {4<<10,  8, {0, 0, 4, 4, 1<<15, 1<<10}}, {4<<10, 16, {0, 0, 2, 4, 1<<15, 1<<10}},
        //xinw changed for bigger dram
	// {4<<10,  4, {0, 0, 4, 4, 1<<16, 1<<10}}, {4<<10,  8, {0, 0, 4, 4, 1<<19, 1<<10}}, {4<<10, 16, {0, 0, 2, 4, 1<<15, 1<<10}},
	 {4<<10,  4, {0, 0, 4, 4, 1<<16, 1<<10}}, {4<<10,  8, {0, 0, 4, 4, 1<<20, 1<<10}}, {4<<10, 16, {0, 0, 2, 4, 1<<15, 1<<10}},
        {8<<10,  4, {0, 0, 4, 4, 1<<17, 1<<10}}, {8<<10,  8, {0, 0, 4, 4, 1<<16, 1<<10}}, {8<<10, 16, {0, 0, 2, 4, 1<<16, 1<<10}},
        {4<<10,  8, {0, 0, 4, 4, 1<<15, 1<<10}}, {4<<10,  8, {0, 0, 4, 4, 1<<15, 1<<10}}, {4<<10,  8, {0, 0, 4, 4, 1<<15, 1<<10}},//16,32,64B cacheline
        {4<<10,  8, {0, 0, 4, 4, 1<<15, 1<<10}},//8B cacheline
        {4<<10,  8, {0, 0, 4, 16, 1<<15, 1<<10}},
        {4<<10,  8, {0, 0, 4, 8, 1<<15, 1<<10}}
    }, org_entry;

    void set_channel_number(int channel);
    void set_rank_number(int rank);

    /* Speed */
    enum class Speed : int
    {
        DDR4_100K,
        DDR4_200K,
        DDR4_400K,
        DDR4_800K,
        DDR4_1600_base,
        DDR4_1600K,
	DDR4_1600L,
        DDR4_1866M,
	DDR4_1866N,
        DDR4_2133P,
	DDR4_2133R,
        DDR4_2400R,
        // gagan
        DDR4_2400R_base,
	DDR4_2400R_ideal_nbr_lbb,
	DDR4_2933R_ideal_nbr_lbb_ts,
	DDR4_2933R_ideal_nbr_lbb_sts,
        // Ramulator
	DDR4_2400U,
        DDR4_3200,
        // daz3
        DDR4_3200_base,
	DDR4_3200_base_reduced,
        DDR4_3200_ideal_v1,// tRFC=0
        DDR4_3200_ideal_v2,// tRFC=0,tWTR_*=0
        DDR4_3200_ideal_v2_modified,// tRFC=0,tWTR_*=0
        DDR4_3200_ideal_v3,// tRFC=0,tWTR_*=0,tRRD_*=0,tFAW_*=0
        DDR4_3200_ideal_v4,// tRFC=0,tWTR_*=0,tRRD_*=0,tFAW_*=0,tRDC*75%,tRP*75%
        DDR4_3200_ideal_v7,// tRFC=0
	DDR4_3200_ideal_v4s,
	DDR4_3200_ideal_v5a,
	DDR4_3600_ideal_v4,
	DDR4_3866_ideal_v4,
	DDR4_3200_ideal_nbr_lbb, // tRFC=0,tWTR_=0,tWR=0,tRRD_=0,tFAW_=0
	DDR4_3866_ideal_nbr_lbb_ts, // tRFC=0,tWTR_=0,tWR=0,tRRD_=0,tFAW_=0,tRDC*=75%,tRP*=75%
	DDR4_3866_ideal_nbr_lbb_sts, // tRFC=0,tWTR_=0,tWR=0,tRRD_=0,tFAW_=0,tRDC=1,tRP=1
        DDR4_3200_base_half_w8,// for different cacheline size
        DDR4_3200_base_half_w16,// for different cacheline size
        DDR4_3200_base_half_w32,
        DDR4_3200_base_half_w64,
        DDR4_3200_base_full_w8,// for different cacheline size
        DDR4_3200_base_full_w16,// for different cacheline size
        DDR4_3200_base_full_w32,
        DDR4_3200_base_full_w64,
        DDR4_3200_base_quarter_w8,// for different cacheline size
        DDR4_3200_base_quarter_w16,// for different cacheline size
        DDR4_3200_base_quarter_w32,
        DDR4_3200_base_quarter_w64,
        DDR4_3734_reduced_tRCD_tRP,
        DDR4_2666_base,
        DDR4_4000_base,
        DDR4_4000_base_reduced,
        DDR4_2000_base,
        MAX
    }mySpeed;
    // };

    enum class RefreshMode : int
    {
        Refresh_1X,
        Refresh_2X,
        Refresh_4X,
        MAX
    } refresh_mode = RefreshMode::Refresh_1X;

    int prefetch_size = 8; // 8n prefetch DDR
    int channel_width = 64;

    struct SpeedEntry {
        int rate;
        double freq, tCK;
        int nBL, nCCDS, nCCDL, nRTRS;
        int nCL, nRCD, nRP, nCWL;
        int nRAS, nRC;
        int nRTP, nWTRS, nWTRL, nWR;
        int nRRDS, nRRDL, nFAW;
        int nRFC, nREFI;
        int nPD, nXP, nXPDLL; // XPDLL not found in DDR4??
        int nCKESR, nXS, nXSDLL; // nXSDLL TBD (nDLLK), nXS = (tRFC+10ns)/tCK
    } speed_table[int(Speed::MAX)] = {
        {100, 50, 		20,    4, 4, 0, 2, 1,  1,   1,  0, 1, 2, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {200, 100, 		10,    4, 4, 0, 2, 2,  2,   2,  1, 3, 5, 1, 0,  1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {400, 200, 		5,    4, 4, 1, 2, 3,  3,   3,  2, 7, 10, 2, 0,  2, 3, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0},
        {800, 400, 		2,    4, 4, 2, 2, 5,  5,   5,  4, 14, 19, 3, 1, 3, 6, 0, 0, 0, 0, 0, 2,  3, 0, 2, 0, 0},
        {1600, (400.0/3)*6, (3/0.4)/6, 4, 4, 5, 2, 11, 11, 11,  9, 28, 39, 6, 2, 6, 12, 0, 0, 0, 0, 0, 4, 5, 0, 5, 0, 0},
        {1600, (400.0/3)*6, (3/0.4)/6, 4, 4, 5, 2, 11, 11, 11,  9, 28, 39, 6, 2, 6, 12, 0, 0, 0, 0, 0, 4, 5, 0, 5, 0, 0},
        {1600, (400.0/3)*6, (3/0.4)/6, 4, 4, 5, 2, 12, 12, 12,  9, 28, 40, 6, 2, 6, 12, 0, 0, 0, 0, 0, 4, 5, 0, 5, 0, 0},
        {1866, (400.0/3)*7, (3/0.4)/7, 4, 4, 5, 2, 13, 13, 13, 10, 32, 45, 7, 3, 7, 14, 0, 0, 0, 0, 0, 5, 6, 0, 6, 0, 0},
        {1866, (400.0/3)*7, (3/0.4)/7, 4, 4, 5, 2, 14, 14, 14, 10, 32, 46, 7, 3, 7, 14, 0, 0, 0, 0, 0, 5, 6, 0, 6, 0, 0},
        {2133, (400.0/3)*8, (3/0.4)/8, 4, 4, 6, 2, 15, 15, 15, 11, 36, 51, 8, 3, 8, 16, 0, 0, 0, 0, 0, 6, 7, 0, 7, 0, 0},
        {2133, (400.0/3)*8, (3/0.4)/8, 4, 4, 6, 2, 16, 16, 16, 11, 36, 52, 8, 3, 8, 16, 0, 0, 0, 0, 0, 6, 7, 0, 7, 0, 0},
        {2400, (400.0/3)*9, (3/0.4)/9, 4, 4, 6, 2, 16, 16, 16, 12, 39, 55, 9, 3, 9, 18, 0, 0, 0, 0, 0, 6, 8, 0, 7, 0, 0},
	// gagan
        {2400, 1200,   0.833, prefetch_size/2/*DDR*/,   4,    6,    2, 16, 16,  16, 12,  39, 55,   9,    3,     9,  18,   4,   6,   26, 660,  9360,    6,    8,    0,   7,   672,  768},
        {2400, 1200,   0.833, prefetch_size/2/*DDR*/,   4,    6,    2, 16, 16,  16, 12,  39, 55,   9,    0,     0,   0,   0,   0,    0,   0,  9360,    6,    8,    0,   7,   672,  768},
	{2933, 2933/2, 0.681, prefetch_size/2/*DDR*/,   4,    7,    2, 16, 20,  20, 15,  47, 67,  11,    0,     0,   0,   0,   0,    0,   0, 11439,    7,   10,    0,   9,   821,  939},
	{2933, 2933/2, 0.681, prefetch_size/2/*DDR*/,   4,    7,    2, 16, 20,  20, 15,  47, 67,  11,    0,     0,   0,   0,   0,    0,   0, 11439,    7,   10,    0,   9,   821,  939},
	//rate, freq,    tCK,             nBL,    nCCDS nCCDL nRTRS nCL nRCD nRP nCWL nRAS nRC nRTP nWTRS nWTRL nWR nRRDS nRRDL nFAW nRFC  nREFI   nPD   nXP  nXPDLL nCKESR nXS  nXSDLL
	// Ramulator
        {2400, (400.0/3)*9, (3/0.4)/9, 4, 4, 6, 2, 18, 18, 18, 12, 39, 57, 9, 3, 9, 18, 0, 0, 0, 0, 0, 6, 8, 0, 7, 0, 0},
        {3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,     10,   2,    22, 22,  22, 16,  56,  78, 12,  4,    12,   24, 8,    10,   40,  0,   0,    8,  10, 0,     8,     0,  0},
        // daz3
        // {3200, 1600, 0.625, prefetch_size/2[>DDR<], 4,      8,   2,    22, 22,  22, 20,  52,  74, 12,  4,    12,   24, 4,     8,   34,  880, 12480,   8,   10,  0,   9,   896, 1024},
	//xinw increased the nREFI to verfy big to elimiate refresh
        //{3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   2,    22, 22,  22, 20,  52,  74, 12,  4,    12,   24, 4,     8,   34,  560, 12480,   0,    0,  0,   0,     0,    0},
        //{3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   2,    22, 22,  22, 20,  52,  74, 12,  4,    12,   24, 4,     8,   34,  560, 1800000000,   0,    0,  0,   0,     0,    0},
        //xinw set the nRTRS to 0, to eliminate the latency for  switching between ranks.
        {3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   0,    22, 22,  22, 20,  52,  74, 12,  4,    12,   24, 4,     8,   34,  560, 12480,   0,    0,  0,   0,     0,    0},
	//1600 base
        //{1600, 800, 1.25, prefetch_size/2/*DDR*/, 4,        4,   0,    11, 11,  11, 10,  26,  37,  6,  2,    6,    12, 2,     4,   17,  280, 6240,   0,    0,  0,   0,     0,    0},
	
	//800 base
        //{800, 400, 2.5, prefetch_size/2/*DDR*/, 4,        2,   0,    5,    5,    5,  5,  13,  19,  3,  1,    3,    6,  1,     2,   9,   140, 3120,   0,    0,  0,   0,     0,    0},

	//400 base
        //{400, 200, 5, prefetch_size/2/*DDR*/, 4,        1,   0,       3,    3,    3,  3,  6,  9,   1,  0,    1,    3,  0,     1,   4,   70,  1560,   0,    0,  0,   0,     0,    0},

	//xinw set the nWTRS, nWTRL to 0, the eliminate the latency for switching between read and write.
        //{3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   0,    22, 22,  22, 20,  52,  74, 12,  0,    0,   24, 4,     8,   34,  560, 12480,   0,    0,  0,   0,     0,    0},
	//xinw increased the nREFI to verfy big to elimiate refresh
        //{3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   0,    22, 22,  22, 20,  52,  74, 12,  4,    12,   24, 4,     8,   34,  560, 1800000000,   0,    0,  0,   0,     0,    0},

	{3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   2,    22, 19,  20, 20,  33,  74, 12,  4,    12,   24, 4,     8,   34,  560, 24960,   0,    0,  0,   0,     0,    0},
        {3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   2,    22, 22,  22, 20,  52,  74, 12,  0,     0,   24, 4,     8,   34,  560, 12480,   8,   10,  0,   9,   896, 1024},
        {3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   2,    22, 22,  22, 20,  52,  74, 12,  0,     0,   24, 4,     8,   34,    0, 12480,   8,   10,  0,   9,   896, 1024},
        {3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   2,    22, 22,  22, 20,  52,  74, 12,  0,     0,   24, 4,     8,   48,    0, 12480,   8,   10,  0,   9,   896, 1024},
        {3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   2,    22, 22,  22, 20,  52,  74, 12,  0,     0,   24, 0,     0,    0,    0, 12480,   8,   10,  0,   9,   896, 1024},
        {3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   2,    22, 22,  22, 20,  52,  74, 12,  0,     0,   24, 0,     0,    0,    0, 12480,   8,   10,  0,   9,   896, 1024},
        {3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   2,    22, 22,  22, 20,  52,  74, 12,  4,    12,   24, 4,     8,   34,    0, 12480,   8,   10,  0,   9,   896, 1024},
	{3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   2,    22, 22,  22, 20,  52,  74, 12,  0,     0,   24, 0,     0,    0,    0, 12480,   8,   10,  0,   9,   896, 1024},
	{3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   2,    22, 22,  22, 20,  52,  74, 12,  0,     0,   24, 0,     0,    0,    0, 12480,   8,   10,  0,   9,   896, 1024},
	{3600, 1800, 0.555, prefetch_size/2/*DDR*/, 4,      9,   2,    25, 25,  25, 22,  57,  82, 13,  0,     0,   27, 0,     0,    0,    0, 14040,   9,   11,  0,   10, 1008, 1152},
	{3866, 1933, 0.517, prefetch_size/2/*DDR*/, 4,     10,   2,    27, 27,  27, 24,  61,  88, 14,  0,     0,   29, 0,     0,    0,    0, 15077,  10,   12,  0,   11, 1083, 1237},
	{3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   2,    22, 22,  22, 20,  52,  74, 12,  0,     0,    0, 0,     0,    0,    0, 12480,   8,   10,  0,   9,   896, 1024},
	{3866, 1933, 0.517, prefetch_size/2/*DDR*/, 4,     10,   2,    27, 27,  27, 24,  61,  88, 14,  0,     0,    0, 0,     0,    0,    0, 15077,  10,   12,  0,   11, 1083, 1237},
	{3866, 1933, 0.517, prefetch_size/2/*DDR*/, 4,     10,   2,    27, 27,  27, 24,  61,  88, 14,  0,     0,    0, 0,     0,    0,    0, 15077,  10,   12,  0,   11, 1083, 1237},
	//{3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   2,    22,  0,   0, 20,  10,  32, 12,  0,     0,   24, 0,     0,    0,    0, 12480,   8,   10,  0,   9,   896, 1024},
	//rate, freq, tCK,  nBL,                    nCCDS nCCDL nRTRS nCL nRCD nRP nCWL nRAS nRC nRTP nWTRS nWTRL nWR nRRDS nRRDL nFAW nRFC nREFI   nPD   nXP  nXPDLL nCKESR nXS  nXSDLL

        {12800, 6400, 0.15625, prefetch_size*2/*DDR*/, 16,      32,   8,    88, 88,  88, 80, 208,  296, 48,  0,     0,   96, 0,     0,    0,    0, 49920,   32,   40,  0,   36,   3584, 4096},
        {6400, 3200, 0.3125, prefetch_size/*DDR*/, 8,      16,   4,    44, 44,  44, 40,  104,  148, 24,  0,     0,   48, 0,     0,    0,    0, 24960,   16,   20,  0,   18,   1792, 2048},
        {3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   2,    22, 22,  22, 20,  52,  74, 12,  4,    12,   24, 4,     8,   34,  880, 12480,   8,   10,  0,   9,   896, 1024},
        {1600, 800, 1.25, prefetch_size/4/*DDR*/, 2,      4,   1,    11, 11,  11, 10,  26,  37, 6,  0,     0,   12, 0,     0,    0,    0, 6240,   4,   5,  0,   5,   448, 512},

        {25600, 12800, 0.078125, prefetch_size*2/*DDR*/, 32,      64,   16,    176, 176,  176, 160, 416,  592, 96,  0,     0,   192, 0,     0,    0,    0, 99840,   64,   80,  0,   72,   7168, 8192},
        {12800, 6400, 0.15625, prefetch_size*2/*DDR*/, 16,      32,   8,    88, 88,  88, 80, 208,  296, 48,  0,     0,   96, 0,     0,    0,    0, 49920,   32,   40,  0,   36,   3584, 4096},
        {6400, 3200, 0.3125, prefetch_size/*DDR*/, 8,      16,   4,    44, 44,  44, 40,  104,  148, 24,  0,     0,   48, 0,     0,    0,    0, 24960,   16,   20,  0,   18,   1792, 2048},
        {3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   2,    22, 22,  22, 20,  52,  74, 12,  4,    12,   24, 4,     8,   34,  880, 12480,   8,   10,  0,   9,   896, 1024},

        {6400, 3200, 0.3125, prefetch_size/*DDR*/, 8,      16,   4,    44, 44,  44, 40,  104,  148, 24,  0,     0,   48, 0,     0,    0,    0, 24960,   16,   20,  0,   18,   1792, 2048},
        {3200, 1600, 0.625, prefetch_size/2/*DDR*/, 4,      8,   2,    22, 22,  22, 20,  52,  74, 12,  4,    12,   24, 4,     8,   34,  880, 12480,   8,   10,  0,   9,   896, 1024},
        {1600, 800, 1.25, prefetch_size/4/*DDR*/, 2,      4,   1,    11, 11,  11, 10,  26,  37, 6,  0,     0,   12, 0,     0,    0,    0, 6240,   4,   5,  0,   5,   448, 512},
        {800, 400, 2.5, prefetch_size/8/*DDR*/, 1,      2,   1,    6, 6,  6, 5,  13,  14, 3,  0,     0,   6, 0,     0,    0,    0, 3120,   2,   3,  0,   3,   224, 256},
        //rate, freq, tCK,  nBL, nCCDS nCCDL nRTRS nCL nRCD nRP nCWL nRAS nRC nRTP nWTRS nWTRL nWR nRRDS nRRDL nFAW nRFC nREFI nPD nXP nXPDLL nCKESR nXS nXSDLL
        // rate, freq, tCK,  nBL,                  nCCDS  nCCDL nRTRS nCL nRCD nRP nCWL nRAS nRC nRTP nWTRS nWTRL nWR nRRDS nRRDL 480 nFAW nRFC nREFI nPD nXP nXPDLL nCKESR nXS nXSDLL
        {3734, 1867, 0.537, prefetch_size/2/*DDR*/, 4,      10,   3,    26, 26,  26, 24,  61,  87, 14,  5,    14,   28, 5,     10,   40,   560, 14563,   12,   12,  0,   11,   1046, 1195},
        {2666, 1333, 0.75, prefetch_size/2/*DDR*/, 4,        7,   2,    19, 19,  19, 17,  44,  62, 10,  4,    10,   20, 4,     7,    29,   734, 10400,   7,   9,   0,   8,   747, 854},
	//xinw set the tRTRS to zero
        //{2666, 1333, 0.75, prefetch_size/2/*DDR*/, 4,        7,   0,    19, 19,  19, 17,  44,  62, 10,  4,    10,   20, 4,     7,    29,   734, 10400,   7,   9,   0,   8,   747, 854},
        {4000, 2000, 0.5, prefetch_size/2/*DDR*/,  4,        8,   2,    22, 27,  27, 25,  65,  92, 15,  4,    15,   30,  4,    10,   42,   700, 15600,   0,    0,  0,    0,      0,    0},
        {4000, 2000, 0.5, prefetch_size/2/*DDR*/,  4,        8,   2,    22, 23,  22, 25,  44,  66, 15,  4,    15,   30, 4,     10,   42,   700, 31200,   0,    0,  0,    0,      0,    0},
	{2000, 1000, 1.0, prefetch_size/2/*DDR*/,    4,        4,   0,    12, 14,  14, 10,   32,  46,  8,  2,     8,   15,   2,    5,    21,   350,  7800,   0,    0,  0,    0,      0,    0},
    }, speed_entry;

    int read_latency;

private:
    void init_speed();
    void init_lambda();
    void init_prereq();
    void init_rowhit();  // SAUGATA: added function to check for row hits
    void init_rowopen();
    void init_timing();
};

} /*namespace ramulator*/

#endif /*__DDR4_H*/
