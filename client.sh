#!/bin/bash

echo "Welcome to our penguin chat!"
echo "----------------------------"
if [[ ! -e login.txt ]]; then
    echo "Create a user login"
    read -p "username: " username
    read -sp "password: " password
    echo "$username" >> login.txt
    echo "$password" >> login.txt
    echo
else
    read -p "username: " username
    read -sp "password: " password
    if [[ ! $username == $(head -1 login.txt) && $password == $(head -2 login.txt) ]]; then
        echo "invalid login try again"
        exit
    fi
    echo
fi

until [[ $VARNAME == "q" ]] ; do  
    
    read -r VARNAME

    if [[ $VARNAME =~ ^message\/ ]] ; then
        USERMSG=$(echo $VARNAME | cut -d '/' -f2 )
        if [[ -z "$USERMSG"  ]]; then
            echo "please specify user to message to"
        fi
       
        echo $USERMSG
        echo
    elif [[ $VARNAME == "mailbox/" ]] ; then
        echo "youve got mail!"
        echo
        
    elif [[ $VARNAME == "/" ]] ; then
        echo "commands"
        echo "-----------------------"
        echo "message/username - message a fellow penguin"
        echo "mailbox/ - recieve all messages"
        echo "/ - options"
        echo "q - quit"
        echo
    else 
        echo "invalid input"
        echo
    fi
done
