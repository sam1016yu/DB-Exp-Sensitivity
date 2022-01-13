set size 1.5,1.5
set terminal postscript eps color solid "Helvetica" 48
set pointsize 3.0
set output 'gam_c4.eps'

# legend info
#set key autotitle columnhead
set key right top Left reverse
set key font ",36"
#set key off

# axis info
#set rmargin 5
#set xtics nomirror rotate by -45 scale 0
set xtics 10
set ytics 40000
 set format y '%.1s%c'
#set grid ytics ls 3

# line style

set style line 4   lt 4 lw 7 pt 4 
set style line 3   lt 3 lw 7 pt 3
set style line 2   lt 2 lw 7 pt 2
set style line 1   lt 1 lw 7 pt 1 

#set ytics auto nomirror
#set y2tics auto nomirror
#set logscale x
#set autoscale y
set xlabel "% cross warehouse transactions"
set ylabel "Throughput (txn/sec)"
plot 'gam_c4.dat' using 1:2 ls 1 with linespoints t '32 warehouses (10%)', '' using 1:3 ls 3 with linespoints t '32 warehouses (new)',\
 '' using 1:4 ls 2 with linespoints t '64 warehouses (new)', '' using 1:5 ls 4 with linespoints t '128 warehouses (new)'

