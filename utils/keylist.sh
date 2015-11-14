#!/bin/sh
#   keylist - list hotkey->actions found in .res files in a html table, per HID
#   Copyright (C) 2015 Tibor 'Igor2' Palinkas
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
#   http://repo.hu/projects/pcb-rnd

AWK=awk

if test -z "$*"
then
	echo ""
	echo "$0: Generate a html table from pcb menu res files."
	echo "Usage: $0 file1 [file2 [file3 ... [fileN]]]"
	echo ""
	exit
else
	res_files="$@"
fi

# split the file into one token per line using a state machine
# tokens are:
#  - quoted strings ("")
#  - control characters: {,  }, =
#  - other words
tokenize()
{
	$AWK '
		BEGIN {
			q = "\""
		}

# echo a character and remember if it was a newline
		function echo(c) {
			had_nl = (c == "\n")
			printf "%s", c
		}

# echo a newline if the previous character written was not a newline
		function nl() {
			if (!had_nl)
				echo("\n")
		}

# parse state machine: eats input character by character
		function parse(c) {
#			ignore comments
			if (in_comment)
				return

#			quote state machine
			if (in_quote) {
				if (bslash) {
					echo(c)
					bslash = 0
					return
				}
				if (c == "\\")
					bslash = 1
				if (c == q) {
					nl();
					in_quote = 0
					return
				}
				echo(c)
				return
			}

			if (c == "#") {
				in_comment = 1
				return
			}

#			quote start
			if (c == q) {
				in_quote = 1
				echo(c)
				return
			}

#			whitespace are non-redundant newlines
			if (c ~ "[ \t]") {
				nl()
				return
			}

#			"keywords"
			if (c ~ "[{}=]") {
				nl()
				echo(c)
				nl()
				return
			}

#			anything else
			echo(c)
		}

# each new line of the input is a set of characters
# reset comment first, as it spanned to the previous newline
		{
			in_comment = 0
			l = length($0)
			for(n = 1; n <= l; n++)
				parse(substr($0, n, 1))
		}
	'
}

# "grammar": read tokens and output "key src action" where
#   key is in base-modified-modifier format, modifiers ordered
#   src is the source res file we are working from, passed as $1
#   action is the action given right after the key or before the key
extract_from_res()
{
	tokenize | $AWK -v "src=$1" '
		BEGIN {
			sub(".*/", "", src)
		}
		/^=$/ {
			if (last != "a") {
				last = ""
				getline t
				next
			}
			last = ""

			getline t
			if (t != "{")
				next
			getline k1
			getline k2
			getline t
			if (t != "}")
				next
			getline action
			if (action == "}")
				action = last_act
			sub("^\"", "", k2)
			sub("\"$", "", k2)
			gsub(" \t", "", k2)
			split(tolower(k2), K, "<key>")
			if (K[1] != "") {
				mods = ""
				if (K[1] ~ "alt")   mods = mods "-alt"
				if (K[1] ~ "ctrl")  mods = mods "-ctrl"
				if (K[1] ~ "shift") mods = mods "-shift"
			}
			else
				mods = ""
			key = K[2] mods
			gsub("[ \t]", "", key)
			gsub("[ \t]", "", src)
			gsub("[ \t]", "", action)
			print key, src, action
			last_act = ""
			next
		}
		/[a-zA-Z]/ {
			if (last != "")
				last_act = last
		}
		{ last = $0 }
	'
}

# convert a "key src action" to a html table with rowspans for base keys
gen_html()
{
	$AWK '
	BEGIN {
		HIDNAMES["gpcb-menu.res"] = "gtk"
		HIDNAMES["pcb-menu.res"]  = "lesstif"
		CLR[0] = "#FFFFFF"
		CLR[1] = "#DDFFFF"
		key_combos = 0
	}
	function to_base_key(combo)
	{
		sub("-.*", "", combo)
		return combo
	}

	function to_mods(combo)
	{
		if (!(combo ~ ".-"))
			return ""
		sub("^[^-]*[-]", "", combo)
		return combo
	}

	{
		if (last != $1) {
			LIST[key_combos++] = $1
			ROWSPAN[to_base_key($1)]++
		}
		ACTION[$2, $1] = $3
		HIDS[$2]++
		last = $1
	}

	END {
		print "<html><body>"
		print "<h1> Key to action bindings </h1>"
		print "<table border=1 cellspacing=0>"
		printf("<tr><th> key <th> modifiers")
		colspan = 2
		for(h in HIDS) {
			printf(" <th>%s<br>%s", h, HIDNAMES[h])
			colspan++
		}
		print ""
		for(n = 0; n < key_combos; n++) {
			key = LIST[n]
			base = to_base_key(key)
			mods = to_mods(key)
			if (base != last_base)
				clr_cnt++
			print "<tr bgcolor=" CLR[clr_cnt % 2] ">"
			if (base != last_base)
				print "	<th rowspan=" ROWSPAN[base] ">" base
			if (mods == "")
				mods = "&nbsp;"
			print "	<td>" mods
			for(h in HIDS) {
				act = ACTION[h, key]
				if (act == "")
					act = "&nbsp;"
				print "	<td>", act
			}
			last_base = base
		}
		print "</table>"
		print "</body></html>"
	}
	'
}

for n in $res_files 
do
	extract_from_res "$n" < $n
done | sort | gen_html

