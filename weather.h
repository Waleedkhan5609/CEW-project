#ifndef WEATHER_H
#define WEATHER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Constants
#define API_KEY "96f86bfb6e973d648a5a342a7ccd6e6d"
#define RAW_DATA_FILE "raw_data.txt"
#define PROCESSED_DATA_FILE "processed_data.txt"
#define THRESHOLD_HOT 20.0
#define THRESHOLD_COLD 5.0
#define MAX_HISTORY 24

// Function Declarations
void fetch_data_for_city(const char *city, char *buffer);
void parse_weather_data(const char *data, double *temperature, double *humidity, double *wind_speed);
void store_raw_data(const char *city, double temperature, double humidity, double wind_speed);
double calculate_24_hour_average(double *temperatures, int count);
double calculate_24_hour_average_humidity(double *humidities, int count);
void write_processed_data(const char *city, double *temperatures, double *humidities, int count, double average_temp, double average_humidity);
void generate_alert(const char *message);
void check_and_alert(double current_temp, double average_temp, double current_humidity, double average_humidity);
void scheduler(const char *city);

#endif // WEATHER_H

