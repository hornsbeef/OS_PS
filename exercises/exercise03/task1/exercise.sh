#!/bin/bash

# check if we are running on a bsd or linux system
unamestr=$(uname)
if [ "$unamestr" = 'Linux' ]; then
    permission_check_options+=(-c "%A")
elif [ "$unamestr" = 'FreeBSD' ] || [ "$unamestr" = 'Darwin' ]; then
    permission_check_options=(-f "%Sp")
fi

echo "Congratulations, you already solved the first part of this task about permissions."
echo "Next, follow the prompts all the way to the end."
echo "-------"

if [ ! -f "task1.txt" ]; then
    echo "'task1.txt' not found. Please create 'task1.txt' in the current directory."
    exit 1
else
    echo "'task1.txt' found."
fi

PERMISSIONS=$(stat "${permission_check_options[@]}" task1.txt)
CORRECT_PERMISSIONS="-rw-------"  # Readable and writable by the owner only

if [ "$PERMISSIONS" != "$CORRECT_PERMISSIONS" ]; then
    echo "Incorrect permissions for 'task1.txt'."
    echo "Required permissions: Only the user can read and write to the file."
    echo "Please set the correct permissions."
    exit 1
else
    echo "'task1.txt' has correct permissions."
    echo "Answer the following questions:" > task1.txt
    echo "- Which command did you use to set the permissions for 'task1.txt'?" >> task1.txt
fi

if [ ! -f "file.txt" ]; then
    echo "'file.txt' not found. Ensure that the provided 'file.txt' is placed in the current directory."
    exit 1
fi

PERMISSIONS=$(stat "${permission_check_options[@]}" file.txt)
CORRECT_PERMISSIONS="-r--------"

if [ "$PERMISSIONS" != "$CORRECT_PERMISSIONS" ]; then
    echo "Incorrect permission for 'file.txt'."
    echo "Make sure 'file.txt' has only a single read permission for the user."
    exit 1
else
    echo "'file.txt' has correct permissions."
    echo "- Which command did you use to set the permissions for 'file.txt'?" >> task1.txt 
    cat file.txt >> task1.txt
    echo "Congratulations. As a final step, answer the questions in 'task1.txt' to finish this task."
fi