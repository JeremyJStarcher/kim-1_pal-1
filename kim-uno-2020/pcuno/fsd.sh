#!/bin/bash

: '
                   filebrowse.sh written by Claude Pageau
This is a whiptail file browser demo that allows navigating through a directory
structure and select a specified file type per filext variable.
It Returns a filename path if selected.  Esc key exits.
This sample code can be used in a script menu to perform an operation that
requires selecting a file.
'

startdir="./"
filext='*'
menutitle="$filext Select a Directory"

#------------------------------------------------------------------------------
function Filebrowser()
{
# first parameter is Menu Title
# second parameter is optional dir path to starting folder
# otherwise current folder is selected

    if [ -z $2 ] ; then
        dir_list=$(ls -lhp  | awk -F ' ' ' { print $9 " " $5 } ')
    else
        cd "$2"
        dir_list=$(ls -lhp  | awk -F ' ' ' { print $9 " " $5 } ')
    fi

    curdir=$(pwd)
    if [ "$curdir" == "/" ] ; then  # Check if you are at root folder
        selection=$(whiptail --title "$1" \
                              --menu "ESC to cancel" 0 0 0 \
                              --cancel-button Select_Dir \
                              --ok-button Go_Into $dir_list 3>&1 1>&2 2>&3)
    else   # Not Root Dir so show ../ BACK Selection in Menu
        selection=$(whiptail --title "$1" \
                              --menu "ESC to cancel" 0 0 0 \
                              --cancel-button Select_Dir \
                              --ok-button Go_Into ../ BACK $dir_list 3>&1 1>&2 2>&3)
    fi

    RET=$?
    if [ $RET -eq 1 ]; then  # Check if User Selected Cancel
                filename="$selection"
                filepath="$curdir"    # Return full filepath  and filename as selection variables
       return 1
    elif [ $RET -eq 0 ]; then
       if [[ -d "$selection" ]]; then  # Check if Directory Selected
          Filebrowser "$1" "$selection"
       elif [[ -f "$selection" ]]; then  # Check if File Selected
#          if [[ $selection == *$filext ]]; then   # Check if selected File has .jpg extension
#            if (whiptail --title "Confirm Selection" --yesno "DirPath : $curdir\nFileName: $selection" 0 0 \
#                         --yes-button "Confirm" \
#                         --no-button "Retry"); then
#                filename="$selection"
#                filepath="$curdir"    # Return full filepath  and filename as selection variables
#            else
                Filebrowser "$1" "$curdir"
#            fi
#          else   # Not correct extension so Inform User and restart
#             whiptail --title "ERROR: File Must have $filext Extension" \
#                      --msgbox "$selection\nYou Must Select a $filext file" 0 0
#             Filebrowser "$1" "$curdir"
#          fi
       else
          # Could not detect a file or folder so Try Again
          whiptail --title "ERROR: Selection Error" \
                   --msgbox "Error Changing to Path $selection" 0 0
          Filebrowser "$1" "$curdir"
       fi
    fi
}


Filebrowser "$menutitle" "$startdir"

exitstatus=$?
if [ $exitstatus -eq 0 ]; then
    if [ "$selection" == "" ]; then
        echo "cancelled"
        echo "cancelled"
    else
        echo 'Err'$filepath
    fi
else
        echo $filepath'/'
        whiptail --inputbox "Filename:" 0 0
        exitstatus=$?
        if [ $exitstatus -eq 0 ]; then
#    if [ "$selection" == "" ]; then
#        echo "exit 0 with no selection"
#    else
#        echo "exit 0"
#    fi
            echo
        else
            echo "cancelled"
        fi
fi


