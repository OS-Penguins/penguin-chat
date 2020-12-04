# Penguin Chat
This program is a client-server networked chat system.

## Build Instructions
Dependencies:
 * Linux
 * OpenSSL (packaged differently on different distros)
 * g++ (must support C++17)
 * make

Run `make` in the root of the project.

## Deployment Instructions
The server requires the following to run:
 * a file called `localhost.crt` containing an SSL certificate.
 * a file called `localhost.key` containing an SSL private key.
 * an open port 8080

Run the resulting `./penguin-chat` application.

## Usage
`./client.sh` connects to localhost:8080. 
Change the address at the end of the `curl` command to where the server is running.
