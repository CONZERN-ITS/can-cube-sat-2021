import os
import re
import itertools


def collect_csvs(path: str):
    retval = []
    for root, dirs, files in os.walk(path):
        for dirname in dirs:
            if "sdr" in dirname: # Не ходим в папки с SDR, оно только мешает
                dirs.remove(dirname)

        for f in files:
            if f.endswith(".csv"):
                retval.append('/'.join([root, f]))

    return retval


def get_table_name(csv_file_name: str):
    _, csv_file_name = os.path.split(csv_file_name)

    match = re.match(r"(.+)(-\d+-\d+)\.csv", csv_file_name)
    if not match:
        retval, _ = os.path.splitext(csv_file_name)
    else:
        base_name = match.group(1)
        retval = base_name

    retval = retval.lower()
    retval = retval.replace(".", "_")
    retval = retval.replace("-", "_")
    return retval


def import_table(db_path, file_path, table_name):
    callstring = 'sqlite3 "%s" -csv ".import %s %s"' % (db_path, file_path, table_name)
    #print(callstring)
    os.system(callstring)


board_csv_files = [("brd_" + get_table_name(x), x,) for x in collect_csvs("SD/parsed_merged_fixed/")]
ground_csv_files =[("gnd_" + get_table_name(x), x,) for x in collect_csvs("RADIO/")]

db_file = "its-telemetry.sqlite"
try:    
    os.remove(db_file)
except FileNotFoundError:
    pass

for record in itertools.chain(board_csv_files, ground_csv_files):
    table_name, file_path = record
    print("loading file %s to table %s" % (file_path, table_name))
    import_table(db_file, file_path, table_name)
