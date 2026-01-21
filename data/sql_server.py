#!/usr/bin/env python3
"""
Basic Python Server for STOMP Assignment – Stage 3.3

IMPORTANT:
DO NOT CHANGE the server name or the basic protocol.
Students should EXTEND this server by implementing
the methods below.
"""

import socket
import sys
import threading

import sqlite3
import atexit


SERVER_NAME = "STOMP_PYTHON_SQL_SERVER"  # DO NOT CHANGE!
DB_FILE = "stomp_server.db"              # DO NOT CHANGE!


class Repository:
    def __init__(self):
        self._conn = sqlite3.connect(DB_FILE, check_same_thread=False)
        self._cursor = self._conn.cursor()
        self._cursor.execute("PRAGMA foreign_keys = ON;")
    
    def close(self):
        self._conn.close()
    
    def create_tables(self):
        self._cursor.execute("""
            CREATE TABLE IF NOT EXISTS users (
                username TEXT PRIMARY KEY,
                password TEXT NOT NULL,
                registration_date DATETIME DEFAULT CURRENT_TIMESTAMP
            );
        """)
        
        self._cursor.execute("""
            CREATE TABLE IF NOT EXISTS login_history (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT NOT NULL,
                login_time DATETIME DEFAULT CURRENT_TIMESTAMP,
                logout_time DATETIME,
                FOREIGN KEY(username) REFERENCES users(username) ON DELETE CASCADE
            );
        """)

        self._cursor.execute("""
            CREATE TABLE IF NOT EXISTS file_tracking (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT NOT NULL,
                filename TEXT NOT NULL,
                upload_time DATETIME DEFAULT CURRENT_TIMESTAMP,
                game_channel TEXT,
                FOREIGN KEY(username) REFERENCES users(username) ON DELETE CASCADE
            );
        """)
        
        self._conn.commit()
        print(f"[{SERVER_NAME}] Database initialized and tables created.")
 
    def execute_command(self, script):
        try:
            self._cursor.execute(script)
            self._conn.commit()
            return "done"
        except Exception as e:
            return f"Error: {e}"
 
    def execute_query(self, query):
        try:
            self._cursor.execute(query)
            rows = self._cursor.fetchall()
            return "SUCCESS|" + "|".join([str(row) for row in rows])
        except Exception as e:
            return f"Error: {e}"

repo = Repository()
atexit.register(repo.close)


def recv_null_terminated(sock: socket.socket) -> str:
    data = b""
    while True:
        chunk = sock.recv(1024)
        if not chunk:
            return ""
        data += chunk
        if b"\0" in data:
            msg, _ = data.split(b"\0", 1)
            return msg.decode("utf-8", errors="replace")


def handle_client(client_socket: socket.socket, addr):
    print(f"[{SERVER_NAME}] Client connected from {addr}")

    try:
        while True:
            message = recv_null_terminated(client_socket)
            if message == "":
                break

            print(f"[{SERVER_NAME}] Received SQL: {message}")
            
            if message.strip().upper().startswith("SELECT"):
                response = repo.execute_query(message)
            else:
                response = repo.execute_command(message)

            client_socket.sendall((response + "\0").encode("utf-8"))

    except Exception as e:
        print(f"[{SERVER_NAME}] Error handling client {addr}: {e}")
    finally:
        try:
            client_socket.close()
        except Exception:
            pass
        print(f"[{SERVER_NAME}] Client {addr} disconnected")


def start_server(host="127.0.0.1", port=7778):
    repo.create_tables()
    
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server_socket.bind((host, port))
        server_socket.listen(5)
        print(f"[{SERVER_NAME}] Server started on {host}:{port}")
        print(f"[{SERVER_NAME}] Waiting for connections...")

        while True:
            client_socket, addr = server_socket.accept()
            t = threading.Thread(
                target=handle_client,
                args=(client_socket, addr),
                daemon=True
            )
            t.start()

    except KeyboardInterrupt:
        print(f"\n[{SERVER_NAME}] Shutting down server...")
    finally:
        try:
            server_socket.close()
        except Exception:
            pass


if __name__ == "__main__":
    port = 7778
    if len(sys.argv) > 1:
        raw_port = sys.argv[1].strip()
        try:
            port = int(raw_port)
        except ValueError:
            print(f"Invalid port '{raw_port}', falling back to default {port}")

    start_server(port=port)
