#!/bin/sh

ACPATH="${HOME}/bin/cflags"
ACINCLUDE="${ACPATH}/acinclude.m4"
AUTOGEN="${ACPATH}/autogen.sh"

diff_copy()
{
	if test -f "$1" ; then
		if test -f "$2" ; then
			diff -q $1 $2
			if test "$?" -ne "0" ; then
				echo "Replacing $2 with $1"
				cp $1 $2
			fi
		else
			echo "Installing $1 to $2"
			cp $1 $2
		fi
	fi
}

diff_copy ${ACINCLUDE} acinclude.m4
diff_copy ${AUTOGEN} autogen.sh
autoreconf -fiv
exit 0
