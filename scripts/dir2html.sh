#!/bin/bash

shopt -s nullglob

# Author: Wiley Edward Hill
# Date: 04/30/13
#
# gEDA - GPL Electronic Design Automation
# gcheck-Library - gEDA Symbol Library Batch Processor
# Copyright (C) 2013 Wiley Edward Hill <wileyhill@gmail.com>
#
#------------------------------------------------------------------
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#------------------------------------------------------------------
#
# Abstract: The script is used to create the html files within the
#           footprint subdirectory folders
#

VER=1.0

MINARGS=0          # Script requires no arguments.
ERR_FILE_NOT_FOUND=2
ERR_BAD_ARGS=65

Be_Quite=false     # default noise
Be_Verbose=false        # toggle babel on off

FILTER="*.fp"
HTML_EXT="html"
HTML_NAME="index"

#css="http://odie.mcom.fr/~clucas/titux.css"
Library=""
TopIndex=""
NextIndex=""
PreviousIndex=""

# ------- Script Colorization -------

ESCAPE="\033"
ATT_NORMAL=0     # All attributes off (can be omitted )
ATT_BOLD=1       # Bold (appears bright)
ATT_UNDERSCORE=4 # Underscore (monomchrome adapters only!)
ATT_BLINKING=5   # Blinking
ATT_REVERSE=7    # Reverse video - black text on a white background
ATT_HIDDEN=8     # Hidden.

FG_BLACK=30
FG_RED=31
FG_GREEN=32
FG_YELLOW=33
FG_BLUE=34
FG_MAGENTA=35
FG_CYAN=36
FG_WHITE=37

BG_BLACK=40
BG_RED=41
BG_GREEN=42
BG_YELLOW=43
BG_BLUE=44
BG_MAGENTA=45
BG_CYAN=46
BG_WHITE=47

BLACK="$ESCAPE[$ATT_NORMAL;47;;30m\b"
RED="$ESCAPE[$ATT_NORMAL;40;;31m\b"
GREEN="$ESCAPE[$ATT_NORMAL;40;;32m\b"
YELLOW="$ESCAPE[$ATT_NORMAL;40;;33m\b"
BLUE="$ESCAPE[$ATT_NORMAL;40;;34m\b"
MAGENTA="$ESCAPE[$ATT_NORMAL;40;;35m\b"
CYAN="$ESCAPE[$ATT_NORMAL;40;;36m\b"
WHITE="$ESCAPE[$ATT_NORMAL;40;;37m\b"

BOLD='\033[1m'
BOLD_OFF='\033[00m'
NORMAL='\b\E[00m'

alias Reset="tput sgr0"       #  Reset console text attributes to normal without clearing screen.
alias BoldText="\033[1m$1\033[0m"

# Colorization from http://tldp.org/LDP/abs/html/colorizing.html - Spectacular!
#
# Function: cecho
#
# Purpose: Colorize text output to console so as to draw attention to critical output and
#          to enhance the text menus.
#
#  Author: Wiley Edward Hill
#
#    Date: 06/23/2112
#

cecho (){                      # Color-echo.
                               # Argument $1 = message
                               # Argument $2 = color
   if [[ $1 = "-n" ]] ; then   # passing no line-feed arguments to "echo"
      opt=-ne
      shift
   else
      opt=-e
   fi

   if [[ $1 = "" ]] ; then    # if no message then just do reset
      echo -ne $NORMAL        # Reset to normal.
   else
      color=${2:-$WHITE}      # Defaults to white on black, if not specified.
      echo "$opt" "$color" "$1"
      if [ -z $3 ] ; then     # Any 3rd argument means no reset
         echo -ne $NORMAL     # Reset to normal
      fi
   fi
   return
}

vecho()
{
   if [[ ! $1 = "" ]] ; then
     if $Be_Verbose ; then
      echo $1
     fi
   fi
}

# ------------------------- Define Functions ----------------------

do_show_help() 
{
   case $1 in
         "1") # Quick help
              echo
              cecho "A script to generate HTML files for PCB footprint Libraries" $BLUE false
              echo
              echo Usage:   `basename $0` '[-options] [[-l] Library]'
              echo
              cecho ;;
 
         "2") # Basic help 
              echo
              cecho "A script to generate HTML files for PCB footprint Libraries" $GREEN
              echo
              echo Usage:   `basename $0` '[-options] [[-l] Library]'
              echo
              echo '   -s | --sdd          add package to repository'
              echo '   -e | --extension    Change file extension of the HTML files, default="html"'
              echo '   -f | --filter       Change the file filter, default="*.fp"'
              echo '   -n | --no-overwite  Do not overwrite any of the existing html files'
              echo '   -o | --output       Set the name of the HTML files, default="index.html"'
              echo '   -l | --library      Set the name of the library, default is to scan for "xxx.html"'
              echo
              echo '   -h | --help         Show this information'
              echo '   -u | --help         Show basic command-line usage'
              echo '   -v | --verbose      Display extra information'
              echo '        --version      Display version information'
              echo
              echo Example 1: `basename $0` '-v stdlib  (verbose mode and process stdlib folder)'
              echo Example 2: `basename $0` '-q         (No extra output, scan for library folder)'
              echo
              echo 'Note: arguments are case sensitive.'
              echo  ;;
         "3") # Alternative syntax
              echo Usage:   `basename $0` '"plain english statements go here!"'
              echo
              echo 'Example 1 (formal explicit):' `basename $0` 'add package icedtea to distribution wheezy)'
              echo 'Example 2 (slang implicit):' `basename $0` ' remove banshee from natty'
              echo 'Example 2 (explicit):' `basename $0` 'list all packages in sid'
              echo
              echo 'Note: For uploading the reference to package has to be translatable to a file name,'
              echo 'which can be the fully qualified file name with or without the "extension" but when'
              echo 'referencing packages in the database only the base packname can be used. English and'
              echo '"geek" can be mixed:'
              echo
              echo 'Example 3 (implicit geek):' `basename $0` 'add -p libgtk-3-0_3.0.8-0_i386.dsc to wheezy)'
              echo ;;
   esac
}

do_scream()
{
   if [[ ! $1 = "" ]] ; then
     if $Be_Verbose ; then
      echo $1
     elif ! $Be_Quite ; then
      echo $1
     fi
   else
    echo "Nothing to babel!"    # The function was called without an argument
   fi
}

write_header () {
  libname=$1
  output=$2
  PreviousLink=$3
  NextLink=$4

  # Note: The first line does not append, i.e. only gets one ">"
  echo '<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">' >$output
  echo '<a name = "top"> </a>' >>$output
  echo '<html>' >>$output
  echo '  <head>' >>$output
  echo '    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">' >>$output
  echo '    <title>PCB Footprint Library</title>' >>$output
  echo '    <script type="text/JavaScript" src="../../html/footprints.js"></script>' >>$output
  echo '  </head>' >>$output
  echo '  <body style="color: rgb(0, 0, 0); background-color: rgb(255, 255,255);">' >>$output
  echo '    <table style=" text-align: left; width: 100%; background-color:' >>$output
  echo '      rgb(102, 255, 255);" border="2" cellpadding="2" cellspacing="2">' >>$output
  echo '      <tbody>' >>$output
  echo '        <tr>' >>$output
  echo '          <th style=" vertical-align: middle; background-color: rgb(51, 204, 0);"' >>$output
  echo '              colspan="7" rowspan="1">' >>$output
  echo '            <h1 style="text-align: center; ">gEDA PCB Library</h1>' >>$output
  echo '          </th>' >>$output
  echo '         </tr>' >>$output

  # The Left (Back) Arrow
  echo '        <tr>' >>$output
  echo '        <td style="vertical-align: middle; text-align: center;"><img' >>$output
  echo '            style=" border: 0px solid; width: 64px; height: 64px;"' >>$output
  echo '            onclick="goBack()" title="Previous" alt="Previous"' >>$output
  echo '            src="../../html/arrow_left.gif"><br>' >>$output
  echo '        </td>' >>$output

  # The Up Arrow
  echo '        <td style="vertical-align: middle; text-align: center;"><a' >>$output
  echo "            href=\"$PreviousLink\"><img alt=\"Up\" title=\"Up\"" >>$output
  echo '              src="../../html/arrow_up.gif" style="border: 0px solid;' >>$output
  echo '              border: 0px solid; width: 64px; height: 64px;"></a><br>' >>$output
  echo '        </td>' >>$output

  # The Left-side Icon image
  echo '        <td style=" vertical-align: middle; text-align: center;"><a' >>$output
  echo "            href=\"$TopIndex\"><img alt=\"Library Contents\"" >>$output
  echo '              src="../../html/pcb-icon.gif" style="border: 0px solid;' >>$output
  echo '              width: 51px; height: 64px;"></a><br>' >>$output
  echo '        </td>' >>$output

  # The Center sub-title Text
  echo '        <td style="vertical-align: middle; width: 360px;">' >>$output
  echo '          <h1 style="text-align: center;">PCB Library</h1>' >>$output
  echo "          <h2 style=\" text-align: center;\">$libname footprints</h2>" >>$output
  echo '        </td>' >>$output

  # The Right-side Icon image
  echo '        <td style=" vertical-align: middle; text-align: center;"><a' >>$output
  echo "            href=\"$TopIndex\"><img alt=\"Library Contents\"" >>$output
  echo '              src="../../html/pcb-icon.gif" style="border: 0px solid;' >>$output
  echo '              width: 51px; height: 64px;"></a><br>' >>$output
  echo '        </td>' >>$output

  # The Down Arrow
  echo '        <td style="vertical-align: middle; text-align: center;"><a' >>$output
  echo "            href=\"$NextLink\"><img alt=\"Down\"" >>$output
  echo '              title="Down" src="../../html/arrow_down.gif"' >>$output
  echo '              style="border: 0px solid; border: 0px solid; width:' >>$output
  echo '              64px; height: 64px;" align="middle"></a><br>' >>$output
  echo '        </td>' >>$output

  # The Right (Forward) Arrow
  echo '        <br>' >>$output
  echo '        <td style="vertical-align: middle; text-align: center;"><img' >>$output
  echo '            onclick="goFoward()" style=" border: 0px solid; width:' >>$output
  echo '            64px; height: 64px;" title="Next" alt="Next"' >>$output
  echo '            src="../../html/arrow_right.gif"><br>' >>$output
  echo '         </td>' >>$output

  echo '         </tr>' >>$output
  echo '         </tbody>' >>$output
  echo '         </table>' >>$output

  echo '          <table border=2>' >>$output
  echo '            <tbody>' >>$output
  echo '              <tr>' >>$output
  echo '                <td><em>Comment</em></td>' >>$output
  echo '                <td><em>Footprint Name</em></td>' >>$output
  echo '              </tr>' >>$output
}

write_body () {

  name=$1
  folder=$2
  output=$3

  descrip=$(cat "$folder/$name" |grep -m 1 Element |cut -d '"' -f2)
  iname=$(cat "$folder/$name" |grep -m 1 Element |cut -d '"' -f6)

  echo '            <tr>' >>$output
  echo "              <td>$iname, $descrip </td>" >>$output
  echo "              <td><a href="\"$1\"">$1</a></td>" >>$output
  echo '            </tr>' >>$output
}

do_Create_HTML_Files () {

  # Load names of all sub-directories into an array
  folders=( $(find . -maxdepth 1 -type d -printf '%P\n' | sort) )
  count=${#folders[*]}
  count=$((count - 1))  # adjusting the count, array is BASE 0

  for index in ${!folders[*]} ; do

      folder=${folders[$index]}
      if [ $index -eq 0 ] ; then
        PreviousIndex="$TopIndex"
      else
        PreviousIndex="../${folders[$index - 1]}/$HTML_NAME.$HTML_EXT"
      fi
      if [[ $index -eq $count ]] ; then
        NextIndex="../${folders[0]}/$HTML_NAME.$HTML_EXT"
      else
        NextIndex="../${folders[$index + 1]}/$HTML_NAME.$HTML_EXT"
      fi

      OutPutFile="$folder/$HTML_NAME.$HTML_EXT"
      vecho "Creating $OutPutFile"
      #echo "folder=$folder, PreviousIndex=$PreviousIndex,  NextIndex=$NextIndex, index=$index, count=$count"
      write_header  $folder $OutPutFile $PreviousIndex $NextIndex

      line_count=0
      for file_name in $folder/$FILTER; do   # loop for all files in globbed subfolder
        name=$(basename $file_name)
        write_body $name $folder $OutPutFile
       (( line_count++ ))
      done
      echo '            </tbody>' >>$OutPutFile
      echo '          </table>' >>$OutPutFile
      if [[ $line_count -gt 20 ]] ; then
        echo '<a href="#top">top of the page</a>' >>$OutPutFile
      fi
      echo '      </body>' >>$OutPutFile
      echo '    </html>' >>$OutPutFile
  done
}

SetTopIndex() {

  if [ ! -z $Library ] ; then
    if [ ! -f "$Library.$HTML_EXT" ] ; then
      echo -e "$RED" "$Library.$HTML_EXT" "$NORMAL" "does not exist"
      return $ERR_FILE_NOT_FOUND;
    else
      TopIndex="../$Library.$HTML_EXT"
    fi
  else # get
    Library=$(basename $PWD)
    if [ -f "$Library.$HTML_EXT" ] ; then
      TopIndex="../$Library.$HTML_EXT"
    else
      echo -e "Did" "$RED" $'NOT' "$NORMAL" "find the top level Library html file"
      return $ERR_FILE_NOT_FOUND;
    fi
  fi
  vecho "Top Index:$TopIndex"
  return 0;
}

ParseCommandLine(){
   for Arg in $*; do
      case $Arg in
#       --filter | -f) ;                                               ;; # Do nothing, yet
         --help | -h) do_show_help 2 ; exit 0                         ;;
      --verbose | -v) Be_Verbose=true                                 ;;
        --quite | -q) Be_Quite=true                                   ;;
        --usage | -u) do_show_help 1 ; exit 0                         ;;
           --version) echo `basename $0` "Version $VER";
                      exit 0                                          ;;
    	            *) case $LAST_ARG in
                    --extension | -e) HTML_NAME="$HTML_NAME$Arg"      ;;
                       --filter | -f) FILTER="$Arg"                   ;;
                       --output | -o) HTML_NAME="$Arg"                ;;
                      --Library | -l) Library="$Arg"                  ;;
                                   *) Library="$Arg"                  ;;
                      esac                                            ;;
      esac
      LAST_ARG=$Arg
   done
}
ParseCommandLine $*
SetTopIndex
if [ $? != 0 ] ; then exit $?; fi
do_Create_HTML_Files

do_scream "Done!"

