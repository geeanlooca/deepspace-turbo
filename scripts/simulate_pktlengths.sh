#!/bin/bash

if [ -z "$1" ]; then
    echo "1 is unset"
    PKT_COUNT=1000
else
    PKT_COUNT=$1
fi

if [ -z "$2" ]; then
    CODE=1
else
    CODE=$2
fi

if [ -z "$3" ]; then
    ITER=3
else
    ITER=$3
fi

MIN_SNR=0.0
MAX_SNR=1.25
SNR_POINTS=10
CORES=4
TIMESTAMP=$(date +%F-%H-%M-%S)

echo "CODE: ${CODE}"
echo "PACKET COUNT: ${PKT_COUNT}"
echo "ITER: ${ITER}"

# create results directory
mkdir -p results

# create directory for this specific simulation
DIR="./results/$TIMESTAMP"
# DIR="./results"

mkdir -p $DIR

# log file to save simulation time
LOG="$DIR/time.log"

echo "CODE: ${CODE}" >> $LOG
echo "PACKET COUNT: ${PKT_COUNT}" >> $LOG
echo "ITER: ${ITER}" >> $LOG
echo "" >> $LOG
echo "" >> $LOG

PARAMS=""
for i in 1 2 4 5
do

    echo "Simulating with ${i} octets..."

    # save results in the following file
    FILENAME="$DIR/${PKT_COUNT}pkts_${i}octets.csv"
    LABEL="${i} octets"

    echo $FILENAME >> $LOG
    
    PARAMS="$PARAMS $FILENAME \"${LABEL}\""

    # run simulator
    /usr/bin/time -f "%E" -a -o $LOG ../bin/deepspace_turbo -y -m $MIN_SNR -M  $MAX_SNR -n $SNR_POINTS -i ${ITER} -o $FILENAME -c $PKT_COUNT -C $CORES -t ${CODE} -k ${i}
    
    echo "" >> $LOG
    echo "Done..."

done

PARAMS="$PARAMS $DIR/pktlenghts_plot.png"

echo ${PARAMS} | xargs python2.7 -u ./plotting.py
