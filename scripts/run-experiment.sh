DB_DIR="/tmp/rocksdb_perf_example"

../examples/perf_example 4 100000

mv $DB_DIR/io.log io.log
mv $DB_DIR/perf.log perf.log

python analyse_results.py io.log io-condensed.log
python analyse_results.py perf.log perf-condensed.log

