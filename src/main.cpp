#include <SensorBuffer.hpp>
#include <libnatkit-core.hpp>
#include <thread>
#include <iostream>
#include <mutex>

#define DELAY_BETWEEN_SAMPLES 20000

std::mutex write_mutex{};
std::mutex read_mutex{};
bool write = true;
bool read = true;

uint64_t getCurrentTimeMillis() {
    // Get the current time point from the system clock
    auto now = std::chrono::system_clock::now();

    // Convert the time point to a duration since the epoch
    auto duration_since_epoch = now.time_since_epoch();

    // Cast the duration to milliseconds and get the count
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration_since_epoch).count();
}

uint64_t getCurrentTimeMicro() {
    // Get the current time point from the system clock
    auto now = std::chrono::system_clock::now();

    // Convert the time point to a duration since the epoch
    auto duration_since_epoch = now.time_since_epoch();

    // Cast the duration to microseconds and get the count
    return std::chrono::duration_cast<std::chrono::microseconds>(duration_since_epoch).count();
}

void write_thread(std::shared_ptr<SensorBuffer<DataArray<3>>> buffer) {
    uint64_t last_timestamp = 0;
    while (1) {
        {
            std::lock_guard<std::mutex> gaurd(write_mutex);
            if (!write) {
                break;
            }
        }
        DataArray<3> data{};
        data.data[0] = rand();
        data.data[1] = rand();
        data.data[2] = rand();
        SensorDataPoint<DataArray<3>> data_point{};
        uint64_t current_timestamp = getCurrentTimeMicro();
        data_point.timestamp = current_timestamp > last_timestamp ? current_timestamp : last_timestamp + 1;
        last_timestamp = data_point.timestamp;
        //std::cout << data_point.timestamp << '\n';
        data_point.dataMaybe = data;
        data_point.calibration = rand() % 256;
        buffer->push(data_point);
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

void read_thread(std::shared_ptr<SensorBuffer<DataArray<3>>> buffer) {
    while (1) {
        {
            std::lock_guard<std::mutex> gaurd(read_mutex);
            if (!read) {
                break;
            }
        }
        auto next_maybe = buffer->tryGetNext();
    }
}

int main() {
    std::shared_ptr<SensorBuffer<DataArray<3>>> buffer = std::make_shared<SensorBuffer<DataArray<3>>>(DELAY_BETWEEN_SAMPLES, DELAY_BETWEEN_SAMPLES + 1000);

    std::cout << "Starting\n";

    std::thread writer(write_thread, buffer);
    std::thread reader(read_thread, buffer);

    std::this_thread::sleep_for(std::chrono::seconds(30));

    {
        std::lock_guard<std::mutex> gaurd(write_mutex);
        write = false;
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));

    {
        std::lock_guard<std::mutex> gaurd(read_mutex);
        read = false;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "Finished\n";

    return 0;
}