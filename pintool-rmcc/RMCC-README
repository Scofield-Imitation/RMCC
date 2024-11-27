Table of Contents
=================
1) Compiling
2) Environment Set
3) Example Running Command
4) Setting Options
5) Output File
6) Processing Script

1) Compiling:
=============
1. cd pintool-rmcc/source/tools/ManualExamples
2. make obj-intel64/rmcc.so TARGET=intel64

2) Environment Set
================
1. Huge page

3) Example Running command (for mcf in SPEC2006):
==============================================
1. For Baseline (without memoization)

cd CPU2006/429.mcf/run/run_base_ref_gcc43-64bit.0000

LD_PRELOAD=libhugetlbfs.so HUGETLB_MORECORE=yes setarch $(uname -m) -R ~/RMCC/pintool-rmcc/pin -t ~/RMCC/pintool-rmcc/source/tools/ManualExamples/obj-intel64/rmcc.so -output_dir ~/RMCC/pintool-rmcc/output/429.mcf -f 0 -tree_cpt /mnt/xinw/temp_run_pintool/long_part_fix_index_33_0_random_whole_tree_baseline_tree_nodes.cpt -random_cpt /mnt/xinw/temp_run_pintool/random_numbers_512k.cpt -table_cpt /mnt/xinw/temp_run_pintool/long_part_fix_index_33_0_random_whole_tree_baseline_aes_table.cpt  -dccm_overhead 1  -predictive_decryption 0  -record_recent_aes 1 -relevel_to_next_group 0  -run_insts 500000000000  --  ../run_base_ref_gcc43-64bit.0000/mcf_base.gcc43-64bit inp.in > inp.out 2>> inp.err  > ~/RMCC/pintool-rmcc/output/429.mcf/end & 


2. For RMCC (with memoization of L0 counters)

cd CPU2006/429.mcf/run/run_base_ref_gcc43-64bit.0000

LD_PRELOAD=libhugetlbfs.so HUGETLB_MORECORE=yes setarch $(uname -m) -R ~/RMCC/pintool-rmcc/pin -t ~/RMCC/pintool-rmcc/source/tools/ManualExamples/obj-intel64/rmcc.so -output_dir ~/RMCC/pintool-rmcc/output/429.mcf -f 0 -tree_cpt /mnt/xinw/temp_run_pintool/long_part_fix_index_33_0_random_whole_tree_baseline_tree_nodes.cpt -random_cpt /mnt/xinw/temp_run_pintool/random_numbers_512k.cpt -table_cpt /mnt/xinw/temp_run_pintool/long_part_fix_index_33_0_random_whole_tree_baseline_aes_table.cpt  -dccm_overhead 1  -predictive_decryption 0  -record_recent_aes 1 -relevel_to_next_group 0  -run_insts 500000000000  --  ../run_base_ref_gcc43-64bit.0000/mcf_base.gcc43-64bit inp.in > inp.out 2>> inp.err  > ~/RMCC/pintool-rmcc/output/429.mcf/end & 

4) Setting Options
================

1. Storage size (Configured by changing global definition in rmcc.cpp and recompiling)

L2 cache size:

Counter cache size:

Group size in memoization table: 

2. 

5) Output File
===========

6) Processing Script
=================
To help processing the results, we provide the Python scripts for systematically get the results for specified stats, benchmarks, mechanisms (i.e., baseline, RMCC) and observation window.

Here are two example commands to use the script:
a. collect results for whole life time excluding the first 4 billion insts:
	python process.py --params target_results.ini --whole-life-time  --skip-insts 4
b. collect results for   
	


