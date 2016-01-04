#!/bin/bash
VIEWER=/usr/bin/thunar
SCRIPT_DIR=$(dirname $0)
MOUNTER=$SCRIPT_DIR/isomounter
MOUNTPOINT=$($MOUNTER $@)
$VIEWER $MOUNTPOINT &


