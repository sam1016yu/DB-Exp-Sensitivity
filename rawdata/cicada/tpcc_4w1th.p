set size 1.5,1.5
set terminal postscript eps color solid "Helvetica" 40
set pointsize 3.0
set output 'tpcc_4w1th.eps'

# legend info
#set key autotitle columnhead
set key left top Left reverse
set key font ",32"
#set key off

# axis info
#set rmargin 5
#set xtics nomirror rotate by -45 scale 0
set xtics auto
set ytics 5e5
set format y '%.1s%c'
set grid ytics ls 3

# line style
set style line 9   lt 9 lw 7 pt 9
set style line 8   lt 8 lw 7 pt 7 
set style line 7   lt 7 lw 7 pt 4 
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
set datafile missing NaN
set xlabel "Thread count"
set ylabel "Throughput (txn/sec)"
set xrange[1:30]
plot 'tpcc_4w1th.dat' using 1:4 ls 1 with linespoints t 'Cicada', '' using 1:5 ls 2 with linespoints t "Silo'",\
 '' using 1:6 ls 3 with linespoints t 'TicToc','' using 1:7 ls 4 with linespoints t 'Hekaton','' using 1:8 ls 5 with linespoints t '2PL',\
 '' using 1:9 ls 6 with linespoints t 'Silo', '' using 1:10 ls 7 with linespoints t 'ERMIA', '' using 1:11 ls 8 with linespoints t 'Silo',\
 '' using 1:12 ls 9 with linespoints t 'MOCC'


