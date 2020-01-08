#!/bin/bash

echo "Formato Log: <operazione> - totali: <numero> - successi: <numero> - fallimenti: <numero>" > testout.log

#1000 STORE

for i in {1..50}
do
	./client usr_$i 1 >> $i.log 2>&1 &
	pids[$i]=$!
done

for pid in ${pids[*]}; do
	wait $pid
done

for i in {1..30}
do
	./client usr_$i 2 >> $i.log 2>&1 &
	pids[$i]=$!
done


for i in {31..50}
do
	./client usr_$i 3 >> $i.log 2>&1 &
	pids[$i]=$!
done

for pid in ${pids[*]}; do
	wait $pid
done

for i in {1..50}
do
	cat $i.log >> testout.log
	rm $i.log
done


