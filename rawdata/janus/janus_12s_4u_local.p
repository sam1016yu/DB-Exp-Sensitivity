set size 1.5,1.5
set terminal postscript eps color solid "Helvetica-Bold" 32
set pointsize 3.0
set output 'janus_12s_4u_local.eps'

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
set grid ytics ls 6
set grid xtics ls 6

# line style
set style line 6   lt 6 lw 5 pt 6 
set style line 5   lt 5 lw 5 pt 5 
set style line 4   lt 4 lw 5 pt 4 
set style line 3   lt 3 lw 5 pt 3 
set style line 2   lt 2 lw 5 pt 2 
set style line 1   lt 1 lw 5 pt 1 

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
plot '12shard_4cpu_local.dat' using 1:2 ls 4 with linespoints t '2PL', '' using 1:3 ls 3 with linespoints t 'Tapir',\
 '' using 1:4 ls 2 with linespoints t 'OCC','' using 1:5 ls 1 with linespoints t 'Janus'

