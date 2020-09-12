archivo=$1
comm=`head -1 $archivo | awk -F"," '{print $4}'`

if [ -z $archivo ]
then
echo "Se necesita el nombre del archivo";
exit 1;
fi

if [ -z $comm ]
then
echo "Se necesita el numero del primer espacio de memoria.";
echo "Se asignaran de 10 en 10"
exit 1;
fi

cat $archivo | while read line 
do 
    id=`echo $line | awk -F"," '{print $1}'`
    tipo=`echo $line | awk -F"," '{print $2}'`
    th=`echo $line | awk -F"," '{print $3}'`
    velocidad=`echo $line | awk -F"," '{print $6}'`

    rand=`shuf -i 1-$th -n 1`
    max=$(($th+$rand)) #Le puse esto porque se me ocurrio

    min=`shuf -i 10-$th -n 1`
    permitidos=$(($max-$min+1))

    #------------------------------------------se levantara el proceso------------------------------------------------------
    verificar=`ps -edaf | grep "sensorx $id" | grep -v grep | wc -l` #verifico si ya hay un proceso levantado con ese id

    if [ $verificar -eq 0 ]
    then
        ./sensorx $id $tipo $comm $velocidad $min $max 1 &
    fi

    comm=$(($comm+10))
    echo ""
done