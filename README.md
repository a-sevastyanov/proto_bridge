# protobridge
## Реализация общения между двумя процессами

Учебный проект: обмен данными между процессом на Python и процессом на C++
через TCP-сокет с сериализацией сообщений в Protobuf (proto3).

## Структура проекта

```
protobridge/
├── proto/
│   └── user.proto        # схема сообщения — единственный источник правды о формате
├── sender/
│   └── sender.py          # клиент: собирает UserInfo, сериализует, отправляет по TCP
├── receiver/
│   ├── receiver.cpp        # сервер: принимает TCP-соединение, парсит protobuf
│   └── receiver.hpp        # объявление recv_exact и вспомогательных функций
└── README.md
```

## Схема сообщения (`proto/user.proto`)

```protobuf
syntax = "proto3";

package protobridge;

message UserInfo {
  uint64 id = 1;
  string username = 2;
  string email = 3;
  int32 age = 4;
  bool is_active = 5;
  int64 created_at_unix = 6;
  repeated string roles = 7;
}
```

Генерация кода из схемы (понадобится protobuf-compiler):

```bash
protoc --python_out=sender/ proto/user.proto
protoc --cpp_out=receiver/ proto/user.proto
```

## Протокол передачи (транспортный уровень)

Данные передаются через **TCP-сокет** (`127.0.0.1:5000`). Так как TCP —
поток байт без границ сообщений, используется **length-prefix framing**:

```
[4 байта: длина сообщения N, big-endian][N байт: сериализованный UserInfo]
```

- Заголовок из 4 байт кодируется как `uint32` в **network byte order**
  (big-endian) — в Python через `struct.pack('>I', n)`, в C++ через `htonl`/`ntohl`.
- Приём данных выполняется через паттерн **`recv_exact(sock, n)`** — цикл,
  гарантирующий получение ровно `n` байт (защита от "short read", когда
  один вызов `recv()` может вернуть меньше байт, чем запрошено).

## Архитектура клиент-сервер

- **Сервер** (`receiver.cpp`, C++): `socket()` → `bind()` → `listen()` → `accept()`.
  Слушает порт 5000, дожидается подключения клиента.
- **Клиент** (`sender.py`, Python): `socket()` → `connect()` → `sendall()`.
  Подключается к серверу и отправляет один пакет с данными пользователя.

Установление соединения проходит через стандартный **TCP three-way handshake**
(`SYN` → `SYN-ACK` → `ACK`) на уровне ядра ОС, прозрачно для кода приложения.

`accept()` и `recv()` — блокирующие вызовы: процесс приостанавливается ОС до
наступления нужного события (новое подключение / новые данные), не расходуя
процессорное время впустую.

## Текущий статус

- [x] Схема `UserInfo` спроектирована, код сгенерирован для Python и C++
- [x] `sender.py` — сериализация и отправка пакета с length-prefix
- [x] `receiver.cpp` — базовый TCP-сервер (socket/bind/listen/accept), проверено
      сквозное подключение с `sender.py`
- [ ] `receiver.cpp` — приём данных через `recv_exact` + парсинг protobuf +
      вывод информации о пользователе в консоль (в работе)
- [ ] LRU-cache на стороне receiver (в планах)

## Как запустить (текущее состояние)

```bash
# Терминал 1 — собрать и запустить сервер
mkdir build && cd build
cmake ..
make
./receiver/build/receiver

# Терминал 2 — запустить клиента
python sender/sender.py
```

## Планы

В дальнейшем видится реализации LRU-Cache на стороне C++
