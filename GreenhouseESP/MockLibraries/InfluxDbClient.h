#ifndef MOCK_INFLUX_DB_CLIENT_H
#define MOCK_INFLUX_DB_CLIENT_H

#include <iostream>

class Point {
    public:
        Point (const char* pointName) {}
        void addTag(const char* tag, const char* value) {}
        void addField(const char* field, float value) {}
};

class InfluxDBClient {
    public:
        InfluxDBClient(const char *url, const char *org, const char *bucket, const char *token) {
            std::cout << "InfluxDBClient constructor called" << std::endl;
        }

        bool validateConnection(){
            return true;
        }

        bool writePoint(Point point){
            return true;
        }

        std::string getLastErrorMessage() {
            return "[ERROR]: Some InfluxDBClient error message!";
        }
};

#endif // MOCK_INFLUX_DB_CLIENT_H