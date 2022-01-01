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
% theta = [-30 0 60];	% ?????p????
% snr = 10;             % ?H????


% ?c???H??????
% A=exp(-1i*2*pi*d.'*sin(theta*derad));
% S=randn(M,K); X=A*S;
% X1=awgn(X,snr,'measured');

% MUSIC ?t???k
% load('60.mat');     % ???J????
%load("C:\Users\user\Desktop\20200605\save_data\recv_csi_data_5G_3M_1.mat")
%csv_tmp=save_csi_data{1,1}(1,1:168);
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
    
       
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%calibration%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%{    
%ubx, just substract constant
    phase_csi_1 =angle(csi_1) - cal_ubx_3139672;
    
        
    %sbx, need consider two kind of csi
    if(mean(diff(angle(csi_2)-angle(csi_0))) > mean(diff(ang1)))
        phase_csi_2 = angle(csi_2) - cal_sbx_F33434_sl2;
    else
        phase_csi_2 = angle(csi_2) - cal_sbx_F33434_sl1;
    end
    if(mean(diff(angle(csi_3)-angle(csi_0))) > mean(diff(ang1)))
        phase_csi_3 = angle(csi_3) - cal_sbx_F372B8_sl2;
    else
        phase_csi_3 = angle(csi_3) - cal_sbx_F372B8_sl1;
    end
    
    %convert phase back to complex
    [csi_x,csi_y] = pol2cart(phase_csi_1,1);
    csi_1 = [csi_x + i*csi_y];
    [csi_x,csi_y] = pol2cart(phase_csi_2,1);
    csi_2 = [csi_x + i*csi_y];   
    [csi_x,csi_y] = pol2cart(phase_csi_3,1);
    csi_3 = [csi_x + i*csi_y];
    ang1 = angle(csi_1)-angle(csi_0);
    ang2 = angle(csi_2)-angle(csi_0);
    ang3 = angle(csi_3)-angle(csi_0);
%}
    
%%%%%%%%%%%%%%%%%%%%%%%%%%prepare CSI for estimation%%%%%%%%%%%%%%%%%%%%%%%

%         
%     %move phase to fit -2pi~2pi for plotting
%     for kk=1:length(ang1)/2
%         if(ang1(kk) > pi)
%             ang1 = ang1 - 2*pi;
%             break;
%         elseif(ang1(kk) < -pi)
%             ang1 = ang1 + 2*pi;
%             break;
%         end
%     end
%     for kk=1:length(ang2)/2
%         if(ang2(kk) > pi)
%             ang2 = ang2 - 2*pi;
%             break;
%         elseif(ang2(kk) < -pi)
%             ang2 = ang2 + 2*pi;
%             break;
%         end 
%     end
%     for kk=1:length(ang3)/2
%         if(ang3(kk) > pi)
%             ang3 = ang3 - 2*pi;
%             break;
%         elseif(ang3(kk) < -pi)
%             ang3 = ang3 + 2*pi;
%             break;
%         end 
%     end
%     diff1 = diff(diff(ang1));
%     ansDiff = max(diff1) - min(diff1);
%     diff2 = diff(diff(ang2));
%     ansDiff2 = max(diff2) - min(diff2);
%     diff3 = diff(diff(ang3));
%     ansDiff3 = max(diff3) - min(diff3);
%     %usb-AC56:212
%     %s21 ultra:142
%     if(calculate_number == 0)
%        preang1 = ang1;
%        preang2 = ang2;
%        preang3 = ang3;
%     end
%     if(y1(7)==y2(7) && y2(7)==y3(7) && y3(7)==y4(7) ...filt tx
%         && y1(66)==y2(66) && y1(66)==y3(66) && y1(66)==y4(66) ...%same frame
%         && y1(121)==y2(121) && y1(121)==y3(121) && y1(121)==y4(121)...
%         && y1(119)>20 && y2(119)>20 && y3(119)>20 && y4(119)>20) %...
        %&& mean(diff(csi_0)) < 0.05 && mean(diff(csi_1)) < 0.05 ...
        %&& mean(diff(csi_2)) < 0.05 && mean(diff(csi_3)) < 0.05 ... 
        
     if(1)
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
        %plot(1:52,ang2,'g')
        %plot(1:52,ang3,'b')
        legend("phase difference1(rx1-rx0)","phase difference1(rx2-rx0)","phase difference1(rx3-rx0)");
        xlabel("subcarrier");
        ylabel("phase difference(radian)");
        calculate_number = calculate_number + 1;
        c1= X1(1,1:27);
        c2= X1(2,1:27);
        c3= X1(3,1:27);
        c4= X1(4,1:27);
        for j=2:26
            c1 = [c1;X1(1,j:26+j)];
            c2 = [c2;X1(2,j:26+j)];
            c3 = [c3;X1(3,j:26+j)];
            c4 = [c4;X1(4,j:26+j)];
        end
        Xe = [[c1;c2].';[c2;c3].';[c3;c4].'].';
    
        Rxx=Xe*Xe'/K;           % ?p???????t?x?}
        [EV,D]=eig(Rxx);        % ?S????????
        EVA=diag(D)';           % ???S?x??
        [EVA,I]=sort(EVA);      % ?S?x?????Cm
        EV=fliplr(EV(:,I));     % ???S?x???j?p???C?S?x?V?q
        En=EV(:,M+1:vN);         % ???T?l????
    
        % ?M???C???????A?p????????
        for iang = 1:361
            for j = -100:1:100
                ang(iang)=(iang-181)/2;
                tof(j+101)=j;
                phim=derad*ang(iang);
                a=(exp(-1i*2*pi*d*fc*sin(phim)/c).'*exp (-1i*2*pi*fd*j*1e-9)).';
                a= reshape(a,1,numel(a)).';
                Pmusic(iang,j+101)=abs(a'*a)/(a'*En*En'*a);
            end
        end
        Pmusic=abs(Pmusic);         % ????????
        Pmusic=10*log10(Pmusic);    % ??dB
    
%     ?@??
%         figure(2);
%         plot(ang,Pmusic,'Linewidth',0.5);
%         xlabel('?J?g??/(degree)');
%         ylabel('??????/(dB)');
%         set(gca, 'XTick',[-90:30:90]);
%         grid on;
%         hold on;
        %figure(1);
        %[ptof, pang] = meshgrid(tof,ang) ;
        %mesh(pang, ptof, Pmusic);
   
    
        [x y]=find(Pmusic==max(max(Pmusic)));
        x = x/2-90
    %y = abs(y-100)*30;
        rssi = [10*log(y1(65)) 10*log(y2(65)) 10*log(y3(65)) 10*log(y4(65))];
    %70
        if(y1(7) == 58)
            y = (42-max(rssi))/0.0458;
        elseif(y1(7) == 212)
            y = (45-max(rssi))/0.0458;
        end
        %display("angle="+x);
        %display("distance="+y);
        plotang = [x plotang];
        plotdis = [y plotdis];
        plotMAC = [y1(7) plotMAC];
        save_rssi = [rssi;save_rssi];
        figure(3);
        clf
        hold on;
        plot([-180,750],[-220,-220],'k','LineWidth',2);
        plot([-180,750],[220,220],'k','LineWidth',2);
        plot([-180,-180],[-220,220],'k','LineWidth',2);
        plot([750,750],[220,-220],'k','LineWidth',2);
        plot([0,0],[-5,5],'k','LineWidth',2);
        plot([25,340],[-120,-120],'k','LineWidth',2);
        plot([340,340],[-120,-220],'k','LineWidth',2);
        plot([-130,-130],[220,-120],'k','LineWidth',2);
        plot([-130,-180],[-120,-120],'k','LineWidth',2);
        plot([440,660],[-150,-150],'k','LineWidth',2);
        plot([440,440],[-150,-220],'k','LineWidth',2);
        plot([660,660],[-150,-220],'k','LineWidth',2);
        plot([530,470],[180,180],'k','LineWidth',2);
        plot([530,530],[180,220],'k','LineWidth',2);
        plot([470,470],[180,220],'k','LineWidth',2);
        plot([520,480],[160,160],'k','LineWidth',2);
        plot([520,480],[120,120],'k','LineWidth',4);
        plot([520,520],[160,120],'k','LineWidth',2);
        plot([480,480],[160,120],'k','LineWidth',2);
        plot(0,0,'r','Marker','x');
        t1=text(-15,-20,'Rx');
        set(t1, 'Color',[1, 0 ,0]);
        t2=text(200,-180,'stair');
        set(t2, 'Color',[0, 0 ,1]);
        t3=text(480,-180,'Sundries');
        set(t3, 'Color',[0, 0 ,1]);
        t4=text(580,180,'Desk');
        set(t4, 'Color',[0, 0 ,1]);
        axis([-200,800,-250,250]);
        if length(plotang) < plotlength
            for ii=1:length(plotang)-1
                plot(plotdis(ii)*cos(plotang(ii)*derad),plotdis(ii)*sin(plotang(ii)*derad),'*');
                %%tt=text(plotdis(ii)*cos(plotang(ii)*derad),plotdis(ii)*sin(plotang(ii)*derad),int2str(plotMAC(ii)));
                %%set(tt, 'Color',[0, 1 ,0]);
            end
        else
            for ii=1:plotlength
                plot(plotdis(ii)*cos(plotang(ii)*derad),plotdis(ii)*sin(plotang(ii)*derad),'*');
                %%tt=text(plotdis(ii)*cos(plotang(ii)*derad),plotdis(ii)*sin(plotang(ii)*derad),int2str(plotMAC(ii)));
                %%set(tt, 'Color',[0, 1 ,0]);
            end
        end
        hold off;
    
        % AoA=(x-180)/2
        % ToF=y-101
    end
    fclose('all');
 end
