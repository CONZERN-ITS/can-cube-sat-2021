#include "icao_table_calc.h"


#include <stdbool.h>
#include <assert.h>
#include <stddef.h>
#include <float.h>
#include <math.h>


static float _flt_upper_bound(float value, const float * array, size_t array_size)
{
	assert(array_size > 0);

	size_t left_bound = 0;
	size_t count = array_size;

	while(count > 0)
	{
		// Поставим каретку на середину диапазона
		const size_t step = count / 2;
		const size_t carret = left_bound + step;
		// И посмотрим перескочили мы значение или нет
		float carret_value = array[carret];
		if (carret_value <= value)
		{
			// Это значит, что нашего значения нет до каретки
			// прыгаем левой границей на следующее значение
			// за кареткой справа
			left_bound = carret + 1;
			// прыгаем на середину диапазона
			count -= step + 1;
		}
		else
		{
			// Это значит что искомое значение до каретки
			// обрезаем массив по каретке (не включая её саму)
			count = step;
		}
	}

	return left_bound;
}


static float _flt_lower_bound(float value, const float * array, size_t array_size)
{
	assert(array_size > 0);

	size_t left_bound = 0;
	size_t count = array_size;

	while(count > 0)
	{
		// Поставим каретку на середину диапазона
		const size_t step = count / 2;
		const size_t carret = left_bound + step;
		// И посмотрим перескочили мы значение или нет
		float carret_value = array[carret];
		if (carret_value >= value)
		{
			// Это значит, что нашего значения нет до каретки
			// прыгаем левой границей на следующее значение
			// за кареткой справа
			left_bound = carret + 1;
			// прыгаем на середину диапазона
			count -= step + 1;
		}
		else
		{
			// Это значит что искомое значение до каретки
			// обрезаем массив по каретке (не включая её саму)
			count = step;
		}
	}

	return left_bound;
}


static float _flt_slerp(
		float lower_arg, float upper_arg,
		float lower_value, float upper_value,
		float arg
)
{
	const float relative_arg = (arg - lower_arg)/(upper_arg - lower_arg);
	return lower_value + (upper_value - lower_value) * relative_arg;
}


icao_table_altitude_t icao_table_alt_for_pressure(icao_table_pressure_t pressure)
{
	// Находим строку таблицы, которая показывает давление выше чем есть у нас
	const size_t upper_bound = _flt_upper_bound(pressure, icao_table_pressure_data, ICAO_ATM_TABLE_RECORDS_COUNT);

	// Если мы находимся ниже чем наша таблица может показать
	if (0 == upper_bound)
	{
		// штош, уходим в зашкал - показываем самое низкое что можем
		return icao_table_altitude_data[0];
	}

	// Если мы получили указатель на элемент за последним элементом массива
	// значит наша таблица не покрывает такое значение
	if (ICAO_ATM_TABLE_RECORDS_COUNT == upper_bound)
	{
		// Штош! тоже уходим в зашкал, делать нечего
		return icao_table_altitude_data[ICAO_ATM_TABLE_RECORDS_COUNT-1];
	}

	// Теперь находим ближайшую строку на которой давление ниже чем у нас
	const size_t lower_bound = upper_bound - 1;

	// Интерполируем!
	// Аргумент - давление, функция - высота
	return _flt_slerp(
			icao_table_pressure_data[lower_bound], icao_table_pressure_data[upper_bound],
			icao_table_altitude_data[lower_bound], icao_table_altitude_data[upper_bound],
			pressure
	);
}


icao_table_pressure_t icao_table_pressure_for_alt(icao_table_altitude_t altitude)
{
	// Находим строку таблицы, которая показывает высоту выше чем у нас
	const size_t lower_bound = _flt_lower_bound(altitude, icao_table_altitude_data, ICAO_ATM_TABLE_RECORDS_COUNT);

	// Если мы находимся ниже чем наша таблица может показать
	if (0 == lower_bound)
	{
		// штош, уходим в зашкал - показываем самое низкое что можем
		return icao_table_pressure_data[0];
	}

	// Если мы получили указатель на элемент за последним элементом массива
	// значит наша таблица не покрывает такое значение
	if (ICAO_ATM_TABLE_RECORDS_COUNT == lower_bound)
	{
		// Штош! тоже уходим в зашкал, делать нечего
		return icao_table_pressure_data[ICAO_ATM_TABLE_RECORDS_COUNT-1];
	}

	// Теперь находим ближайшую строку на которой высота выше чем у нас
	const size_t upper_bound = lower_bound - 1;

	// Интерполируем!
	// Аргумент - высота, функция - давление
	return _flt_slerp(
			icao_table_altitude_data[lower_bound], icao_table_altitude_data[upper_bound],
			icao_table_pressure_data[lower_bound], icao_table_pressure_data[upper_bound],
			altitude
	);
}
