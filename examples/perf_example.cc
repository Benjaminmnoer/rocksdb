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

inline bool IsLittleEndian() {
  uint32_t x = 1;
  return *reinterpret_cast<char*>(&x) != 0;
}

static std::string Key(uint64_t k) {
  std::string ret;
  if (IsLittleEndian()) {
    ret.append(reinterpret_cast<char*>(&k), sizeof(k));
  } else {
    char buf[sizeof(k)];
    buf[0] = k & 0xff;
    buf[1] = (k >> 8) & 0xff;
    buf[2] = (k >> 16) & 0xff;
    buf[3] = (k >> 24) & 0xff;
    buf[4] = (k >> 32) & 0xff;
    buf[5] = (k >> 40) & 0xff;
    buf[6] = (k >> 48) & 0xff;
    buf[7] = (k >> 56) & 0xff;
    ret.append(buf, sizeof(k));
  }
  size_t i = 0, j = ret.size() - 1;
  while (i < j) {
    char tmp = ret[i];
    ret[i] = ret[j];
    ret[j] = tmp;
    ++i;
    --j;
  }
  return ret;
}

static Slice GenerateRandomValue(const size_t max_length, char scratch[]) {
  size_t sz = 1 + (std::rand() % max_length);
  int rnd = std::rand();
  for (size_t i = 0; i != sz; ++i) {
    scratch[i] = static_cast<char>(rnd ^ i);
  }
  return Slice(scratch, sz);
}

void put(int thread_id, long requests, DB* db){
  Status s;
  std::cout << "Writing from thread " << thread_id << ", number of requests " << requests << "\n";

  rocksdb::SetPerfLevel(rocksdb::PerfLevel::kEnableTime);

  rocksdb::get_perf_context()->Reset();
  rocksdb::get_iostats_context()->Reset();

  char val_buf[256] = {0};
  for (long i = 0; i < requests; i++)
  {
    Slice key = Key((thread_id * requests) + i);
    Slice value = GenerateRandomValue(256, val_buf);
    s = db->Put(WriteOptions(), key, value);
    assert(s.ok());
  }

  std::string value;
  for (int j = 0; j < requests; ++j){
    auto key = Key((thread_id * requests) + j);
    s = db->Get(ReadOptions(), key, &value);
    assert(s.ok());
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
