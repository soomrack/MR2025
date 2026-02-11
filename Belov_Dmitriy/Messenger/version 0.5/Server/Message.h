#pragma once // чтобы избежать ошибок дублирования определений
#include <cstdint>// Даёт точные типы чисел
#include <vector>// динамический массив, для сообщений произвольной длины

enum class MessageType : uint32_t {//  Сообщение может быть одним из нескольких типов
    Text = 1,
    Image = 2,
    Video = 3
};

struct MessageHeader {//Заголовок сообщения, тип и размер, чтобы знать сколько данных читать
    MessageType type;
    uint32_t size;
};

struct Message {
    MessageHeader header;// служебная информация
    std::vector<char> data;// само сообщение
    // vector<char> универсален, так что при желании можно переслать не только строку текста
};