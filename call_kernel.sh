#!/bin/sh

check_cmd()
{
    if [ $? -eq 0 ]; then
        #echo "cmd to [${*}] success"
        :
    else
        echo "cmd [${*}] failed"
        exit -1
    fi
}

#Check param
if [ "$#" -ne 1 ]; then
    echo "Illegal number of parameters"
    exit -1
fi

#Check if module mod_process_string exist
result=$(lsmod | grep 'mod_process_string' | cut -d ' ' -f1)
check_cmd "lsmod"

echo $result

#insmod mod_process_string
#if [ -z "$result" ]; then
comp_str="mod_process_string"
if [ -z "${result##*$comp_str*}" ]; then
    insmod mod_process_string.ko
    check_cmd "insmod"
else
    rmmod mod_process_string.ko
    check_cmd "rmmod"
    insmod mod_process_string.ko
    check_cmd "insmod"
fi

#call appl C++
./App $1

#rmmod module
rmmod mod_process_string.ko
check_cmd "rmmod"