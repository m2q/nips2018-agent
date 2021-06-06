#! /bin/sh
mkdir -p Release
cd Release
cmake -DCMAKE_BUILD_TYPE=Release -DTEST_SHOW_GAME_PROGRESS=OFF ..

make pomcpp_test
(./pomcpp_test)
