#!/bin/bash
PKT_COUNT=100
for i in 1 2 3
do
    FILENAME="${PKT_COUNT}pkts_${i}iter.csv"
    echo $FILENAME
    ./deepspace_turbo -y -m -2 -M 1 -n 10 -i $i -o $FILENAME -c $PKT_COUNT
done
