set datafile separator comma

set xzeroaxis
set xlabel "Время от старта (минуты)" font ",18"
set xtics 10 nomirror font ",18"
set xrange[-10:110]
set mxtics

set ylabel "Скорость счёта (импульсы/с)" font ",18"
set ytics 50 nomirror font ",18"
set yrange [0:200]
set mytics

set y2zeroaxis
set y2label "Барометрическая высота (м)" font ",18" offset 2,0,0
set y2range [-1000:30000]
set y2tics 2000 nomirror font ",18"
set my2tics

set key default
set key width -10 box noopaque enhanced samplen 8 right top font ",18"

set arrow from 59.0, graph 0 to 59.0, graph 1 nohead back dt (5, 5, 10, 10) lc rgb 'red' lw 3

set border 1+2+8
set grid xtics y2tics

set style line 1 lt rgb "orange" lw 1
set style line 2 lt rgb "blue" lw 1
set style line 3 lt rgb "gray" lw -1 pt 0 ps 1


set terminal png size 1920,800
set output 'radiation.png'

plot \
	'radiation+PLD_DOSIM_DATA-13-0.csv' using 'time_fs_mins':'tics_per_sec' \
		with lines ls 1 \
		axis x1y1 \
		title "Скорость счёта" \
	,\
	\
	\
	\
	'radiation+PLD_MS5611_DATA-13-1.csv' using 'time_fs_mins':'altitude' \
		with lines ls 2\
		axis x1y2 \
		title "Высота полёта" \
	,\






set output 'radiation-acceleration.png'

set y2label "Перегрузка (g)" font ",18" offset 2,0,0
set y2range [0:3]
set y2tics 1 nomirror font ",18"
set my2tics

plot \
	'radiation+SINS_ISC-11-0.csv' using 'time_fs_mins':'accel_g' \
		every ::10 \
		with lines ls 3 \
		axis x1y2 \
		title "Перегрузка" \
	,\
	\
	'radiation+PLD_DOSIM_DATA-13-0.csv' using 'time_fs_mins':'tics_per_sec' \
		with lines ls 1 \
		axis x1y1 \
		title "Скорость счёта" \
	\
	\








set output 'radiation-nice.png'

set xzeroaxis
set xlabel "Время от старта (минуты)" font ",18"
set xtics 10 nomirror font ",18"
set xrange[-10:75]
set mxtics

set ylabel "Скорость счёта (импульсы/с)" font ",18"
set ytics 5 nomirror font ",18"
set yrange [0:25]
set mytics

unset y2zeroaxis
unset y2label
unset y2range
unset y2tics
unset my2tics

plot \
	'radiation+ascent+PLD_DOSIM_DATA-13-0.csv' using 'time_fs_mins':'tics_per_sec' \
		with lines ls 1 \
		axis x1y1 \
		title "Скорость счёта" \
	,\
	'radiation+ascent+PLD_DOSIM_DATA-13-0.csv' using 'time_fs_mins':'smooth_tics_per_sec' \
		with lines lt rgb "red" lw 3 \
		axis x1y1 \
		title "Скорость счёта (скользящее среднее)"




set output 'radiation-altitude.png'

set xzeroaxis
set xlabel "Барометрическая высота (метры)" font ",18"
set xtics 2000 nomirror font ",18"
set xrange[-10:30000]
set mxtics

set ylabel "Скорость счёта (импульсы/с)" font ",18"
set ytics 5 nomirror font ",18"
set yrange [0:25]
set mytics

unset y2zeroaxis
unset y2label
unset y2range
unset y2tics
unset my2tics

unset arrow

set y2label
set y2tics
set my2tics

set grid xtics ytics

plot \
	'radiation+ascent+PLD_MS5611_DATA-13-1.csv' using 'altitude':'tics_per_sec' \
		with points lt rgb "orange" pt 7 ps 1 \
		axis x1y1 \
		title "Скорость счёта" \
	,\
	\
	'radiation+ascent+PLD_MS5611_DATA-13-1.csv' using 'altitude':'smooth_tics_per_sec' \
		with lines lt rgb "red" lw 3 \
		axis x1y1 \
		title "Скорость счёта (скользящее среднее)"