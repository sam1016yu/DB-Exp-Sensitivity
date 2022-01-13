set size 1.5,1.5
set terminal postscript eps color solid "Helvetica" 48
set pointsize 3.0
set output 'tpcc_1w1th_5vs2.eps'

# legend info
#set key autotitle columnhead
set key left top Left reverse
set key font ",36"
#set key off

# axis info
#set rmargin 5
#set xtics nomirror rotate by -45 scale 0
set xtics 4
set ytics 8e5
set format y '%.1s%c'
#set grid ytics ls 3

# line style
set style line 10   lt 5 lw 7 pt 5
set style line 9   lt 4 lw 7 pt 4
set style line 8   lt 3 lw 7 pt 8
set style line 7   lt 7 lw 7 pt 7
set style line 6   lt 1 lw 7 pt 6
set style line 5   lt 5 lw 7 pt 5
set style line 4   lt 4 lw 7 pt 9
set style line 3   lt 3 lw 7 pt 3
set style line 2   lt 2 lw 7 pt 2
set style line 1   lt 1 lw 7 pt 1

#set ytics auto nomirror
#set y2tics auto nomirror
#set logscale x
#set autoscale y
# set datafile missing NaN
set xlabel "Number of worker threads"
set ylabel "Throughput (txn/sec)"
set xrange[1:29]

plot 'tpcc_full_1w1th.dat' using 1:4 ls 1 with linespoints t 'Cicada-5',\
 'tpcc_NP_1w1th.dat' using 1:4 ls 6 with linespoints t 'Cicada-2',\
 'tpcc_full_1w1th.dat' using 1:6 ls 3 with linespoints t 'TicToc-5',\
 'tpcc_NP_1w1th.dat' using 1:6 ls 8 with linespoints t 'TicToc-2',\
 'tpcc_full_1w1th.dat' using 1:7 ls 4 with linespoints t 'Hekaton-5',\
 'tpcc_NP_1w1th.dat' using 1:7 ls 9 with linespoints t 'Hekaton-2'


#plot 'tpcc_full_1w1th.dat' using 1:4 ls 1 with linespoints t 'Cicada-full', '' using 1:5 ls 2 with linespoints t "Silo-full'",\
# '' using 1:6 ls 3 with linespoints t 'TicToc-full','' using 1:7 ls 4 with linespoints t 'Hekaton-full','' using 1:8 ls 5 with linespoints t '2PL-full',\
# 'tpcc_NP_1w1th.dat' using 1:4 ls 6 with linespoints t 'Cicada-NP', '' using 1:5 ls 7 with linespoints t "Silo-NP'",\
# '' using 1:6 ls 8 with linespoints t 'TicToc-NP','' using 1:7 ls 9 with linespoints t 'Hekaton-NP','' using 1:8 ls 10 with linespoints t '2PL-NP'



