#!/bin/bash
if [ ! -f "$1" ] ; then
	echo error: must give bootfile as first argument to this script
	echo usage:  $0 '<bootfile>'
	exit 1
fi

if [ -z "$FOOBAR_QEMU" ] ; then
	echo error: must source a build environment that sets FOOBAR_QEMU
	exit 1
fi

if [ -z "$FOOBAR_NCAT" ] ; then
	echo error: must source a build environment that sets FOOBAR_NCAT
	exit 1
fi

if [ -z "$FOOBAR_GDB" ] ; then
	echo error: must source a build environment that sets FOOBAR_GDB
	exit 1
fi

if [ -n "$EXECUTOR_NUMBER" ] ; then
	# run by Jenkins: use executor number for offset
	PORT_OFFSET=$(($EXECUTOR_NUMBER * 10))
	echo "INFO: run by jenkins, using offset $PORT_OFFSET for TCP ports"
fi

if [ -z "$PORT_OFFSET" ] ; then
	echo "WARNING: PORT_OFFSET not set!"
	PORT_OFFSET=$(( (($RANDOM / 10) % 100) * 10 ))
	echo "WARNING: using random offset $PORT_OFFSET"
fi

GDBPORT=$(( 5000 + $PORT_OFFSET ))
SERIALPORT=$(( 5001 + $PORT_OFFSET ))

echo "using port $GDBPORT for debugger, port $SERIALPORT for serial"

set -x

$FOOBAR_QEMU  -machine tricore_testboard -cpu aurix \
              -nographic -no-reboot -net none -kernel "$1" \
              -S -gdb tcp::$GDBPORT,ipv4 -serial mon:telnet:127.0.0.1:$SERIALPORT,server,nowait & QEMU_PID=$!

$FOOBAR_NCAT -t4 localhost $SERIALPORT > serial_out.txt 2>&1 & NC_PID=$!

# ncat must run a short time to be able to connect, or we'll lose the first bytes of serial output
sleep 1

GDBSCRIPT=autorun.gdbscript.$$
trap "rm -f $GDBSCRIPT" EXIT
sed -e s/REMOTETARGETPORT/$GDBPORT/ autorun.gdbscript > $GDBSCRIPT

$FOOBAR_GDB -batch -n -x $GDBSCRIPT 2>&1 | tee gdb_out.txt

#echo killing qemu...
kill $QEMU_PID
#echo killing ncat...
kill $NC_PID
# wait for childs
wait

echo ============ serial output
cat serial_out.txt
