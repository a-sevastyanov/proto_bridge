# protobridge
## Реализация общения между двумя процессами

Учебный проект: обмен данными между процессом на Python и процессом на C++
через TCP-сокет с сериализацией сообщений в Protobuf (proto3).

## Структура проекта

```
protobridge/
├── proto/
│   └── user.proto      # схема сообщения — единственный источник правды о формате
├── sender/
│   ├── sender.py       # клиент: собирает UserInfo, сериализует, отправляет по TCP
│   └── proto/          # сюда генерируется user_pb2.py
├── receiver/
│   ├── receiver.cpp    # сервер: принимает TCP-соединение, парсит protobuf
│   ├── receiver.hpp    # объявление recv_exact
│   ├── CMakeLists.txt  # сборка через CMake (см. "Как запустить")
│   └── proto/          # сюда генерируется user.pb.h / user.pb.cc
├── .gitignore
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
  int64 timestamp = 6;
  repeated Role roles = 7;
}

enum Role {
  UNSPECIFIED = 0;
  USER = 1;
  ADMIN = 2;
  MODERATOR = 3;
  DEVELOPER = 4;
  QA = 5;
}
```

`roles` — `repeated` поле числового типа (enum на wire-уровне кодируется как
varint), поэтому в отличие от `repeated string` сериализуется через packed
encoding: один тег на всё поле целиком, а не отдельный тег на каждый элемент.

Генерация кода из схемы (выполняется заново при любом изменении `.proto` —
для обеих сторон одновременно, иначе будет тихое расхождение форматов):

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

## Приём и разбор сообщения (`receiver.cpp`)

1. `recv_exact(sock, 4)` — читает ровно 4 байта заголовка (цикл на случай
   short read, когда один `recv()` может вернуть меньше байт, чем запрошено).
2. `memcpy` в `uint32_t` + `ntohl(...)` — переводит заголовок из network byte
   order в порядок байт текущей машины, получая длину сообщения `N`.
3. `recv_exact(sock, N)` — читает ровно `N` байт payload.
4. `UserInfo::ParseFromArray(data, size)` — разбирает байты в объект;
   возвращаемое значение (`bool`) обязательно проверяется.
5. Поля читаются через геттеры (`user.id()`, `user.username()`, ...),
   `repeated`-поля — через `user.roles_size()` + `user.roles(i)`,
   значения `enum` — через `Role_Name(...)` для читаемого вывода.

Контракт `recv_exact`: возвращает вектор размера **ровно** `n` при успехе,
и **пустой** вектор при любой неудаче (ошибка `recv()` или закрытие
соединения до получения всех данных) — поэтому на вызывающей стороне
результат всегда сравнивается с ожидаемым размером (`!= n`), а не с `0`,
чтобы не путать «легитимный нулевой размер» с «ошибкой чтения».

## Текущий статус

- [x] Схема `UserInfo` спроектирована, код сгенерирован для Python и C++
- [x] `sender.py` — сериализация и отправка пакета с length-prefix
- [x] `receiver.cpp` — базовый TCP-сервер (socket/bind/listen/accept), проверено
      сквозное подключение с `sender.py`
- [+] `receiver.cpp` — приём данных через `recv_exact` + парсинг protobuf +
      вывод информации о пользователе в консоль (в работе)
- [ ] LRU-cache на стороне receiver (в планах)

## Как запустить

`receiver/CMakeLists.txt` собирает `receiver.cpp` вместе со сгенерированным
`proto/user.pb.cc`, находит установленную в системе `libprotobuf` через
`find_package(Protobuf REQUIRED)` и линкует её.

```bash
# Терминал 1 — собрать и запустить сервер
cd receiver
mkdir build && cd build
cmake ..
make
cd ../..
./receiver/build/receiver

# Терминал 2 — запустить клиента
python sender/sender.py
```

## Планы

В дальнейшем видится реализации LRU-Cache на стороне C++
