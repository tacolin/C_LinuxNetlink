#!/usr/bin/env bash

function DoMake()
{
    make -C $1
}

function DoClean()
{
    make clean -C $1
}

function ShowUsage()
{
    >&2 echo ""
    >&2 echo "Usage: $0 "
    >&2 echo -e "\t-u: make user_part"
    >&2 echo -e "\t-k: make kernel_part"
    >&2 echo -e "\t-c: clean user_part & kernel_part"
    >&2 echo -e "\t-t: do test"
    >&2 echo -e "\t-a: make all and do test"
    >&2 echo ""
    exit 1
}

function DoTest()
{
    cd kernel_part/
    sudo insmod taco.ko

    cd ../user_part/
    ./main &

    pid=`ps -fu $USER | grep "main" | grep -v "grep" | awk '{print $2}'`
    kill -INT "${pid}"

    sudo rmmod taco

    tail /var/log/kern.log
}

getopts "ukhcta" option
case "${option}" in
    u)
        DoMake user_part
        ;;

    k)
        DoMake kernel_part
        ;;

    c)
        DoClean user_part
        DoClean kernel_part
        ;;

    t)
        DoTest
        ;;

    a)
        DoClean user_part
        DoMake user_part
        DoClean kernel_part
        DoMake kernel_part
        DoTest
        ;;

    *|?)
        ShowUsage $0
        ;;
esac
