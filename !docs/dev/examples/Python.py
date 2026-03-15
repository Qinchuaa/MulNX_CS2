import websocket
import threading
import time
import sys

# 日志函数
def log(*args):
    s = ' '.join(str(a) for a in args)
    print(f"[{time.strftime('%H:%M:%S')}] {s}")

# WebSocket事件处理器
def on_message(ws, message):
    log("Received:", message)

def on_error(ws, error):
    log("Error:", error)

def on_close(ws, close_status_code, close_msg):
    log("Closed (code=" + str(close_status_code) + ", reason=" + str(close_msg) + ")")

def on_open(ws):
    log("Connected")
    global connected
    connected = True

# 全局变量
connected = False
ws = None

# 用户输入处理函数
def handle_user_input():
    global ws, connected
    while True:
        if not connected:
            time.sleep(0.1)
            continue
        try:
            command = input("Enter message to send (or 'disconnect' to close, 'clear' to clear log): ").strip()
            if command.lower() == 'disconnect':
                if ws:
                    ws.close()
                break
            elif command.lower() == 'clear':
                # 在命令行中，清空屏幕
                print("\033c", end="")
            elif command:
                if ws and connected:
                    ws.send(command)
                    log("Sent:", command)
                else:
                    log("Not connected")
        except KeyboardInterrupt:
            if ws:
                ws.close()
            break

def main():
    global ws, connected
    url = input("Server URL (default ws://localhost:55202): ").strip()
    if not url:
        url = "ws://localhost:55202"

    log("Connecting to", url)
    ws = websocket.WebSocketApp(url,
                                on_open=on_open,
                                on_message=on_message,
                                on_error=on_error,
                                on_close=on_close)

    # 启动WebSocket线程
    wst = threading.Thread(target=ws.run_forever)
    wst.daemon = True
    wst.start()

    # 启动用户输入处理
    handle_user_input()

if __name__ == "__main__":
    main()