
#ifndef ICAO_ATM_TABLE_H_
#define ICAO_ATM_TABLE_H_

#include <stdint.h>

#define ICAO_ATM_TABLE_RECORDS_COUNT 5100

// Тип данных для хранения значений давления
typedef float icao_table_pressure_t;

// Тип данных для хранения значений высоты
typedef float icao_table_altitude_t;

extern const icao_table_pressure_t icao_table_pressure_data[ICAO_ATM_TABLE_RECORDS_COUNT];
extern const icao_table_altitude_t icao_table_altitude_data[ICAO_ATM_TABLE_RECORDS_COUNT];

#endif // ICAO_ATM_TABLE_H_

