#!/bin/bash
PKT_COUNT=100
MIN_SNR=-3
MAX_SNR=1
SNR_POINTS=10
TIMESTAMP=$(date +%F-%H-%M-%S)

# create results directory
mkdir -p results

# create directory for this specific simulation
DIR="./results/$TIMESTAMP"
# DIR="./results"

mkdir -p $DIR

# log file to save simulation time
LOG="$DIR/time.log"

PARAMS=""
# simulate with different number of iterations
for i in 1 2 3 5
do
    # save results in the following file
    FILENAME="$DIR/${PKT_COUNT}pkts_${i}iter.csv"
    LABEL="${i} iterations"
    
    PARAMS="$PARAMS $FILENAME \"${LABEL}\""

    # run simulator
    /usr/bin/time -f "%E" -a -o $LOG ../bin/deepspace_turbo -y -m $MIN_SNR -M  $MAX_SNR -n $SNR_POINTS -i $i -o $FILENAME -c $PKT_COUNT > /dev/null

done

PARAMS="$PARAMS $DIR/iterations_plot.png"

echo ${PARAMS} | xargs python2.7 -u ./plotting.py
