set size 1.5,1.5
set terminal postscript eps color solid "Helvetica" 48
set pointsize 3.0
set output 'aria_tpcc_dist_15.eps'

# legend info
#set key autotitle columnhead
set key right bottom Left reverse
set key font ",36"
#set key off

# axis info
#set rmargin 5
#set xtics nomirror rotate by -45 scale 0
set xtics auto
set ytics 1e5
set format y '%.s%c'
#set grid ytics ls 3

# line style
set style line 6   lt 5 lw 7 pt 5 
set style line 5   lt 4 lw 7 pt 4 
set style line 4   lt 3 lw 7 pt 3 
set style line 3   lt 2 lw 7 pt 2 
set style line 2   lt 1 lw 7 dt 2 pt 1
set style line 1   lt 1 lw 7 pt 1 

#set ytics auto nomirror
#set y2tics auto nomirror
#set logscale x
#set autoscale y
set xlabel "Number of warehouses"
set ylabel "Throughput (txn/sec)"
set xrange[0:180]

# using AriaFB-2 and Calvin-4

plot 'dist_15.dat' using 1:3 ls 1 with linespoints t 'Aria',\
 '' using 1:8 ls 3 with linespoints t 'Bohm','' using 1:11 ls 4 with linespoints t 'Calvin','' using 1:13 ls 5 with linespoints t 'Pwv'


#plot 'dist_15.dat' using 1:3 ls 1 with linespoints t 'Aria', '' using 1:5 ls 2 with linespoints t 'AriaFB',\
# '' using 1:8 ls 3 with linespoints t 'Bohm','' using 1:11 ls 4 with linespoints t 'Calvin','' using 1:13 ls 5 with linespoints t 'Pwv'



