set size 1.5,1.5
set terminal postscript eps color solid "Helvetica" 48
set pointsize 3.0
set output '6b_ycsb_28thd_write_40M.eps'

# legend info
#set key autotitle columnhead
set key top Left reverse
set key maxrows 3
set key font ",32"
#set key off

# axis info
#set rmargin 5
#set xtics nomirror rotate by -45 scale 0
set xtics auto
set ytics 5e5
set format y '%.1s%c'

# line style
set style line 9   lt 9 lw 7 pt 9
set style line 8   lt 8 lw 7 pt 7 
set style line 7   lt 7 lw 7 pt 4 
set style line 6   lt 6 lw 7 pt 6 
set style line 5   lt 5 lw 7 pt 5 
set style line 4   lt 4 lw 7 pt 4 
set style line 3   lt 3 lw 7 pt 3 
set style line 2   lt 2 lw 7 pt 2 
set style line 1   lt 1 lw 7 pt 1 ps 3

#set ytics auto nomirror
#set y2tics auto nomirror
#set logscale x
#set autoscale y
set datafile missing NaN
set xlabel "Zipfian skewness"
set ylabel "Throughput (txn/sec)"
set xrange[1:7]
set yrange[0:4000000]

plot '6b_ycsb_28thd_write_40M.dat' using 1:7:xtic(2) ls 1 with linespoints t 'Cicada(new)',\
 '' using 1:9:xtic(2) ls 2 with linespoints t 'TicToc (new)',\
 '' using 1:12:xtic(2) ls 3 with linespoints t 'Silo (new)',\
 '' using 1:13:xtic(2)ls 4 with linespoints t 'ERMIA (new)',\
 '' using 1:14:xtic(2) ls 5 with linespoints t 'FOEDUS (new)',\
 '' using 1:15:xtic(2) ls 6 with linespoints t 'MOCC (new)'

#plot '6b_ycsb_28thd_write_40M.dat' using 1:7:xtic(2) ls 1 with linespoints t 'Cicada', '' using 1:8:xtic(2) ls 2 with linespoints t "Silo'",\
# '' using 1:9:xtic(2) ls 3 with linespoints t 'TicToc','' using 1:10:xtic(2) ls 4 with linespoints t 'Hekaton','' using 1:11:xtic(2) ls 5 with linespoints t '2PL',\
# '' using 1:12:xtic(2) ls 6 with linespoints t 'Silo', '' using 1:13:xtic(2)ls 7 with linespoints t 'ERMIA', '' using 1:14:xtic(2) ls 8 with linespoints t 'Silo',\
# '' using 1:15:xtic(2) ls 9 with linespoints t 'MOCC'


