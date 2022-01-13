set size 1.5,1.5
set terminal postscript eps color solid "Helvetica-Bold" 32
set pointsize 1.0
set output 'janus_6s2u_vs_12s4u_local.eps'

# legend info
#set key autotitle columnhead
set key left bottom Left
#set key off

# axis info
#set rmargin 5
#set xtics nomirror rotate by -45 scale 0
set xtics auto
set ytics auto
# set ytics 2e6
# set format y '%.s%c'
set grid ytics ls 9
set grid xtics ls 9

# line style
set style line 9   lt 6 lw 5 pt 6 
set style line 8   lt 4 lw 5 pt 4 dt 2
set style line 7   lt 3 lw 5 pt 3 dt 2
set style line 6   lt 2 lw 5 pt 2 dt 2
set style line 5   lt 1 lw 5 pt 1 dt 2
set style line 4   lt 4 lw 5 pt 4 dt 1
set style line 3   lt 3 lw 5 pt 3 dt 1
set style line 2   lt 2 lw 5 pt 2 dt 1
set style line 1   lt 1 lw 5 pt 1 dt 1

#set ytics auto nomirror
#set y2tics auto nomirror
#set logscale x
#set autoscale y
set xlabel "Clients"
set ylabel "Throughput ( txns/s )"
set logscale x 10
set logscale y 10
set xrange[1:5000]
set yrange[1:3e4]

# set xrange[1:3500]
# set yrange[1:2e4]

plot '6shard_2cpu_local.dat' using 1:2 ls 4 with linespoints t '2PL-6s', '' using 1:3 ls 3 with linespoints t 'Tapir-6s',\
 '' using 1:4 ls 2 with linespoints t 'OCC-6s','' using 1:5 ls 1 with linespoints t 'Janus-6s',\
 '12shard_4cpu_local.dat' using 1:2 ls 8 with linespoints t '2PL-12s', '' using 1:3 ls 7 with linespoints t 'Tapir-12s',\
 '' using 1:4 ls 6 with linespoints t 'OCC-12s','' using 1:5 ls 5 with linespoints t 'Janus-12s'

