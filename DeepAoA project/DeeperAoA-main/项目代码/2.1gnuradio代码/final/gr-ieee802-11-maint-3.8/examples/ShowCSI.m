X1 = [];
clearvars 
close all;
preang1 = [];
preang2 = [];
preang3 = [];
calculate_number = 0;
plotfile = [];
plotlength = 5;
plotang = [];
plotdis = [];
plotMAC = [];
save_rssi = [];
rssi = [];
while(1)

%%%%%%%% MUSIC for Uniform Linear Array%%%%%%%%
% ?????]?w
derad = pi/180;         % ????/????
N = 4;                 % 3?}??????
vN = 4;                % 52?????}??????
M = 1;                  % ?H??????
K = 52;                % ??????
fc = 2.412e9;            % ?????W?v
c = 3e8;                % ???t
dd = 6.22e-2;              % ?}?????Z
d=0:dd:ceil((N/2-1))*dd;        % ?}?????Z?}?C
fdd = 312.5e3;          % ?l???i?W?v???Z
fd = 0:fdd:ceil(52/2-1)*fdd;  % ?l???i?W?v???Z?}?C

    fid1 = fopen('/home/lion/workarea/csv/temp1.csv');
    fid2 = fopen('/home/lion/workarea/csv/temp2.csv');
    fid3 = fopen('/home/lion/workarea/csv/temp3.csv');
    fid4 = fopen('/home/lion/workarea/csv/temp4.csv');
    x1 = textscan(fid1,'%s','delimiter',",");
    x2 = textscan(fid2,'%s','delimiter',",");
    x3 = textscan(fid3,'%s','delimiter',",");
    x4 = textscan(fid4,'%s','delimiter',",");
    xx1 = erase(x1{1},["(",")"]);
    xx2 = erase(x2{1},["(",")"]);
    xx3 = erase(x3{1},["(",")"]);
    xx4 = erase(x4{1},["(",")"]);
    y1 = str2double(xx1);
    y2 = str2double(xx2);
    y3 = str2double(xx3);
    y4 = str2double(xx4);
    yyy1 = zeros(1,3);
    yyy2 = zeros(1,3);
    yyy3 = zeros(1,3);
    yyy4 = zeros(1,3);
    yy1 = zeros(1,3);
    yy2 = zeros(1,3);
    yy3 = zeros(1,3);
    yy4 = zeros(1,3);
    for k=1:60
        yy1(k) = y1(2*k-1) + i*y1(2*k);
        yy2(k) = y2(2*k-1) + i*y2(2*k);
        yy3(k) = y3(2*k-1) + i*y3(2*k);
        yy4(k) = y4(2*k-1) + i*y4(2*k);
    end
        
    %read the valid csi
    csi_0 = [yy1(7:32) yy1(34:59)];
    csi_1 = [yy2(7:32) yy2(34:59)];
    csi_2 = [yy3(7:32) yy3(34:59)];
    csi_3 = [yy4(7:32) yy4(34:59)];
    ang1 = angle(csi_1./csi_0);
    ang2 = angle(csi_2./csi_0);
    ang3 = angle(csi_3./csi_0);
 
      if(y1(7)==y2(7) && y2(7)==y3(7) && y3(7)==y4(7) ...filt tx
        && y1(66)==y2(66) && y1(66)==y3(66) && y1(66)==y4(66) ...%same frame
        && y1(121)==y2(121) && y1(121)==y3(121) && y1(121)==y4(121)...
        && y1(119)>25 && y2(119)>25 && y3(119)>25 && y4(119)>25 ...
        && mean(diff(csi_0)) < 0.05 && mean(diff(csi_1)) < 0.05 ...
        && mean(diff(csi_2)) < 0.05 && mean(diff(csi_3)) < 0.05)%    
    

    
     %if(1)
       %&& (abs(ang1(1) - preang1(1)) < 0.5 || abs(ang1(1) - preang1(1)) > 3.5) ...
       %&& (abs(ang2(1) - preang2(1)) < 0.5 || abs(ang2(1) - preang2(1)) > 3.5) ...
       %&& (abs(ang3(1) - preang3(1)) < 0.5 || abs(ang3(1) - preang3(1)) > 3.5) ...
      %)
      disp([y1(7),y2(7),y3(7),y4(7)])
      disp([y1(66),y2(66),y3(66),y4(66)])
      disp([y1(121),y2(121),y3(121),y4(121)])
      disp([y1(119),y2(119),y3(119),y4(119)])
      
      
        
        preang1 = ang1;
        preang2 = ang2;
        preang3 = ang3;
        X1(1,1:52) = csi_0;
        X1(2,1:52) = csi_1;
        X1(3,1:52) = csi_2;
        X1(4,1:52) = csi_3;     
       
        figure(2)
        hold on
        axis([0, 52, -3.5, 3.5]);
        plot(1:52,ang1,'r')
        plot(1:52,ang2,'g')
        plot(1:52,ang3,'b')
        legend("phase difference1(rx1-rx0)","phase difference1(rx2-rx0)","phase difference1(rx3-rx0)");
        xlabel("subcarrier");
        ylabel("phase difference(radian)");
     end
     end