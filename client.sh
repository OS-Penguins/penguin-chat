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
    if [[ $username != $(head -1 login.txt) ||  $password != $(tail -1 login.txt) ]]; then
        echo
        echo "invalid login try again"
        exit
    fi
    echo
fi

until [[ $VARNAME == "q" ]] ; do  
    
    read -r VARNAME

    if [[ $VARNAME =~ ^message\/ ]] ; then
        reciever=$(echo "$VARNAME" | cut -d '/' -f2 )
        
        if [[ -z "$reciever"  ]]; then
            echo "please specify user to message to"
        fi

        message=$(echo "$VARNAME" | cut -d ' ' -f2)
        curl -H "sender: $username" -H "sender-password: $password" -d "$message" -X POST LOCALHOST:3000/mailbox
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
