#!/bin/bash

while read line; do
	# and if input is not empty
	if [ -n "$line" ]
	then
		# input from arduino can be from 0 to 64
		# but "pacmd" can recive input from 0 to 65535
		# so, we need to multiply value by 512: 64 * 512 = 65535
		x=512
		# parse value
		value=$(echo $(( x * line )))
		# log it
		echo "$value"
		# and set new value to system
		pacmd set-sink-volume alsa_output.pci-0000_00_1f.3.analog-stereo $value
	fi
# redirect input to loop
done < /dev/ttyUSB0