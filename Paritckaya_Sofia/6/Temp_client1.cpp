#define _WIN32_WINNT 0x0600  // #define - директива препроцессора для определения макроса, "имя_пробел_значение"; определяет версию windows
#define _CRT_SECURE_NO_WARNINGS // отключает предупреждения о небезопасных функциях CRT
#define _WINSOCK_DEPRECATED_NO_WARNINGS // отключает предупреждения об устаревших функциях Winsock
#define WIN32_LEAN_AND_MEAN // уменьшает размер включаемых заголовочных файлов Windows

#include <winsock2.h> // #include - директива вклюения, "<>" - поиск в системных каталогах
#include <ws2tcpip.h> // включает дополнительные заголовки Winsock
#include <windows.h> // Windows API
#include <iostream> // ввод-вывод
#include <string> // строки
#include <fstream> // операции с файлами
#include <ctime> // время
#include <conio.h> // консольный ввод-вывод
#include <sstream> // строковые потоки
#include <cctype> // функции классификация символов
#include <iomanip> // манипуляторы форматирования вывода
#include <vector> // для контейнера
#include <algorithm> // для алгоритмов STL
#include <memory> // для умных указателей

#pragma comment(lib, "ws2_32.lib") // #pragma - директива компилятора, comment - тип прагмы, (и) -скобки, содержащие аргументы прагмы, lib - тип комментария(подключение библиотеки)

using namespace std; // using - ключевое слово для использования пространства имен, std - стандартное пространство имен C++

// Конфигурация
const double OVERHEAT_THRESHOLD = 51.0; // const - значение нельзя изменить, double - тип данных (число с плавающей точкой двойной точности), = - оператор присваивания, ; - завершение оператора

// Базовый класс для всех полей лога 
class LogField { // class - ключ.слово для определения класса, далее - имя класса, {} - начало и конец тела класса
public: // спецификатор доступа - открытые члены класса
    virtual ~LogField() = default; // virtual - виртуальный деструктор (обозначение тильда ~) - специальный метод класса, который автоматически вызывается, когда объект уничтожается, LogField - имя конструктора, default - по умолч.
    virtual string get_name() const = 0; // virtual - виртуальная функция (обеспечивает полиморфизм), string - тип возвращаемого знач-я, () - пустой список параметров - отсутствие аргументов, const - функция не изменяет объект, 0 - функция (абстрактная), метод get_name объявлен, но не используется.
    virtual string get_value() const = 0; //get_value - имя метода (получает знач-е поля)
    virtual void parse_from_line(const string& line) = 0; // void - ничего не возвращает, parse_from_line - имя метода (разобрать из строки), const string - не изменит переданную строку, & - амперсанд, ссылка (передача по ссылке, чтобы избежать копирования), line - имя параметра (строка лога для парсинга); абстрактный метод, который извлекает значение конкретного поля из строки лога
    virtual string format_for_display() const = 0; //format_for_display - имя метода (форматировать для отображения); возвращает "красиво" отформатированное значение для вывода пользователю
    virtual unique_ptr<LogField> clone() const = 0; // unique_ptr<LogField> - умный указатель с шаблонным параметром, "<>" - в них параметры шаблона
}; // (Благодаря этим чисто виртуальным методам, класс LogEntry может работать с любым типом поля, не зная его конкретной реализации)

// Конкретные реализации полей
class TimestampField : public LogField { // : - двоеточие, начало списка базовых классов, public - спецификатор наследования (из базового класса LogField) 
private: // спецификатор доступа - закрытые члены класса
    string value; // тип данных и тип переменной
public:
    string get_name() const override { return "Timestamp"; } //override - ключ.слово, указывает на переопределение виртуальной функции, return - оператор возврата
    string get_value() const override { return value; }

    void parse_from_line(const string& line) override {
        size_t time_start = line.find('[') + 1; //size_t - беззнаковый целочисленный тип, time_start - имя переменной, вызов метода find у объекта line, . - точка, оператор доступа к объекту, () - вызов функции, '[' - символьный литерал (одинарные кавычки)
        size_t time_end = line.find(']', time_start); //line.find(']', time_start) - поиск с указанной позиции
        if (time_start != string::npos && time_end != string::npos) { // () - условие, != - сравнение не равно, string::npos - статическая константа класса string (макс.значение size_t), :: - оператор разрешения области видимости, && - логическое И
            value = line.substr(time_start, time_end - time_start); //line.substr(...) - метод получения подстроки, time_start - первый аргумент (нач. позиция), time_end - time_start - вычисление длины
        }
    }

    string format_for_display() const override { //format_for_display - имя метода, const - метод не изменяет объект
        return value; //value - имя переменной (член класса TimestampField), которая хранит временную метку
    }

    unique_ptr<LogField> clone() const override { //unique_ptr<LogField> - тип возвращаемого значения (умный указатель на базовый класс LogField), clone - имя метода (создание копии объекта)
        return make_unique<TimestampField>(*this); //функция для создания unique_ptr:, TimestampField - тип создаваемого объекта, *this - разыменование указателя this, this - указатель на текущий объект, * -оператор разыменования(получаем сам объект, а не указатель на него)
    }
};

class TemperatureField : public LogField {
private:
    double value = 0.0; //value - имя переменной (будет хранить температуру)
public:
    string get_name() const override { return "Temperature"; } //возвращает имя поля
    string get_value() const override { return to_string(value); } //функция преобразования числа в строку, value - переменная, хранящая температуру

    void parse_from_line(const string& line) override {
        size_t pos = line.find("TEMP:"); //pos - имя переменной (позиция найденной подстроки), поиск подстроки "TEMP:" в строке line
        if (pos != string::npos) { // проверка, найдена ли подстрока; string::npos - специальная константа (максимальное значение size_t), означающая "не найдено"
            value = stod(line.substr(pos + 5)); //функция "string to double" (преобразует строку в число), line.substr(pos + 5) - извлечение подстроки, pos + 5 - начальная позиция (pos указывает на начало "TEMP:", +5 пропускает эти 5 символов)
        }
    }

    string format_for_display() const override {
        stringstream ss; //класс для строковых потоков и имя объекта
        ss << fixed << setprecision(2) << value << "°C"; //<< - оператор вставки в поток (сдвиг влево), fixed - манипулятор для фиксированного формата чисел, setprecision(2) - манипулятор для установки точности (2 знака после запятой)
        return ss.str(); //метод, возвращающий строку из потока
    }

    double get_numeric_value() const { return value; }

    unique_ptr<LogField> clone() const override { //clone - имя метода
        return make_unique<TemperatureField>(*this); //функция для создания unique_ptr для TemperatureField
    }
};

class FrequencyField : public LogField {
private:
    int value = 0; //int - целочисленный тип (частота в MHz - целое число)
public:
    string get_name() const override { return "CPU Frequency"; }
    string get_value() const override { return to_string(value); }

    void parse_from_line(const string& line) override { //const string& line - константная ссылка на строку лога (см.выше)
        size_t pos = line.find("FREQ:"); //поиск подстроки "FREQ:" в строке line
        if (pos != string::npos) {
            string freq_str = line.substr(pos + 5); //freq_str - имя переменной (хранит часть строки после "FREQ:")
            size_t mhz_pos = freq_str.find("MHz"); //mhz_pos - переменная для позиции "MHz" в полученной строке
            if (mhz_pos != string::npos) {
                value = stoi(freq_str.substr(0, mhz_pos)); //stoi(...) - функция "string to integer" (преобразует строку в целое число), извлечение подстроки от начала до позиции
            }
        }
    }

    string format_for_display() const override {
        return to_string(value) + "MHz";
    }

    unique_ptr<LogField> clone() const override {
        return make_unique<FrequencyField>(*this); // создает копию текущего объекта (см.выше)
    }
};

class LoadField : public LogField { //класс для загрузки процессора
private:
    double value = 0.0; //value - переменная для хранения загрузки (обычно в процентах)
public:
    string get_name() const override { return "Load"; }
    string get_value() const override { return to_string(value); }

    void parse_from_line(const string& line) override {
        size_t pos = line.find("LOAD:"); //Поиск "LOAD:" в строке
        if (pos != string::npos) {
            value = stod(line.substr(pos + 5));
        }
    }

    string format_for_display() const override {
        stringstream ss; //создание строкового потока
        ss << fixed << setprecision(2) << value;
        return ss.str(); //возврат строки из потока
    }

    unique_ptr<LogField> clone() const override {
        return make_unique<LoadField>(*this);
    }
};

// Здесь легко регистрируется поле
class FieldFactory { //(создает и управляет полями лога)
private:
    vector<unique_ptr<LogField>> field_prototypes; //vector<...> - шаблонный контейнер (динамический массив), unique_ptr<LogField> - тип элементов (умные указатели на базовый класс LogField), field_prototypes - имя переменной (хранит прототипы всех возможных полей)

public:
    FieldFactory() { //имя конструктора, без параметров
        // Регистрируем все доступные поля
        field_prototypes.push_back(make_unique<TimestampField>()); //field_prototypes - имя вектора, push_back - метод вектора для добавления элемента в конец, () - его аргумент
        field_prototypes.push_back(make_unique<TemperatureField>());
        field_prototypes.push_back(make_unique<FrequencyField>());
        field_prototypes.push_back(make_unique<LoadField>());
        // каждая строка добавляет в вектор прототип соответствующего класса
    }

    vector<unique_ptr<LogField>> create_all_fields() const { //vector<unique_ptr<LogField>> - возвращаемый тип (вектор умных указателей на LogField), field_prototypes остается неизменным
        vector<unique_ptr<LogField>> fields; //тип локал. переменной и имя переменной (пустой вектор для рез-в)
        for (const auto& prototype : field_prototypes) { //const auto& prototype - объявление переменной для каждого элемента, & - ссылка(без копирования), prototype - имя переменной (будет ссылаться на каждый прототип по очереди), : - двоеточие, разделяет объявление и коллекцию, field_prototypes - коллекция для итерации
            fields.push_back(prototype->clone()); //fields - вектор, куда добавляем новые поля, (prototype->clone() - создание копии прототипа: -> - оператор доступа к члену через указатель (стрелка), prototype - умный указатель на прототип
        }
        return fields; //fields - возвращаемый вектор с новыми полями
    }
};

// Класс записи лога
class LogEntry {
private:
    vector<unique_ptr<LogField>> fields; //fields - имя переменной (хранит все поля одной записи лога)
    //Для каждой записи лога создается вектор из 4 полей (Timestamp, Temperature, Frequency, Load)

public:
    LogEntry(const FieldFactory& factory) { //FieldFactory - тип, factory - имя параметра
        auto all_fields = factory.create_all_fields(); //auto - автоматическое определение типа, all_fields - имя локал. переменной, factory.create_all_fields() - вызов метода фабрики, который создает все поля 
        for (auto& field : all_fields) { //auto& field - переменная-ссылка на каждый элемент, all_fields - коллекция для обхода
            fields.push_back(move(field)); //fields - член класса (вектор, куда сохраняем поля), push_back - метод добавления в конец, move(field) - перемещение владения unique_ptr: move - стандарт. ф-я, field - перемещаемый указатель
        } //unique_ptr нельзя копировать (только перемещать). move передает владение объектом из all_fields в fields
    }

    // Конструктор копирования
    LogEntry(const LogEntry& other) { //LogEntry - имя конструктора, const LogEntry& other - константная ссылка на другой объект LogEntry
        for (const auto& field : other.fields) { //const auto& field - константная ссылка на поле: auto& - ссылка на unique_ptr, field - имя переменной, other.fields - поля копируемого объекта
            fields.push_back(field->clone()); //fields - вектор текущего объекта (куда копируем), push_back - добавление, field->clone() - создание копии поля : field - указатель на оригинальное поле,  -> - доступ к члену через указатель
        } //Создаем глубокие копии всех полей, а не копируем указатели
    } 



    void parse_from_line(const string& line) { //const string& line - константная ссылка на строку лога
        for (auto& field : fields) { //Цикл по всем полям текущей записи, auto& field - ссылка на каждый unique_ptr
            field->parse_from_line(line); //Вызов метода parse_from_line для каждого поля
            //Каждое поле само решает, как извлечь свои данные из строки
        }
    }

    string format_for_display() const {
        stringstream ss;
        bool first = true; //Флаг для отслеживания первого поля. сначала это первое поле
        for (const auto& field : fields) { //Цикл по всем полям (константные ссылки, т.к. не изменяем)
            if (!first) ss << " | "; //(!first) - если это не первое поле (инверсия first) - то добавляем разделитель " | " в поток
            ss << field->format_for_display();// Добавляем отформатированное значение поля.  Каждое поле форматируется по - своему
            first = false; //После первого поля сбрасываем флаг. Теперь это не первое поле
        }
        return ss.str(); // возврат собранной строки, преобразованной из потока
    }

    bool is_overheat(double threshold) const { //метод - проверка перегрева, threshold - порог температуры
        for (const auto& field : fields) { //Цикл по всем полям
            TemperatureField* temp_field = dynamic_cast<TemperatureField*>(field.get()); //TemperatureField* - указатель на TemperatureField, dynamic_cast<TemperatureField*> - безопасное приведение типа (целевого указателя), dynamic_cast - оператор приведения с проверкой типа, field.get() - получение сырого указателя из unique_ptr, get() - метод, возвращающий сырой указатель
            //Если field указывает на объект TemperatureField, dynamic_cast вернет указатель на него. Если нет - вернет nullptr.
            if (temp_field) { // Проверка, успешно ли привелось (не nullptr)
                return temp_field->get_numeric_value() > threshold; //temp_field->get_numeric_value() - получение числового значения температуры
            }
        }
        return false; //Если TemperatureField не найден, возвращаем false
    }

    string get_timestamp() const { //Возвращает временную метку как строку
        for (const auto& field : fields) { //Цикл по всем полям
            TimestampField* ts_field = dynamic_cast<TimestampField*>(field.get()); //аналогично (см.выше)
            if (ts_field) {
                return ts_field->get_value(); //Возвращаем значение временной метки
            }
        }
        return ""; //Если не найдено, возвращаем пустую строку
    }

    double get_temperature() const { //Возвращает температуру как число
        for (const auto& field : fields) { //Цикл по всем полям
            TemperatureField* temp_field = dynamic_cast<TemperatureField*>(field.get()); //аналогично (см.выше)
            if (temp_field) {
                return temp_field->get_numeric_value(); //Возвращаем числовое значение температуры
            }
        }
        return 0.0; //Если не найдено, возвращаем 0.0
    }
};

// Класс для парсинга логов
class LogParser {
private:
    FieldFactory field_factory; // тип и имя переменной
    //Парсер использует фабрику для создания объектов LogEntry с правильным набором полей

public:
    vector<LogEntry> parse_logs(const string& log_text) { //vector<LogEntry> - возвращаемый тип (вектор записей лога), parse_logs - имя метода (разобрать логи), const string& log_text - параметр, где тип, ссылка и имя параметра (текст лога для парсинга)
        vector<LogEntry> entries; //тип и имя локал.переменной (пустой вектор для результатов)
        stringstream ss(log_text); // ss - имя переменной (поток), log_text - строка для инициализации потока
       //Создает поток из строки лога, чтобы можно было читать построчно
        string line; //line - имя переменной (хранит текущую строку)

        while (getline(ss, line)) { //цикл с предусловием, getline(ss, line) - функция чтения строки из потока, а именно ss - поток для чтения, line - строка для записи результата
           //Читает по одной строке из потока, пока они есть. getline возвращает ссылку на поток, который преобразуется в bool (true, если чтение успешно)
            if (line.find("[METRIC] TEMP:") != string::npos) { //line.find("[METRIC] TEMP:") - поиск подстроки в строке, "[METRIC] TEMP:" - искомая подстрока (маркер метрики с температурой), string::npos - константа "не найдено"
                LogEntry entry(field_factory); // Создается новый объект LogEntry, который через фабрику создает внутри себя все 4 поля (Timestamp, Temperature, Frequency, Load)
                entry.parse_from_line(line); // Каждое поле внутри entry извлекает свое значение из строки
                entries.push_back(entry); //entries - вектор результатов, entry - заполненная запись лога
            }
        }

        return entries;//возвращает entries - вектор с распарсенными записями
    }
};

// Класс для анализа перегрева
class OverheatAnalyzer {
private:
    double threshold;

public:
    OverheatAnalyzer(double thr) : threshold(thr) {} //OverheatAnalyzer - конструктор, double thr - параметр (пороговое значение), : - двоеточие, начало списка инициализации, threshold(thr) - инициализация члена threshold значением thr

    string format_warnings(const vector<LogEntry>& entries) { //const vector<LogEntry>& entries - константная ссылка на вектор записей
        stringstream result; // создание строкового потока для построения отчета
        int warning_count = 0; //счетчик предупреждений
        double max_temp = -1000.0; //инициализация очень маленьким числом
        string max_temp_time; //переменная для времени максимальной температуры

        result << "\n" << string(50, '=') << endl; //result - строковый поток, << -оператор вставки, создание строки из 50 символов '=', endl - конец строки (перевод строки + сброс буфера)
        result << "!!! OVERHEAT WARNINGS (Temperature > " << fixed << setprecision(1) << threshold << "°C) !!!" << endl;
        result << string(50, '=') << endl; // разделительная линия

        for (const auto& entry : entries) { //цикл по всем записям, const auto& entry - константная ссылка на каждую запись, entries - коллекция для обхода после разделителя
            if (entry.is_overheat(threshold)) { // Проверка, была ли в этой записи температура выше порога, is_overheat(threshold) - метод LogEntry, возвращает true/false
                warning_count++; //warning_count++ - инкремент счетчика (увеличение на 1)
                result << ">> " << entry.format_for_display() << endl; // добавляем в отчет отформатированную запись с префиксом ">> "

                double temp = entry.get_temperature(); // Получаем температуру из записи
                if (temp > max_temp) { // если текущая больше максимальной
                    max_temp = temp; // обновляем максимальную
                    max_temp_time = entry.get_timestamp(); // сохраняем время этой записи
                }
            }
        }

        if (warning_count == 0) { // если не было ни одного перегрева, == - оператор сравнения на равенство
            result << "NO overheating detected in this period" << endl; // сообщение об отсутствии перегрева
        }
        else { // иначе
            result << string(50, '-') << endl; // разделительная линия из дефисов
            result << "SUMMARY:" << endl;
            result << "   Total overheat events: " << warning_count << endl; // вывод статистики
            if (max_temp > -1000) { // Проверка, что нашли хотя бы одну температуру (если max_temp осталась -1000, значит не было записей с температурой)
                result << "   Maximum temperature: " << fixed << setprecision(2) << max_temp
                    << "°C at " << max_temp_time << endl; // вывод макс температуры
            }
            result << "   Threshold: " << fixed << setprecision(1) << threshold << "°C" << endl; // вывод порогового знач-я
        }
         
        result << string(50, '=') << endl; // нижняя граница отчета

        return result.str(); // Возврат собранной строки, str() - метод stringstream, возвращающий строку
    }
};

// Класс для работы с файлами
class FileHelper {
public:
    static string generate_filename(const string& prefix = "temp_log_") { //static - статический метод (можно вызывать без создания объекта), generate_filename - имя метода, const string& prefix - параметр, где константная ссылка, тип, имя параметра (префикс имени файла), "temp_log_" - значение по умолчанию (если параметр не указан)
        time_t now = time(nullptr); // time_t - тип для хранения времени (целое число секунд), now - имя переменной, time(nullptr) - функция получения текущего времени, nullptr - нулевой указатель (не сохраняем время в переменную)
        tm tm_info; // tm - структура для хранения времени в человеко-читаемом формате (год, месяц, день, час, минута, секунда), tm_info - имя переменной
        localtime_s(&tm_info, &now); // localtime_s - безопасная версия функции преобразования времени в локальное, &tm_info - адрес структуры tm (куда сохранить результат), &now - адрес переменной с временем
        char buffer[64]; // char - символьный тип, buffer - имя массива, размер массива (64 символа)
        strftime(buffer, sizeof(buffer), (prefix + "%Y%m%d_%H%M%S.txt").c_str(), &tm_info); //strftime - функция форматирования времени в строку, buffer - куда записывать результат, sizeof(buffer) - размер буфера (чтобы не выйти за границы), (prefix + "%Y%m%d_%H%M%S.txt").c_str() - формат строки, %Y%m%d_%H%M%S.txt" - формат: год месяц день _ час минута секунда .txt, .c_str() - метод, возвращающий C-строку (массив char), &tm_info - указатель на структуру с временем
        return string(buffer); //string(buffer) - создание строки C++ из C-строки buffer
    }

    static bool save(const string& filename, const string& content) { //const string& filename - имя файла, const string& content - содержимое для сохранения
        ofstream file(filename); //ofstream - класс для записи в файл (output file stream), file - имя объекта, filename - имя файла для открытия
        if (!file.is_open()) return false; //проверка, открыт ли файл, is_open() - метод, возвращающий true, если файл открыт
        file << content; // content - строка для записи
        file.close();
        return true;
    }

    static void open_in_notepad(const string& filename) { //const string& filename - имя файла для открытия
        system(("notepad " + filename).c_str()); //system - функция выполнения системной команды, ("notepad " + filename).c_str() - формирование команды: конкатенация строк (объединение нескольких в одну)  И .c_str() - преобразование в C-строку
    }
};

// Класс сетевого клиента
class NetworkClient {
private:
    SOCKET sock; //SOCKET - тип сокета (дескриптор соединения), sock - переменная для хранения сокета
    string server_ip; //IP-адрес сервера
    int port; // порт сервера
    bool connected; // флаг подключения

    string receive_all() { //получение всех данных
        string response; // строка накопления ответа
        char buffer[4096]; // буфер для приема данных (4096 байт)
        int bytes; // переменная для количества полученных байт
        while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) { //присваивание в условии: bytes = - сохраняем результат recv в переменную bytes, recv(sock, buffer, sizeof(buffer) - 1, 0) - функция приема данных, 0 - флаги (нет специальных флагов) 
            //recv возвращает количество принятых байт или 0 при закрытии соединения, или -1 при ошибке. Цикл продолжается, пока получаем данные (>0).
            buffer[bytes] = '\0'; //обращение к элементу массива по индексу bytes, '\0' - нуль-символ (признак конца строки). Добавляем нуль-терминатор, чтобы buffer можно было использовать как C-строку
            response += buffer; //добавляем полученные данные к строке ответа
            if (bytes < sizeof(buffer) - 1) break; //если получено меньше, чем размер буфера, то выход из цикла
        }
        return response;
    }

public:
    NetworkClient(const string& ip, int p) : server_ip(ip), port(p), connected(false) { //NetworkClient - конструктор, const string& ip - IP-адрес, int p - порт, server_ip(ip) - инициализация server_ip значением ip, port(p) - инициализация port, connected(false) - инициализация connected значением false
        sock = INVALID_SOCKET; //INVALID_SOCKET - константа для недействительного сокета
    }

    ~NetworkClient() { //деструктор без параметров
        disconnect(); //вызов метода отключения
    }

    bool connect() {
        WSADATA wsaData; //WSADATA - структура для информации о реализации Winsock, wsaData - переменная
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false; //WSAStartup - инициализация библиотеки Winsock, MAKEWORD(2, 2) - макрос, создающий слово из двух байт (версия 2.2), &wsaData - указатель на структуру для получения информации, != 0 - проверка на ошибку (0 означает успех)

        sock = socket(AF_INET, SOCK_STREAM, 0); // создание сокета (AF_INET - семейство адресов (IPv4), SOCK_STREAM - тип сокета (потоковый, TCP), 0 - протокол (0 = выбирается автоматически)) 
        if (sock == INVALID_SOCKET) {  // Проверка на ошибку создания сокета
            WSACleanup(); // освобождение ресурсов Winsock
            return false;
        }

        sockaddr_in server_addr; // структура для адреса сервера
        server_addr.sin_family = AF_INET; // установка семейства адресов (IPv4)
        server_addr.sin_port = htons(port); //host to network short (преобразование порта в сетевой порядок байт)
        server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str()); //inet_addr - преобразование строки IP в 32-битное число, server_ip.c_str() - преобразование string в C-строку

        if (server_addr.sin_addr.s_addr == INADDR_NONE) { //INADDR_NONE - константа, означающая неверный IP-адрес
            closesocket(sock);
            WSACleanup();
            return false;
        }

        if (::connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) { //::connect - глобальная функция connect (префикс :: для избежания конфликта имен), (sockaddr*)&server_addr - приведение типа к sockaddr* (требуется функцией), SOCKET_ERROR - константа, означающая ошибку
            closesocket(sock);
            WSACleanup();
            return false;
        }

        connected = true;
        return true;
    }

    void disconnect() { // метод отключения
        if (connected) {
            closesocket(sock);
            WSACleanup();
            connected = false;
        }
    }

    string send_command(const string& cmd) {
        if (!connected) return ""; // если не подключено, то возвращаем пустую строку
        send(sock, cmd.c_str(), cmd.length(), 0); //cmd.c_str() - преобразование команды в C-строку, cmd.length() - длина команды, флаги 0
        return receive_all();
    }

    bool is_connected() const { return connected; } // проверка подключения, возвращает состояние подключения
};

// Класс для отображения пользовательского интерфейса
class UI {
public: //открытая секция (все методы статические, доступны без создания объекта)
    static void print_header() { //вывод заголовка
        cout << "\n=== Raspberry Pi Temperature Monitor ===" << endl; //\n - (перевод строки)
        cout << "Overheat threshold: " << fixed << setprecision(1) << OVERHEAT_THRESHOLD << "°C" << endl; //OVERHEAT_THRESHOLD - глобальная константа (51.0)
        cout << "Commands:" << endl;
        cout << "  1 - Get current temperature" << endl;
        cout << "  2 - Get recent logs" << endl;
        cout << "  3 - Get statistics (with time period)" << endl;
        cout << "  4 - Help" << endl;
        cout << "  Q - Quit" << endl; //вывод списка доступных команд
        cout << "=====================================\n" << endl; 
    }

    static void print_stats_help() { // справка по статистике. Выводит пояснения по форматам временных периодов
        cout << "\nStatistics formats:" << endl;
        cout << "  10m - last 10 minutes" << endl;
        cout << "  2h  - last 2 hours" << endl;
        cout << "  1d  - last 1 day" << endl;
        cout << "  30m - last 30 minutes" << endl;
        cout << "Example: 10m, 2h, 1d\n" << endl;
    }

    static void wait_for_enter() {
        cout << "\nPress Enter to continue...";
        cin.get(); //cin.get() - метод объекта cin, читает один символ (ждет нажатия Enter)
    }
};

// Класс для обработки команд
class CommandProcessor {
private:
    NetworkClient& client; //ссылка на сетевой клиент
    LogParser parser; //объект парсера логов (создается здесь)
    OverheatAnalyzer analyzer{ OVERHEAT_THRESHOLD }; //анализатор перегрева, {..} - передает константу в конструктор

    void handle_current() {
        cout << "Requesting current temperature..." << endl;
        string response = client.send_command("GET_CURRENT"); //response - строка с ответом сервера, отправляем команду на сервер через сетевого клиента

        // Добавляем информацию о пороге перегрева
        if (response.find("Temperature:") != string::npos) { //Проверяем, содержит ли ответ подстроку "Temperature:"
            size_t temp_pos = response.find("Temperature:") + 12; // Находим позицию, где начинается число температуры, и пропускаем саму подстроку
            if (temp_pos != string::npos) { //Проверка, что позиция найдена
                double temp = stod(response.substr(temp_pos)); //response.substr(temp_pos) - извлекаем подстроку с позиции temp_pos до конца
                if (temp > OVERHEAT_THRESHOLD) { // если больше, добавим предупреждение, если нет, то информ. сообщение
                    response += "\n!!! WARNING: Current temperature exceeds threshold ("
                        + to_string(OVERHEAT_THRESHOLD).substr(0, 4) + "°C) !!!\n"; //to_string(OVERHEAT_THRESHOLD).substr(0, 4) - преобразуем порог в строку и берем первые 4 символа (например, "51.0")
                }
                else {
                    response += "\nTemperature is normal (threshold: "
                        + to_string(OVERHEAT_THRESHOLD).substr(0, 4) + "°C)\n";
                }
            }
        }

        cout << "\n" << response << endl;
    }

    void handle_recent() {
        cout << "Requesting recent logs..." << endl; // запрашиваем последние логи
        string response = client.send_command("GET_RECENT");

        auto entries = parser.parse_logs(response); //auto - автоматическое определение типа (vector<LogEntry>), parser.parse_logs(response) - парсим полученный текст логов в вектор записей
        string warnings = analyzer.format_warnings(entries); // анализируем записи и получаем строку с предупреждениями о перегреве

        string full_response = response + "\n" + warnings; // объединяем в одну строку
        string filename = FileHelper::generate_filename("recent_temp_"); // генерируем имя файла с префиксом "recent_temp_"

        if (FileHelper::save(filename, full_response)) { // сохраняем полный ответ в файл
            cout << "Recent logs saved to: " << filename << endl;
            cout << warnings << endl;
            FileHelper::open_in_notepad(filename); //Выводим информацию и открываем файл в блокноте
        }
    }

    void handle_stats() {
        UI::print_stats_help(); //Вызываем статический метод класса UI для вывода справки по форматам, :: - оператор разрешения области видимости
        cout << "Enter time period (e.g., 10m, 2h, 1d): "; //Запрашиваем у пользователя временной период
        string period;
        getline(cin, period); //читает целую строку (включая пробелы)

        if (!period.empty()) { //Проверяем, что пользователь ввел непустую строку
            string cmd = "GET_STATS " + period;
            cout << "Requesting statistics for " << period << "..." << endl;
            string response = client.send_command(cmd); //Формируем команду "GET_STATS <период>" и отправляем на сервер

            if (response.find("ERROR") != string::npos) { //Если в ответе есть слово "ERROR", выводим ошибку
                cout << "Error: " << response << endl;
            }
            else {
                auto entries = parser.parse_logs(response); //Парсим ответ и анализируем перегрев
                string warnings = analyzer.format_warnings(entries);

                string full_response = response + "\n" + warnings;
                string filename = FileHelper::generate_filename("stats_");

                if (FileHelper::save(filename, full_response)) {
                    cout << "Statistics saved to: " << filename << endl;
                    cout << warnings << endl;
                    FileHelper::open_in_notepad(filename);
                }
            }
        }
    }

    void handle_help() {
        string response = client.send_command("HELP"); //Запрашивает справку с сервера и выводит её
        cout << response << endl;
        cout << "\nOverheat threshold: " << fixed << setprecision(1) << OVERHEAT_THRESHOLD << "°C" << endl; //Добавляет информацию о пороге перегрева
    }

public:
    CommandProcessor(NetworkClient& c) : client(c) {} // CommandProcessor - конструктор класса, NetworkClient& c - параметр (ссылка на сетевого клиента), : - двоеточие, начало списка инициализации, client(c) - инициализация ссылки client значением c, {} - пустое тело конструктора

    bool process(const string& input) { // обработка введенной команды, const string& input - константная ссылка на введенную строку
        if (input == "q" || input == "Q") return false; //input == "q" - сравнение строки с "q", || - логическое ИЛИ

        switch (input[0]) { //switch - оператор множественного выбора, input[0] - первый символ введенной строки (доступ по индексу [0])
        case '1': handle_current(); break; //case '1': - метка для случая, когда символ равен '1', handle_current(); - вызов соответствующего обработчика, break; - выход из switch (без проваливания в следующий case)
        case '2': handle_recent(); break;
        case '3': handle_stats(); break;
        case '4': handle_help(); break;
        default: //метка для всех остальных случаев (по умолчанию)
            cout << "Invalid command." << endl; //вывод сообщения об ошибке
            return true; //продолжаем работу (не выходим)
        }
        return true;
    }
};

// Основной класс приложения
class TemperatureApp {
private: // закрытая секция класса
    NetworkClient client; //объект сетевого клиента: тип и имя переменной
    CommandProcessor processor; //объект обработчика команд: тип и имя переменной
    bool running; //флаг работы приложения: bool - логический тип (true/false), running - имя переменной

public: //открытая секция класса
    TemperatureApp(const string& ip) : client(ip, 8889), processor(client), running(false) {} //TemperatureApp - имя конструктора, const string& ip - параметр (константная ссылка на строку с IP-адресом), client(ip, 8889) - инициализация члена client (создание сетевого клиента), processor(client) - инициализация члена processor (передает ссылку на клиента обработчику команд) 

    bool initialize() { // метод без параметров
        SetConsoleOutputCP(1251); //Функции Windows API для установки кодовой страницы консоли, эта - устанавливает кодовую страницу для вывода (кириллица), 1251 - кодовая страница Windows для кириллицы
        SetConsoleCP(1251); //устанавливает кодовую страницу для ввода(кириллица)

        cout << "=== Raspberry Pi Temperature Monitor ===" << endl;
        cout << "Overheat threshold: " << fixed << setprecision(1) << OVERHEAT_THRESHOLD << "°C" << endl;
        cout << "=====================================\n" << endl; // вывод заголовка программы

        cout << "Raspberry Pi IP address: " << flush; //flush - манипулятор, принудительно сбрасывает буфер вывода (гарантирует, что текст появится на экране до ввода)
        string ip; //объявление строковой переменной
        getline(cin, ip); //чтение целой строки из стандартного ввода (позволяет вводить IP с пробелами, хотя их обычно нет)

        // Создаем новый клиент с введенным IP
        NetworkClient new_client(ip, 8889); //создание нового объекта клиента, в скобках - вызов конструктора с IP пользователя и портом 8889
        client = move(new_client); //client - существующий член класса (созданный в конструкторе с пустым IP), move(new_client) - стандартная функция для перемещения ресурсов (move - преобразует new_client в rvalue reference)
        //NetworkClient не поддерживает копирование (содержит сокет), но поддерживает перемещение. Это позволяет заменить временный объект, созданный с правильным IP, на член класса.

        cout << "Connecting to " << ip << ":8889..." << endl; // вывод сообщения о подключении
        if (!client.connect()) { //!client.connect() - вызов метода connect() и инверсия результата
            cout << "Failed to connect to server!" << endl;
            return false; 
        }

        cout << "Connected successfully!\n" << endl;
        running = true; //устанавливаем флаг работы приложения
        return true;
    }

    void run() { // метод без параметров
        while (running) {
            UI::print_header(); //Вызов статического метода класса UI для вывода заголовка меню, :: - оператор разрешения области видимости
            cout << "> "; //Вывод приглашения для ввода команды

            string input;
            getline(cin, input); //Чтение команды пользователя (целая строка)

            if (!processor.process(input)) { //!processor.process(input) - вызов метода обработки команды и инверсия результата
                running = false;//process() возвращает false только для команд выхода ('q' или 'Q')
            } 
            else {
                UI::wait_for_enter(); //Если не команда выхода, вызываем метод ожидания нажатия Enter
            }
        }
    }

    void shutdown() {
        client.disconnect(); //закрытие сетевого соединения и освобождение ресурсов
        cout << "Program terminated." << endl;
    }
};

int main() { // тип и имя функции без параметров
    TemperatureApp app(""); // тип объекта и имя, () - вызов конструктора, "" - пустая строка в качестве IP-адреса (временное значение)
    //Создается объект app с пустым IP. Конструктор инициализирует client с пустым IP и портом 8889.

    if (!app.initialize()) { //!app.initialize() - вызов метода инициализации и инверсия результата
        system("pause"); //Вызов системной команды "pause" 
        return 1; //возврат кода ошибки (ненулевое значение обычно означает ошибку)
    }

    app.run(); //запуск основного цикла приложения
    app.shutdown(); //завершение работы (будет вызвано после выхода из цикла)

    return 0; //возврат кода успешного завершения (0 обычно означает успех)
} 
