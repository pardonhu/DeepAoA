clear all;
close all;
clc;

%Prefix = '/home/lion/Videos/pattern_4rx/'
Prefix = '/home/lion/Videos/fivecaishu/5/'
d1 = [Prefix '10_2'];
d2 = [Prefix '10_4'];
d3 = [Prefix '20_5'];
d4 = [Prefix '20_3'];

f1 = fopen(d1);
f2 = fopen(d2);
f3 = fopen(d3);
f4 = fopen(d4);

csi1 = fread(f1,'float32');
csi2 = fread(f2,'float32');
csi3 = fread(f3,'float32');
csi4 = fread(f4,'float32');

real1=csi1(1:2:length(csi1));
img1=csi1(2:2:length(csi1));

real2=csi2(1:2:length(csi2));
img2=csi2(2:2:length(csi2));

real3=csi3(1:2:length(csi3));
img3=csi3(2:2:length(csi3));

real4=csi4(1:2:length(csi4));
img4=csi4(2:2:length(csi4));

csi1_c = real1+i*img1;
csi2_c = real2+i*img2;
csi3_c = real3+i*img3;
csi4_c = real4+i*img4;

CSI1 = reshape(csi1_c,64, []);
CSI2 = reshape(csi2_c,64, []);


CSI1_row2=CSI1(:,2);
CSI2_row2=CSI2(:,2);
%CSI3_row2=CSI3(:,2);
%CSI4_row2=CSI4(:,2);
for cc=3:size(CSI1,2)-1
    if CSI1(60,cc)~=CSI1(60,cc-1)
        CSI1_row2=[CSI1_row2 CSI1(:,cc+1)];
        CSI2_row2=[CSI2_row2 CSI2(:,cc+1)];
        %CSI3_row2=[CSI3_row2 CSI3(:,cc+1)];
        %CSI4_row2=[CSI4_row2 CSI4(:,cc+1)];
    end
end


% CSI1_row2(60:64)=[]
% CSI1_row2(33,:)=[]
% CSI1_row2(1:6,:)=[]
% 
% CSI2_row2(1:6,:)=[]
% CSI2_row2(33,:)=[]
% CSI2_row2(60:64)=[]

Phase1 = angle(CSI1_row2./CSI2_row2)
Phase_re = reshape(Phase1,[],1)
figure
plot(46080:46720,Phase_re(46080:46720))
title('CSI phase difference')

CSI1_phase = angle(reshape(CSI1_row2,[],1));
CSI2_phase = angle(reshape(CSI2_row2,[],1));
% CSI3_phase = angle(csi3);
% CSI4_phase = angle(csi4);


figure
hold on
plot(46080:46720,CSI1_phase(46080:46720),'r')
plot(46080:46720,CSI2_phase(46080:46720),'b')
title('CSI phase')
legend('ant1','ant2')

% 
% figure
% diff12 = angle(csi1_c./csi2_c);
% plot(1:length(diff12(1000:1600)),diff12(1000:1600),'r')




