#include "weather.h"

// It's a function which fetch weather data using curl
void fetch_data_for_city(const char *city, char *buffer) {
    char url[256];
    snprintf(url, sizeof(url),
             "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric",
             city, API_KEY);

    // Construct curl command to fetch data
    char command[512];
    snprintf(command, sizeof(command), "curl -s \"%s\"", url);
    FILE *fp = popen(command, "r");

    if (!fp) {
        perror("Failed to fetch data");
        return;
    }

    fread(buffer, 1, 1024, fp);
    pclose(fp);
}

// Function to parse temperature and humidity from API response
void parse_weather_data(const char *data, double *temperature, double *humidity, double *wind_speed) {
    char *temp_ptr = strstr(data, "\"temp\":");
    char *humidity_ptr = strstr(data, "\"humidity\":");
    char *wind_speed_ptr = strstr(data, "\"speed\":");

    if (temp_ptr) {
        sscanf(temp_ptr, "\"temp\":%lf", temperature);
    }
    if (humidity_ptr) {
        sscanf(humidity_ptr, "\"humidity\":%lf", humidity);
    }
    if (wind_speed_ptr) {
        sscanf(wind_speed_ptr, "\"speed\":%lf", wind_speed);
    }
}

// Function to store raw data in a file
void store_raw_data(const char *city, double temperature, double humidity, double wind_speed) {
    FILE *file = fopen(RAW_DATA_FILE, "a");
    if (file == NULL) {
        perror("Failed to open raw data file");
        return;
    }

    time_t now = time(NULL);
    fprintf(file, "%s,%.2f,%.2f,%.2f,%s", city, temperature, humidity, wind_speed, ctime(&now));
    fclose(file);
}

// Function to calculate the 24-hour average temperature
double calculate_24_hour_average(double *temperatures, int count) {
    double total_temp = 0;
    for (int i = 0; i < count; i++) {
        total_temp += temperatures[i];
    }
    return (count > 0) ? total_temp / count : -1;
}

// Function to calculate the 24-hour average humidity
double calculate_24_hour_average_humidity(double *humidities, int count) {
    double total_humidity = 0;
    for (int i = 0; i < count; i++) {
        total_humidity += humidities[i];
    }
    return (count > 0) ? total_humidity / count : -1;
}

// Function to write processed data (average temp, humidity, and alerts) to a file
void write_processed_data(const char *city, double *temperatures, double *humidities, int count, double average_temp, double average_humidity) {
    FILE *file = fopen(PROCESSED_DATA_FILE, "a");
    if (file == NULL) {
        perror("Failed to open processed data file");
        return;
    }

    time_t now = time(NULL);
    fprintf(file, "City: %s\n", city);
    fprintf(file, "Last 24 Hour Temperatures: ");
    for (int i = 0; i < count; i++) {
        fprintf(file, "%.2f°C ", temperatures[i]);
    }
    fprintf(file, "\nLast 24 Hour Humidity: ");
    for (int i = 0; i < count; i++) {
        fprintf(file, "%.2f%% ", humidities[i]);
    }
    fprintf(file, "\n24-Hour Average Temp: %.2f°C\n", average_temp);
    fprintf(file, "24-Hour Average Humidity: %.2f%%\n", average_humidity);
    fprintf(file, "Timestamp: %s", ctime(&now));

    if (average_temp > THRESHOLD_HOT) {
        fprintf(file, "Alert: Hot weather detected! Stay hydrated and avoid direct sunlight.\n");
    } else if (average_temp < THRESHOLD_COLD) {
        fprintf(file, "Alert: Cold weather detected! Stay warm and wear appropriate clothing.\n");
    } else {
        fprintf(file, "Weather is within normal range.\n");
    }
    fprintf(file, "----------------------------\n");

    fclose(file);
}

// Function to generate system alerts
void generate_alert(const char *message) {
    if (fork() == 0) { // Child process
        execlp("notify-send", "notify-send", "Weather Alert", message, NULL);
        perror("Failed to send system alert");
        exit(1);
    }
}

void send_email_alert(const char *recipient, const char *subject, const char *message) {
    char command[512];
    snprintf(command, sizeof(command),
             "echo \"%s\" | mailx -s \"%s\" %s",
             message, subject, recipient);
    int status = system(command);
    if (status == -1) {
        perror("Failed to send email");
    } else {
        printf("Email sent successfully to %s.\n", recipient);
    }
}

// Function to check thresholds and generate alerts
void check_and_alert(double current_temp, double average_temp, double current_humidity, double average_humidity) {
    if (current_temp > THRESHOLD_HOT) {
        char message[256];
        snprintf(message, sizeof(message),
                 "Hot weather alert!\nCurrent Temp: %.2f°C\nAvg Temp: %.2f°C\nCurrent Humidity: %.2f%%\nAvg Humidity: %.2f%%.\nStay hydrated and avoid direct sunlight.",
                 current_temp, average_temp, current_humidity, average_humidity);
        generate_alert(message);
        
        const char *recipient="waleedsofty125@gmail.com";
        const char *subject="Weather Alert";
        printf("\nALERT: %s\n", message);
        send_email_alert(recipient,subject,message);
        
    } else if (current_temp < THRESHOLD_COLD) {
        char message[256];
        snprintf(message, sizeof(message),
                 "Cold weather alert!\nCurrent Temp: %.2f°C\nAvg Temp: %.2f°C\nCurrent Humidity: %.2f%%\nAvg Humidity: %.2f%%\nStay warm and avoid exposure to cold.",
                 current_temp, average_temp, current_humidity, average_humidity);
        generate_alert(message);
        
        const char *recipient="waleedsofty125@gmail.com";
        const char *subject="Weather Alert";
        printf("\nALERT: %s\n", message);
        send_email_alert(recipient,subject,message);
        
    } else {
        printf("\nWeather is within normal range (Current Temp: %.2f°C, Avg Temp: %.2f°C, Current Humidity: %.2f%%, Avg Humidity: %.2f%%).\n",
               current_temp, average_temp, current_humidity, average_humidity);
    }
}

// Main scheduling loop for fetching data every 15 minutes
void scheduler(const char *city) {
    double temperature_history[MAX_HISTORY];
    double humidity_history[MAX_HISTORY];
    double wind_speed_history[MAX_HISTORY];
    int history_count = 0;
    
    double current_temp = 0.0;
    double current_humidity = 0.0;
    double current_wind_speed = 0.0;

    while (history_count<5) {
        printf("\nFetching weather data for %s...\n", city);
        char buffer[1024];
        fetch_data_for_city(city, buffer);

        parse_weather_data(buffer, &current_temp, &current_humidity, &current_wind_speed);

        if (current_temp < 0) {
            printf("Failed to retrieve data for %s.\n", city);
            sleep(10);
            continue;
        }

        // Store raw data
        printf("Current Weather in %s:\n", city);
        printf("  Temperature: %.2f°C\n", current_temp);
        printf("  Humidity: %.2f%%\n", current_humidity);
        printf("  Wind Speed: %.2f m/s\n", current_wind_speed);
        store_raw_data(city, current_temp, current_humidity, current_wind_speed);

        // Add current temperature and humidity to history
        if (history_count < MAX_HISTORY) {
            temperature_history[history_count] = current_temp;
            humidity_history[history_count] = current_humidity;
            wind_speed_history[history_count] = current_wind_speed;
            history_count++;
        } else {
            // Shift history if it's full
            for (int i = 1; i < MAX_HISTORY; i++) {
                temperature_history[i - 1] = temperature_history[i];
                humidity_history[i - 1] = humidity_history[i];
                wind_speed_history[i - 1] = wind_speed_history[i];
            }
            temperature_history[MAX_HISTORY - 1] = current_temp;
            humidity_history[MAX_HISTORY - 1] = current_humidity;
            wind_speed_history[MAX_HISTORY - 1] = current_wind_speed;
        }
        sleep(10);
        }
        // Calculate the average temperature and humidity over the last 24 hours
        double average_temp = calculate_24_hour_average(temperature_history, history_count);
        double average_humidity = calculate_24_hour_average_humidity(humidity_history, history_count);
        double average_wind_speed = calculate_24_hour_average(wind_speed_history, history_count);
        
        if (average_temp >= 0 && average_humidity >= 0) {
            printf("\n*************************************************\n");
            printf("Weather Summary for %s\n", city);
            printf("-------------------------------------------------\n");
            printf("  Average Temperature: %.2f°C\n", average_temp);
            printf("  Average Humidity: %.2f%%\n", average_humidity);
            printf("  Average Wind Speed: %.2f m/s\n", average_wind_speed);
            printf("-------------------------------------------------\n");
            printf("  Stay safe and informed!\n");
            printf("*************************************************\n");

            // Write processed data to the file
            write_processed_data(city, temperature_history, humidity_history, history_count, average_temp, average_humidity);

            // Check for alerts and notify
            check_and_alert(current_temp, average_temp, current_humidity, average_humidity);
        }
    
}

int main() {
    char city[50];
    printf("Enter the city: ");
    scanf("%49s", city);

    printf("Starting weather monitoring for %s...\n", city);
    scheduler(city);

    return 0;
}
