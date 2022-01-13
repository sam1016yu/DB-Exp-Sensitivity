set size 1.5,1.5
set terminal postscript eps color solid "Helvetica" 48
set pointsize 3.0
set output 'herd_uniform.eps'

# legend info
#set key autotitle columnhead
set key left bottom Left reverse
set key font ",36"
#set key off

# axis info
set xtics (128e3,1e6,2e6,4e6,8e6)
set ytics 4e6
set format x '%.s%c'
set format y '%.s%c'
set yrange [0:28e6]
#set grid ytics ls 3

# line style

set style line 4   lt 4 lw 7 pt 4 
set style line 3   lt 3 lw 7 pt 3
set style line 2   lt 2 lw 7 pt 2
set style line 1   lt 1 lw 7 pt 1 

#set ytics auto nomirror
#set y2tics auto nomirror
set logscale x 
#set autoscale y
set xlabel "Number of KVs"
set ylabel "Throughput (txn/sec)"
plot 'herd_uniform.dat' using 1:2 ls 1 with linespoints t '32B (95:5) (new)', '' using 1:3 ls 3 with linespoints t '32B (50:50) (new)',\
 '' using 1:4 ls 2 with linespoints t '128B (95:5) (new)', '' using 1:5 ls 4 with linespoints t '128B (50:50) (new)'

