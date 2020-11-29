#!/bin/bash

echo "Welcome to our penguin chat!"
echo "----------------------------"

#if new user
if [[ ! -e login.txt ]]; then
    echo "Create a user login"
    read -p "username: " username
    read -sp "password: " password
    echo "$username" >> login.txt
    echo "$password" >> login.txt
    echo
else
#if old user verify
    read -p "username: " username
    read -sp "password: " password
    if [[ $username != $(head -1 login.txt) ||  $password != $(tail -1 login.txt) ]]; then
        echo
        echo "invalid login try again"
        exit
    fi
    echo
fi

#do-while loop
until [[ $VARNAME == "q" ]] ; do  
    
    read -r VARNAME
    
    #if it begins with message 
    if [[ $VARNAME =~ ^message\/ ]] ; then
        reciever=$(echo "$VARNAME" | cut -d '/' -f2 )
        
        #if no user is specified
        if [[ -z "$reciever"  ]]; then
            echo "please specify user to message to"
            continue
        fi
        
        #message body to be sent
        message=$(echo "$VARNAME" | cut -d ' ' -f2)

        #curl command invoked to send message
        curl -H "sender: $username" -H "sender-password: $password" -d "$message" -X POST LOCALHOST:3000/message/$reciever
        echo

    #if it is a mailbox command 
    elif [[ $VARNAME == "mailbox/" ]] ; then
        echo "youve got mail!"

        #curl command to recieve messages
        curl -H "sender: $username" -H "sender-password: $password" -X GET LOCALHOST:3000/mailbox
        echo

    #if it is the options command 
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
