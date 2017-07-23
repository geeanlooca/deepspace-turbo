#!/bin/bash

if [ -z "$1" ]; then
    PKT_COUNT=1000
else
    PKT_COUNT=$1
fi

if [ -z "$2" ]; then
    ITER=3
else
    ITER=$2
fi

if [ -z "$3" ]; then
    OCTETS=4
else
    OCTETS=$3
fi

MIN_SNR=0
SNR_POINTS=10
TIMESTAMP=$(date +%F-%H-%M-%S)
CORES=4

# create results directory
mkdir -p results

# create directory for this specific simulation
DIR="./results/$TIMESTAMP"
# DIR="./results"

mkdir -p $DIR

# log file to save simulation time
LOG="$DIR/time.log"

echo "PACKET COUNT: ${PKT_COUNT}" >> $LOG
echo "ITERATIONS ${ITER}" >> $LOG
echo "OCTETS: ${OCTETS}" >> $LOG
echo "" >> $LOG
echo "" >> $LOG

declare -a LABERARRAY[3]
LABELARRAY[0]='Rate 1/2'
LABELARRAY[1]='Rate 1/3'
LABELARRAY[2]='Rate 1/4'
LABELARRAY[3]='Rate 1/6'

declare -a MAX_SNR[3]
MAX_SNR[0]=2
MAX_SNR[1]=1.5
MAX_SNR[2]=1.1
MAX_SNR[3]=1

PARAMS=""
for i in  1 2 3 4
do
    echo "Starting run #${i}..."
    # save results in the following file
    FILENAME="$DIR/${PKT_COUNT}pkts_code${i}.csv"

    echo $FILENAME >> $LOG
    echo "" >> $LOG
    
    LABEL=${LABELARRAY[i-1]}
    PARAMS="$PARAMS $FILENAME \"${LABEL}\""

    # run simulator
    /usr/bin/time -f "%E" -a -o $LOG ../bin/deepspace_turbo -y -m $MIN_SNR -M  ${MAX_SNR[i-1]} -n $SNR_POINTS -i $ITER -o $FILENAME -c $PKT_COUNT -k ${OCTETS} -C ${CORES} -t ${i}

    echo "Done."
    echo ""
done

PARAMS="$PARAMS $DIR/rate_plot.png"

echo ${PARAMS} | xargs python2.7 -u ./plotting.py
