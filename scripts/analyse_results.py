import sys

stats = {}
white_list = ["key_lock_wait_time", "key_lock_wait_count", "db_mutex_lock_nanos"]

def white_listed(input):
    for white in white_list:
        if input == white:
            return True
    return False

def read_file(input_file):
    file = open(input_file)
    lines = file.readlines()
    combined_string = ""

    for line in lines:
        combined_string = combined_string + line.replace('\n', '')

    line_lst = combined_string.split(',')
    for line in line_lst:
        values = list(map(lambda x: x.strip(), str(line).split('=')))
        stats[values[0]] = values[1]

def write_result(out_file):
    file = open(out_file, 'w')

    for key, value in stats.items():
        if int(value) > 0 or white_listed(key):
            file.write(str(key) + ": " + str(value) + "\n")

def main(input_file, output_file):
    read_file(input_file)
    write_result(output_file)

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])