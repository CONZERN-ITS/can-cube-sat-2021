set datafile separator comma

set xzeroaxis
set xlabel "Время от старта (минуты)" #font ",18"
set xtics 10 nomirror #font ",18"
#set xrange[-10:110]
set mxtics

#set logscale y 10
#set yzeroaxis
set ylabel "Угол между векторами" #font ",18"
#set ytics 50 nomirror #font ",18"
#set yrange [0:200]
set mytics

set arrow from 59.0, graph 0 to 59.0, graph 1 nohead back dt (5, 5, 10, 10) lc rgb 'red' lw 3

set key default
set key box noopaque enhanced samplen 2 font ",7"
set key top right
set key width -40

set border 1+2+8
set grid xtics y2tics my2tics

set style line 1 lt rgb "red" lw 1
set style line 2 lt rgb "green" lw 1
set style line 3 lt rgb "blue" lw 1

#set terminal png size 1920,800
#set output 'collect-hvel.png'

# set terminal wxt 0 enhanced
# set terminal wxt font "Arial,12"

plot \
		'+SINS_ISC-11-0.csv' using 'time_fs_mins':'theta' \
		every 50 \
		with lines ls 1 \
		axis x1y1 \
		title "Угол theta вектора солнца в ССК" \
	,\
		'+SINS_ISC-11-0.csv' using 'time_fs_mins':'angle_acc_sun' \
		every 50 \
		with lines ls 2 \
		axis x1y1 \
		title "Угол между ускорением и вектором Солнца в ССК" \
	,\
		'+SINS_ISC-11-0.csv' using 'time_fs_mins':'angle_acc_verical' \
		every 50 \
		with lines ls 3 \
		axis x1y1 \
		title "Угол между вектором ускорения и местной вертикалью в ССК" \
	\
