set(0,'DefaultFigureWindowStyle','docked')

clear all;
close all;

file1 = 'prova/20000pkts_1iter.csv';
table1 = readtable(char(file1), 'Delimiter', 'comma');
data1 = table2array(table1);

file2 = 'prova/10000pkts_2iter.csv';
table2 = readtable(char(file2), 'Delimiter', 'comma');
data2 = table2array(table2);

file3 = 'prova/10000pkts_3iter.csv';
table3 = readtable(char(file3), 'Delimiter', 'comma');
data3 = table2array(table3);

file4 = 'prova/10000pkts_5iter.csv';
table4 = readtable(char(file4), 'Delimiter', 'comma');
data4 = table2array(table4);

file5 = 'prova/10000pkts_7iter.csv';
table5 = readtable(char(file5), 'Delimiter', 'comma');
data5 = table2array(table5);

SNR1 = data1(:, 1);
SNR2 = data2(:, 1);
SNR3 = data3(:, 1);
SNR4 = data4(:, 1);
SNR5 = data5(:, 1);

BER1 = data1(:, 2);
BER2 = data2(:, 2);
BER3 = data3(:, 2);
BER4 = data4(:, 2);
BER5 = data5(:, 2);

EbN0dB = linspace(0,15, 128);
EbN0 = 10.^(EbN0dB/10);

uncoded = qfunc(sqrt(2*EbN0));

figure;
   %semilogy(SNR1, BER1, '--kx');
   %hold on;
        %semilogy(SNR2, BER2, '-.ro');
        %semilogy(SNR3, BER3, ':b*');
        %semilogy(SNR4, BER4, '--ms');
        %semilogy(SNR5, BER5, '--c^');
semilogy(EbN0dB, uncoded, 'g');
   %hold off;
   legend('Rate 1/2');
   xlim([0.5 10]);
   grid on;
   grid minor;
   
csvwrite('uncoded.csv', [EbN0dB', uncoded']);