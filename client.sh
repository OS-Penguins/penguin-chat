#!/bin/bash

echo "Welcome to our penguin chat!"
echo "----------------------------"

#login
read -p "username: " username
read -sp "password: " password
echo


#do-while loop
until [[ $VARNAME == "q" ]] ; do  
    
    read -r VARNAME
    
    #if it begins with message 
    if [[ $VARNAME =~ ^message\/ ]] ; then
        reciever=$(echo "$VARNAME" | cut -d '/' -f2 | cut -d ' ' -f1 )
        
        #if no user is specified
        if [[ -z "$reciever"  ]]; then
            echo "please specify user to message to"
            continue
        fi
        
        #message body to be sent
        message=$(echo "$VARNAME" | cut -d ' ' -f2-)

        #curl command invoked to send message
        curl --cacert localhost.crt -H "sender: $username" -H "sender-password: $password" -d "$message" -X POST "https://LOCALHOST:8080/message/$reciever"
        echo

    #if it is a mailbox command 
    elif [[ $VARNAME == "mailbox/" ]] ; then

        #curl command to recieve messages
        curl --cacert localhost.crt -H "sender: $username" -H "sender-password: $password" -X GET "https://LOCALHOST:8080/mailbox"
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
    elif [[ $VARNAME == "q" || $VARNAME == "quit" ]]; then 
        exit 0
    else 
        echo "invalid input"
        echo
    fi
done
