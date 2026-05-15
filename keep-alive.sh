#!/bin/bash
cd /home/z/my-project/sophon-stream-web
while true; do
  PORT=3001 node server.js
  echo "Server exited, restarting in 2s..."
  sleep 2
done
