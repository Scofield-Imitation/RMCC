import os
import re
import sys
import csv
import glob
import string
import StringIO
import ConfigParser
import numpy as np
import scipy.stats.mstats as scm
from natsort import natsorted
from collections import defaultdict

# dimension of list
def dimList(_list):
    if not type(_list) == list:
        return []
    return [len(_list)] + dimList(_list[0])

# appropriately typecast to int, float or string
def num(_str):
    try:
        return int(_str)
    except ValueError:
        try:
            return float(_str)
        except ValueError:
            return _str

def ConfigSectionMap(section):
    dict1 = {}
    options = Config.options(section)
    for option in options:
        try:
            dict1[option] = Config.get(section, option)
            if dict1[option] == -1:
                DebugPrint("skip: %s" % option)
        except:
            print("exception on %s!" % option)
            dict1[option] = None
    return dict1

def parseStats(fileName):
    with open(fileName) as input:
        contentRaw = input.readlines()

    contentRaw = [line.split('#')[0] for line in contentRaw]

    # one final.out file can have multiple simulations
    # begin and endSimMarkers contain begin and end line
    # numbers for all simulations
    beginSimMarker = []
    endSimMarker = []
    lineNum = 0
    for line in contentRaw:
        if "STATS_AFTER_WARMUP" in line:
            beginSimMarker.append(lineNum + 1)
        if "RatioGroupStats:" in line:
            endSimMarker.append(lineNum - 1)
        lineNum += 1
        
    # initialize empty list
    simulationData = [[] for i in range(len(beginSimMarker))]

    for i in range(0, len(beginSimMarker)):
	#print(fileName)
        for j in range(beginSimMarker[i], endSimMarker[i]):
            line = contentRaw[j].split()
            
            data = []
           # if len(line) >0: 
	   #     print line[0]
            if len(line) > 1:
                for k in range(1, len(line)):
                    data.append(num(line[k]))
                simulationData[i].append([line[0], data])
            # simulationData[i].append((line[0], data))

    # print(simulationData[0][0])


    # List that contains dictionaries for each simulation in final.out file
    simulationDictList = []

    # Change simulationData[i] to dictionary
    for i in range(0, len(beginSimMarker)):
        simData = simulationData[i]
        dictData = defaultdict(list)
        for key, value in simData:
            dictData[key].append(value)
        simulationDictList.append(dictData)

    # print(simulationDictList[0]["system.cpu.op_class::No_OpClass"])
    # print(simulationDictList[1]["sim_seconds"])

    # simulationDictList[0] = Atomic, simulationDictList[1] = DerivO3CPU
    return simulationDictList

def getCores(configPath):
    coresList = ['cpu']
    # for single-core, ['cpu']
    # for multi-core, ['cpu00', 'cpu01', 'cpu02']
    return coresList

class Stat:
    def __init__(self, _stat, cores):
        self.stat = _stat
        #self.normalize = _normalizeAfter

        # initialize statVec. Contains key(s) for the final.out dictionary
        # add more statistics here

        if self.stat == 'dcache.WriteReq_accesses::total':
            self.statVec = ['system.' + s + '.' + self.stat for s in cores]
        elif self.stat == 'normalized_TotalVersionsFetches':
	    self.statVec = [['TotalMemFetches:'],['TotalVersionsFetches:']]
        elif self.stat == 'normalized_TotalL1CountersFetches':
	    self.statVec = [['TotalMemFetches:'],['TotalL1CountersFetches:']]
        elif self.stat == 'normalized_TotalL2CountersFetches':  
	    self.statVec = [['TotalMemFetches:'],['TotalL2CountersFetches:']]
        elif self.stat == 'normalized_TotalOverflowFetches':
	    self.statVec = [['TotalMemFetches:'],['TotalOverflowFetches:']]
 	elif self.stat == 'pintool_aes_table_hit_rate' or self.stat == 'pintool_aes_l1_hit_rate':
	    self.statVec = [['OTP_TABLE_Hits:'],['OTP_TABLE_Misses:']]
	elif self.stat == 'otp_table_hit_rate_under_counter_miss':
	    self.statVec = [['OTP_TABLE_Hits_While_Wc_Cache_Misses:'],['OTP_TABLE_Misses_While_Wc_Cache_Misses:']]
        elif self.stat == 'normalzied_TotalMemWriteBacks':
	    self.statVec = [['TotalMemFetches:'],['TotalMemWriteBacks:']]
	elif self.stat == 'normalized_TotalWcCacheWrites':
	    self.statVec = [['TotalMemFetches:'],['TotalWcCacheWrites:']]
	elif self.stat == 'page_l1_miss_ratio':
	    self.statVec = [['L2DataAccesses:'],['L2InstAccesses:'],['L2DataHits:'],['L2InstHits:'],['L1CacheMissesForPT1:'],['L1CacheMissesForPT2:'],['L1CacheMissesForPT3:'],['L1CacheMissesForPT4:']]
	elif self.stat == 'page_l2_miss_ratio':
	    self.statVec = [['L2DataAccesses:'],['L2InstAccesses:'],['L2DataHits:'],['L2InstHits:'],['L2CacheMissesForPT1:'],['L2CacheMissesForPT2:'],['L2CacheMissesForPT3:'],['L2CacheMissesForPT4:']]
	elif self.stat == 'VersionMissRateInPrivateMetadataCache':
	    self.statVec = [['TotalMemFetches:'], ['TotalPageZeroEvents:'],['VersionCountersAccessesInLLCDueToRead:']]
	elif self.stat == 'VersionMissRateInLLC':
	    self.statVec = [['TotalMemFetches:'], ['TotalPageZeroEvents:'],['VersionCountersMissesInLLCDueToRead:']]
	elif self.stat == 'NormalDataMemRead':
	    self.statVec = [['TotalMemFetches:'], ['TotalPageZeroEvents:']]
	elif self.stat == 'NormalDataMemWrite':
	    self.statVec = [['TotalMemWriteBacks:'], ['TotalPageZeroEvents:']]
	elif self.stat == 'MetaMPKIInLLC':
	    self.statVec = [['TotalInstCount:'], ['VersionCountersMissesInLLCDueToRead:']]
	elif self.stat == 'MetaMPKIInPrivateMetaCache':
	    self.statVec = [['TotalInstCount:'], ['VersionCountersAccessesInLLCDueToRead:']]
	elif self.stat == 'WriteMetaMPKIInLLC':
	    self.statVec = [['TotalInstCount:'], ['VersionCountersMissesInLLCDueToWrite:']]
	elif self.stat == 'WriteMetaMPKIInPrivateMetaCache':
	    self.statVec = [['TotalInstCount:'], ['VersionCountersAccessesInLLCDueToWrite:']]
  	elif 'Frequency' in self.stat:
	    self.statVec = [['TotalMemWriteBacks:'],[self.stat]]
	elif self.stat == 'multithread_L1_data_accesses':
	    self.statVec = [['L1_0_DataAccesses:'],['L1_1_DataAccesses:'],['L1_2_DataAccesses:'],['L1_3_DataAccesses:'],['L1_4_DataAccesses:'],['L1_5_DataAccesses:'],['L1_6_DataAccesses:'],['L1_7_DataAccesses:']]
	elif self.stat == 'multithread_L1_data_hits':
	    self.statVec = [['L1_0_DataHits:'],['L1_1_DataHits:'],['L1_2_DataHits:'],['L1_3_DataHits:'],['L1_4_DataHits:'],['L1_5_DataHits:'],['L1_6_DataHits:'],['L1_7_DataHits:']]
	elif self.stat == 'L1_counter_cache_num_of_total_counter_accesses':
	    self.statVec = [['L1_counter_cache_0_num_of_total_counter_accesses:'],['L1_counter_cache_1_num_of_total_counter_accesses:'],['L1_counter_cache_2_num_of_total_counter_accesses:'],['L1_counter_cache_3_num_of_total_counter_accesses:']]
	elif self.stat == 'L1_counter_cache_num_of_useless_counter_accesses':
	    self.statVec = [['L1_counter_cache_0_num_of_useless_counter_accesses:'],['L1_counter_cache_1_num_of_useless_counter_accesses:'],['L1_counter_cache_2_num_of_useless_counter_accesses:'],['L1_counter_cache_3_num_of_useless_counter_accesses:']]
	elif self.stat == 'read_related_traffic_overhead':
	    self.statVec = [['TotalMemFetches:'],['TotalMemWriteBacks:'],['VersionCountersMissesInLLCDueToRead:'],['L1CountersMissesInLLCDueToRead:']]
	elif self.stat == 'write_related_traffic_overhead':
	    self.statVec = [['TotalMemFetches:'],['TotalMemWriteBacks:'],['VersionCountersMissesInLLCDueToWrite:'],['L1CountersMissesInLLCDueToWrite:'],['L2MetadataWritebacks:'],['TotalOverflowFetches:']]   
    	else:
            self.statVec = [self.stat]        
        self.dimStatVec = dimList(self.statVec)
        #print(self.dimStatVec)

    # for handling nested lists in self.statVec
    def helper_filterStatVec(self, a, _dict):
        for i, nestedList in enumerate(self.statVec):
            if isinstance(nestedList, list):
                temp = []
                for i in nestedList:
                    # handle if real simulation crashed
                    try:
                      #  temp.append(_dict[2][i][0][0])
			end_of_window = 0
			if args.whole_life_time == True:
			    end_of_window = int(np.size(_dict, 0))-1
			else:
			    end_of_window = int(args.observed_insts)-1
			    #if args.observed_insts == '0':
			    #	temp.append(_dict[int(np.size(_dict, 0))-1][i][0][0])
			    #else:
			    #	temp.append(_dict[int(np.size(_dict, 0))-1][i][0][0]-_dict[int(np.size(_dict, 0))-1-int(args.observed_insts)][i][0][0])
			if args.skip_insts == '0':
		       	    temp.append(_dict[end_of_window][i][0][0])
		       	    #temp.append(_dict[int(args.observed_insts)-1][i][0][0])
			else:
		       	    temp.append(_dict[end_of_window][i][0][0]-_dict[int(args.skip_insts)-1][i][0][0])
		       	    #temp.append(_dict[int(args.observed_insts)-1][i][0][0]-_dict[int(args.skip_insts)-1][i][0][0])
		       	    #temp.append(_dict[15][i][0][0])
                    except IndexError:
                        temp.append(0)
            a.append(temp)
    
    def filterStatVec(self, _dict):
        a = []
        if any(isinstance(i, list) for i in self.statVec):
            # self.statVec is nested
            self.helper_filterStatVec(a, _dict)
        else:
            # self.statVec is not nested
            for i in self.statVec:
                # handle if real simulation crashed
               try:
		    end_of_window = 0
		    if args.whole_life_time == True:
			 end_of_window = int(np.size(_dict, 0))-1
		    else:
			 end_of_window = int(args.observed_insts)-1
		    if args.skip_insts == '0':
		       	 a.append(_dict[end_of_window][i][0][0])
		    else:
		       	 a.append(_dict[end_of_window][i][0][0]-_dict[int(args.skip_insts)-1][i][0][0])
	
               except IndexError:
                   a.append(0)
                   

        self.np_statVec = np.array(a)


    # do actual work
    def getStat(self):
        # add what needs to be done for this statistic
        if self.stat == 'ipc' or self.stat== 'dcache.WriteReq_accesses::total' or self.stat == 'committedInsts' or self.stat =='l2_pointer_occ':
            return np.sum(self.np_statVec)
        elif self.stat == 'normalzied_TotalMemWriteBacks' or self.stat == 'normalized_TotalWcCacheWrites' or self.stat == 'normalized_TotalVersionsFetches' or self.stat =='normalized_TotalL1CountersFetches' or  self.stat == 'normalized_TotalL2CountersFetches' or self.stat =='normalized_TotalOverflowFetches':
       	    return np.sum(self.np_statVec[1])*1.0/np.sum(self.np_statVec[0])
	elif self.stat == 'pintool_aes_table_hit_rate' or self.stat == 'pintool_aes_l1_hit_rate' or self.stat == 'otp_table_hit_rate_under_counter_miss':
	    return np.sum(self.np_statVec[0])*1.0/(np.sum(self.np_statVec[0])+np.sum(self.np_statVec[1]))
	elif self.stat == 'page_l1_miss_ratio' or self.stat == 'page_l2_miss_ratio':
	    return (np.sum(self.np_statVec[4])+np.sum(self.np_statVec[5])+np.sum(self.np_statVec[6])+np.sum(self.np_statVec[7]))*1.0/(np.sum(self.np_statVec[0])+np.sum(self.np_statVec[1])-np.sum(self.np_statVec[2])-np.sum(self.np_statVec[3]))
	elif 'Frequency' in self.stat:
       	    return np.sum(self.np_statVec[1])*1.0/np.sum(self.np_statVec[0])
	elif self.stat == 'VersionMissRateInLLC' or self.stat == 'VersionMissRateInPrivateMetadataCache':
	    return np.sum(self.np_statVec[2])*1.0/(np.sum(self.np_statVec[0])-32768*np.sum(self.np_statVec[1]))
	elif self.stat == 'NormalDataMemRead' or self.stat == 'NormalDataMemWrite':
	    return (np.sum(self.np_statVec[0])-32768*np.sum(self.np_statVec[1]))
	elif self.stat == 'MetaMPKIInPrivateMetaCache' or self.stat == 'MetaMPKIInLLC' or self.stat == 'WriteMetaMPKIInPrivateMetaCache' or self.stat == 'WriteMetaMPKIInLLC':
	    return np.sum(self.np_statVec[1])*1000.0/np.sum(self.np_statVec[0])
	elif self.stat == 'multithread_L1_data_hits' or self.stat == 'multithread_L1_data_accesses':
	    return (np.sum(self.np_statVec[0])+np.sum(self.np_statVec[1])+np.sum(self.np_statVec[2])+np.sum(self.np_statVec[3])+np.sum(self.np_statVec[4])+np.sum(self.np_statVec[5])+np.sum(self.np_statVec[6])+np.sum(self.np_statVec[7]))
	elif self.stat == 'L1_counter_cache_num_of_total_counter_accesses' or self.stat == 'L1_counter_cache_num_of_useless_counter_accesses':
	    return (np.sum(self.np_statVec[0])+np.sum(self.np_statVec[1])+np.sum(self.np_statVec[2])+np.sum(self.np_statVec[3]))		
	elif self.stat == 'read_related_traffic_overhead':
	    return (np.sum(self.np_statVec[2])+np.sum(self.np_statVec[3]))*1.0/(np.sum(self.np_statVec[0])+np.sum(self.np_statVec[1]))
	elif self.stat == 'write_related_traffic_overhead':
	    return (np.sum(self.np_statVec[2])+np.sum(self.np_statVec[3])+np.sum(self.np_statVec[4])+np.sum(self.np_statVec[5]))*1.0/(np.sum(self.np_statVec[0])+np.sum(self.np_statVec[1]))
        
        else:
            return self.np_statVec
'''
    def printStat(self):
        print("\n"),
        printself.stat)
        if self.normalize:
            print " - normalized"
        else:
            print ""
        print "\n",
'''
def coresCheck(filePathList):
    _cores = []
    for i in filePathList:
        _cores.append(getCores(i))

    if not all(i == _cores[0] for i in _cores):
        print "All simulations don't have same number of cores. Certain statistics are not going to be universal.\n"


# Credit to: https://stackoverflow.com/questions/20025235/how-to-pretty-print-a-csv-file-in-python
# Function modified to act on a string instead of file.
# string.splitlines()
def pretty_csv(csv_str, **options):
    """
    @summary:
        Reads a CSV file and prints visually the data as table to a new file.
    @param filename:
        is the path to the given CSV file.
    @param **options:
        the union of Python's Standard Library csv module Dialects and Formatting Parameters and the following list:
    @param new_delimiter:
        the new column separator (default " | ")
    @param border:
        boolean value if you want to print the border of the table (default True)
    @param border_vertical_left:
        the left border of the table (default "| ")
    @param border_vertical_right:
        the right border of the table (default " |")
    @param border_horizontal:
        the top and bottom border of the table (default "-")
    @param border_corner_tl:
        the top-left corner of the table (default "+ ")
    @param border_corner_tr:
        the top-right corner of the table (default " +")
    @param border_corner_bl:
        the bottom-left corner of the table (default same as border_corner_tl)
    @param border_corner_br:
        the bottom-right corner of the table (default same as border_corner_tr)
    @param header:
        boolean value if the first row is a table header (default True)
    @param border_header_separator:
        the border between the header and the table (default same as border_horizontal)
    @param border_header_left:
        the left border of the table header (default same as border_corner_tl)
    @param border_header_right:
        the right border of the table header (default same as border_corner_tr)
    @param newline:
        defines how the rows of the table will be separated (default "\n")
    @param new_csv_str:
        the new file's csv_str (*default* "/new_" + csv_str)
    """

    #function specific options
    new_delimiter           = options.pop("new_delimiter", " | ")
    border                  = options.pop("border", True)
    border_vertical_left    = options.pop("border_vertical_left", "| ")
    border_vertical_right   = options.pop("border_vertical_right", " |")
    border_horizontal       = options.pop("border_horizontal", "-")
    border_corner_tl        = options.pop("border_corner_tl", "+ ")
    border_corner_tr        = options.pop("border_corner_tr", " +")
    border_corner_bl        = options.pop("border_corner_bl", border_corner_tl)
    border_corner_br        = options.pop("border_corner_br", border_corner_tr)
    header                  = options.pop("header", True)
    border_header_separator = options.pop("border_header_separator", border_horizontal)
    border_header_left      = options.pop("border_header_left", border_corner_tl)
    border_header_right     = options.pop("border_header_right", border_corner_tr)
    newline                 = options.pop("newline", "\n")

    
    column_max_width = {} #key:column number, the max width of each column
    num_rows = 0 #the number of rows

    #with open(csv_str, "rb") as input: #parse the file and determine the width of each column
    reader = csv.reader(csv_str.splitlines(), **options)
    for row in reader:
        num_rows += 1
        for col_number, column in enumerate(row):
            width = len(column)
            try:
                if width > column_max_width[col_number]:
                    column_max_width[col_number] = width
            except KeyError:
                column_max_width[col_number] = width

    max_columns = max(column_max_width.keys()) + 1 #the max number of columns (having rows with different number of columns is no problem)
    #print max_columns
    #print column_max_width

    if max_columns > 1:
        total_length = sum(column_max_width.values()) + len(new_delimiter) * (max_columns - 1)
        left = border_vertical_left if border is True else ""
        right = border_vertical_right if border is True else ""
        left_header = border_header_left if border is True else ""
        right_header = border_header_right if border is True else ""

#       with open(csv_str, "rb") as input:
        reader = csv.reader(csv_str.splitlines(), **options)

        # output = prettyCsvOut
        output = StringIO.StringIO()
        #with open(new_csv_str, "w") as output:
        for row_number, row in enumerate(reader):
            max_index = len(row) - 1
            for index in range(max_columns):
                if index > max_index:
                    row.append(' ' * column_max_width[index]) #append empty columns
                else:
                    diff = column_max_width[index] - len(row[index])
                    row[index] = row[index] + ' ' * diff #append spaces to fit the max width

            if row_number==0 and border is True: #draw top border
                output.write(border_corner_tl + border_horizontal * total_length + border_corner_tr + newline)
            output.write(left + new_delimiter.join(row) + right + newline) #print the new row
            if row_number==0 and header is True: #draw header's separator
                output.write(left_header + border_header_separator * total_length + right_header + newline)
            if row_number==num_rows-1 and border is True: #draw bottom border
                output.write(border_corner_bl + border_horizontal * total_length + border_corner_br)

        outputStr = output.getvalue()
        output.close()
    return outputStr


if __name__ == "__main__":
    #main()

    #works with Python 3
    #files = [f for f in os.listdir('.') if os.path.isfile(f)]
    #print(files)
    #for filename in glob.glob('./**/final.out', recursive=True):
    #    print(filename)

  
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--whole-life-time", action='store_true', help="Specify whether to use the total stats across whole lifetime.\n")
    parser.add_argument("--skip-insts", default=0, help="Specify how many sets of stats to skip whether to skip.\n")
    parser.add_argument("--observed-insts", default=0, help="Specify how many sets of stats to observe.\n")
    parser.add_argument("--no-table", action='store_true', help="Specify this parameter if only CSV text needs to be displayed.\n")
    parser.add_argument("--params", default="params.ini", help="Parameters.ini file. Contains benchmarks, stats, directories and their aliases.")

    args, unknown = parser.parse_known_args()

    process_ini = ConfigParser.ConfigParser()
    if not os.path.exists(args.params):
        print str(args.params) + "does not exist\n"
        sys.exit(1)
    process_ini.read(args.params)

    # create benchmarkslist
    if not process_ini.get('benchmarks', 'benchmarks'):
        sys.exit("In " + args.params + ", make sure you have [benchmarks] with option benchmarks\n")
    benchmarks = list(process_ini.get('benchmarks', 'benchmarks').split())


    dirs_dict = dict(process_ini.items('directories'))

    dirs = []
    aliases = []
    for key in sorted(dirs_dict.iterkeys()):
        i = dirs_dict[key]
        dirs.append(i.split()[0])
        aliases.append(i.split()[1])

    
    try:
        normalize = process_ini.get('directories', process_ini.get('benchmarks', 'normalize')).split()[1]
        normalizeIdx = aliases.index(normalize)
    except:
        normalize = False
    
    #print normalize
    #print normalizeIdx

    # create statList from [benchmarks].stats
    if not process_ini.get('benchmarks', 'stats'):
        sys.exit("In " + args.params + ", make sure you have [benchmarks] with option stats.\n")
    statList = list(process_ini.get('benchmarks', 'stats').split())



    #print benchmarks
    #print dirs
    # create all dictionaries

    float_formatter = lambda x: "%.4f" % x
    all_stats_dict = {}
    for d in dirs:
        for benchmark in benchmarks:
            statsFile = d + "/" + benchmark + "/" + "final.out"
            #statsFile = d + "/" + benchmark + "_morphtree_dccm_march_11_dccm_seqpage_microbench_debug__morphtree_4_4_micro_baseline_debug_new_final.out"
            #statsFile = d + "/" + benchmark + "_morphtree_4_4_micro_baseline_debug_new_final.out"
            #statsFile = d + "/" + benchmark + "_morphtree_dccm_2_16_withoutprecomputation_randpage_microbench_debug__morphtree_4_4_micro_baseline_debug_new_final.out"
            if not os.path.isfile(statsFile):
		with open(statsFile, 'w') as input:
		    pass
                print(statsFile + " does not exist, or is empty. Please rerun simulation for this case or/and check simout for errors.")
            	all_stats_dict[statsFile] = parseStats(statsFile)
            else:
            	all_stats_dict[statsFile] = parseStats(statsFile)

    #for key in all_stats_dict.iterkeys():
    #    print key
            
    stat_mat_dict = {}
    for stat in statList:
        #print stat + ":" + "\n"
        mat = np.zeros(shape=(len(dirs), len(benchmarks)))
        dIdx = 0
        for d in dirs:
            row = np.zeros(shape=(len(benchmarks)))
            benchmarkIdx = 0
            for benchmark in benchmarks:
                statsFile = d + "/" + benchmark + "/" + "final.out"
                #statsFile = d + "/" + benchmark + "_morphtree_dccm_march_11_dccm_seqpage_microbench_debug__morphtree_4_4_micro_baseline_debug_new_final.out"
                #statsFile = d + "/" + benchmark + "_morphtree_4_4_micro_baseline_debug_new_final.out"
                #statsFile = d + "/" + benchmark + "_morphtree_dccm_2_16_withoutprecomputation_randpage_microbench_debug__morphtree_4_4_micro_baseline_debug_new_final.out"
                configFile = d + "/" + benchmark + "/" + "config.ini"
                #print statsFile
                statObj = Stat(stat, getCores(configFile))
                # x is the dictionary
                _dic = all_stats_dict[statsFile]
                statObj.filterStatVec(_dic)
                row[benchmarkIdx] = statObj.getStat()
                benchmarkIdx += 1
            mat[dIdx] = row
            dIdx += 1
            #print row
            #mat.append(row)

        idx = np.isnan(mat)
        mat[idx] = 1
        idx = np.isinf(mat)
        mat[idx] = 1
        for i in range(0, mat.shape[0]):
            for j in range(0, mat.shape[1]):
                if mat[i][j] == 0:
                    mat[i][j] = -1
                    
        #idx = np.where(mat == 0)[0]
        #mat[idx] = 1

        #print(mat)
        #print normalize
        if normalize:
            mat = mat / mat[normalizeIdx, :]

            
        #print mat
        #print "\n\n"
        mat = np.asmatrix(mat)
        #_mean = np.split(_mean,
        _mean = np.mean(mat, axis=1)
        _gmean = scm.gmean(mat, axis=1)
        mat = np.hstack((mat, _mean))
        mat = np.hstack((mat, _gmean))

        #print mat
        # printing

        #print(mat.shape[0])
        #print(mat.shape[1])

       
        csvOut = StringIO.StringIO()
        #csvOut.write("_,")
        csvOut.write(stat+",")
        for j in xrange(0, mat.shape[1] - 2):
            csvOut.write(benchmarks[j] + ",")
        csvOut.write("mean,gmean,\n")
            
        for i in xrange(0, mat.shape[0]):
            csvOut.write(aliases[i] + ",")
            for j in xrange(0, mat.shape[1]):
                if mat[i, j].is_integer():
                    csvOut.write(str(int(mat[i, j]))+",")
                else:
                    csvOut.write(str(float_formatter(mat[i, j]))+",")
            csvOut.write("\n")
        csvOut.write("\n\n")
        #csvOut.close()
        csvOutStr = csvOut.getvalue()
	if args.no_table == True:
           print csvOutStr
        #print "\n\n\n"
        
	if args.no_table == False:
	   prettyCsvOutStr = pretty_csv(csvOutStr, header=True, border=False, delimiter=",")
           print prettyCsvOutStr
        csvOut.close()
