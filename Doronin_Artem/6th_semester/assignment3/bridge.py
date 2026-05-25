import socket
import struct
import os
import time

USB_PORT = '/dev/ttyUSB0'
SERVER_IP = '127.0.0.1'
SERVER_PORT = 54000

def connect_to_server():
    while True:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((SERVER_IP, SERVER_PORT))
            print("Успешно подключено к C++ серверу.")
            return sock
        except:
            print("Ожидание сервера ./main...")
            time.sleep(2)

def run_bridge():
    sock = connect_to_server()
    
    while True:
        try:
            if not os.path.exists(USB_PORT):
                print(f"Потеряна связь с {USB_PORT}. Жду переподключения...")
                while not os.path.exists(USB_PORT):
                    time.sleep(1)
                print("Ардуино снова в сети!")
                os.system(f"stty -F {USB_PORT} 9600 raw -echo")
                time.sleep(1)

            with open(USB_PORT, 'rb') as ser:
                print("Читаю данные...")
                while True:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        # Упаковка в протокол
                        payload = line.encode('utf-8')
                        header = struct.pack("<II", 1, len(payload))
                        try:
                            sock.sendall(header + payload)
                        except:
                            print("Сервер упал. Переподключаюсь...")
                            sock.close()
                            sock = connect_to_server()
        except Exception as e:
            print(f"Ошибка порта: {e}. Перезапуск цикла...")
            time.sleep(1)

if __name__ == "__main__":
    run_bridge()
