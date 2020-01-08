#!/bin/bash

filename=testout.log

if [ ! -f $filename ]; then
    echo "$filename is not regular or it does not exist"
    exit 1
fi

if [ ! -s $filename ]; then
    echo "$filename is empty"
    exit 1
fi

KOString=()
KOCount=()

ConnectTot=0
ConnectSuc=0
ConnectFail=0

DisconnectTot=0
DisonnectSuc=0
DisonnectFail=0

StoreTot=0
StoreSuc=0
StoreFail=0

RetrieveTot=0
RetrieveSuc=0
RetrieveFail=0

DeleteTot=0
DeleteSuc=0
DeleteFail=0

exec 3< $filename   				#apro il file in lettura
while IFS=" " read -u 3 line ; do
    read -r -a elem <<< "$line"    # separo gli elementi sulla linea e li memorizzo in un array
    case ${elem[0]} in
    CONNECT) 
    	ConnectTot=$[ConnectTot+${elem[3]}]
    	ConnectSuc=$[ConnectSuc+${elem[6]}]
    	ConnectFail=$[ConnectFail+${elem[9]}] 
    	;;
    DISCONNECT) 
    	DisconnectTot=$[DisconnectTot+${elem[3]}]
    	DisconnectSuc=$[DisconnectSuc+${elem[6]}]
    	DisconnectFail=$[DisconnectFail+${elem[9]}]
    	;;
    STORE) 
    	StoreTot=$[StoreTot+${elem[3]}]
    	StoreSuc=$[StoreSuc+${elem[6]}]
    	StoreFail=$[StoreFail+${elem[9]}] 
    	;;	
    RETRIEVE) 
    	RetrieveTot=$[RetrieveTot+${elem[3]}]
    	RetrieveSuc=$[RetrieveSuc+${elem[6]}]
    	RetrieveFail=$[RetrieveFail+${elem[9]}] 
    	;;
     DELETE) 
    	DeleteTot=$[DeleteTot+${elem[3]}]
    	DeleteSuc=$[DeleteSuc+${elem[6]}]
    	DeleteFail=$[DeleteFail+${elem[9]}] 
    	;;
    KO)
    	j=0
    	found=0	#1 se l'elemento è stato trovato
    	
    	while [ $j -le ${#KOString[@]} ] && [ $found -le 0 ]; do
    		if (( $j == ${#KOString[@]} )); then #KO non ancora trovato
    			KOString[$j]=$line
    			KOCount[$j]=1
    			found=1
    		else
    			if [ "${KOString[$j]}" == "$line" ]; then
    				KOCount[$j]=$[${KOCount[$j]}+1]
    				found=1
  				fi
    		fi
    		j=$[j+1]
    	done
    	;;
    esac
    	
done

echo "Total Planned Connect: 100"
echo "Connect Result:"
printf "total:%d - successes:%d - failures:%d\n" $ConnectTot $ConnectSuc $ConnectFail 

echo "Total Planned Disconnect: 100"
echo "Disconnect Result:"
printf "total:%d - successes:%d - failures:%d\n" $DisconnectTot $DisconnectSuc $DisconnectFail 

echo "Total Planned Store: 1000"
echo "Store Result:"
printf "total:%d - successes:%d - failures:%d\n" $StoreTot $StoreSuc $StoreFail 

echo "Total Planned Retrieve: 600"
echo "Retrieve Result:"
printf "total:%d - successes:%d - failures:%d\n" $RetrieveTot $RetrieveSuc $RetrieveFail 

echo "Total Planned Delete: 400"
echo "Delete Result:"
printf "total:%d - successes:%d - failures:%d\n" $DeleteTot $DeleteSuc $DeleteFail 

echo ""

echo "KO detected:"
z=0
while [ $z -lt ${#KOString[@]} ]; do
	echo "${KOString[$z]} - count:${KOCount[$z]}"
	z=$[z+1]
done

if (( z == 0 )); then
	echo 0 KO detected!
fi

serverPid=$(pidof ./server)
kill -10 $serverPid

exit 0

