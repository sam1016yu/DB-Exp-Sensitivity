set size 1.5,1.5
set terminal postscript eps color solid "Helvetica" 48
set output 'zipfGraph.eps'


set key right top Left reverse
set key font ",36"

set logscale x
set xrange [10000:*]
set yrange [0:*]

set ylabel "% of total requests"
set format y "%.f%%"
set format x '%.s%c'

set xlabel "Number of KVs"

set style line 2 lt 2 lw 7 dashtype 4
set style line 1 lt 1 lw 7

plot "zipfGraph.dat" using 1:2 ls 1 smooth bezier title 'Hottest 1 key' with lines,\
    "zipfGraph.dat" using 1:3 ls 2 smooth bezier title "Hottest 32 keys" with lines