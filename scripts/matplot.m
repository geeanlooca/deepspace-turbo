set(0,'DefaultFigureWindowStyle','docked')

clear all;
close all;

file1 = 'prova/5000pkts_code1.csv';
table1 = readtable(char(file1), 'Delimiter', 'comma');
data1 = table2array(table1);

file2 = 'prova/5000pkts_code2.csv';
table2 = readtable(char(file2), 'Delimiter', 'comma');
data2 = table2array(table2);

file3 = 'prova/1000pkts_code3.csv';
table3 = readtable(char(file3), 'Delimiter', 'comma');
data3 = table2array(table3);

file4 = 'prova/1000pkts_code4.csv';
table4 = readtable(char(file4), 'Delimiter', 'comma');
data4 = table2array(table4);

SNR1 = data1(:, 1);
SNR2 = data2(:, 1);
SNR3 = data3(:, 1);
SNR4 = data4(:, 1);

BER1 = data1(:, 2);
BER2 = data2(:, 2);
BER3 = data3(:, 2);
BER4 = data4(:, 2);

EbN0dB = linspace(0,5, 100);
EbN0 = 10.^(EbN0dB/10);

uncoded = qfunc(sqrt(2*EbN0));

figure;
   semilogy(SNR1, BER1, '--kx');
   hold;
   semilogy(SNR2, BER2, '-.ro');
    hold on;
   semilogy(SNR3, BER3, ':b*');
   semilogy(SNR4, BER4, '--ms');
   semilogy(EbN0dB, uncoded, 'g');
   hold off;