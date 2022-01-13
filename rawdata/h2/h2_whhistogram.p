set size 1.5,1.5
set terminal postscript eps color solid "Helvetica" 48
set output 'h2_whhistogram.eps'


set key left top Left reverse
set key font ",36"

set format y '%.s%c'
set ytics 1e3
# set grid ytics ls 3
set ylabel "Throughput (txn/s)"

set boxwidth 0.9 relative
set style data histograms
set style histogram clustered gap 2

set offsets -0.5,-0.5,0,0

set auto x
set yrange [0:6e3]
#plot for [COL=2:4:1] 'h2_whhistogram.dat' using COL:xticlabels(1)
plot 'h2_whhistogram.dat' using 2:xtic(1) title 'Interactive Transaction' fillstyle pattern 4 lw 7, \
        '' using 3:xtic(1) title 'Stored Procedure' fillstyle solid 1.0 lw 7