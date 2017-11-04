#!/bin/sh
guidir=`dirname $0`/../gui

if [ "$1" = "" ]; then
    icons=$(grep -r fromTheme ${guidir}/* | perl -ne "print \$2.\"\n\" if /fromTheme\(('|\")(.*)\\1\)/" | sort | uniq)
else
    icons=$@
fi

for icon in ${icons}; do
    echo "Adding ${icon}"
    for size in 16 24 32; do
        iconfile=${guidir}/icons/hicolor/${size}x${size}/actions/${icon}.png
        if [ ! -f ${iconfile} ]; then
            cp /usr/share/icons/Tango/${size}x${size}/actions/${icon}.png ${iconfile} && \
            git add ${iconfile}
        fi
    done
    iconfile=${guidir}/icons/hicolor/scalable/actions/${icon}.svg
    if [ ! -f ${iconfile} ]; then
        cp /usr/share/icons/Tango/scalable/actions/${icon}.svg ${iconfile} && \
        git add ${iconfile}
    fi
done
