#!/bin/sh

echo "quit" | openssl s_client -showcerts -connect localhost:8080 -servername localhost > cacert.pem

curl https://localhost:8080/ --cacert cacert.pem
