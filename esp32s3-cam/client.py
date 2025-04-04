import socket

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.settimeout(10)  # 设置 10 秒超时
try:
    client.connect(("192.168.0.109", 8000))
    print("Connected to ESP32 camera server")
except Exception as e:
    print(f"Connection failed: {e}")
    exit()

index = 0
while True:
    try:
        length_bytes = client.recv(4)
        if not length_bytes:
            print("Server closed connection")
            break
        length = int.from_bytes(length_bytes, "little")
        print(f"Received length: {length} bytes")
        data = client.recv(length)
        if len(data) == length:
            with open(f"frame_{index}.jpg", "wb") as f:
                f.write(data)
            print(f"Saved frame_{index}.jpg, size: {length} bytes")
            index += 1
        else:
            print(f"Incomplete frame received: {len(data)}/{length} bytes")
            break
    except Exception as e:
        print(f"Error: {e}")
        break
client.close()
print("Client disconnected")
