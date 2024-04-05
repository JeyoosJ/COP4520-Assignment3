#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>

using namespace std;

const int NUM_SENSORS = 8;
const int NUM_READINGS_PER_HOUR = 60;
const int NUM_HOURS = 24;
const int NUM_READINGS = NUM_READINGS_PER_HOUR * NUM_HOURS;

mutex sharedMemoryMutex;
mutex resultsMutex;

vector<vector<double>> sharedMemory(NUM_READINGS, vector<double>(NUM_SENSORS));
vector<double> maxTemperatures(NUM_HOURS, numeric_limits<double>::lowest());
vector<double> minTemperatures(NUM_HOURS, numeric_limits<double>::max());
vector<pair<int, int>> maxTemperatureDifferenceIntervals(NUM_HOURS);

void sensorThread(int sensorId) {
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dis(-100.0, 70.0);

    for (int i = 0; i < NUM_READINGS; ++i) {
        double temperature = dis(gen);

        {
            lock_guard<mutex> lock(sharedMemoryMutex);
            sharedMemory[i][sensorId] = temperature;
        }

        int hour = i / NUM_READINGS_PER_HOUR;
        {
            lock_guard<mutex> lock(resultsMutex);
            if (temperature > maxTemperatures[hour]) {
                maxTemperatures[hour] = temperature;
            }
            if (temperature < minTemperatures[hour]) {
                minTemperatures[hour] = temperature;
            }
        }
    }
}

void analyzeData() {
    for (int hour = 0; hour < NUM_HOURS; ++hour) {
        double maxTempDifference = 0;
        int startInterval = 0;
        double maxTempInInterval = numeric_limits<double>::lowest();
        double minTempInInterval = numeric_limits<double>::max();

        for (int i = 0; i < NUM_READINGS_PER_HOUR - 10; ++i) {
            int startIndex = hour * NUM_READINGS_PER_HOUR + i;
            int endIndex = startIndex + 10;
            double maxTemp = numeric_limits<double>::lowest();
            double minTemp = numeric_limits<double>::max();

            for (int j = startIndex; j < endIndex; ++j) {
                for (int sensorId = 0; sensorId < NUM_SENSORS; ++sensorId) {
                    double temp;
                    {
                        lock_guard<mutex> lock(sharedMemoryMutex);
                        temp = sharedMemory[j][sensorId];
                    }
                    if (temp > maxTemp) maxTemp = temp;
                    if (temp < minTemp) minTemp = temp;
                }
            }

            double tempDifference = maxTemp - minTemp;
            if (tempDifference > maxTempDifference) {
                maxTempDifference = tempDifference;
                startInterval = i;
                maxTempInInterval = maxTemp;
                minTempInInterval = minTemp;
            }
        }

        maxTemperatureDifferenceIntervals[hour] = {startInterval, startInterval + 10};

        // Sort temperatures for the current hour
        vector<double> hourlyTemperatures;
        for (int i = hour * NUM_READINGS_PER_HOUR; i < (hour + 1) * NUM_READINGS_PER_HOUR; ++i) {
            for (int sensorId = 0; sensorId < NUM_SENSORS; ++sensorId) {
                double temp;
                {
                    lock_guard<mutex> lock(sharedMemoryMutex);
                    temp = sharedMemory[i][sensorId];
                }
                hourlyTemperatures.push_back(temp);
            }
        }
        sort(hourlyTemperatures.begin(), hourlyTemperatures.end());

        // Print report
        cout << "Hour " << hour << " Report:" << endl;
        cout << "Top 5 Highest Temperatures: ";
        for (int i = max(0, (int)hourlyTemperatures.size() - 5); i < hourlyTemperatures.size(); ++i)
            cout << hourlyTemperatures[i] << " ";
        cout << endl;

        cout << "Top 5 Lowest Temperatures: ";
        for (int i = 0; i < min(5, (int)hourlyTemperatures.size()); ++i)
            cout << hourlyTemperatures[i] << " ";
        cout << endl;

        cout << "Max Temperature Difference Interval: [" << maxTemperatureDifferenceIntervals[hour].first << ", " << maxTemperatureDifferenceIntervals[hour].second << "]" << endl;
        cout << "Max Temperature in Interval: " << maxTempInInterval << endl;
        cout << "Min Temperature in Interval: " << minTempInInterval << endl;
    }
}

int main() {
    vector<thread> threads;

    for (int i = 0; i < NUM_SENSORS; ++i) {
        threads.push_back(thread(sensorThread, i));
    }

    for (auto& t : threads) {
        t.join();
    }

    analyzeData();

    return 0;
}
