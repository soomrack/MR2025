// Определяем пины подключения
const int trigPin = 12; 
const int echoPin = 11;

void setup() {
  Serial.begin(9600); // Инициализация монитора порта
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void loop() {
  long duration;
  int distance;

  // Очищаем пин Trig
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Генерируем ультразвуковой импульс длительностью 10 микросекунд
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Измеряем время (в микросекундах), пока сигнал не вернется
  duration = pulseIn(echoPin, HIGH);

  // Расчет расстояния по формуле: скорость звука (34,3 см/мс) / 2 (так как сигнал идет туда и обратно)
  distance = duration * 0.034 / 2;

  // Вывод значения расстояния в монитор порта
  Serial.print("Расстояние: ");
  Serial.print(distance);
  Serial.println(" см");

  // Задержка перед следующим измерением 500 мс
  delay(500);
}
