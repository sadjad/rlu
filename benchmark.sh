#!/bin/bash

BENCH_BIN=./benchmarks/bench-list

if [ ! -f "${BENCH_BIN}" ]
then
  echo "error: Could not find the benchmark program at \`${BENCH_BIN}\`. Please build the project first."
  exit 1
fi

if [ $# -lt 3 ]; then
  echo "Usage: $(basename $0) (rlu|rcu) UPDATE-RATIO OUTPUT-DIR"
  exit 1
fi

N_THREADS=(1 2 4 6 8 10 12 14 16)
MIN_VALUE=0
MAX_VALUE=2047
INITIAL_SIZE=1024
DURATION=30

SCHEME=$1
UPDATE_RATIO=$2
OUTPUT_DIR=$3

mkdir -p ${OUTPUT_DIR}

OUTPUT_FILE=${OUTPUT_DIR}/benchmark_${SCHEME}_${UPDATE_RATIO}.txt

echo "scheme,threads,update_ratio,ops,time,ops_per_us,add,erase,contains,found" >${OUTPUT_FILE}

for CORES in ${N_THREADS[*]}
do
  echo -n "scheme=${SCHEME}, update-ratio=${UPDATE_RATIO}, cores=${CORES}... "

  OUTPUT=`${BENCH_BIN} ${SCHEME} --min-value ${MIN_VALUE} --max-value \
    ${MAX_VALUE} --initial-size ${INITIAL_SIZE} --duration ${DURATION} \
    --threads ${CORES} --update-ratio ${UPDATE_RATIO} 2>/dev/null | grep -v '#'`

  echo "${SCHEME},${UPDATE_RATIO},${CORES},${OUTPUT}" >>${OUTPUT_FILE}
  echo "done."
done
