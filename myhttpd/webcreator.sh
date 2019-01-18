#!/bin/bash

if [ ! -d $1 ]; then
    echo directory not exists
    exit 1
fi

if [ ! -e $2 ]; then
    echo file not exists
    exit 1
fi

re='^[0-9]+$'
if ! [[ $3 =~ $re ]] ; then
   echo $3 "Not a number" >&2; exit 1
fi

re='^[0-9]+$'
if ! [[ $4 =~ $re ]] ; then
   echo $4 "Not a number" >&2; exit 1
fi

lines=$(wc -l < $2)
echo lines = $lines

(("lines" < "10000")) && echo file $2 has less than 10000 lines && exit 1

#remove root directory's elements, if any
if test "$(ls -A "$1")"; then
    echo Warning: directory is full, purging ...
    rm -r $1/* 
fi

#k=$((2 + RANDOM % (lines-1998)))
#m=$(((RANDOM % 1000) + 1000))

k=$(shuf -i 2-$lines-2001 -n 1)
m=$(shuf -i 1001-1999 -n 1)

let f=$4/2+1
let q=$3/2+1

fullEntries=() #create an empty array of all entries
for ((dir=0; dir<$3; dir++)) do
    entries=($(shuf -i 0-999 -n $4)) #produce names of different pages in the website dir
    fullEntries=( ${fullEntries[@]} ${entries[@]} ) #names of pages in other websites
    let dirArray[$dir]=$dir
done

#echo fullEntries = ${fullEntries[@]}
#echo ${#fullEntries[*]}
arrayIndex=0
#start creating websites
for ((dir=0; dir<$3; dir++)) do
    echo Creating web site $dir ...

    mkdir -p $1/site$dir

    firstPart="${fullEntries[@]:0:$(($arrayIndex))}"
    secondPart="${fullEntries[@]:$(($arrayIndex+$4))}"
    externalArray=( ${firstPart[@]} ${secondPart[@]} )

    firstDirPart="${dirArray[@]:0:$((${arrayIndex} / ${4}))}"
    secondDirPart="${dirArray[@]:$(($arrayIndex / $4 + 1))}"
    externalDirArray=( ${firstDirPart[@]} ${secondDirPart[@]} )

    #internalArray="${fullEntries[@]:0:$((${arrayIndex} / ${4}))}"

    #echo externalDirArray = ${externalDirArray[@]}

    #echo externalArray = ${externalArray[@]}

    for ((file=0; file<$4; file++)) do
        page=/site$dir/page${dir}_${fullEntries[$dir * $4 + $file]}.html
        printf "\tCreating page $page with $(($m / ($f+$q))) lines starting at line $k\n"

        #write html headers
        printf "<!DOCTYPE html>\n<html>\n\t<body>\n\t" >> $1/$page

        #current page must not be contained in the links
        tempArray=("${fullEntries[@]}")

        #swap current page to be always the first element of the temp array
        temp=${tempArray[$arrayIndex]}
        tempArray[$arrayIndex]=${tempArray[$dir * $4 + $file]}
        tempArray[$dir * $4 + $file]=$temp
        #produce N times from the rest array

        
        #echo tempArray = ${tempArray[@]}
        i=0
        for index in $(shuf --input-range=$(($arrayIndex+1))-$(( $arrayIndex + $4 - 1 )) -n ${f})
        do
            fSet[i]=/site${dir}/page${dir}_${tempArray[$index]}.html
            let i++
        done
        i=0
        for index in $(shuf --input-range=0-$(( ${#externalArray[*]} - 1 )) -n ${q})
        do
            external_link=/site${externalDirArray[$index / $4]}/page${externalDirArray[$index / $4]}_${externalArray[$index]}.html
            qSet[$i]=$external_link
            let i++
        done

        wholeSet=( ${fSet[@]} ${qSet[@]} )
        #echo wholeSet = ${wholeSet[@]}
        #echo fSet = ${fSet[@]}

        line_num=k
        for i in "${wholeSet[@]}"
        do
            cat $2 | tail -$(($lines-$line_num)) | head -$(($m / ($f+$q))) >> $1/$page #write text
            printf "\tAdding link to $i\n"
            printf "\n<a href=\"$i\">Link to $i!</a>\n" >> $1/$page #write link
            let line_num=$line_num+$((m/(f + q)))
        done
        
    done
    let arrayIndex=arrayIndex+$4
done
echo Done.

#write the final html headers
for ((dir=0; dir<$3; dir++)) do
    for ((file=0; file<$4; file++)) do
        printf "\n\t</body>\n</html>" >> $1/site$dir/page${dir}_${fullEntries[$dir * $4 + $file]}.html
    done
done
