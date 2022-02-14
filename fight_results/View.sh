#!/bin/bash

for var in "$@"
do
	echo $var
	./tools/ViewResults $var
	echo "==============================================================="
done

echo "Overview: "
awk 'BEGIN { printf("%-30s %-7s %-5s (%-7s %-7s %-7s %-7s)\n","Name","WR","Games","BW","WW","DRAW","UNKNOWN"); }'
for var in "$@"
do
	./tools/ViewResults $var | awk -v name=$var -F ":| " '{
        if($1=="Total") { games = $3; }
        else if($1=="W") { wr = $2; unknown = $NF; }
        else if($1=="BW") { bw = $2; }
        else if($1=="WW") { ww = $2; }
        else if($1=="DRAW") { draw = $2; }

        if($1=="DRAW") {
            printf("%-30s %-7s %5d (%-7s %-7s %-7s %-7s)\n",name,wr,games,bw,ww,draw,unknown);
        }
    }'
done | sort -V
