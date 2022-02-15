set datafile separator comma

set xzeroaxis
set xlabel "Время от старта (минуты)" font ",18"
set xtics 10 nomirror font ",18"
set xrange[-10:110]
set mxtics

#set logscale y 10
#set yzeroaxis
set ylabel "Скорость счёта (импульсы/с)" font ",18"
set ytics 50 nomirror font ",18"
set yrange [0:200]
set mytics

set y2zeroaxis
set y2label "Вертикальная скорость (м)" font ",18" offset 2,0,0
set y2tics 10 nomirror font ",18"
set y2range [-60:20]
set my2tics 2

set arrow from 59.0, graph 0 to 59.0, graph 1 nohead back dt (5, 5, 10, 10) lc rgb 'red' lw 3

set key default
set key width -20 box noopaque enhanced samplen 8 right top font ",18"

set border 1+2+8
set grid xtics y2tics my2tics

set style line 1 lt rgb "orange" lw 1
set style line 2 lt rgb "blue" lw 2

set terminal png size 1920,800
set output 'collect-hvel.png'

plot \
	'+PLD_DOSIM_DATA-13-0.csv' using 'time_fs_mins':'tics_per_sec' \
		with lines ls 1 \
		axis x1y1 \
		title "Скорость счёта" \
	,\
	'+PLD_MS5611_DATA-13-1.csv' using 'time_fs_mins':'msmooth_h_vel' \
		with lines ls 2 \
		axis x1y2 \
		title "Вертикальная скорость"

#pause mouse close
#==========================================

set y2label "Температура (°C)" font ",18" offset 2,0,0
set y2range [-80:30]
set y2tics 10 nomirror font ",18"
set my2tics 2

set key default
set key width 0
set key below
set key box noopaque enhanced samplen 8 font ",12"

set style line 2 lw 2

set output 'collect-temp.png'

plot \
	'+PLD_DOSIM_DATA-13-0.csv' using 'time_fs_mins':'tics_per_sec' \
		with lines ls 1 \
		axis x1y1 \
		notitle \
	,\
	\
	\
	\
	'+THERMAL_STATE-10-0.csv' using 'time_fs_mins':'temperature' \
		with lines \
		axis x1y2 \
		title "Температура БСК-1" \
	,\
	'+THERMAL_STATE-10-1.csv' using 'time_fs_mins':'temperature' \
		with lines \
		axis x1y2 \
		title "Температура БСК-2" \
	,\
	'+THERMAL_STATE-10-2.csv' using 'time_fs_mins':'temperature' \
		with lines \
		axis x1y2 \
		title "Температура БСК-3" \
	,\
	'+THERMAL_STATE-10-3.csv' using 'time_fs_mins':'temperature' \
		with lines \
		axis x1y2 \
		title "Температура БСК-4" \
	,\
	'+THERMAL_STATE-10-4.csv' using 'time_fs_mins':'temperature' \
		with lines \
		axis x1y2 \
		title "Температура БСК-5" \
	,\
	'+THERMAL_STATE-10-5.csv' using 'time_fs_mins':'temperature' \
		with lines \
		axis x1y2 \
		title "Температура БСК-6" \
	,\
	\
	\
	\
	'+THERMAL_STATE-12-0.csv' using 'time_fs_mins':'temperature' \
		with lines \
		axis x1y2 \
		title "Температура АКБ-1" \
	,\
	'+THERMAL_STATE-12-1.csv' using 'time_fs_mins':'temperature' \
		with lines \
		axis x1y2 \
		title "Температура АКБ-2" \
	,\
	'+THERMAL_STATE-12-2.csv' using 'time_fs_mins':'temperature' \
		with lines \
		axis x1y2 \
		title "Температура АКБ-3" \
	,\
	'+THERMAL_STATE-12-3.csv' using 'time_fs_mins':'temperature' \
		with lines \
		axis x1y2 \
		title "Температура АКБ-4" \
	,\
	\
	\
	\
	'+OWN_TEMP-11-0.csv' using 'time_fs_mins':'deg' \
		with lines \
		axis x1y2 \
		title "Температура БИНС" \
	,\
	'+OWN_TEMP-12-0.csv' using 'time_fs_mins':'deg' \
		with lines \
		axis x1y2 \
		title "Температура АРК" \
	,\
	'+OWN_TEMP-13-0.csv' using 'time_fs_mins':'deg' \
		with lines \
		axis x1y2 \
		title "Температура PLD" \
	,\
	\
	\
	\
	'+PLD_MS5611_DATA-13-1.csv' using 'time_fs_mins':'temperature' \
		with lines \
		axis x1y2 \
		title "Температура MS5611 внешего" \
	,\
	'+PLD_MS5611_DATA-13-2.csv' using 'time_fs_mins':'temperature' \
		with lines \
		axis x1y2 \
		title "Температура MS5611 внутреннего" \
#	,\
#	\
#	\
#	\
#	'+PLD_BME280_DATA-13-1.csv' using 'time_fs_mins':'temperature' \
#		with lines \
#		axis x1y2 \
#		title "Температура BME280 внутреннего" \
#	,\
#	'+PLD_BME280_DATA-13-2.csv' using 'time_fs_mins':'temperature' \
#		with lines \
#		axis x1y2 \
#		title "Температура BME280 внешнего" \


#pause mouse close
#==========================================

set y2label "Барометрическая высота (м)" font ",18" offset 2,0,0
set y2range [-1000:30000]
set y2tics 2000 nomirror font ",18"
set my2tics 

unset grid
set grid xtics y2tics

set key default
set key width -10 box noopaque enhanced samplen 8 right top font ",18"

set style line 2 lc rgb "blue" lw 2

set output 'collect-alt.png'

plot \
	'+PLD_DOSIM_DATA-13-0.csv' using 'time_fs_mins':'tics_per_sec' \
		with lines ls 1 \
		axis x1y1 \
		title "Скорость счёта" \
	,\
	\
	\
	\
	'+PLD_MS5611_DATA-13-1.csv' using 'time_fs_mins':'altitude' \
		with lines ls 2\
		axis x1y2 \
		title "Высота полёта" \
	,\
