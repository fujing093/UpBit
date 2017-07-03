PLOT=gnuplot 

all: make_dir fence getvalue intro thres core_experiments tpch
	#temporarily removed:  mem upbit_vs_scan

make_dir:
	mkdir -p ../graphs


  
#Needs: ../dat/getvalue.dat
#Figure 21
getvalue: getvalue.gnu ../dat/getvalue.dat
	$(PLOT) getvalue.gnu

#Needs: ../dat/intro_U_D.dat ../dat/intro_EB.dat ../dat/intro_Q_D.dat ../dat/intro_Q_A.dat ../dat/breakdown_throughput.dat ../dat/figure_1_ucb.dat ../dat/figure_1_ub.dat
#Figure 1, 5, 6, 9
intro: intro.gnu ../dat/intro_U_D.dat ../dat/intro_EB.dat ../dat/intro_Q_D.dat ../dat/intro_Q_A.dat ../dat/breakdown_throughput.dat ../dat/figure_1_ucb.dat ../dat/figure_1_ub.dat
	$(PLOT) intro.gnu

#Needs: ../dat/thres.dat ../dat/thres_0.2.dat ../dat/threshold_normalized.dat
#Figure 19a, 19b, 19c
thres: thres.gnu ../dat/thres.dat ../dat/thres_0.2.dat ../dat/threshold_normalized.dat
	$(PLOT) thres.gnu

#Needs: ../dat/delete_new.dat ../dat/update_aggregate.dat ../dat/update_aggregate_fp.dat ../dat/insert_new.dat ../dat/mix_new.dat ../dat/mix_1000.dat ../dat/mix_zipf.dat ../dat/mix_earth_10.dat ../dat/mix_1B.dat ../dat/multi_latency.dat ../dat/update_breakup.dat
#Figure 10, 11,12a, 12b, 12c, 13, 14, 15, 16, 17, 22, 24
core_experiments: core_experiments.gnu ../dat/delete_new.dat ../dat/update_aggregate.dat ../dat/update_aggregate_fp.dat ../dat/insert_new.dat ../dat/mix_new.dat ../dat/mix_1000.dat ../dat/mix_zipf.dat ../dat/mix_earth_10.dat ../dat/mix_1B.dat ../dat/multi_latency.dat ../dat/update_breakup.dat
	$(PLOT) core_experiments.gnu

#Needs: ../dat/tpch_sel.dat
#Figure 27
tpch: tpch.gnu ../dat/tpch_sel.dat
	$(PLOT) tpch.gnu




#############
# THESE RECIPES MAY NOT BE EASY TO REPRODUCE
#Needs: ../dat/upbit_vs_scan.dat
#Figure 18
upbit_vs_scan: upbit_vs_scan.gnu ../dat/upbit_vs_scan.dat
	$(PLOT) upbit_vs_scan.gnu

#Needs: ../dat/mem_vm.dat ../dat/mem_ub.dat ../dat/mem_fp.dat ../dat/mem_eb.dat
#Figure 25
mem: mem.gnu ../dat/mem_vm.dat ../dat/mem_ub.dat ../dat/mem_fp.dat ../dat/mem_eb.dat
	$(PLOT) mem.gnu

#Needs: ../dat/fp_ub_su.dat ../dat/fp_ucb.dat ../dat/fp_ub.dat
#Figure 20, 23, 26
fence: fence.gnu ../dat/fp_ub_su.dat ../dat/fp_ucb.dat ../dat/fp_ub.dat
	$(PLOT) fence.gnu
