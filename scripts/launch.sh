#!/bin/bash
VIEWER=/usr/bin/thunar
MOUNTER=isomounter
MOUNTPOINT=$($MOUNTER $@)
$VIEWER $MOUNTPOINT &


