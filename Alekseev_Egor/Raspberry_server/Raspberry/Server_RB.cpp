#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm>
#include <set>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <deque>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <random>

const unsigned short SERVER_PORT = 8080;
const std::string LOG_FILE = "sensor_data.log";
const std::string DANGER_LOG_FILE = "dangerous_data.log";
const size_t MAX_LOG_ENTRIES = 120;

struct NormalRanges {
    float temp_min = 18.0f;
    float temp_max = 26.0f;
    float humidity_min = 30.0f;
    float humidity_max = 70.0f;
    float soil_moisture_min = 40.0f;
    float soil_moisture_max = 80.0f;
    float light_min = 300.0f;
    float light_max = 800.0f;
};

std::atomic<bool> server_running{true};

void signal_handler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    server_running = false;
}

struct SensorData {
    float temperature = 0.0f;
    float humidity = 0.0f;
    float soil_moisture = 0.0f;
    float light = 0.0f;
    std::chrono::system_clock::time_point timestamp;
    bool is_dangerous = false;
    std::vector<std::string> danger_reasons;

    std::string toString() const {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) 
           << temperature << " " << humidity << " " 
           << soil_moisture << " " << light;
        return ss.str();
    }
    
    std::string toFormattedString() const {
        std::stringstream ss;
        ss << "Temperature: " << std::fixed << std::setprecision(1) 
           << temperature << "°C, "
           << "Humidity: " << humidity << "%, "
           << "Soil Moisture: " << soil_moisture << "%, "
           << "Light: " << light << " lux";
        
        if (is_dangerous) {
            ss << " [DANGER: ";
            for (size_t i = 0; i < danger_reasons.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << danger_reasons[i];
            }
            ss << "]";
        }
        
        return ss.str();
    }
    
    std::string toCsvString() const {
        std::stringstream ss;
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << "," << std::fixed << std::setprecision(1)
           << temperature << "," << humidity << "," 
           << soil_moisture << "," << light;
        return ss.str();
    }
    
    std::string toDangerCsvString() const {
        std::stringstream ss;
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << "," << std::fixed << std::setprecision(1)
           << temperature << "," << humidity << "," 
           << soil_moisture << "," << light << ",";
        
        for (size_t i = 0; i < danger_reasons.size(); ++i) {
            if (i > 0) ss << "; ";
            ss << danger_reasons[i];
        }
        
        return ss.str();
    }
    
    void checkDangerous(const NormalRanges& ranges) {
        is_dangerous = false;
        danger_reasons.clear();
        
        if (temperature < ranges.temp_min) {
            is_dangerous = true;
            std::stringstream ss;
            ss << "Temperature too low: " << std::fixed << std::setprecision(1) 
               << temperature << "°C (min: " << ranges.temp_min << "°C)";
            danger_reasons.push_back(ss.str());
        } else if (temperature > ranges.temp_max) {
            is_dangerous = true;
            std::stringstream ss;
            ss << "Temperature too high: " << std::fixed << std::setprecision(1) 
               << temperature << "°C (max: " << ranges.temp_max << "°C)";
            danger_reasons.push_back(ss.str());
        }
        
        if (humidity < ranges.humidity_min) {
            is_dangerous = true;
            std::stringstream ss;
            ss << "Humidity too low: " << std::fixed << std::setprecision(1) 
               << humidity << "% (min: " << ranges.humidity_min << "%)";
            danger_reasons.push_back(ss.str());
        } else if (humidity > ranges.humidity_max) {
            is_dangerous = true;
            std::stringstream ss;
            ss << "Humidity too high: " << std::fixed << std::setprecision(1) 
               << humidity << "% (max: " << ranges.humidity_max << "%)";
            danger_reasons.push_back(ss.str());
        }
        
        if (soil_moisture < ranges.soil_moisture_min) {
            is_dangerous = true;
            std::stringstream ss;
            ss << "Soil moisture too low: " << std::fixed << std::setprecision(1) 
               << soil_moisture << "% (min: " << ranges.soil_moisture_min << "%)";
            danger_reasons.push_back(ss.str());
        } else if (soil_moisture > ranges.soil_moisture_max) {
            is_dangerous = true;
            std::stringstream ss;
            ss << "Soil moisture too high: " << std::fixed << std::setprecision(1) 
               << soil_moisture << "% (max: " << ranges.soil_moisture_max << "%)";
            danger_reasons.push_back(ss.str());
        }
        
        if (light < ranges.light_min) {
            is_dangerous = true;
            std::stringstream ss;
            ss << "Light too low: " << std::fixed << std::setprecision(1) 
               << light << " lux (min: " << ranges.light_min << " lux)";
            danger_reasons.push_back(ss.str());
        } else if (light > ranges.light_max) {
            is_dangerous = true;
            std::stringstream ss;
            ss << "Light too high: " << std::fixed << std::setprecision(1) 
               << light << " lux (max: " << ranges.light_max << " lux)";
            danger_reasons.push_back(ss.str());
        }
    }
};

class DataLogger {
private:
    std::ofstream logFile;
    std::ofstream dangerLogFile;
    std::atomic<bool> logging_active{true};
    std::thread logging_thread;
    std::function<SensorData()> dataProvider;
    std::chrono::seconds logging_interval;
    std::deque<std::string> log_cache;
    std::deque<std::string> danger_log_cache;
    NormalRanges normal_ranges;
    
    void logData() {
        while (logging_active) {
            std::this_thread::sleep_for(logging_interval);
            
            if (!logging_active) break;

            SensorData data = dataProvider();

            data.checkDangerous(normal_ranges);

            if (logFile.is_open()) {
                std::string log_entry = data.toCsvString();
                logFile << log_entry << std::endl;
                logFile.flush();
 
                log_cache.push_back(log_entry);
                if (log_cache.size() > MAX_LOG_ENTRIES) {
                    log_cache.pop_front();
                }
                
                std::cout << "[Logger]: Data logged at " 
                          << std::put_time(std::localtime(&std::chrono::system_clock::to_time_t(data.timestamp)), 
                                         "%Y-%m-%d %H:%M:%S")
                          << " - " << data.toFormattedString() << std::endl;
            }

            if (data.is_dangerous && dangerLogFile.is_open()) {
                std::string danger_entry = data.toDangerCsvString();
                dangerLogFile << danger_entry << std::endl;
                dangerLogFile.flush();

                danger_log_cache.push_back(danger_entry);
                if (danger_log_cache.size() > MAX_LOG_ENTRIES) {
                    danger_log_cache.pop_front();
                }
                
                std::cout << "[Logger]: DANGER! Hazardous conditions detected!" << std::endl;
                for (const auto& reason : data.danger_reasons) {
                    std::cout << "  - " << reason << std::endl;
                }
            }
        }
    }
    
public:
    DataLogger(std::function<SensorData()> provider, std::chrono::seconds interval = std::chrono::seconds(5))
        : dataProvider(provider), logging_interval(interval) {

        logFile.open(LOG_FILE, std::ios::out | std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "[Logger]: Failed to open log file: " << LOG_FILE << std::endl;
        } else {
            logFile.seekp(0, std::ios::end);
            if (logFile.tellp() == 0) {
                logFile << "Timestamp,Temperature,Humidity,Soil_Moisture,Light" << std::endl;
            }
            std::cout << "[Logger]: Log file opened: " << LOG_FILE << std::endl;
        }

        dangerLogFile.open(DANGER_LOG_FILE, std::ios::out | std::ios::app);
        if (!dangerLogFile.is_open()) {
            std::cerr << "[Logger]: Failed to open danger log file: " << DANGER_LOG_FILE << std::endl;
        } else {
            dangerLogFile.seekp(0, std::ios::end);
            if (dangerLogFile.tellp() == 0) {
                dangerLogFile << "Timestamp,Temperature,Humidity,Soil_Moisture,Light,Danger_Reasons" << std::endl;
            }
            std::cout << "[Logger]: Danger log file opened: " << DANGER_LOG_FILE << std::endl;
        }
 
        loadLastEntries();
        
        std::cout << "[Logger]: Normal ranges configured:" << std::endl;
        std::cout << "  Temperature: " << normal_ranges.temp_min << "°C - " << normal_ranges.temp_max << "°C" << std::endl;
        std::cout << "  Humidity: " << normal_ranges.humidity_min << "% - " << normal_ranges.humidity_max << "%" << std::endl;
        std::cout << "  Soil Moisture: " << normal_ranges.soil_moisture_min << "% - " << normal_ranges.soil_moisture_max << "%" << std::endl;
        std::cout << "  Light: " << normal_ranges.light_min << " lux - " << normal_ranges.light_max << " lux" << std::endl;

        logging_thread = std::thread(&DataLogger::logData, this);
    }
    
    ~DataLogger() {
        stop();
    }
    
    void stop() {
        logging_active = false;
        if (logging_thread.joinable()) {
            logging_thread.join();
        }
        if (logFile.is_open()) {
            logFile.close();
            std::cout << "[Logger]: Log file closed" << std::endl;
        }
        if (dangerLogFile.is_open()) {
            dangerLogFile.close();
            std::cout << "[Logger]: Danger log file closed" << std::endl;
        }
    }
    
    void loadLastEntries() {
        std::ifstream file(LOG_FILE);
        if (file.is_open()) {
            std::string line;
            std::vector<std::string> all_lines;
            
            std::getline(file, line); 
            
            while (std::getline(file, line)) {
                all_lines.push_back(line);
            }
            file.close();
            
            log_cache.clear();
            size_t start = all_lines.size() > MAX_LOG_ENTRIES ? all_lines.size() - MAX_LOG_ENTRIES : 0;
            for (size_t i = start; i < all_lines.size(); ++i) {
                log_cache.push_back(all_lines[i]);
            }
            
            std::cout << "[Logger]: Loaded " << log_cache.size() << " entries from log file" << std::endl;
        }

        std::ifstream dangerFile(DANGER_LOG_FILE);
        if (dangerFile.is_open()) {
            std::string line;
            std::vector<std::string> all_lines;
            
            std::getline(dangerFile, line); 
            
            while (std::getline(dangerFile, line)) {
                all_lines.push_back(line);
            }
            dangerFile.close();
            
            danger_log_cache.clear();
            size_t start = all_lines.size() > MAX_LOG_ENTRIES ? all_lines.size() - MAX_LOG_ENTRIES : 0;
            for (size_t i = start; i < all_lines.size(); ++i) {
                danger_log_cache.push_back(all_lines[i]);
            }
            
            std::cout << "[Logger]: Loaded " << danger_log_cache.size() << " entries from danger log file" << std::endl;
        }
    }
    
    std::string getLastEntries(size_t count) const {
        std::stringstream ss;
        
        if (log_cache.empty()) {
            ss << "No log entries available";
            return ss.str();
        }
        
        size_t num_entries = std::min(count, log_cache.size());
        size_t start = log_cache.size() - num_entries;
        
        ss << "Last " << num_entries << " log entries:\n";
        ss << "----------------------------------------------------------------------------------------------------\n";
        ss << std::left << std::setw(20) << "Timestamp" 
           << std::setw(12) << "Temp(°C)" 
           << std::setw(12) << "Hum(%)" 
           << std::setw(15) << "Soil(%)" 
           << std::setw(10) << "Light(lux)" << "\n";
        ss << "----------------------------------------------------------------------------------------------------\n";
        
        for (size_t i = start; i < log_cache.size(); ++i) {
            std::string entry = log_cache[i];
            std::stringstream entry_ss(entry);
            std::string timestamp, temp, hum, soil, light;
            
            std::getline(entry_ss, timestamp, ',');
            std::getline(entry_ss, temp, ',');
            std::getline(entry_ss, hum, ',');
            std::getline(entry_ss, soil, ',');
            std::getline(entry_ss, light, ',');
            
            ss << std::left << std::setw(20) << timestamp
               << std::setw(12) << temp
               << std::setw(12) << hum
               << std::setw(15) << soil
               << std::setw(10) << light << "\n";
        }
        
        ss << "----------------------------------------------------------------------------------------------------\n";
        ss << "Total entries in log: " << log_cache.size() << "\n";
        
        return ss.str();
    }
    
    std::string getLastDangerEntries(size_t count) const {
        std::stringstream ss;
        
        if (danger_log_cache.empty()) {
            ss << "No dangerous events recorded";
            return ss.str();
        }
        
        size_t num_entries = std::min(count, danger_log_cache.size());
        size_t start = danger_log_cache.size() - num_entries;
        
        ss << "Last " << num_entries << " dangerous events:\n";
        ss << "--------------------------------------------------------------------------------------------------------\n";
        ss << std::left << std::setw(20) << "Timestamp" 
           << std::setw(12) << "Temp(°C)" 
           << std::setw(12) << "Hum(%)" 
           << std::setw(15) << "Soil(%)" 
           << std::setw(10) << "Light(lux)"
           << std::setw(40) << "Danger Reasons" << "\n";
        ss << "--------------------------------------------------------------------------------------------------------\n";
        
        for (size_t i = start; i < danger_log_cache.size(); ++i) {
            std::string entry = danger_log_cache[i];
            std::stringstream entry_ss(entry);
            std::string timestamp, temp, hum, soil, light, reasons;
            
            std::getline(entry_ss, timestamp, ',');
            std::getline(entry_ss, temp, ',');
            std::getline(entry_ss, hum, ',');
            std::getline(entry_ss, soil, ',');
            std::getline(entry_ss, light, ',');
            std::getline(entry_ss, reasons);
            
            if (reasons.length() > 38) {
                reasons = reasons.substr(0, 35) + "...";
            }
            
            ss << std::left << std::setw(20) << timestamp
               << std::setw(12) << temp
               << std::setw(12) << hum
               << std::setw(15) << soil
               << std::setw(10) << light
               << std::setw(40) << reasons << "\n";
        }
        
        ss << "--------------------------------------------------------------------------------------------------------\n";
        ss << "Total dangerous events: " << danger_log_cache.size() << "\n";
        
        return ss.str();
    }
    
    size_t getTotalEntries() const { return log_cache.size(); }
    size_t getTotalDangerEntries() const { return danger_log_cache.size(); }
    NormalRanges getNormalRanges() const { return normal_ranges; }
};

class MockSensorData {
private:
    SensorData mockData;
    
    void generateRandomData(SensorData& data) {
        struct MinValues {
            float temperature = -10.0f;
            float humidity = 0.0f;
            float soil_moisture = 0.0f;
            float light = 0.0f;
        };
        
        struct MaxValues {
            float temperature = 40.0f;
            float humidity = 100.0f;
            float soil_moisture = 100.0f;
            float light = 1000.0f;
        };
        
        MinValues min_vals;
        MaxValues max_vals;
        
        static std::random_device rd;  
        static std::mt19937 gen(rd()); 
        
        std::uniform_real_distribution<float> temp_dist(min_vals.temperature, max_vals.temperature);
        std::uniform_real_distribution<float> hum_dist(min_vals.humidity, max_vals.humidity);
        std::uniform_real_distribution<float> soil_dist(min_vals.soil_moisture, max_vals.soil_moisture);
        std::uniform_real_distribution<float> light_dist(min_vals.light, max_vals.light);
        
        data.temperature = temp_dist(gen);
        data.humidity = hum_dist(gen);
        data.soil_moisture = soil_dist(gen);
        data.light = light_dist(gen);
        data.timestamp = std::chrono::system_clock::now();
    }
    
public:
    MockSensorData() {
        generateRandomData(mockData);
    }
    
    SensorData getSensorData() {
        generateRandomData(mockData);
        return mockData;
    }
    
    SensorData getCurrentSensorData() {
        return mockData;
    }
    
    void updateData(float temp, float hum, float soil, float light_val) {
        mockData.temperature = temp;
        mockData.humidity = hum;
        mockData.soil_moisture = soil;
        mockData.light = light_val;
        mockData.timestamp = std::chrono::system_clock::now();
    }
};

class CommandHandler {
public:
    enum class CommandType {
        SERVER,
        TERMINATE
    };

private:
    std::map<std::string, std::function<bool(int, const std::string&)>> commands;
    std::map<std::string, std::string> commandDescriptions;
    std::set<std::string> terminateCommands;
    MockSensorData* mockSensor;
    DataLogger* logger;
    std::atomic<bool> streaming_active{false};
    std::thread streaming_thread;
    int current_socket = -1;
    
    bool sendResponse(int socket, const std::string& response) {
        ssize_t result = send(socket, response.c_str(), response.length(), 0);
        if (result == -1) {
            std::cerr << "[Error]: Failed to send response: " << strerror(errno) << std::endl;
            return false;
        }
        return true;
    }
    
    void startDataStream() {
        std::cout << "[Stream]: Started data stream" << std::endl;
        
        while (streaming_active && server_running) {
            SensorData sensorData = mockSensor->getSensorData();
            sensorData.checkDangerous(logger->getNormalRanges());
            
            std::string response = "DATA: " + sensorData.toFormattedString() + "\n";
            sendResponse(current_socket, response);
            
            for (int i = 0; i < 20 && streaming_active && server_running; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        
        std::cout << "[Stream]: Stopped data stream" << std::endl;
    }

public:
    CommandHandler(MockSensorData* sensor, DataLogger* dataLogger) : mockSensor(sensor), logger(dataLogger) {        
        registerCommand("GET_DATA", "Get sensor data ", CommandType::SERVER,
            [this](int sock, const std::string& cmd) -> bool {
                SensorData sensorData = mockSensor->getSensorData();
                sensorData.checkDangerous(logger->getNormalRanges());
                std::string response = sensorData.toString();
                if (sensorData.is_dangerous) {
                    response += " [DANGER]";
                }
                sendResponse(sock, response);
                std::cout << "[Command]: GET_DATA - Sent new sensor data to client: " 
                          << sensorData.toFormattedString() << std::endl;
                return true;
            });
        
        registerCommand("GET_FORMATTED_DATA", "Get formatted sensor data (generates new random data)", CommandType::SERVER,
            [this](int sock, const std::string& cmd) -> bool {
                SensorData sensorData = mockSensor->getSensorData();
                sensorData.checkDangerous(logger->getNormalRanges());
                std::string response = sensorData.toFormattedString();
                sendResponse(sock, response);
                std::cout << "[Command]: GET_FORMATTED_DATA - Sent formatted data to client" << std::endl;
                return true;
            });
        
        registerCommand("START_STREAM", "Start streaming sensor data every 2 seconds", CommandType::SERVER,
            [this](int sock, const std::string& cmd) -> bool {
                if (streaming_active) {
                    sendResponse(sock, "ERROR: Data stream already active");
                    return true;
                }
                
                current_socket = sock;
                streaming_active = true;
                streaming_thread = std::thread(&CommandHandler::startDataStream, this);
                
                sendResponse(sock, "OK: Data streaming started (every 2 seconds). Use STOP_STREAM to stop.");
                std::cout << "[Command]: START_STREAM - Started data streaming" << std::endl;
                return true;
            });
        
        registerCommand("STOP_STREAM", "Stop streaming sensor data", CommandType::SERVER,
            [this](int sock, const std::string& cmd) -> bool {
                if (!streaming_active) {
                    sendResponse(sock, "ERROR: No active data stream");
                    return true;
                }
                
                streaming_active = false;
                if (streaming_thread.joinable()) {
                    streaming_thread.join();
                }
                
                sendResponse(sock, "OK: Data streaming stopped");
                std::cout << "[Command]: STOP_STREAM - Stopped data streaming" << std::endl;
                return true;
            });
        
        registerCommand("GET_LOG", "Get last 120 log entries from sensor data history", CommandType::SERVER,
            [this](int sock, const std::string& cmd) -> bool {
                std::string log_entries = logger->getLastEntries(120);
                sendResponse(sock, log_entries);
                std::cout << "[Command]: GET_LOG - Sent " << logger->getTotalEntries() 
                          << " log entries to client" << std::endl;
                return true;
            });
        
        registerCommand("GET_DANGER_LOG", "Get last 120 dangerous events from sensor data history", CommandType::SERVER,
            [this](int sock, const std::string& cmd) -> bool {
                std::string danger_entries = logger->getLastDangerEntries(120);
                sendResponse(sock, danger_entries);
                std::cout << "[Command]: GET_DANGER_LOG - Sent " << logger->getTotalDangerEntries() 
                          << " dangerous events to client" << std::endl;
                return true;
            });
        
        registerCommand("QUIT", "Disconnect from server", CommandType::TERMINATE,
            [this](int sock, const std::string& cmd) -> bool {
                if (streaming_active) {
                    streaming_active = false;
                    if (streaming_thread.joinable()) {
                        streaming_thread.join();
                    }
                }
                sendResponse(sock, "OK:Disconnected");
                std::cout << "[Command]: QUIT - Client disconnected" << std::endl;
                return false;
            });
    }
    
    void registerCommand(const std::string& command, const std::string& description, 
                        CommandType type, std::function<bool(int, const std::string&)> handler) {
        commands[command] = handler;
        commandDescriptions[command] = description;
        
        if (type == CommandType::TERMINATE) {
            terminateCommands.insert(command);
        }
    }
    
    bool handleCommand(int socket, const std::string& message, bool& shouldExit) {
        auto it = commands.find(message);
        if (it != commands.end()) {
            bool result = it->second(socket, message);
            if (!result) {
                shouldExit = true;
            }
            return true;
        }
        
        std::string error = "ERROR:Unknown command. Type HELP for available commands.";
        send(socket, error.c_str(), error.length(), 0);
        std::cout << "[Command]: Unknown command from client: " << message << std::endl;
        return false;
    }
    
    bool isTerminateCommand(const std::string& message) const {
        return terminateCommands.find(message) != terminateCommands.end();
    }
    
    void cleanup() {
        if (streaming_active) {
            streaming_active = false;
            if (streaming_thread.joinable()) {
                streaming_thread.join();
            }
        }
    }
};

class ClientHandler {
private:
    int client_socket;
    CommandHandler& cmdHandler;
    std::atomic<bool>& running;
    
    void log(const std::string& message) {
        std::cout << message << std::endl;
    }
    
    std::string receiveFromClient() {
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        
        ssize_t received_bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (received_bytes > 0) {
            buffer[received_bytes] = '\0';
            std::string message(buffer);
            message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());
            message.erase(std::remove(message.begin(), message.end(), '\r'), message.end());
            return message;
        } else if (received_bytes == 0) {
            return "Client disconnected";
        } else {
            if (errno != ECONNRESET && errno != EAGAIN) {
                log("[Error]: Error receiving from client: " + std::string(strerror(errno)));
            }
            return "ERROR";
        }
    }

public:
    ClientHandler(int sock, CommandHandler& handler, std::atomic<bool>& run)
        : client_socket(sock), cmdHandler(handler), running(run) {}
    
    ~ClientHandler() {
        cmdHandler.cleanup();
    }
    
    void run() {
        log("[System]: Started");

        bool client_active = true;
        bool shouldExit = false;
        
        while (running && client_active && !shouldExit) {
            std::string command = receiveFromClient();
            
            if (command.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            
            if (command == "ERROR") {
                log("[System]: Connection error, closing client session");
                break;
            }
            
            if (command == "Client disconnected") {
                log("[System]: Client disconnected");
                break;
            }
            
            cmdHandler.handleCommand(client_socket, command, shouldExit);
            
            if (shouldExit) {
                log("[System]: Client requested disconnect");
                client_active = false;
            }
        }
        
        cmdHandler.cleanup();
        log("[System]: Stopped");
    }
};

int create_socket() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
    }
    return server_socket;
}

sockaddr_in configure_socket() {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);
    return address;
}

bool bind_socket(int server_socket, const sockaddr_in& address) {
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    if (bind(server_socket, (sockaddr*)&address, sizeof(address)) == -1) {
        std::cerr << "Bind failed: " << strerror(errno) << "\n";
        return false;
    }
    return true;
}

bool start_listening(int server_socket) {
    if (listen(server_socket, SOMAXCONN) == -1) {
        std::cerr << "Listen failed: " << strerror(errno) << "\n";
        return false;
    }
    return true;
}

int accept_connection(int server_socket) {
    sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_socket = accept(server_socket, (sockaddr*)&client_addr, &addr_len);
    
    if (client_socket == -1) {
        if (errno != EINTR) {
            std::cerr << "Accept failed: " << strerror(errno) << "\n";
        }
    } else {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        std::cout << "[System]: Client connected from " << client_ip << ":" 
                  << ntohs(client_addr.sin_port) << std::endl;
    }
    return client_socket;
}

void close_socket(int server_socket, int client_socket) {
    if (client_socket != -1) {
        shutdown(client_socket, SHUT_RDWR);
        close(client_socket);
    }
    if (server_socket != -1) {
        close(server_socket);
    }
}

int main() {
    signal(SIGINT, signal_handler);
    
    std::cout << "Starting server with mock sensor data and automatic logging...\n" << std::endl;
    
    MockSensorData mockSensor;
    
    DataLogger logger([&mockSensor]() { return mockSensor.getCurrentSensorData(); }, 
                      std::chrono::seconds(5));
    
    SensorData initialData = mockSensor.getCurrentSensorData();
    initialData.checkDangerous(logger.getNormalRanges());
    std::cout << "[System]: Initial sensor data: " << initialData.toFormattedString() << std::endl;
    std::cout << "[System]: Automatic logging started...\n" << std::endl;

    CommandHandler cmdHandler(&mockSensor, &logger);
    
    int server_socket = create_socket();
    if (server_socket == -1) {
        return 1;
    }
    
    sockaddr_in address = configure_socket();
    if (!bind_socket(server_socket, address)) {
        close_socket(server_socket, -1);
        return 1;
    }
    
    if (!start_listening(server_socket)) {
        close_socket(server_socket, -1);
        return 1;
    }
    
    std::cout << "[System]: Server listening on port " << SERVER_PORT << std::endl;
    std::cout << "[System]: Waiting for client connections...\n" << std::endl;
    
    while (server_running) {
        int client_socket = accept_connection(server_socket);
        
        if (client_socket == -1) {
            if (server_running) {
                std::cerr << "[Error]: Failed to accept connection" << std::endl;
            }
            continue;
        }
        
        std::atomic<bool> client_active{true};
        ClientHandler handler(client_socket, cmdHandler, client_active);
        
        std::thread client_thread([&handler]() {
            handler.run();
        });
        
        client_thread.join();
        close(client_socket);
        
        if (server_running) {
            std::cout << "[System]: Client disconnected. Waiting for next client..." << std::endl;
        }
    }
    
    logger.stop();
    close_socket(server_socket, -1);
    std::cout << "\n[System]: Server stopped" << std::endl;
    
    return 0;
}