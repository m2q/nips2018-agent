#! /bin/sh
mkdir -p Release
cd Release
cmake -DCMAKE_BUILD_TYPE=Release ..

make pomcpp_test
if [ "$#" -eq 0 ]; then
	(./pomcpp_test "[performance]")
else
	if [ "$1" = "-t" ]; then
		(./pomcpp_test "[performance]" --threads $2)
	else
		echo "Didn't recognize argument. Use -t x for concurrent testing."
	fi
fi
