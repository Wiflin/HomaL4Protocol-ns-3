#!/usr/bin/env python3
# 0      1            2       3    4                 5    6  7   8   9
# TP 3000057705 all_pkt_count 40 all_pkt_size_count 58400 TP 9 MsgId 1
#   0        1             2                 3       4  5
# flow_tp 3000004000 10.0.1.1:10001 10.0.140.1:10140 0 23
import sys
import numpy as np
import matplotlib.pyplot as plt


figsize = (10,6)
dict_line = {"label":0, "time":1, "pkt_count":3, "pkt_size_count":5, "throughput":7, "MsgId":9}

folderName = "./"
file_name = "test_log"

TimeList = []
MsgIdList = []
TpList = []
time_unit = 1e-3 #1e-6=ms 1e-3=us  1=ns
tp_unit = 1 # 方便直接转换为Gbps

start_time = 0
end_time = 0
all_pkt_size_count = 0
all_msg_is_end = 10000 #设置三次，如果三次全网TP都为0，说明流都走完了 #前面刚开始的两次可能也是0

time_tp_dict = {} # start_time:{key:tp, key2:tp2...}
flow_tp_dict = {} # key:{time1:tp1,time2:tp2...}
src_tp_dict = {} # src:[key:{time1:tp1, time2:tp2}]
dst_tp_dict = {}

mean_tp_in_ns3 = 0

def plot_function(NumOfFigure, time_list, tp_list):
	plt.figure(figsize=figsize)
	plt.grid()
	plt.step(time_list, tp_list, label='throughput', color='C0')

	plt.title("Homa Throughput"+str(NumOfFigure))
	plt.xlabel("Time(ns)")
	plt.ylabel("Throughput (Gbps)")
	plt.legend(loc='upper right')

	plt.gca().spines["right"].set_visible(False)
	plt.gca().spines["top"].set_visible(False)
	plt.tight_layout()

	plt.savefig(folderName+ str(NumOfFigure)+'_MsgThroughput.png')

with open(file_name) as f:
	for line in f:
		TpLog = line.split()
		#处理flow_tp
		if (TpLog[ dict_line["label"] ]=="flow_tp"): 
			now_time = TpLog[1]
			src_port = TpLog[2]
			dst_port = TpLog[3]
			msg_id = TpLog[4]
			tp_info = TpLog[5]
			key = (src_port, dst_port, msg_id)

			if (now_time in time_tp_dict) : #汇总
				time_tp_dict[now_time][key] = tp_info
			else :
				time_tp_dict[now_time] = {}
				time_tp_dict[now_time][key] = tp_info
			#按照流来
			if (key in flow_tp_dict) : #汇总
				flow_tp_dict[key][now_time] = tp_info
			else :
				flow_tp_dict[key] = {}
				flow_tp_dict[key][now_time] = tp_info
			#按照发端来
			# if (src_port in src_tp_dict) : #汇总
			# 	if (key in src_tp_dict[src_port]) :
			# 		src_tp_dict[src_port][key][now_time] = tp_info
			# 	else :
			# 		src_tp_dict[src_port][key] = {}
			# 		src_tp_dict[src_port][key][now_time] = tp_info
			# else :
			# 	src_tp_dict[src_port] = {}
			# 	if (key in src_tp_dict[src_port]) :
			# 		src_tp_dict[src_port][key][now_time] = tp_info
			# 	else :
			# 		src_tp_dict[src_port][key] = {}
			# 		src_tp_dict[src_port][key][now_time] = tp_info

			#按照收端来
			# if (dst_port in dst_tp_dict) : #汇总
			# 	if (key in dst_tp_dict[dst_port]) :
			# 		dst_tp_dict[dst_port][key][now_time] = tp_info
			# 	else :
			# 		dst_tp_dict[dst_port][key] = {}
			# 		dst_tp_dict[dst_port][key][now_time] = tp_info
			# else :
			# 	dst_tp_dict[dst_port] = {}
			# 	if (key in dst_tp_dict[dst_port]) :
			# 		dst_tp_dict[dst_port][key][now_time] = tp_info
			# 	else :
			# 		dst_tp_dict[dst_port][key] = {}
			# 		dst_tp_dict[dst_port][key][now_time] = tp_info


		if (TpLog[ dict_line["label"] ]=="Average_TP_Time") :
			# print(TpLog)
			mean_tp_in_ns3 = float(TpLog[ 7 ])

		#处理全网TP
		if (TpLog[ dict_line["label"] ]!="TP") :
			if (TpLog[ dict_line["label"] ]=="all_flow_start_time") and start_time==0 : #记录下第一条开始的时间
				start_time = float(TpLog[ dict_line["time"] ])*tp_unit
			continue
		time = float(TpLog[ dict_line["time"] ])*time_unit 
		MsgId = int(TpLog[ dict_line["MsgId"] ])
		throughput = int(TpLog[ dict_line["throughput"] ]) #Gpbs

		if throughput>0 : #记录下最后的pktsize和时间
			all_pkt_size_count = int(TpLog[ dict_line["pkt_size_count"] ])
			end_time = float(TpLog[ dict_line["time"] ])*tp_unit
		else :
			all_msg_is_end -= 1; 
			if all_msg_is_end<=0 :
				break;

		TimeList.append(time)
		MsgIdList.append(MsgId)
		TpList.append(throughput)
	f.close()

#处理flow_tp
# print("time_tp_dict")
# print(time_tp_dict) 
# print("flow_tp_dict")
# print(flow_tp_dict) 
# print("len(flow_tp_dict)")
# print(len(flow_tp_dict))
# 按照发端来的
# print("src_tp_dict")
# print(src_tp_dict) 
# src:[key:{time1:tp1, time2:tp2}]
# fl = open('./flow_tp.csv', 'w')
# num = 0
# for src_port, key_dict in src_tp_dict.items() :
# 	# print(key_dict)
# 	num += 1
# 	time_t_list = []
# 	tp_t_list = []
# 	for key, time_dict in key_dict.items():
# 		# print(time_dict)
# 		# print(key)
# 		print(key, file=fl)
# 		for time, tp in time_dict.items():
# 			# print(time, ",", tp, sep=' ')
# 			time_t_list.append(time)
# 			tp_t_list.append(tp)
# 			print(time, ",", tp, sep=' ', file=fl)
# 		plot_function("sender"+str(src_port), time_t_list, tp_t_list)
# 		# print('\n')
# 		print('\n', file=fl)

#按照流的来
fl = open('./flow_tp.csv', 'w')
# print(flow_tp_dict)
for src_port, key_dict in flow_tp_dict.items() :
	# print(src_port)
	# print(key_dict)
	time_t_list = []
	tp_t_list = []
	for time_key, tp in key_dict.items():
		# print(time_dict)
		# print(key)
		print(time_key, file=fl)
		time_t_list.append(time_key)
		tp_t_list.append(tp)
		print(time_key, ",", tp, sep=' ', file=fl)
	plot_function("sender"+str(src_port), time_t_list, tp_t_list)
	# print('\n')
	print('\n', file=fl)

		

# f1 = open('./flow_tp1.csv', 'w')
# f2 = open('./flow_tp2.csv', 'w')
# f3 = open('./flow_tp3.csv', 'w')
# test = 0
# time_dict1 = {}
# time_dict2 = {}
# time_dict3 = {}
# for src_port, key_dict in src_tp_dict.items() :
# 	# print(key_dict)
# 	test += 1
# 	for key, time_dict in key_dict.items():
# 		# print(time_dict)
# 		print(key)
# 		if test==1 :
# 			time_dict1 = time_dict
# 			print(key, file=f1)
# 			for time, tp in time_dict1.items():
# 				print(time, ",", tp, sep=' ', file=f1)
# 			print('\n')
# 			print('\n', file=f1)
# 		elif test==2 :
# 			time_dict2 = time_dict
# 			print(key, file=f2)
# 			for time, tp in time_dict2.items():
# 				print(time, ",", tp, sep=' ', file=f2)
# 			print('\n')
# 			print('\n', file=f2)
# 		else :
# 			time_dict3 = time_dict
# 			print(key, file=f3)
# 			for time, tp in time_dict3.items():
# 				print(time, ",", tp, sep=' ', file=f3)
# 			print('\n')
# 			print('\n', file=f3)


# 按照收端来的
# print("dst_tp_dict")
# print(dst_tp_dict) 
# fl = open('./flow_tp.csv', 'w')
# num = 0
# for dst_port, key_dict in dst_tp_dict.items() :
# 	# print(key_dict)
# 	num += 1
# 	time_t_list = []
# 	tp_t_list = []
# 	for key, time_dict in key_dict.items():
# 		# print(time_dict)
# 		print(key)
# 		print(key, file=fl)
# 		for time, tp in time_dict.items():
# 			time_t_list.append(time)
# 			tp_t_list.append(tp)
# 			print(time, ",", tp, sep=' ')
# 			print(time, ",", tp, sep=' ', file=fl)
# 		plot_function("recevier"+str(dst_port), time_t_list, tp_t_list)
# 		print('\n')
# 		print('\n', file=fl)


#处理tp
print(str(all_pkt_size_count) + " " + str(start_time) + " " + str(end_time) )
if (mean_tp_in_ns3!=0 ) : #以ns3内部的计算为准
	print("real_mean_tp " + str(mean_tp_in_ns3))
else :
	mean_tp = all_pkt_size_count*8.0/(end_time-start_time)
	print("mean_tp " + str(mean_tp))

zipData = sorted(zip(TimeList,
					 TpList,
					 MsgIdList
					))

TimeList = np.array([x for x,_,_ in zipData])
TpList = np.array([x for _,x,_ in zipData])
MsgIdList = np.array([x for _,_,x in zipData])

# print(*TimeList, sep='\n') #输出整体的时间和吞吐
# print(*TpList, sep='\n')

#画图
plot_function("Network", TimeList, TpList)
# plt.figure(figsize=figsize)
# plt.grid()

# plt.step(TimeList, TpList, label='throughput', color='C0')

# plt.title("Homa Throughput")
# plt.xlabel("Time(ns)")
# plt.ylabel("Throughput (Gbps)")
# plt.legend(loc='upper right')

# plt.gca().spines["right"].set_visible(False)
# plt.gca().spines["top"].set_visible(False)
# plt.tight_layout()

# plt.savefig(folderName+'MsgThroughput.png')
