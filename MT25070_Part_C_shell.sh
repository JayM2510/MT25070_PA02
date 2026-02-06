#!/bin/bash
# Roll No: MT25070
# PA02 Automated Experiments Script

set -e      # Exit immediately if any command fails

# Output CSV file
CSV_FILE="MT25070_Part_C_AllResults.csv"

# Write CSV header (overwrite existing file)
echo "implementation,message_size,threads,throughput_gbps,avg_latency_us,cycles,instructions,context_switches,cache_references,cache_misses" \
> "$CSV_FILE"

# Network namespace configuration
SERVER_NS="ns_server"
CLIENT_NS="ns_client"
SERVER_IP="10.0.0.1"
PORT=5000
DURATION=10

# Experiment parameters
MSG_SIZES=(64 256 1024 4096) # Message sizes in bytes
THREADS=(1 2 4 8)            # Thread counts

# Function to run experiments for one implementation
run_test() {
	IMPL=$1           # Implementation name (two_copy / one_copy / zero_copy)
	SERVER_BIN=$2     # Server binary
	CLIENT_BIN=$3     # Client binary
  
  # Iterate over message sizes
	for MSG in "${MSG_SIZES[@]}"; do
     # Iterate over thread counts
		for TH in "${THREADS[@]}"; do
			echo "[RUN] $IMPL msg=$MSG threads=$TH"
      
      # Start server in server namespace (background)
			sudo ip netns exec $SERVER_NS ./$SERVER_BIN $PORT $MSG &
			SRV_PID=$!
			sleep 1   # Allow server time to start
      
      # Run client with perf stat to collect hardware counters
			OUT=$(sudo ip netns exec $CLIENT_NS perf stat \
				-e cycles,instructions,context-switches,cache-references,cache-misses \
				-x, \
				./$CLIENT_BIN $SERVER_IP $PORT $MSG $TH $DURATION \
				2> perf.tmp)
      
      # Stop the server after client finishes
			sudo kill $SRV_PID || true
			wait $SRV_PID 2>/dev/null || true

			TP=$(echo "$OUT" | grep THROUGHPUT_GBPS | cut -d= -f2)
			LAT=$(echo "$OUT" | grep AVG_LATENCY_US | cut -d= -f2)
      
      # Extract perf metrics from perf output
			read CYC INS CTX CRE CMISS <<EOF
$(awk -F, '
/cycles/ {c=$1}
/instructions/ {i=$1}
/context-switches/ {x=$1}
/cache-references/ {r=$1}
/cache-misses/ {m=$1}
END {print c,i,x,r,m}' perf.tmp)
EOF
      # Append results as a single row in the CSV file
			echo "$IMPL,$MSG,$TH,$TP,$LAT,$CYC,$INS,$CTX,$CRE,$CMISS" \
			>> "$CSV_FILE"

      # Cleanup temporary perf output
			rm -f perf.tmp
		done
	done
}

# Compile all implementations
make clean
make

# Run experiments for all implementations
run_test "two_copy"  MT25070_A1_Server MT25070_A1_Client
run_test "one_copy"  MT25070_A2_Server MT25070_A2_Client
run_test "zero_copy" MT25070_A3_Server MT25070_A3_Client


