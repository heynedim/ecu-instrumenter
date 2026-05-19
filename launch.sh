#!/bin/sh

echo -ne "\n\n"
echo --------------------------------------------------------------------
echo ":: ECU INSTRUMENTER LAUNCH"
echo --------------------------------------------------------------------

AppDir=$(pwd)
AppExecutable="bin/ecu-instrumenter"
PerformanceMode=1

echo --------------------------------------------------------------------
echo ":: APPLYING ADDITIONAL CONFIGURATION"
echo --------------------------------------------------------------------

if [ "$PerformanceMode" = "1" ]; then 
    echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
fi

cd "$AppDir"
HOME="$AppDir"

echo --------------------------------------------------------------------
echo ":: RUNNING THE NATIVE APP"
echo --------------------------------------------------------------------

chmod +x "$AppExecutable" 2>/dev/null || true
"$AppExecutable" "$@"

echo --------------------------------------------------------------------
echo ":: POST RUNNING TASKS"
echo --------------------------------------------------------------------

unset LD_PRELOAD
echo -ne "\n\n" 
