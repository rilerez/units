CXX="clang++ -std=c++2a"
lineno=0
RED='\033[0;31m'
NC='\033[0m' # No Color

$CXX $1 -D BAD=""

if [ $? != "0" ]
then
    >&2 echo -e ${RED} $1 is already broken
    exit 1
fi

allpass=true
while read -r line
do
    ((lineno++))
    $CXX $1 -D BAD="$line" #2> /dev/null
    if [ $? != "0" ]
    then
        echo -e ${NC} "${lineno}: ${line} breaks compilation"
    else
        allpass=false;
        >&2 echo -e ${RED} "${lineno}: ${line} does not break compilation"
    fi
done < $1.bad

if ($allpass)
then
    echo "ALL STATIC TESTS PASS"
    ./a.out
else
    echo "DOES NOT PASS"
fi
