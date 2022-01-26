#!/usr/bin/env python3
# import sys
# !{sys.executable} -m pip install scipy
# 
#                                                     MsgId Global_MsgId
# 0     1                2             3            4    5  6
# new 3037771794  10.0.109.1:1109 10.0.97.1:1097  42340  2  9
# 0      1            2               3           4      5        6    7
# fct 3037809611 10.0.109.1:1109 10.0.97.1:1097 42340 3037771794 37817 2
# 0      1            2       3    4                 5    6  7   8   9
# TP 3000057705 all_pkt_count 40 all_pkt_size_count 58400 TP 9 MsgId 1
# flowid_fct 图(+,fct)  meanfct  T0-Tn的平均吞吐
import json
import numpy as np
from scipy import stats
import math
import matplotlib.pyplot as plt

figsize = (10,6)
# figsize = (30, 10)
Flow_line = {'label':0, 'time':1, 'sender':2, 'receiver':3, 'msgsize':4}
Msg_info = {'MsgId':5, 'Global_MsgId':6}
Fct_info = {'Start_Time':5, 'FCT':6, 'MsgId':7}
Tp_info = {'label':0, 'time':1, 'pkt_count':3, 'pkt_size_count':5, 'throughput':7, 'MsgId':9}
Log_File = './test_log'
 

TP_Dict = {} #Time:throughput
Global_MsgId_Dict = {} 
Start_Time_Dict = {}
Fct_Dict = {}
Global_Fct_Dict = {}
FCT_LIST = []

T1,Tn = 0, 0
pkt_size_count, pkt_size_count_t = 0, 0
num_of_fct = 0
mean_fct = 0
time_unit = 1e-3 #1e-6=ms 1e-3=us  1=ns
if __name__ == '__main__':
	#相关信息入字典
	with open(Log_File) as f:
		for line in f:
			msgLog = line.split()

			if (msgLog[0]!='TP' and msgLog[0]!='New' and msgLog[0]!='fct') :
				continue
			# 整TP相关信息
			if (msgLog[0]=="TP") :
				pkt_size_count_t = int(msgLog[ Tp_info['pkt_size_count'] ])
				continue
			

			#整理Flow的相关信息
			# time = float(TpLog[ Flow_line['time'] ]) #ns
			sender = msgLog[ Flow_line['sender'] ] 
			receiver = msgLog[ Flow_line['receiver'] ]
			if (msgLog[0]=='New') : #开始时间和msgid作为唯一标识保留下
				start_time = int(msgLog[ Flow_line['time'] ]) #ns
				msgid = int(msgLog[ Msg_info['MsgId'] ])
			elif (msgLog[0]=='fct') :
				FCT_LIST.append(line)
				start_time = int(msgLog[ Fct_info['Start_Time'] ]) #ns
				msgid = int(msgLog[ Fct_info['MsgId'] ])

			key = (start_time, sender, receiver, msgid) #唯一标识

			if (msgLog[0]=='New') :
				if T1==0 : #记录第一个时刻
					T1 = int(msgLog[Flow_line['time']])

				if (key in Global_MsgId_Dict) :
					print("ERROR:Global Msgid key exit!!")
				else :
					Global_MsgId_Dict[key] = msgLog[ Msg_info['Global_MsgId'] ]
					Tn = int(msgLog[Flow_line['time']])  #用于算T1-Tn时间段的meanTP
					pkt_size_count = int(pkt_size_count_t)

			elif (msgLog[0]=='fct') :
				num_of_fct += 1
				mean_fct += int(msgLog[ Fct_info['FCT'] ])

				if (key in Start_Time_Dict ) :
					print("ERROR:start time key exit!!")
				else :
					# print(Global_MsgId_Dict)
					Start_Time_Dict[ Global_MsgId_Dict[key] ] = msgLog[ Fct_info['Start_Time'] ]

				# if (key in Fct_Dict ) :
				# 	print("ERROR:fct key exit!!")
				# else :
				# 	Fct_Dict[key] = msgLog[ Fct_info['FCT'] ]

				if (key in Global_Fct_Dict ) :
					print("ERROR:global fct key exit!!")
				else :
					Global_Fct_Dict[ int(Global_MsgId_Dict[key]) ] = float(msgLog[ Fct_info['FCT'] ])*time_unit #ms
	print( str(mean_fct) + " " + str(num_of_fct) )
	mean_fct = mean_fct*1.0*time_unit/num_of_fct
	print( str(pkt_size_count) + " " + str(Tn) + " " + str(T1))
	if Tn==T1 :
		print("Tn==T1 测试数据")
		mean_tp = 0
	else :
		mean_tp = float( pkt_size_count*8.0/(Tn-T1) ) #Gbps
	print("FCT_LIST")
	# print(FCT_LIST)
	for key in FCT_LIST:
		print(key)

	# print("TP_Dict")				
	# print(TP_Dict)				

	# print("Global_MsgId_Dict")				
	# print(Global_MsgId_Dict)

	# print("Start_Time_Dict")				
	# print(Start_Time_Dict)		

	global_id_list = []
	fct_list = []
	for global_id, fct in Global_Fct_Dict.items():
		global_id_list.append(global_id)
		fct_list.append(fct)
	global_id_list = np.array(global_id_list)
	fct_list = np.array(fct_list)

	zipData = sorted(zip(global_id_list,
						 fct_list
					))

	global_id_list = np.array([x for x,_ in zipData])
	fct_list = np.array([x for _,x in zipData])

	# print("zipData")
	# print(zipData)
	print("global_id_list")
	print(global_id_list)

	# print(*global_id_list, sep="  ")
	
	print("len(global_id_list) " + str(len(global_id_list)))  
	print("fct_list")
	print(fct_list)
	print("len(fct_list) " + str(len(fct_list)))

	# print(*global_id_list, sep="  ")
	print("FCT_ITEMS")
	print(*fct_list, sep="\n")

	print( 'mean_fct:'+str(mean_fct) )
	print( 'mean_tp:'+str(mean_tp) )

	#画图
	plt.figure(figsize=figsize)
	plt.grid()

	plt.step(global_id_list, fct_list, label='fct', color='C0')

	plt.title("Homa MsgId-Fct")
	plt.xlabel("MsgId")
	plt.ylabel("FCT(ms)")
	plt.legend(loc='upper left')

	# xTickPercentiles = np.linspace(10, 100, 10)
	# xticks = np.percentile(global_id_list, xTickPercentiles).astype(int) 
	# plt.xticks(xTickPercentiles, xticks, rotation=45)

	plt.gca().spines["right"].set_visible(False)
	plt.gca().spines["top"].set_visible(False)
	plt.tight_layout()

	plt.savefig('./MsgId-FCT.png')


				




