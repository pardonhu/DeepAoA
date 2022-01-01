import os
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import time
import argparse as arg
import scipy.io as scio
import math

delx = 0.27


def handleTracker():
    parser = arg.ArgumentParser()
    parser.add_argument('--seq',type=str,default="")
    parser.add_argument('--count',type=int,default=1)
    parser.add_argument('--tarid',type=int,default=0)
    args=parser.parse_args()
    prefix = '/media/lion/Elements SE/Exp0722/results/%s/trk_withid/%s/'%(args.seq,args.seq)
    alldata = os.listdir(prefix)
    alldata.sort(key=lambda x: int(x.split('.')[0]))
    cnt=args.count
    carid = args.tarid

    # new_route = '/media/lion/Elements SE/Exp0722/timeAngle/%s'%args.seq
    # if not os.path.exists(new_route):
    #         os.makedirs(new_route)

    f = open('/media/lion/Elements SE/Exp0722/timeAngle/%s.txt'%args.seq,'a')
    datafile = '/media/lion/Elements SE/Exp0722/time/%s.mat'%args.seq
    data = scio.loadmat(datafile)
    time=data['time_work']
    for item in alldata:
        if int(item.split('.')[0])<cnt:
            continue
        if not os.path.getsize(prefix+item):
            continue
        data = pd.read_csv(prefix+item, header=None, sep=' ', usecols=[11,12,16])
        cur = np.array(data)
        for ii in range(cur.shape[0]):
            if cur[ii][2]==carid:
                #ang = math.atan((cur[ii][1])/(cur[ii][0]+delx))+(math.pi/2) #get angle: arctan((y+dely)/2)+pi/2
                #deep = ((cur[ii][1]^2)+(cur[ii][0]+delx)^2)**0.5
                if cur[ii][0]<0:
                    ang = math.atan((cur[ii][1])/(cur[ii][0]+delx))+(math.pi)
                else:
                    ang = math.atan((cur[ii][1])/(cur[ii][0]+delx))
                deep = ((cur[ii][1]*cur[ii][1])+(cur[ii][0]+delx)*(cur[ii][0]+delx))**0.5
                f.write(time[cnt-1]+' '+str(deep)+' '+str(ang)+'\n')
                break
        if ii==cur.shape[0]-1 and cur[ii][2]!=carid:
            print("frame%d doesn't have carid%d, please give a new carid!!"%(cnt,carid))
            f.close()
            return 
        cnt=cnt+1
    f.close()


if __name__ == "__main__":
    handleTracker()