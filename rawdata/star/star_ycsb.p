set size 1.5,1.5
set terminal postscript eps color solid "Helvetica" 48
set pointsize 3.0
set output 'star_ycsb.eps'

# legend info
#set key autotitle columnhead
set key right top Left reverse
set key font ",36"
#set key off

# axis info
#set rmargin 5
#set xtics nomirror rotate by -45 scale 0
set xtics 10
set ytics 4e5
set format y '%.1s%c'

# line style

set style line 4   lt 4 lw 7 pt 4 
set style line 3   lt 3 lw 7 pt 3
set style line 2   lt 2 lw 7 pt 2
set style line 1   lt 1 lw 7 pt 1 

#set ytics auto nomirror
#set y2tics auto nomirror
#set logscale x
#set autoscale y
set yrange[0:*]

set xlabel "% cross warehouse transactions"
set ylabel "Throughput (txns/sec)"
plot 'star_ycsb.dat' using 1:2 ls 1 with linespoints t '48 partitions', '' using 1:3 ls 3 with linespoints t '96 partitions (new)',\
 '' using 1:4 ls 2 with linespoints t '192 partitions (new)', '' using 1:5 ls 4 with linespoints t '240 partitions (new)'

