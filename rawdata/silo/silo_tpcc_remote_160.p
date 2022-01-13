set size 1.5,1.5
set terminal postscript eps color solid "Helvetica-Bold" 32
set pointsize 3.0
set output 'silo_tpcc_remote_160.eps'

# legend info
#set key autotitle columnhead
set key right top Left
#set key off

# axis info
#set rmargin 5
#set xtics nomirror rotate by -45 scale 0
set xtics auto
# set ytics 1e5
set format y '%.1s%c'
set grid ytics ls 3

# line style
set style line 6 lc rgb 'red'  lt 6 lw 5 pt 6 
set style line 5 lc rgb 'black'  lt 5 lw 5 pt 5 
set style line 4 lc rgb 'red'  lt 4 lw 5 pt 4 
set style line 3 lc rgb 'black'  lt 3 lw 5 pt 3 
set style line 2 lc rgb 'red'  lt 2 lw 5 pt 2 
set style line 1 lc rgb 'black'  lt 1 lw 5 pt 1 


#set ytics auto nomirror
#set y2tics auto nomirror
#set logscale x
#set autoscale y
set xlabel "% cross partition transactions"
set ylabel "Throughput ( txns/sec )"
set xrange[0:100]
plot 'silo_tpcc_remote_160.dat' using ($3*10):4 ls 1 with linespoints t 'PartitionedStore', '' using ($3*10):5 ls 2 with linespoints t 'MemSilo'
 


