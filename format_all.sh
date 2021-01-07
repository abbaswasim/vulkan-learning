#!/bin/bash

for file in src/*/*.{h,cpp,hpp,hh}
do
	echo "Formatting file: $file ..."
	clang-format -style=file -i $file
	dos2unix $file
done
