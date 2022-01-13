set size 1.5,1.5
set terminal postscript eps color solid "Helvetica-Bold" 32
set pointsize 2.0
set output 'aria_tpcc_dist_0.eps'

# legend info
#set key autotitle columnhead
set key right bottom Left
#set key off

# axis info
#set rmargin 5
#set xtics nomirror rotate by -45 scale 0
set xtics auto
set ytics 1e5
set format y '%.s%c'
set grid ytics ls 6

# line style
set style line 6   lt 5 lw 5 pt 5 
set style line 5   lt 4 lw 5 pt 4 
set style line 4   lt 3 lw 5 pt 3 
set style line 3   lt 2 lw 5 pt 2 
set style line 2   lt 1 lw 5 dt 2 pt 1
set style line 1   lt 1 lw 5 pt 1 

#set ytics auto nomirror
#set y2tics auto nomirror
#set logscale x
#set autoscale y
set xlabel "# of warehouses"
set ylabel "Throughput (txns/sec)"
set xrange[0:180]

# using AriaFB-2 and Calvin-4

plot 'dist_0.dat' using 1:3 ls 1 with linespoints t 'Aria', '' using 1:5 ls 2 with linespoints t 'AriaFB',\
 '' using 1:8 ls 3 with linespoints t 'Bohm','' using 1:11 ls 4 with linespoints t 'Calvin','' using 1:13 ls 5 with linespoints t 'Pwv'



