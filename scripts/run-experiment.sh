DB_DIR="/home/dev/rocksdb/dbs/rocksdb_perf_example"
N_THREADS=(1 2 4 8 16 32)
N_REQUESTS=100000

rm -rf logs $DB_DIR
mkdir logs

for t in ${N_THREADS[@]}; do
    for i in {1..5}; do
        echo "Running $t threads, rep $i"
        ../examples/perf_example $t $N_REQUESTS
        mv $DB_DIR/perf.log logs/perf-$t-$i.log

        sleep 10
    done
done

