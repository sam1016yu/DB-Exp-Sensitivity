set size 1.5,1.5
set terminal postscript eps color solid "Helvetica" 48
set pointsize 3.0
set output 'silo_tpcc_local_5vs2.eps'

# legend info
#set key autotitle columnhead
set key left top Left reverse
set key font ",36"
#set key off

# axis info
#set rmargin 5
#set xtics nomirror rotate by -45 scale 0
set xtics 4
set ytics 4e5
set format y '%.1s%c'
#set grid ytics ls 3

# line style
set style line 6   lt 6 lw 7 pt 6 
set style line 5   lt 5 lw 7 pt 5 
set style line 4   lt 4 lw 7 pt 4 
set style line 3   lt 3 lw 7 pt 3 
set style line 2   lt 2 lw 7 pt 2 
set style line 1   lt 1 lw 7 pt 1 


#set ytics auto nomirror
#set y2tics auto nomirror
#set logscale x
#set autoscale y
set xlabel "Number of worker threads"
set ylabel "Throughput (txn/sec)"
set xrange[1:32]
plot 'silo_tpcc_local.dat' using 1:3 ls 1 with linespoints t '5 types', '' using 1:4 ls 2 with linespoints t '2 types (new)'
 


