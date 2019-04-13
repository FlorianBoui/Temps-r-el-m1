#!/bin/bash
DATE='/bin/date'

let Moyenne=0
let ELAPSED=0
for i in `seq 1 16`;
do
	for w in `seq 1 256`;
	do
		BEFORE=$($DATE +'%s')
        ./partie1 $i $w
        AFTER=$($DATE +'%s')
        ELAPSED=$(($AFTER - $BEFORE))
		echo $i" "$ELAPSED >> resultats.dat
		Moyenne=$(($Moyenne + $ELAPSED))
	done
	Moyenne=$(($Moyenne / 256))
	echo $i" "$Moyenne >> resultatsmoyenne.dat
	Moyenne=0
done
echo "set terminal png size 800,500 enhanced background rgb 'white'" > gnuplot_script
echo "set output 'curve.png'" >> gnuplot_script
echo "plot [0:17] [0:200] 'resultats.dat' with linespoint , 'resultatsmoyenne.dat' with linespoint" >> gnuplot_script
gnuplot gnuplot_script 
rm gnuplot_script 


