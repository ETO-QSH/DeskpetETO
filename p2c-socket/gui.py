import socket
import threading
import tkinter as tk
from tkinter import messagebox
import subprocess
import json
import sys


class ExeManagerGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Exe Manager GUI")

        self.conn = None
        self.addr = None
        self.exe_process = None
        self.is_connected = False

        # 创建Socket服务器
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.bind(('localhost', 12345))  # 绑定端口
        self.server_socket.listen(5)
        print("Server started and waiting for connections...")

        # 创建 GUI 元素
        self.message_label = tk.Label(root, text="No connection yet")
        self.message_label.pack()

        self.start_button = tk.Button(root, text="Start Exe", command=self.start_exe)
        self.start_button.pack()

        self.send_data_button = tk.Button(root, text="Send Data to Exe", command=self.send_data_to_exe)
        self.send_data_button.pack()

        self.send_json_button = tk.Button(root, text="Send JSON to Exe", command=self.send_json_to_exe)
        self.send_json_button.pack()

        # 启动监听线程
        threading.Thread(target=self.accept_connections).start()

    def start_exe(self):
        if not self.exe_process:
            # 启动 C++ exe
            self.exe_process = subprocess.Popen(['./client.exe'])
            print("Exe started with PID:", self.exe_process.pid)
            # 等待 exe 连接到服务器
            self.message_label.config(text="Waiting for exe to connect...")

    def accept_connections(self):
        while True:
            self.conn, self.addr = self.server_socket.accept()
            print(f"Connected with {self.addr}")
            self.message_label.config(text=f"Connected to {self.addr}")
            self.is_connected = True
            # 启动接收消息的线程
            threading.Thread(target=self.receive_messages).start()

    def receive_messages(self):
        while self.is_connected:
            try:
                data = self.conn.recv(1024).decode()
                if not data:
                    break  # 如果连接断开，退出循环
                print(f"Received from client: {data}")

                try:
                    # 尝试解析 JSON
                    json_data = json.loads(data)
                    self.handle_json(json_data)
                except json.JSONDecodeError:
                    # 处理非 JSON 消息
                    if "GET_DATA" in data:
                        # exe 请求数据，这里获取并发送数据
                        self.send_data_to_exe()
                    elif "COMMAND" in data:
                        # exe 发送命令，Python 进行相应操作
                        messagebox.showinfo("Info", f"Received command from exe: {data}")
            except:
                break  # 如果出现错误，退出循环

        self.conn.close()
        self.conn = None
        self.addr = None
        self.is_connected = False
        self.message_label.config(text="No connection yet")

    def send_data_to_exe(self):
        if self.conn:
            data = "Data from GUI: Hello, Exe!"
            self.conn.send(data.encode())
            print(f"Sent to client: {data}")
        else:
            messagebox.showwarning("Warning", "No connection to send data")

    def send_json_to_exe(self):
        if self.conn:
            json_data = {
                "type": "command",
                "command": "do_something",
                "data": {
                    "param1": "value1",
                    "param2": 123
                }
            }
            json_str = json.dumps(json_data)
            self.conn.send(json_str.encode())
            print(f"Sent JSON to client: {json_str}")
        else:
            messagebox.showwarning("Warning", "No connection to send data")

    def handle_json(self, json_data):
        if json_data.get("type") == "request":
            if json_data.get("request") == "data":
                self.send_data_to_exe()
            elif json_data.get("request") == "function":
                function_name = json_data.get("function")
                print(f"Received request to call function: {function_name}")
                # 在这里可以调用相应的 Python 函数
        elif json_data.get("type") == "command":
            command = json_data.get("command")
            print(f"Received command: {command}")
            # 在这里可以处理命令

    def on_closing(self):
        if self.exe_process:
            self.exe_process.terminate()
        if self.conn:
            self.conn.close()
        self.root.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    app = ExeManagerGUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()