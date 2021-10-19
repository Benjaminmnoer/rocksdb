DB_DIR="/tmp/rocksdb_perf_example"

../examples/perf_example 1 100000

mv $DB_DIR/io.log io.log
mv $DB_DIR/perf.log perf.log

python io.log io-condensed.log
python perf.log perf-condensed.log

