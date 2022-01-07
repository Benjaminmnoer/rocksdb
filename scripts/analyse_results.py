import math
import statistics
import sys

results={}
threads=[1, 2, 4, 8, 16, 32]
basedir="logs/"

def white_listed(input):
    for white in white_list:
        if input == white:
            return True
    return False

def read_files(n_threads):
    stats = {}
    for rep in range(1,6):
        file = open(basedir + "perf-{}-{}.log".format(n_threads, rep))
        lines = file.readlines()

        for line in lines:
            # print (stats)
            # print(line)
            kv = list(map(lambda x: x.strip(), line.split(":")))
            # print("kv0: {}, kv1: {}".format(kv[0], kv[1]))
            if kv[0] in stats.keys():
                stats[kv[0]].append(int(kv[1]) / 1_000_000)
            else:
                stats[kv[0]] = [ int(kv[1]) / 1_000_000 ]
    
    results[n_threads] = stats


def write_result(out_file):
    file = open(out_file, 'w')

    for n_threads, stats in results.items():
        file.write("Experiment for number of threads: {}\n".format(n_threads))
        for key, value in stats.items():
            file.write("{}: avg = {}, std_dev = {}, std_err = {}\n".format(key, round(statistics.mean(value), 2), round(statistics.stdev(value), 2), round((statistics.stdev(value) / math.sqrt(5)), 2)))


def main(output_file):
    for i in threads:
        read_files(i)
    write_result(output_file)
    # print(results)

if __name__ == "__main__":
    main(sys.argv[1])