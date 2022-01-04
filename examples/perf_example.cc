#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"
#include "rocksdb/iostats_context.h"
#include "rocksdb/perf_context.h"

using namespace ROCKSDB_NAMESPACE;

#if defined(OS_WIN)
std::string kDBPath = "C:\\Windows\\TEMP\\rocksdb_perf_example";
#else
std::string kDBPath = "/home/benjaminmnoer25/rocksdb/dbs/rocksdb_perf_example";
#endif

void put(int thread_id, long requests, DB* db){
  Status s;
  std::cout << "Writing from thread " << thread_id << ", number of requests " << requests << "\n";

  for (long i = 0; i < requests; i++)
  {
    s = db->Put(WriteOptions(), "key" + (thread_id * requests) + i, "dummy value");
    assert(s.ok());
  }
}

int main(int argc, char **argv) {
  unsigned long long  n_threads;
  unsigned long long  n_requests;

  if (argc < 2){
    std::cout << "Enter more arguments" << "\n";
    return -1;
  }

  n_threads = atoi(argv[1]);
  n_requests = atoi(argv[2]);

  DB* db;
  DBOptions options;
  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
  options.IncreaseParallelism();
  // options.OptimizeLevelStyleCompaction();
  // create the DB if it's not already present
  options.create_if_missing = true;

  std::vector<ColumnFamilyDescriptor> cf_descs;
  cf_descs.push_back({kDefaultColumnFamilyName, ColumnFamilyOptions()});

  // open DB
  Status s = DB::Open(Options(options, cf_descs[0].options), kDBPath, &db);
  assert(s.ok());

  rocksdb::SetPerfLevel(rocksdb::PerfLevel::kEnableTime);

  rocksdb::get_perf_context()->Reset();
  rocksdb::get_iostats_context()->Reset();

  std::vector<std::thread> threads;
  for (int i = 0; i < n_threads; ++i) {
    threads.emplace_back(std::thread(put, i, n_requests, db));
  }

  for (auto& t : threads) {
      t.join();
  }

  std::ofstream perf_log_file(kDBPath + "/perf.log");
  std::string perf_log = rocksdb::get_perf_context()->ToString();
  perf_log_file << perf_log;
  perf_log_file.close();

  std::ofstream io_log_file(kDBPath + "/io.log");
  std::string io_log = rocksdb::get_iostats_context()->ToString();
  io_log_file << io_log;
  io_log_file.close();

  delete db;

  return 0;
}
