"""sender.py - клиент - он подключается к уже запущенному процессу

Создаёт процесс, запаковывает данные в Protobuf, отправляет их по TCP."""

import socket
import struct

from proto.user_pb2 import UserInfo, Role


# socket.AF_INET - семейство адресов, IPv4 (адрес вида ("127.0.0.1", 5000))
# socket.SOCK_STREAM- тип сокета, именно TCP (в отличие от SOCK_DGRAM — UDP)
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_fd:
    try:
        client_fd.connect(("127.0.0.1", 5000))

        user = UserInfo(
            id = 1,
            username = "alex",
            email = "alex.23@gmail.com",
            age = 23,
            is_active = True,
            timestamp = 123456789,
            user = [Role.DEVELOPER, Role.QA],
        )

        data: bytes = user.SerializeToString()
        header = struct.pack(">I", len(data))
        print(type(header), ": ", header)
        packet = header + data
        client_fd.sendall(packet)
    except ConnectionRefusedError:
        print("Connection or sending error.")

# При выходе из with сокет закроется автоматически (client_fd.close())
