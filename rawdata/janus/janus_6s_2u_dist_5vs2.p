set size 1.5,1.5
set terminal postscript eps color solid "Helvetica"48 
set pointsize 3.0
set output 'janus_6s_2u_dist_5vs2.eps'

# legend info
#set key autotitle columnhead
set key left bottom Left
set key font ",36"
#set key off

# axis info
#set rmargin 5
#set xtics nomirror rotate by -45 scale 0
set xtics auto
set ytics auto
# set ytics 2e6
# set format y '%.s%c'
#set grid ytics ls 9
#set grid xtics ls 9

# line style
set style line 9   lt 6 lw 7 pt 9 
set style line 8   lt 4 lw 7 pt 8 
set style line 7   lt 3 lw 7 pt 7 
set style line 6   lt 2 lw 7 pt 6
set style line 5   lt 1 lw 7 pt 5
set style line 4   lt 4 lw 7 pt 4
set style line 3   lt 3 lw 7 pt 3
set style line 2   lt 2 lw 7 pt 2
set style line 1   lt 1 lw 7 pt 1

#set ytics auto nomirror
#set y2tics auto nomirror
#set logscale x
#set autoscale y
set xlabel "Number of clients"
set ylabel "Throughput ( txns/s )"
set logscale x 10
set logscale y 10
set xrange[1:5000]
set yrange[1:3e4]

# set xrange[1:3500]
# set yrange[1:2e4]

plot '6shard_2cpu_dist.dat' using 1:2 ls 4 with linespoints t '2PL-5',\
'6shard_2cpu_dist_NP.dat' using 1:2 ls 8 with linespoints t '2PL-2 (new)',\
     '6shard_2cpu_dist.dat' using 1:4 ls 2 with linespoints t 'OCC-5',\
 '6shard_2cpu_dist_NP.dat' using 1:4 ls 6 with linespoints t 'OCC-2 (new)',\
     '6shard_2cpu_dist.dat' using 1:5 ls 1 with linespoints t 'Janus-5',\
 '6shard_2cpu_dist_NP.dat' using 1:5 ls 5 with linespoints t 'Janus-2 (new)'


#plot '6shard_2cpu_dist.dat' using 1:2 ls 4 with linespoints t '2PL-All', '' using 1:3 ls 3 with linespoints t 'Tapir-All',\
# '' using 1:4 ls 2 with linespoints t 'OCC-All','' using 1:5 ls 1 with linespoints t 'Janus-All',\
'6shard_2cpu_dist_NP.dat' using 1:2 ls 8 with linespoints t '2PL-NP', '' using 1:3 ls 7 with linespoints t 'Tapir-NP',\
# '' using 1:4 ls 6 with linespoints t 'OCC-NP','' using 1:5 ls 5 with linespoints t 'Janus-NP'

