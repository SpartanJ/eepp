#!/bin/bash
SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"

cd ${DIR}
cd ../articles

DIR=`pwd`
NAME="md_$(echo ${DIR} | sed -e 's/\//_/g')_"

echo "Replace files with \"${NAME}\""

cd ../xml

find . -type f -name "*${NAME}*" | while read FILE ; do
	echo "Renaming ${FILE}"
	newfile="$(echo ${FILE} | sed -e "s/${NAME}//g")";
	echo "New file ${newfile}";
    mv "${FILE}" "${newfile}";
done

find . -name '*.xml' -exec sed -i -e "s/${NAME}//g" {} \;
