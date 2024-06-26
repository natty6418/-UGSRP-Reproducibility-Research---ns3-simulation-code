#!/bin/bash

ROOT_DIR=`~/`
CSV_FILE="/home/ubuntu/source/ns-3.42/scratch/tcp-bbr-repo/parameters.csv"
OUTPUT_DIR="/home/ubuntu/simulation_data/"
PATH=$PATH:"/home/ubuntu/source/ns-3.42/"
TARGET_ROW=$SLURM_ARRAY_TASK_ID

# Go to working dir
cd $ROOT_DIR

# Create directory for data files
mkdir -p $OUTPUT_DIR

# Read *one* line from CSV file
while IFS=',' read -r qdiscSize bottleneck_bandwidth delay tcpTypeId trial; do

        # Do stuff with the variables
        echo "qdiscSize: $qdiscSize, bottleneck_bandwidth: $bottleneck_bandwidth, delay: $delay, tcpTypeId: $tcpTypeId, trial: $trial"

        # Construct the output file name using the parameters
        OUTPUT_FILE="${OUTPUT_DIR}${qdiscSize}_${bottleneck_bandwidth}_${delay}_${tcpTypeId}/goodput_retransmission_results.txt"

        echo "Output file: $OUTPUT_FILE"
        # Check if the output file already exists
        if [ -f "$OUTPUT_FILE" ]; then
                echo "Output file $OUTPUT_FILE already exists. Skipping..."
                continue
        fi

        # Run the simulation
        COMMAND="ns3 run \"tcp-bbr-replication.cc --qdiscSize=$qdiscSize --bottleneck_bandwidth=$bottleneck_bandwidth --delay=${delay} --tcpTypeId=ns3::$tcpTypeId --dir=${OUTPUT_DIR}\""

        echo "Running: $COMMAND"
        eval $COMMAND
done < <(tail -n +$TARGET_ROW "$CSV_FILE" | head -n 1)