#!/bin/bash

# from 192.168.1.1 to 192.168.1.254

for i in {1..254}; do
    
    # 现在，我们将无用的信息重定向进/dev/null当中
    
    ping -c 2 -i 0.5 192.168.1.$i &> /dev/null
    if [ $? -eq 0 ]; then
	echo "192.168.1.$i is up"
    else
	echo "192.168.1.$i is down"
    fi
done
