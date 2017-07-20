#!/bin/bash
PKT_COUNT=10000
MIN_SNR=0
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



MAX_SNR[1]=2
MAX_SNR[2]=1.5
MAX_SNR[3]=1.1
MAX_SNR[4]=1

PARAMS=""
# simulate with different number of iterations
for i in  1 2 3 4
do
    echo "Starting run # ${i}..."
    # save results in the following file
    FILENAME="$DIR/${PKT_COUNT}pkts_code${i}.csv"
    LABEL=${LABELARRAY[i-1]}
    
    PARAMS="$PARAMS $FILENAME \"${LABEL}\""

    echo "Simulating $LABEL " 

    # run simulator
    /usr/bin/time -f "%E" -a -o $LOG ../bin/deepspace_turbo -y -m $MIN_SNR -M  ${MAX_SNR[i]} -n $SNR_POINTS -i 3 -o $FILENAME -c $PKT_COUNT -k 4 -C 4 -t ${i}

    echo "Done."

done

PARAMS="$PARAMS $DIR/rate_plot.png"

echo ${PARAMS} | xargs python2.7 -u ./plotting.py
