#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"
#include "rocksdb/iostats_context.h"
#include "rocksdb/perf_context.h"

using namespace ROCKSDB_NAMESPACE;

#if defined(OS_WIN)
std::string kDBPath = "C:\\Windows\\TEMP\\rocksdb_perf_example";
#else
std::string kDBPath = "/tmp/rocksdb_perf_example";
#endif

int main(int argc, char **argv) {
  int n_repittions;
  int n_requests;

  if (argc < 2){
    cout << "Enter more arguments" << "\n";
  }

  n_repittions = argv[1];
  n_requests = argv[2];

  DB* db;
  Options options;
  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
  // create the DB if it's not already present
  options.create_if_missing = true;

  // open DB
  Status s = DB::Open(options, kDBPath, &db);
  assert(s.ok());

  rocksdb::SetPerfLevel(rocksdb::PerfLevel::kEnableTimeExceptForMutex);

  rocksdb::get_perf_context()->Reset();
  rocksdb::get_iostats_context()->Reset();

  for (size_t i = 0; i < n_requests; i++)
  {
    s = db->Put(WriteOptions(), "key" + i, "value" + i);
    assert(s.ok());
  }
    
  std::string value;
  for (size_t i = 0; i < n_requests; i++)
  {
    s = db->Get(ReadOptions(), "key" + i, &value);
    assert(s.ok());
    assert(value == ("value" + i));
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
