clear all;
close all;
clc;

Prefix = '/home/lion/Videos/fivecaishu/8/';
%PreRes = '/home/lion/Videos/fivecaishu/6/sixth.txt';
d1 = [Prefix '10_2'];
d2 = [Prefix '20_3'];
d3 = [Prefix '20_5'];
d4 = [Prefix '10_4'];

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
CSI3 = reshape(csi3_c,64, []);
CSI4 = reshape(csi4_c,64, []);

% get second column, which is the second long snyc training
% this method of getting second column is a little werid.
% CSI1_row2=CSI1(:,2);
% CSI2_row2=CSI2(:,2);
% CSI3_row2=CSI3(:,2);
% CSI4_row2=CSI4(:,2);
% pre = 1;
% for cc=2:size(CSI1,2)-1
%     if CSI1(60,cc)~=CSI1(60,cc-1) && cc-pre>10
%         CSI1_row2=[CSI1_row2 CSI1(:,pre)];
%         CSI2_row2=[CSI2_row2 CSI2(:,pre)];
%         CSI3_row2=[CSI3_row2 CSI3(:,pre)];
%         CSI4_row2=[CSI4_row2 CSI4(:,pre)];
%         pre = cc;
%         continue;
%     end
%     if CSI1(60,cc)~=CSI1(60,cc-1)
%         pre = cc;
%     end
% end



CSI1_row2=CSI1(:,5);
CSI2_row2=CSI2(:,5);
CSI3_row2=CSI3(:,5);
CSI4_row2=CSI4(:,5);
for cc=6:size(CSI1,2)-1
    if CSI1(60,cc)~=CSI1(60,cc-1)
        CSI1_row2=[CSI1_row2 CSI1(:,cc)];
        CSI2_row2=[CSI2_row2 CSI2(:,cc)];
        CSI3_row2=[CSI3_row2 CSI3(:,cc)];
        CSI4_row2=[CSI4_row2 CSI4(:,cc)];
    end
end

% get time
cur_hr=CSI1_row2(61,:);
cur_min=CSI1_row2(62,:);
cur_sec=CSI1_row2(63,:);
cur_millisec=CSI1_row2(64,:); 
for cc=2:size(CSI1_row2,2)
    if cur_sec(1,cc)~=cur_sec(1,cc-1)
        del = 1000-cur_millisec(1,cc);
        for dd=1:size(CSI1_row2,2)
            cur_millisec(1,dd)=mod(cur_millisec(1,dd)+del,1000);
        end
        break
    end
end
% get snr
snr1=CSI1_row2(60,:);
snr2=CSI2_row2(60,:);
snr3=CSI3_row2(60,:);
snr4=CSI4_row2(60,:);

%delete the useless value
%detele_row=[1:6 33 60:64]
CSI1_row2(60:64,:)=[];
CSI1_row2(33,:)=[];
CSI1_row2(1:6,:)=[];
CSI2_row2(60:64,:)=[];
CSI2_row2(33,:)=[];
CSI2_row2(1:6,:)=[];
CSI3_row2(60:64,:)=[];
CSI3_row2(33,:)=[];
CSI3_row2(1:6,:)=[];
CSI4_row2(60:64,:)=[];
CSI4_row2(33,:)=[];
CSI4_row2(1:6,:)=[];


%save csi and snr
CSI1_row2=reshape(CSI1_row2,[],52);
CSI2_row2=reshape(CSI2_row2,[],52);
CSI3_row2=reshape(CSI3_row2,[],52);
CSI4_row2=reshape(CSI4_row2,[],52);

res=[cur_hr(1,1) cur_min(1,1) cur_sec(1,1) cur_millisec(1,1)];
res=[res;res;res;res];
for cc=2:size(CSI1_row2,1)
    tmp=[cur_hr(1,cc) cur_min(1,cc) cur_sec(1,cc) cur_millisec(1,cc)];
    res=[res;tmp;tmp;tmp;tmp];
end
tmp=[CSI1_row2(1,:);CSI2_row2(1,:);CSI3_row2(1,:);CSI4_row2(1,:)];
for cc=2:size(CSI1_row2,1)
    tmp=[tmp;CSI1_row2(cc,:);CSI2_row2(cc,:);CSI3_row2(cc,:);CSI4_row2(cc,:)];
end
res=[res tmp];
tmp2=ones([size(CSI1_row2,1)*4,1]);
for cc=0:size(CSI1_row2,1)-1
    tmp2(cc*4+1,1)=snr1(cc+1);
    tmp2(cc*4+2,1)=snr2(cc+1);
    tmp2(cc*4+3,1)=snr3(cc+1);
    tmp2(cc*4+4,1)=snr4(cc+1);
end
res=[res tmp2];

save('tenth.mat','res');

% fRes=fopen(PreRes,'a');
% for cc=1:size(CSI1_row2,1)*4
%     for dd=1:57
%         fprintf(fRes,'%f ', res(cc,dd));
%     end
%     fprintf(fRes,'\n');
% end
% fclose(fRes);