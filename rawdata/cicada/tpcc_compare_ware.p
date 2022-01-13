set size 1.5,1.5
set terminal postscript eps color solid "Helvetica" 48
set pointsize 3.0
set output 'tpcc_compare_ware.eps'

# legend info
#set key autotitle columnhead
set key left top Left reverse
set key font ",32"
set key at 0.5, 2.47e6

# axis info
#set rmargin 5
#set xtics nomirror rotate by -45 scale 0
set xtics auto
set ytics 5e5
set yrange [0:2500000]
set format y '%.1s%c'
#set grid ytics ls 3

# line style
set style line 6  lc rgb 'blue'  lw 7 pt 6
set style line 5  lc rgb 'black'  lw 7 pt 5 
set style line 4  lc rgb 'red'  lw 7 pt 4
set style line 3  lc rgb 'blue' lw 7 pt 3
set style line 2  lc rgb 'black'  lw 7 pt 2 
set style line 1  lc rgb 'red'  lw 7 pt 1 

#set ytics auto nomirror
#set y2tics auto nomirror
#set logscale x
#set autoscale y
set datafile missing NaN
set xlabel "Number of worker threads"
set ylabel "Throughput (txn/sec)"
set xrange[1:30]
plot  'tpcc_4w1th.dat' using 1:4 ls 1 with linespoints t 'Cicada(#WH=4*#worker) (new)',\
  'tpcc_fix1ware.dat' using 1:4 ls 4 with linespoints t 'Cicada(#WH=1)',\
'tpcc_4w1th.dat' using 1:9 ls 2 with linespoints t 'Silo(#WH=4*#worker) (new)',\
'tpcc_fix1ware.dat' using 1:9 ls 5 with linespoints t 'Silo(#WH=1)',\
 'tpcc_4w1th.dat' using 1:12 ls 3 with linespoints t 'MOCC(#WH=4*#worker) (new)',\
 'tpcc_fix1ware.dat' using 1:12 ls 6 with linespoints t 'MOCC(#WH=1)'


