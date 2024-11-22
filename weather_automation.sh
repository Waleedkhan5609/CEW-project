#!/bin/bash

# Check if city name is provided
if [ -z "$1" ]; then
    echo "No city name provided. Please enter the city:"
    read CITY
else
    CITY=$1
fi

# File for logs
LOG_FILE="weather_monitoring_$CITY.log"

# Run the program and save output
echo "Running weather monitoring for $CITY..."
./envmon "$CITY" > "$LOG_FILE"

echo "Execution complete. Log saved in $LOG_FILE."

