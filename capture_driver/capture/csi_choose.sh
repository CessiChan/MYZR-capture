#!/bin/bash
echo " argv1 = $1"
if test "$1" -eq "1080"
then
	echo "**************CSI is 1080**************"
	insmod /capture/csi_1080_1080.ko
elif test "$1" -eq "1920"
then
	echo "**************CSI is 1920**************"
	insmod /capture/csi_1920_1920.ko
elif test "$1" -eq "3000"
then
	echo "**************CSI is 3000**************"
	insmod /capture/csi_3000_3000.ko
elif test "$1" -eq "3360"
then
	echo "**************CSI is 3360**************"
	insmod /capture/csi_3360_3360.ko
elif test "$1" -eq "3840"
then
	echo "**************CSI is 3840**************"
	insmod /capture/csi_3840_3840.ko
fi


