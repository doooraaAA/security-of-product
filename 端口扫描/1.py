import socket
from concurrent.futures import ThreadPoolExecutor

def scan_port(ip, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    socket.setdefaulttimeout(1)  # 设置超时时间为 1 秒
    result = sock.connect_ex((ip, port))
    sock.close()
    return port if result == 0 else None

def main():
    target_ip = input("请输入目标地址: ")
    start_port = int(input("请输入起始端口: "))
    end_port = int(input("请输入结束端口: "))

    open_ports = []
    print(f"正在扫描 {target_ip} 的端口 {start_port} 到 {end_port}...")

    with ThreadPoolExecutor(max_workers=100) as executor:
        
        port_list = [port for port in range(start_port, end_port + 1)]
        results = executor.map(lambda port: scan_port(target_ip, port), port_list)
        
        for port in results:
            if port:
                open_ports.append(port)
    
    if open_ports:
        print("开启的端口有: ")
        for port in open_ports:
            print(f"端口: {port} 开放")
    else:
        print("没有开放的端口。")

if __name__ == "__main__":
    main()
