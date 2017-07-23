#!/bin/bash

if [ -z "$1" ]; then
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
    OCTETS=1
else
    OCTETS=$3
fi

MIN_SNR=0.5
SNR_POINTS=10
CORES=4
TIMESTAMP=$(date +%F-%H-%M-%S)

echo "CODE: ${CODE}"
echo "PACKET COUNT: ${PKT_COUNT}"
echo "OCTETS: ${OCTETS}"

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
echo "OCTETS: ${OCTETS}" >> $LOG
echo "" >> $LOG
echo "" >> $LOG

declare -a MAX_SNR[7]
MAX_SNR[1]=5
MAX_SNR[2]=3
MAX_SNR[3]=2.5
MAX_SNR[5]=2
MAX_SNR[7]=2

PARAMS=""
# simulate with different number of iterations
for i in 1 2 3 5 7
do

    echo "Simulating with ${i} iterations..."
    echo "Max SNR: ${MAX_SNR[i]} dB"

    # save results in the following file
    FILENAME="$DIR/${PKT_COUNT}pkts_${i}iter.csv"
    LABEL="${i} iterations"

    echo $FILENAME >> $LOG
    
    PARAMS="$PARAMS $FILENAME \"${LABEL}\""

    # run simulator
    /usr/bin/time -f "%E" -a -o $LOG ../bin/deepspace_turbo -y -m $MIN_SNR -M  ${MAX_SNR[i]} -n $SNR_POINTS -i $i -o $FILENAME -c $PKT_COUNT -C $CORES -t ${CODE} -k ${OCTETS}
    
    echo "" >> $LOG
    echo "Done..."

done

PARAMS="$PARAMS $DIR/iterations_plot.png"

echo ${PARAMS} | xargs python2.7 -u ./plotting.py
