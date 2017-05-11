#!/bin/bash

set -e
CFLAGS='-m64 -std=gnu++14 -Wall -Wextra -O2 -flto'
LFLAGS='-lboost_program_options'
rm -rf obj/ bin/ || true
mkdir obj/ bin/
for f in src/*.cpp; do
	name=`basename $f`
	out=obj/${name%.cpp}.o
	(set -vx; g++ $CFLAGS -c $f -o $out)
done
objs=`ls -1 obj/* | grep -v 'aucont_'`
for f in src/aucont_*.cpp; do
	name=`basename $f`
	obj=obj/${name%.cpp}.o
	out=bin/${name%.cpp}
	(set -vx; g++ $CFLAGS $obj $objs -o $out $LFLAGS)
done
for f in src/*.sh; do
	name=`basename $f`
	out=bin/${name%.sh}
	cp $f $out
done
sudo chown root:root bin/*
sudo chmod 6755 bin/*
if (( `ls -1 /aucont/cgroup | wc -l` == 0 )) ; then
	sudo mount --bind /sys/fs/cgroup/cpu /aucont/cgroup
fi
sudo sysctl net.ipv4.conf.all.forwarding=1
