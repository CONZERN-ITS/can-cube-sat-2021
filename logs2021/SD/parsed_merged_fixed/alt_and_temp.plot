set datafile separator comma

set border 1+2+8
set lmargin 14
set bmargin 8

set xzeroaxis
set xlabel "Время от старта (минуты)" font ",18"
set xtics 10 nomirror font ",18"
set xrange[-10:110]
set mxtics

set y2label "Температура (°C)" font ",18" offset 2,0,0
set y2range [-80:30]
set y2tics 10 nomirror font ",18"
set my2tics 2

set ylabel "Барометрическая высота (м)" font ",18" offset -2,0,0
set yrange [-1000:30000]
set ytics 2000 nomirror font ",18"
set mytics


set grid xtics y2tics my2tics

set style line 1 lt rgb "orange" lw 1
set style line 2 lt rgb "blue" lw 2

set arrow from 59.0, graph 0 to 59.0, graph 1 nohead back dt (5, 5, 10, 10) lc rgb 'red' lw 3


set key default
set key width 0
set key below
set key box noopaque enhanced samplen 8 font ",12"


set terminal png size 1920,800
set output 'alt-and-temp.png'


plot \
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
	'+PLD_MS5611_DATA-13-1.csv' using 'time_fs_mins':'altitude' \
		with lines ls 2\
		axis x1y1 \
		title "Высота полёта" \
	,\
#	\
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


# unset grid
# set grid xtics y2tics

# set key default
# set key width -10 box noopaque enhanced samplen 8 right top font ",18"
