import numpy as np 
import math
import scipy.io as scio

threshold = 500

def mergeTime():
    ## load file
    data = scio.loadmat('./mergetime/first.mat')
    grdata = data['res']
    for ii in range(grdata.shape[0]):
        grdata[ii,0]=grdata[ii,0].real
        grdata[ii,1]=grdata[ii,1].real
        grdata[ii,2]=grdata[ii,2].real
        grdata[ii,3]=grdata[ii,3].real
        grdata[ii,56]=grdata[ii,56].real
    lidardata=np.zeros((1,6))
    with open('./mergetime/seq09.txt','r') as f:
        for line in f.readlines():
            a,b,c = line.split()
            hr = float(a[0:2])+8
            minute = float(a[3:5])
            second = float(a[6:8])
            millisecond = float(a[9:12])
            deep = float(b)
            angle = float(c)
            cur = np.array([hr,minute,second,millisecond,deep,angle])
            lidardata = np.row_stack((lidardata,cur))
    lidardata=np.delete(lidardata,0,axis=0)
    print(grdata.shape)
    print(lidardata.shape)

    cursor1=0 #cursor for lidardata
    cursor2=0 #cursor for grdata
    res = np.zeros((1,218))
    while cursor1<lidardata.shape[0] and cursor2<grdata.shape[0]:
        onerow = lidardata[cursor1,:]
        center = onerow[0]*60*60*1000+onerow[1]*60*1000+onerow[2]*1000+onerow[3]
        while grdata[cursor2,0]*60*60*1000+grdata[cursor2,1]*60*1000+grdata[cursor2,2]*1000+grdata[cursor2,3]<center-threshold:
            cursor2+=4
        if grdata[cursor2,0]*60*60*1000+grdata[cursor2,1]*60*1000+grdata[cursor2,2]*1000+grdata[cursor2,3]<center+threshold:
            onerow = np.hstack((onerow,grdata[cursor2,4:],grdata[cursor2+1,4:],grdata[cursor2+2,4:],grdata[cursor2+3,4:]))
            res=np.vstack((res,onerow))
            cursor2+=4
        cursor1+=1
    res=np.delete(res,0,axis=0)
    print(res.shape)
    #np.savetxt('./merged.txt',res)
    scio.savemat('./merged.mat',{'res':res})





if __name__ == '__main__':
    mergeTime()
    
    # data = scio.loadmat('./mergetime/first.mat')
    # print(data['res'])