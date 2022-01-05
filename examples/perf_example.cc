#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <mutex>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"
#include "rocksdb/iostats_context.h"
#include "rocksdb/perf_context.h"

using namespace ROCKSDB_NAMESPACE;

#if defined(OS_WIN)
std::string kDBPath = "C:\\Windows\\TEMP\\rocksdb_perf_example";
#else
std::string kDBPath = "/home/dev/rocksdb/dbs/rocksdb_perf_example";
#endif

std::mutex stat_mutex;
long cs_mutex_lock_nanos = 0;
long db_mutex_lock_nanos = 0;
long key_lock_wait_time = 0;
long key_lock_wait_count = 0;


void put(int thread_id, long requests, DB* db){
  Status s;
  std::cout << "Writing from thread " << thread_id << ", number of requests " << requests << "\n";

  rocksdb::SetPerfLevel(rocksdb::PerfLevel::kEnableTime);

  rocksdb::get_perf_context()->Reset();
  rocksdb::get_iostats_context()->Reset();

  for (long i = 0; i < requests; i++)
  {
    s = db->Put(WriteOptions(), "key" + (thread_id * requests) + i, "dummy value");
    assert(s.ok());
  }

  std::string value;
  for (int j = 0; j < requests; ++j){
    s = db->Get(ReadOptions(), "key" + (thread_id * requests) + j, &value);
    assert(s.ok());
    assert(value == "dummy value");
    value = "";
  }

  auto perf_context = rocksdb::get_perf_context();
  auto io_context = rocksdb::get_iostats_context();
  std::lock_guard<std::mutex> guard(stat_mutex);
  db_mutex_lock_nanos += perf_context->db_mutex_lock_nanos;
  key_lock_wait_time += perf_context->key_lock_wait_time;
  key_lock_wait_count += perf_context->key_lock_wait_count;
  cs_mutex_lock_nanos += perf_context->cs_mutex_lock_nanos;
}

int main(int argc, char **argv) {
  unsigned long long  n_threads;
  unsigned long long  n_requests;

  std::cout << kDBPath << "\n";

  if (argc < 2){
    std::cout << "Enter more arguments" << "\n";
    return -1;
  }

  n_threads = atoi(argv[1]);
  n_requests = atoi(argv[2]);

  DB* db;
  Options options;
  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
  // create the DB if it's not already present
  options.create_if_missing = true;
  options.statistics = rocksdb::CreateDBStatistics();

  // open DB
  Status s = DB::Open(options, kDBPath, &db);
  if (!s.ok()) std::cout << s.ToString() << "\n";
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
  perf_log_file << "db_mutex_lock_nanos: " + std::to_string(db_mutex_lock_nanos) + "\n";
  perf_log_file << "key_lock_wait_time: " + std::to_string(key_lock_wait_time) + "\n";
  perf_log_file << "key_lock_wait_count: " + std::to_string(key_lock_wait_count) + "\n";
  perf_log_file.close();

  // std::ofstream io_log_file(kDBPath + "/io.log");
  // std::string io_log = rocksdb::get_iostats_context()->ToString();
  // io_log_file << io_log;
  // io_log_file.close();

  delete db;

  return 0;
}
