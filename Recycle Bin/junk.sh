
#Edward Yaroslavsky
#I pledge my honor that I have abided by the Stevens Honor System.

#!/bin/bash

size_flag=0
flag=""
readonly junkdir="/home/cs392/.junk"
readonly junkfile="/home/cs392/junk/junk.sh"

print_help() {
cat << ENDOFTEXT
Usage: $(basename "$junkfile") [-hlp] [list of files]    
	-h: Display help.    
	-l: List junked files.   
	-p: Purge all files.    
	[list of files] with no other arguments to junk those files.
ENDOFTEXT
}

process_file() {
	if [ -e "/home/cs392/junk/$1" ]; then
		mv "$1" "$junkdir"
	else
		echo "Warning: '$1' not found"
	fi
}

while getopts ":hlp" option; do
	case "$option" in
		h) (( ++size_flag ))
		flag="h";;
		l) (( ++size_flag ))
		flag="l";;
		p) (( ++size_flag ))
		flag="p";;
		?) echo "Error: Unknown option '-%s'.\n." $OPTARG >&2
		print_help
		exit 1;;
	esac
done

if [ $# -eq 0 ]; then
	print_help
	exit 0
fi

if [ $size_flag -gt 1 ]; then
	echo "Error: Too many options enabled."
	print_help
	exit 1
fi

if [ $size_flag -ge 1 ] && [ $# -gt $size_flag ]; then
	echo "Error: Too many options enabled."
	print_help
	exit 1
fi

if [ ! -d "$junkdir" ]; then
	mkdir -p "$junkdir"
fi


if [ $# -ge 1 ] && [ $OPTIND -lt 2 ]; then
	for f in $@; do
		process_file $f
	done

elif [ "$flag" = "h" ]; then
	print_help
	exit 0

elif [ "$flag" = "l" ]; then
	ls "$junkdir" -lAF
	exit 0

elif [ "$flag" = "p" ]; then
	rm -rf "$junkdir"
	exit 0
fi



