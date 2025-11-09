#define pinSensorL A0  // Левый датчик линии (аналоговый пин A0)
#define pinSensorR A1  // Правый датчик линии (аналоговый пин A1)
#define pinShield_LH 6 // Направление ЛЕВОГО мотора (HIGH/LOW = вперед/назад)
#define pinShield_LE 7 // Скорость ЛЕВОГО мотора (ШИМ, 0-255)
#define pinShield_RE 4 // Скорость ПРАВОГО мотора (ШИМ, 0-255)  
#define pinShield_RH 5 // Направление ПРАВОГО мотора (HIGH/LOW = вперед/назад)


const uint16_t valSensor1 = 930;  // Значение датчика НА линии (черный)
const uint16_t valSensor0 = 730;  // Значение датчика ВНЕ линии (белый)
const uint8_t valSpeed = 125;     // Базовая скорость (50% от максимума 255)
const uint8_t valTurning = 20;    // Интенсивность поворота
const uint32_t tmrDelay = 2000;   // Время потери линии до остановки (2 сек)
const bool arrRoute[2] = {1,1};   // Направление моторов [правый, левый]


uint8_t arrSpeed[2];     // Текущие скорости моторов [правый, левый]
uint16_t valSensorM;     // ПОРОГОВОЕ значение датчика (середина между линией и полем)
uint8_t valSensor;       // ТЕКУЩЕЕ состояние датчиков в битах (0bxxxxxxLR)
bool flgLine;            // Флаг: 0=темная линия, 1=светлая линия
int8_t flgTurning;       // Направление: -1=влево, 0=прямо, +1=вправо
bool flgPWM;             // Флаг необходимости обновить скорости моторов
bool flgStop;            // Флаг необходимости остановки
uint32_t tmrMillis;      // Таймер для отслеживания потери линии



void setup(){
//  Узнаём цвет линии используемой на трассе
    flgLine = (valSensor0 > valSensor1);                            // Если условие выполняется - линия светлая, иначе тёмная
//  Вычисляем среднее значение между показаниями датчиков на линии и вне линии
    if(flgLine){ valSensorM = valSensor1 + (valSensor0 - valSensor1) / 2; }
    else       { valSensorM = valSensor0 + (valSensor1 - valSensor0) / 2; }
//  Устанавливаем начальную скорость обоих моторов
    arrSpeed[1] = valSpeed;                                         // Скорость левого мотора
    arrSpeed[0] = valSpeed;                                         // Скорость правого мотора
//  Устанавливаем начальные флаги
    flgPWM = 1; flgTurning = 0; flgStop = 0;
//  Устанавливаем режим работы выводов и направление моторов
    pinMode     (pinSensorL,   INPUT      );                        // Левый датчик линии как вход
    pinMode     (pinSensorR,   INPUT      );                        // Правый датчик линии как вход
    pinMode     (pinShield_LH, OUTPUT     );                        // Направление левого мотора как выход
    pinMode     (pinShield_LE, OUTPUT     );                        // ШИМ левого мотора как выход
    pinMode     (pinShield_RE, OUTPUT     );                        // ШИМ правого мотора как выход
    pinMode     (pinShield_RH, OUTPUT     );                        // Направление правого мотора как выход
    digitalWrite(pinShield_LH, arrRoute[1]);                        // Устанавливаем направление левого мотора
    digitalWrite(pinShield_RH, arrRoute[0]);                        // Устанавливаем направление правого мотора
//  Инициализация Serial для отладки
    Serial.begin(9600); 
    while(!Serial){}                                                // Ждем инициализации Serial
    Serial.println("Робот готов!");
    Serial.print("Пороговое значение датчиков: ");
    Serial.println(valSensorM);
//  Устанавливаем задержку перед стартом и обновляем таймер
    delay(2000);
    tmrMillis = millis();
}

void loop(){
//  Читаем показания датчиков и преобразуем их в логические уровни
    valSensor = 0;                                                  // Сбрасываем все биты переменной valSensor
    valSensor |= ((analogRead(pinSensorL) > valSensorM) ^ flgLine) << 1;  // Устанавливаем 1 бит (левый датчик)
    valSensor |= ((analogRead(pinSensorR) > valSensorM) ^ flgLine) << 0;  // Устанавливаем 0 бит (правый датчик)

//  Определяем действия в соответствии с текущим положением датчиков
    switch(valSensor){
        case 0b00:                                                  // 00 - Оба датчика вне линии
            tmrMillis = millis();
            flgPWM = 1;
            flgTurning = 0;
            flgStop = 0;
            break;
            
        case 0b01:                                                  // 01 - Правый датчик на линии, левый вне линии
            tmrMillis = millis();
            flgPWM = 1;
            flgTurning = 1;                                         // Поворачиваем направо
            flgStop = 0;
            break;
            
        case 0b10:                                                  // 10 - Левый датчик на линии, правый вне линии
            tmrMillis = millis();
            flgPWM = 1;
            flgTurning = -1;                                        // Поворачиваем налево
            flgStop = 0;
            break;
            
        case 0b11:                                                  // 11 - Оба датчика на линии
            tmrMillis = millis();
            flgPWM = 1;
            flgTurning = 0;                                         // Движемся прямо
            flgStop = 0;
            break;
    }
    
    // Проверяем переполнение millis()
    if(tmrMillis > millis()) { 
        tmrMillis = 0;
    }
    
    // Останавливаемся если линия потеряна на долгое время
    if(millis() - tmrMillis > tmrDelay) {
        flgPWM = 1;
        flgTurning = 0;
        flgStop = 1;
    }
    
//  Устанавливаем ШИМ для моторов
    if(flgPWM) {
        flgPWM = 0;
        
        // Устанавливаем скорости в зависимости от направления поворота
        switch(flgTurning) {
            case -1:  // Поворот налево
                arrSpeed[1] = valSpeed - valTurning;                // Замедляем левый мотор
                arrSpeed[0] = valSpeed + valTurning;                // Ускоряем правый мотор
                break;
                
            case 0:   // Прямое движение
                arrSpeed[1] = valSpeed;                             // Оба мотора на базовой скорости
                arrSpeed[0] = valSpeed;
                break;
                
            case 1:   // Поворот направо
                arrSpeed[1] = valSpeed + valTurning;                // Ускоряем левый мотор
                arrSpeed[0] = valSpeed - valTurning;                // Замедляем правый мотор
                break;
        }
        
        // Ограничиваем скорости в диапазоне 0-255
        arrSpeed[1] = constrain(arrSpeed[1], 0, 255);
        arrSpeed[0] = constrain(arrSpeed[0], 0, 255);
        
        // Останавливаем моторы если установлен флаг остановки
        if(flgStop) {
            arrSpeed[1] = 0;
            arrSpeed[0] = 0;
        }
        
        // Применяем скорости к моторам
        analogWrite(pinShield_LE, arrSpeed[1]);
        analogWrite(pinShield_RE, arrSpeed[0]);
        
        // Отладочный вывод (опционально)
        Serial.print("Левый: ");
        Serial.print(arrSpeed[1]);
        Serial.print(" Правый: ");
        Serial.print(arrSpeed[0]);
        Serial.print(" Состояние: ");
        Serial.println(valSensor, BIN);
    }
}
