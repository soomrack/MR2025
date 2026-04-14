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
#include <mutex>
#include <termios.h>  

const unsigned short SERVER_PORT = 8080;
const std::string LOG_FILE = "sensor_data.log";
const std::string DANGER_LOG_FILE = "dangerous_data.log";
const size_t MAX_LOG_ENTRIES = 120;

const std::string UART_PORT = "/dev/ttyACM0"; 
const speed_t UART_BAUD_RATE = B9600; 

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
        auto time_t_value = std::chrono::system_clock::to_time_t(timestamp);
        ss << std::put_time(std::localtime(&time_t_value), "%Y-%m-%d %H:%M:%S");
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

class ArduinoUART {
private:
    int uart_fd;
    std::mutex uart_mutex;
    std::atomic<bool> connected;
    
    bool configureUART(int fd) {
        struct termios tty;
        
        if (tcgetattr(fd, &tty) != 0) {
            std::cerr << "[UART] Failed to get terminal attributes: " << strerror(errno) << std::endl;
            return false;
        }

        cfsetospeed(&tty, UART_BAUD_RATE);
        cfsetispeed(&tty, UART_BAUD_RATE);

        tty.c_cflag &= ~PARENB;  
        tty.c_cflag &= ~CSTOPB; 
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;   
        tty.c_cflag &= ~CRTSCTS;  
        tty.c_cflag |= CREAD | CLOCAL;  
        
        tty.c_lflag &= ~ICANON;   
        tty.c_lflag &= ~ECHO;    
        tty.c_lflag &= ~ECHOE;
        tty.c_lflag &= ~ECHONL;
        tty.c_lflag &= ~ISIG;     
        
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);  
        tty.c_iflag &= ~(INLCR | ICRNL | IGNCR); 
        
        tty.c_oflag &= ~OPOST;    
        
        
        tty.c_cc[VMIN] = 0;      
        tty.c_cc[VTIME] = 10;     
        
        if (tcsetattr(fd, TCSANOW, &tty) != 0) {
            std::cerr << "[UART] Failed to set terminal attributes: " << strerror(errno) << std::endl;
            return false;
        }
        
        return true;
    }
    
public:
    ArduinoUART() : uart_fd(-1), connected(false) {}
    
    ~ArduinoUART() {
        disconnect();
    }
    
    bool connect() {
        uart_fd = open(UART_PORT.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
        if (uart_fd < 0) {
            std::cerr << "[UART] Failed to open port " << UART_PORT 
                      << ": " << strerror(errno) << std::endl;
            return false;
        }
        
        if (!configureUART(uart_fd)) {
            close(uart_fd);
            uart_fd = -1;
            return false;
        }
        
        connected = true;
        std::cout << "[UART] Connected to Arduino on " << UART_PORT << std::endl;
        
        tcflush(uart_fd, TCIFLUSH);
        
        return true;
    }
    
    void disconnect() {
        if (uart_fd != -1) {
            close(uart_fd);
            uart_fd = -1;
            std::cout << "[UART] Disconnected from Arduino" << std::endl;
        }
        connected = false;
    }
    
   SensorData readSensorData() {
    SensorData data;
    data.temperature = 0.0f;
    data.humidity = 0.0f;
    data.light = 0.0f;
    data.soil_moisture = 0.0f;
    data.timestamp = std::chrono::system_clock::now();
    
    if (!connected) {
        return data;
    }
    
    std::lock_guard<std::mutex> lock(uart_mutex);
    
    // Отправляем символ '1' для запроса данных
    char request = '1';
    ssize_t written = write(uart_fd, &request, 1);
    if (written != 1) {
        std::cerr << "[UART] Failed to send request: " << strerror(errno) << std::endl;
        return data;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytes_read = read(uart_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            std::cerr << "[UART] No response from Arduino" << std::endl;
        } else {
            std::cerr << "[UART] Read error: " << strerror(errno) << std::endl;
        }
        return data;
    }
    
    buffer[bytes_read] = '\0';
    std::string response(buffer);

    response.erase(std::remove(response.begin(), response.end(), '\r'), response.end());
    response.erase(std::remove(response.begin(), response.end(), '\n'), response.end());
    
    if (response.empty()) {
        std::cerr << "[UART] Empty response from Arduino" << std::endl;
        return data;
    }

    std::stringstream ss(response);
    float temp, humidity, light, soil_moisture;
    
    if (ss >> temp >> humidity >> light >> soil_moisture) {
        data.temperature = temp;
        data.humidity = humidity;
        data.light = light;
        data.soil_moisture = soil_moisture;

    } else {
        std::cerr << "[UART] Failed to parse data: '" << response << "'" << std::endl;
    }
    
    return data;
}
    
    bool isConnected() const {
        return connected;
    }
};

class RealSensorData {
private:
    ArduinoUART& arduino;
    SensorData lastData;
    std::mutex data_mutex;
    
public:
    RealSensorData(ArduinoUART& uart) : arduino(uart) {
        lastData.timestamp = std::chrono::system_clock::now();
    }
    
    SensorData getSensorData() {
        std::lock_guard<std::mutex> lock(data_mutex);
        SensorData newData = arduino.readSensorData();
        if (newData.temperature != 0.0f || newData.humidity != 0.0f || 
            newData.light != 0.0f || newData.soil_moisture != 0.0f) {
            lastData = newData;
        }
        return lastData;
    }
    
    SensorData getCurrentSensorData() {
        std::lock_guard<std::mutex> lock(data_mutex);
        return lastData;
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
    mutable std::mutex log_mutex;
    
    void logData() {
        while (logging_active) {
            std::this_thread::sleep_for(logging_interval);
            
            if (!logging_active) break;
            
            SensorData data = dataProvider();
            data.checkDangerous(normal_ranges);
            
            std::lock_guard<std::mutex> guard(log_mutex);

            if (logFile.is_open()) {
                std::string log_entry = data.toCsvString();
                logFile << log_entry << std::endl;
                logFile.flush();
                
                log_cache.push_back(log_entry);
                if (log_cache.size() > MAX_LOG_ENTRIES) {
                    log_cache.pop_front();
                }
            }

            if (data.is_dangerous && dangerLogFile.is_open()) {
                std::string danger_entry = data.toDangerCsvString();
                dangerLogFile << danger_entry << std::endl;
                dangerLogFile.flush();
                
                danger_log_cache.push_back(danger_entry);
                if (danger_log_cache.size() > MAX_LOG_ENTRIES) {
                    danger_log_cache.pop_front();
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
        << " " << std::setw(12) << "Temp(°C)" 
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
            << " " << std::setw(12) << temp
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
        ss << "----------------------------------------------------------------------------------------------------\n";
        ss << std::left << std::setw(20) << "Timestamp" 
        << " " << std::setw(12) << "Temp(°C)" 
        << std::setw(12) << "Hum(%)" 
        << std::setw(15) << "Soil(%)" 
        << std::setw(10) << "Light(lux)" << "\n";
        ss << "----------------------------------------------------------------------------------------------------\n";
        
        for (size_t i = start; i < danger_log_cache.size(); ++i) {
            std::string entry = danger_log_cache[i];
            std::stringstream entry_ss(entry);
            std::string timestamp, temp, hum, soil, light;
            
            std::getline(entry_ss, timestamp, ',');
            std::getline(entry_ss, temp, ',');
            std::getline(entry_ss, hum, ',');
            std::getline(entry_ss, soil, ',');
            std::getline(entry_ss, light, ',');
            
            ss << std::left << std::setw(20) << timestamp
            << " " << std::setw(12) << temp
            << std::setw(12) << hum
            << std::setw(15) << soil
            << std::setw(10) << light << "\n";
        }
        
        ss << "----------------------------------------------------------------------------------------------------\n";
        ss << "Total dangerous events: " << danger_log_cache.size() << "\n";
        
        return ss.str();
    }
    
    size_t getTotalEntries() const { return log_cache.size(); }
    size_t getTotalDangerEntries() const { return danger_log_cache.size(); }
    NormalRanges getNormalRanges() const { return normal_ranges; }
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
    RealSensorData* realSensor;
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
            SensorData sensorData = realSensor->getSensorData();
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
    CommandHandler(RealSensorData* sensor, DataLogger* dataLogger) : realSensor(sensor), logger(dataLogger) {        
        registerCommand("GET_DATA", "Get sensor data (formatted)", CommandType::SERVER,
            [this](int sock, const std::string& cmd) -> bool {
                SensorData sensorData = realSensor->getSensorData();
                sensorData.checkDangerous(logger->getNormalRanges());
                std::string response = sensorData.toFormattedString();
                sendResponse(sock, response);
                std::cout << "[Command]: GET_DATA - Sent sensor data: " 
                          << sensorData.toFormattedString() << std::endl;
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

        registerCommand("SHUTDOWN", "Shutdown the server gracefully", CommandType::SERVER,
            [this](int sock, const std::string& cmd) -> bool {
                sendResponse(sock, "OK: Server is shutting down...");
                std::cout << "[Command]: SHUTDOWN - Server shutdown requested by admin" << std::endl;
                server_running = false;  
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
        
        //std::string error = "ERROR:Unknown command. Available: GET_DATA, START_STREAM, STOP_STREAM, GET_LOG, GET_DANGER_LOG, QUIT, SHUTDOWN";
        //send(socket, error.c_str(), error.length(), 0);
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
        char buffer[8192];
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
    
    std::cout << "Starting server...\n" << std::endl;

    ArduinoUART arduino;
    if (!arduino.connect()) {
        std::cerr << "[FATAL] Failed to connect to Arduino on " << UART_PORT << std::endl;
        std::cerr << "Check: 1) Arduino connected? 2) Correct port? 3) Permissions?" << std::endl;
        std::cerr << "Try: sudo chmod 666 " << UART_PORT << std::endl;
        return 1;
    }

    RealSensorData realSensor(arduino);

    DataLogger logger([&realSensor]() { return realSensor.getSensorData(); }, 
                      std::chrono::seconds(5));
    
    // Тестовое чтение
    std::cout << "[System] Testing Arduino communication..." << std::endl;
    SensorData testData = realSensor.getSensorData();
    if (testData.temperature != 0.0f || testData.humidity != 0.0f) {
        std::cout << "[System] Successfully received data from Arduino!" << std::endl;
        std::cout << "[System] Initial sensor data: " << testData.toFormattedString() << std::endl;
    } else {
        std::cout << "[Warning] No valid data from Arduino yet. Check Arduino." << std::endl;
    }
    std::cout << "[System] Automatic logging started (every 5 seconds)\n" << std::endl;
    
    CommandHandler cmdHandler(&realSensor, &logger);
    
    int server_socket = create_socket();
    if (server_socket == -1) {
        arduino.disconnect();
        return 1;
    }
    
    sockaddr_in address = configure_socket();
    if (!bind_socket(server_socket, address)) {
        close_socket(server_socket, -1);
        arduino.disconnect();
        return 1;
    }
    
    if (!start_listening(server_socket)) {
        close_socket(server_socket, -1);
        arduino.disconnect();
        return 1;
    }
    
    std::cout << "[System] Server listening on port " << SERVER_PORT << std::endl;
    std::cout << "\n[System] Waiting for client connections...\n" << std::endl;
    
    while (server_running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(server_socket + 1, &readfds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            if (errno != EINTR) {
                std::cerr << "[Error]: Select failed" << std::endl;
            }
            continue;
        }
        
        if (activity == 0) {
            continue;
        }
        
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
    
    std::cout << "\n[System]: Initiating graceful shutdown..." << std::endl;
    logger.stop();
    arduino.disconnect();
    close_socket(server_socket, -1);
    std::cout << "[System]: Server stopped" << std::endl;
    
    return 0;
}