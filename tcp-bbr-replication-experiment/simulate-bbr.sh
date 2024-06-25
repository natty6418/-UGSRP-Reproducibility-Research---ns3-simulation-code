#!/bin/bash

# Define the CSV file name
ROOT_DIR=`pwd`
CSV_FILE="${ROOT_DIR}/parameters.csv"

#mkdir -p tcp-bbr-cubic-results/throughput

# Read the CSV file line by line
while IFS=',' read -r qdiscSize bottleneck_bandwidth delay tcpTypeId trial; do
	# Skip the header line
	if [[ $qdiscSize == "qdiscSize" ]]; then
		echo "Skipping header line"
		continue
	fi

	# Construct the output file name using the parameters
	OUTPUT_FILE="${ROOT_DIR}/../../../tcp-bbr-cubic-results/${qdiscSize}_${bottleneck_bandwidth}_${delay}_${tcpTypeId}/goodput_retransmission_results.txt"

	echo "Output file: $OUTPUT_FILE"
	# Check if the output file already exists
	if [ -f "$OUTPUT_FILE" ]; then
		echo "Output file $OUTPUT_FILE already exists. Skipping..."
		continue
	fi

	# Run the simulation
	COMMAND="ns3 run \"tcp-bbr-replication.cc --qdiscSize=$qdiscSize --bottleneck_bandwidth=$bottleneck_bandwidth --delay=${delay} --tcpTypeId=ns3::$tcpTypeId\""

	echo "Running: $COMMAND"
	eval $COMMAND

done <"$CSV_FILE"
