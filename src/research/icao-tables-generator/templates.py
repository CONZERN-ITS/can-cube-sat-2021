

RECORD_BYTE_SIZE = 4 + 4
""" Размер одной записи в байтах """

PRESSURE_VALUE_FORMAT = "{:0.9f}"  # 9 знаков должно хватить для точности флота
""" Формат числа давления """

ALTITUDE_VALUE_FORMMAT = "{:0.9f}"  # 9 знаков должно хватить для точности флота
""" Формат значения высоты """

HEADER_FILE_TEMPLATE = """
#ifndef ICAO_ATM_TABLE_H_
#define ICAO_ATM_TABLE_H_

#include <stdint.h>

#define ICAO_ATM_TABLE_RECORDS_COUNT {records_count}

// Тип данных для хранения значений давления
typedef float icao_table_pressure_t;

// Тип данных для хранения значений высоты
typedef float icao_table_altitude_t;

extern const icao_table_pressure_t icao_table_pressure_data[ICAO_ATM_TABLE_RECORDS_COUNT];
extern const icao_table_altitude_t icao_table_altitude_data[ICAO_ATM_TABLE_RECORDS_COUNT];

#endif // ICAO_ATM_TABLE_H_

"""


SOURCE_FILE_TEMPLATE = """
#include "icao_table_data.h"

const icao_table_pressure_t icao_table_pressure_data[ICAO_ATM_TABLE_RECORDS_COUNT] = {{
{pressure_records}
}};

const icao_table_altitude_t icao_table_altitude_data[ICAO_ATM_TABLE_RECORDS_COUNT] = {{
{altitude_records}
}};
"""