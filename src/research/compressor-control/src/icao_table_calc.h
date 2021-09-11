#ifndef ICAO_TABLE_CALC_H_
#define ICAO_TABLE_CALC_H_

#include "icao_table_data.h"

icao_table_altitude_t icao_table_alt_for_pressure(icao_table_pressure_t value);

icao_table_pressure_t icao_table_pressure_for_alt(icao_table_altitude_t value);


#endif /* ICAO_TABLE_CALC_H_ */
