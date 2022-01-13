set size 1.5,1.5
set terminal postscript eps color solid "Helvetica" 48
set output 'h2_histogram.eps'


#set key left top Left reverse
set key font ",32"

set format y '%.s%c'
set ytics 1e5
# set grid ytics ls 3
set ylabel "Throughput (txn/s)"

set boxwidth 0.9 relative
set style data histograms
set style histogram clustered gap 2

set offset -0.6,-0.6,0,0

set auto x
set yrange [0:4e5]
#plot for [COL=2:4:1] 'h2_histogram.dat' using COL:xticlabels(1)
plot 'h2_histogram.dat' using 2:xtic(1) title 'Select on primary key' fillstyle empty lw 7, \
        '' using 3:xtic(1) title 'Stored Procedure with 1 select statement' fillstyle solid 1.0 lw 7, \
        '' using 4:xtic(1) title 'Stored Procedure with 5 select statements' fillstyle pattern 4 lw 7