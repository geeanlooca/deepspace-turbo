import sys
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

plt.rc('text', usetex=True)
plt.rc('font', family='serif')

arg_num = len(sys.argv)

if arg_num < 2 or (arg_num - 1) % 2 == 0:
    print "Incorrect number of arguments"
    exit(1)

files = []
labels = []

output_file = sys.argv[-1]

print 'Files to process:'
for i in xrange(1, arg_num-1, 2):
    files.append(sys.argv[i])
    labels.append(sys.argv[i+1])
    print sys.argv[i] + ' with label ' + sys.argv[i+1]

fig = plt.figure()
for i in range(len(files)):
    df = pd.read_csv(files[i], sep=',')
    SNR = df['EbN0']
    BER = df['BER']
    plt.semilogy(SNR, BER, label=labels[i])

grid_color=np.ones(3)*0.1

plt.legend(loc='best', fontsize=14)
plt.grid(alpha=0.7, which='both')
plt.tick_params(labelsize=12)
plt.xlabel('$\Gamma$', fontsize=16)
plt.ylabel('BER', fontsize=16)
plt.savefig(output_file, dpi=300)
plt.close(fig)

print "Plot saved in " + output_file
