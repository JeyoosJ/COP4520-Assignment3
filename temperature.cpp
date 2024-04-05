#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <random>

using namespace std;

const int NUM_SENSORS = 8;
const int NUM_READINGS_PER_HOUR = 60;
const int NUM_HOURS = 24;
const int NUM_READINGS = NUM_READINGS_PER_HOUR * NUM_HOURS;

mutex sharedMemoryMutex;

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

        // Store temperature reading in shared memory
        lock_guard<mutex> lock(sharedMemoryMutex);
        sharedMemory[i][sensorId] = temperature;

        // Update max and min temperatures
        int hour = i / NUM_READINGS_PER_HOUR;
        if (temperature > maxTemperatures[hour]) {
            maxTemperatures[hour] = temperature;
        }
        if (temperature < minTemperatures[hour]) {
            minTemperatures[hour] = temperature;
        }
    }
}

void analyzeData() {
    for (int hour = 0; hour < NUM_HOURS; ++hour) {
        double maxTempDifference = 0;
        int startInterval = 0;

        for (int i = 0; i < NUM_READINGS_PER_HOUR; ++i) {
            int index = hour * NUM_READINGS_PER_HOUR + i;
            double maxTemp = 71;
            double minTemp = -101;

            for (int sensorId = 0; sensorId < NUM_SENSORS; ++sensorId) {
                double temp = sharedMemory[index][sensorId];
                if (temp > maxTemp) maxTemp = temp;
                if (temp < minTemp) minTemp = temp;
            }

            double tempDifference = maxTemp - minTemp;
            if (tempDifference > maxTempDifference) {
                maxTempDifference = tempDifference;
                startInterval = i;
            }
        }

        maxTemperatureDifferenceIntervals[hour] = {startInterval, startInterval + 10};
    }
}

int main() {
    vector<thread> threads;

    // Create threads for each sensor
    for (int i = 0; i < NUM_SENSORS; ++i) {
        threads.push_back(thread(sensorThread, i));
    }

    // Join threads
    for (auto& t : threads) {
        t.join();
    }

    // Analyze data
    analyzeData();

    // Print results
    for (int hour = 0; hour < NUM_HOURS; ++hour) {
        cout << "Hour " << hour << ":" << endl;
        cout << "Max Temperature: " << maxTemperatures[hour] << endl;
        cout << "Min Temperature: " << minTemperatures[hour] << endl;
        auto interval = maxTemperatureDifferenceIntervals[hour];
        cout << "Max Temperature Difference Interval: [" << interval.first << ", " << interval.second << "]" << endl;
    }

    return 0;
}
