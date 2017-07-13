#!/bin/bash
PKT_COUNT=1000
MIN_SNR=-3
MAX_SNR=2
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

LABELARRAY[0]='Rate 1/2'
LABELARRAY[1]='Rate 1/3'
LABELARRAY[2]='Rate 1/4'
LABELARRAY[3]='Rate 1/6'

PARAMS=""
# simulate with different number of iterations
for i in 1 2 3 4
do
    # save results in the following file
    FILENAME="$DIR/${PKT_COUNT}pkts_code${i}.csv"
    LABEL=${LABELARRAY[i-1]}
    
    PARAMS="$PARAMS $FILENAME \"${LABEL}\""

    echo "Simulating $LABEL " 

    # run simulator
    /usr/bin/time -f "%E" -a -o $LOG ../bin/deepspace_turbo -y -m $MIN_SNR -M  $MAX_SNR -n $SNR_POINTS -i 3 -o $FILENAME -c $PKT_COUNT -k 4 -C 4 -t ${i}> /dev/null

done

PARAMS="$PARAMS $DIR/rate_plot.png"

echo ${PARAMS} | xargs python2.7 -u ./plotting.py
