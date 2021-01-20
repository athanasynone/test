from socket import *
from ssr_interface import *
import time
import threading
import os
import subprocess
import time
import json
import inspect
import ctypes
import copy

##
func = lambda x,y:x*y

mirror_count = 0# count the mirror sources
# source_position = [0,0,0]# initial source position
listener_position = [0,0,0]# initial listener position
wall_x = 4# half length of room
wall_y = 3# half width of room
filter_x = [[0.8,0.8,0.8,0.8],[0.8,0.8,0.8,0.8]]# absorption of wall +-x
filter_y = [[0.8,0.8,0.8,0.8],[0.8,0.8,0.8,0.8]]# absorption of wall +y
mirror_source = []
# ini_mirror_source = copy.deepcopy(source_position)
filt = [1,1,1,1]
filtall = []
height = 4
length = 2*wall_x
width = 2*wall_y
thick = 0.2 
length2 = length + 2*thick
width2 = width + 2*thick
change = 0

udp_socket_send = socket(AF_INET,SOCK_DGRAM)
udp_socket_recv = socket(AF_INET,SOCK_DGRAM)
localaddr = ("localhost",8822) 
udp_socket_recv.bind(localaddr)

##
def wall(point_1,point_2,point_3,point_4,idd):
	w = point_1 + point_2 + point_3 + point_4
	wall_msg = b'/face ' + bytes(str(idd),'gbk') + b' Plaster ' + bytes(' '.join([str(x) for x in w]),'gbk')
	udp_socket_send.sendto(wall_msg,("localhost",8866))

##
def plane_to_mirror(a,b,c,msg,wall_f):
	global ini_mirror_source
	global filt
	m = [a[i] - b[i] for i in range(len(a))]
	n = [a[i] - c[i] for i in range(len(a))]
	A = m[1]*n[2] - m[2]*n[1]
	B = m[2]*n[0] - m[0]*n[2]
	C = m[0]*n[1] - m[1]*n[0]
	D = A*a[0] + B*a[1] + C*a[2]
	# print('msg = ',msg)
	# print('plane = ',A,B,C,D)
	if abs(A*msg[0] + B*msg[1] + C*msg[2] - D) < 1e-3:
		t_x = A
		t_y = B
		t_z = C
		t_numx = ini_mirror_source[0]
		t_numy = ini_mirror_source[1]
		t_numz = ini_mirror_source[2]
		t = (D - A*t_numx - B*t_numy - C*t_numz)/(A*t_x + B*t_y + C*t_z)
		ini_mirror_source = [2*(t_x*t + t_numx) - ini_mirror_source[0],2*(t_y*t + t_numy) - ini_mirror_source[1],2*(t_z*t + t_numz) - ini_mirror_source[2]]
		result = map(func,filt,wall_f)
		filt = list(result)

##
def five_source(source_position):
	global mirror_source
	global mirror_count
	global filt
	global ini_mirror_source
	recv_msg = []
	msg_count = 0

	mirror_source = []
	mirror_count = 0
	ini_mirror_source = copy.deepcopy(source_position)
	# source_1 = b'/listener listener_1 -0.15 0.0 0.0 0.0 -0.0 -0.15 0.0 0.0 0.0 0.0 0.15 0.0 1 1 0 1.0'
	source_1 =  b'/source source_1 -0.15 0.0 0.0 0.0 -0.0 -0.15 0.0 0.0 0.0 0.0 0.15 0.0 ' + bytes(str(source_position[0]),'gbk') + b' ' + bytes(str(source_position[1]),'gbk') + b' ' + bytes(str(source_position[2]),'gbk') + b' 1.0'
	udp_socket_send.sendto(source_1,("localhost",8866))
	while 1:
		recv_data = udp_socket_recv.recvfrom(16384)
		if recv_data[0] == b'all done':
			break
		recv_msg.append(recv_data[0])
		msg_count = msg_count + 1

	for i in range(msg_count):
		if recv_msg[i] != b'new source':
			aa = recv_msg[i].decode('utf-8')
			lst = json.loads(aa)
			# print(lst)
			plane_to_mirror([-4,3,-1.5],[4,3,-1.5],[-4,3,1.5],lst,filter_y[0])
			plane_to_mirror([4,3,-1.5],[4,-3,-1.5],[4,3,1.5],lst,filter_x[0])
			plane_to_mirror([4,-3,-1.5],[-4,-3,-1.5],[4,-3,1.5],lst,filter_y[1])
			plane_to_mirror([-4,-3,-1.5],[-4,3,-1.5],[-4,-3,1.5],lst,filter_x[1])
		if recv_msg[i] == b'new source':
			# print('=',recv_msg[i])
			filtall.append(filt)
			mirror_source.append(ini_mirror_source)
			ini_mirror_source = copy.deepcopy(source_position)
			filt = [1,1,1,1]
			mirror_count = mirror_count + 1

##
## initialization
p1 = [length/2,width/2,-height/2]
p2 = [-length/2,width/2,-height/2]
p3 = [-length/2 - change,-width/2,-height/2]
p4 = [length/2 + change,-width/2,-height/2]
p5 = [length/2,width/2,height/2]
p6 = [-length/2,width/2,height/2]
p7 = [-length/2 - change,-width/2,height/2]
p8 = [length/2 + change,-width/2,height/2]
pp1 = [length2/2,width2/2,-height/2]
pp2 = [-length2/2,width2/2,-height/2]
pp3 = [-length2/2 - change,-width2/2,-height/2]
pp4 = [length2/2 + change,-width2/2,-height/2]
pp5 = [length2/2,width2/2,height/2]
pp6 = [-length2/2,width2/2,height/2]
pp7 = [-length2/2 - change,-width2/2,height/2]
pp8 = [length2/2 + change,-width2/2,height/2]

wall(p1, p2, p6, p5, 0)
wall(p2, p3, p7, p6, 1)
wall(p3, p4, p8, p7, 2)
wall(p4, p1, p5, p8, 3)
# wall(pp1, pp2, pp6, pp5, 4)
# wall(pp2, pp3, pp7, pp6, 5)
# wall(pp3, pp4, pp8, pp7, 6)
# wall(pp4, pp1, pp5, pp8, 7)
# wall(p1, p2, pp2, pp1, 8)
# wall(p2, p3, pp3, pp2, 9)
# wall(p3, p4, pp4, pp3, 10)
# wall(p4, p1, pp1, pp4, 11)
# wall(p5, p6, pp6, pp5, 12)
# wall(p6, p7, pp7, pp6, 13)
# wall(p7, p8, pp8, pp7, 14)
# wall(p8, p5, pp5, pp8, 15)

listener_1 = b'/listener listener_1 0.11 0.0 -0.0 0.0 -0.0 0.11 0.0 0.0 0.0 -0.0 0.11 0.0 ' + bytes(str(listener_position[0]),'gbk') + b' ' + bytes(str(listener_position[1]),'gbk') + b' ' + bytes(str(listener_position[2]),'gbk') + b' 1.0'
udp_socket_send.sendto(listener_1,("localhost",8866))
udp_socket_send.sendto(b"/facefinished",("localhost",8866))

sp1 = [1.1, 1, 0]
sp2 = [0.5, 1, 0]
sp3 = [0, 1, 0]
sp4 = [-0.5, 1, 0]
sp5 = [-1, 1, 0]

five_source(sp1)
mirror_source_1 = mirror_source
mirror_count_1 = mirror_count
filtall_1 = filtall

five_source(sp2)
mirror_source_2 = mirror_source
mirror_count_2 = mirror_count
filtall_2 = filtall

five_source(sp3)
mirror_source_3 = mirror_source
mirror_count_3 = mirror_count
filtall_3 = filtall

five_source(sp4)
mirror_source_4 = mirror_source
mirror_count_4 = mirror_count
filtall_4 = filtall

five_source(sp5)
mirror_source_5 = mirror_source
mirror_count_5 = mirror_count
filtall_5 = filtall

print('mirror1 = ', mirror_source_1)
print('mirror1 = ', mirror_source_2)
print('mirror1 = ', mirror_source_3)
print('mirror1 = ', mirror_source_4)
print('mirror1 = ', mirror_source_5)

sourceId1 = 'jack1'
sourceId2 = 'jack2'
sourceId3 = 'jack3'
sourceId4 = 'jack4'
sourceId5 = 'jack5'

print('WebSocket is connected.')
NewSource({
    sourceId1:{
    "name":"person1",
    "port-number": 1,
    "pos": sp1,
    "volume": 1    
    }
})
NewSource({
    sourceId2:{
    "name":"person2",
    "port-number": 1,
    "pos": sp2,
    "volume": 1    
    }
})
NewSource({
    sourceId3:{
    "name":"person3",
    "port-number": 1,
    "pos": sp3,
    "volume": 1    
    }
})
NewSource({
    sourceId4:{
    "name":"person4",
    "port-number": 1,
    "pos": sp4,
    "volume": 1    
    }
})
NewSource({
    sourceId5:{
    "name":"person5",
    "port-number": 1,
    "pos": sp5,
    "volume": 1    
    }
})

for i in range(0,mirror_count_1):
	src_name = sourceId1 + 'VIRT' + str(i)
	name = 'fake_person1,' + str(i)
	NewSource({
		src_name:{
		"name":name,
		"port-number": 1,
		"pos": mirror_source_1[i],
		"volume": 1,
		"filter": filtall_1[i]
		}
	})
for i in range(0,mirror_count_2):
	src_name = sourceId2 + 'VIRT' + str(i)
	name = 'fake_person2,' + str(i)
	NewSource({
		src_name:{
		"name":name,
		"port-number": 1,
		"pos": mirror_source_2[i],
		"volume": 1,
		"filter": filtall_2[i]
		}
	})
for i in range(0,mirror_count_3):
	src_name = sourceId3 + 'VIRT' + str(i)
	name = 'fake_person3,' + str(i)
	NewSource({
		src_name:{
		"name":name,
		"port-number": 1,
		"pos": mirror_source_3[i],
		"volume": 1,
		"filter": filtall_3[i]
		}
	})
for i in range(0,mirror_count_4):
	src_name = sourceId4 + 'VIRT' + str(i)
	name = 'fake_person4,' + str(i)
	NewSource({
		src_name:{
		"name":name,
		"port-number": 1,
		"pos": mirror_source_4[i],
		"volume": 1,
		"filter": filtall_4[i]
		}
	})
for i in range(0,mirror_count_5):
	src_name = sourceId5 + 'VIRT' + str(i)
	name = 'fake_person' + str(i)
	NewSource({
		src_name:{
		"name":name,
		"port-number": 1,
		"pos": mirror_source_5[i],
		"volume": 1,
		"filter": filtall_5[i]
		}
	})
msg = json.dumps(["subscribe",["scene"]])
ws.send(msg)
# print('ws.send')

