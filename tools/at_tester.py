#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
GSM Tools AT Command Tester & Diagnostic Utility
=================================================
A cross-platform Python utility for communicating with mobile phone modems
(Samsung, Qualcomm, Mediatek) via Serial (COM/TTY) or ADB shell.
"""

import sys
import time
import argparse
import subprocess
from typing import List, Optional

try:
    import serial
    import serial.tools.list_ports
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

try:
    from colorama import init, Fore, Style
    init(autoreset=True)
    GREEN = Fore.GREEN
    RED = Fore.RED
    YELLOW = Fore.YELLOW
    CYAN = Fore.CYAN
    RESET = Style.RESET_ALL
except ImportError:
    GREEN = RED = YELLOW = CYAN = RESET = ""


class AtSerialClient:
    """Manages raw serial AT communication over USB Modem COM/TTY."""
    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 3.0):
        if not HAS_SERIAL:
            raise RuntimeError("pyserial is not installed. Please run 'pip install pyserial'.")
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.conn: Optional[serial.Serial] = None

    def open(self):
        self.conn = serial.Serial(
            port=self.port,
            baudrate=self.baudrate,
            timeout=self.timeout,
            rtscts=False,
            dsrdtr=False
        )

    def close(self):
        if self.conn and self.conn.is_open:
            self.conn.close()

    def send_cmd(self, cmd: str) -> str:
        if not self.conn or not self.conn.is_open:
            raise RuntimeError("Serial connection is not open.")
        
        clean_cmd = cmd.strip()
        formatted = f"{clean_cmd}\r\n".encode('ascii', errors='ignore')
        self.conn.reset_input_buffer()
        self.conn.write(formatted)
        self.conn.flush()

        response = []
        start_time = time.time()
        while time.time() - start_time < self.timeout:
            line = self.conn.readline().decode('utf-8', errors='replace')
            if line:
                response.append(line)
                if "OK" in line or "ERROR" in line or "+CME ERROR" in line:
                    break
        return "".join(response).strip()


class AtAdbClient:
    """Fallback client executing AT commands via ADB shell device node."""
    def __init__(self, device_id: Optional[str] = None, node: str = "/dev/umts_atc0"):
        self.device_id = device_id
        self.node = node

    def _run_adb(self, cmd_args: List[str]) -> str:
        base = ["adb"]
        if self.device_id:
            base.extend(["-s", self.device_id])
        base.extend(cmd_args)
        proc = subprocess.run(base, capture_output=True, text=True)
        return proc.stdout.strip()

    def send_cmd(self, cmd: str) -> str:
        clean_cmd = cmd.strip()
        # Direct injection into modem tty node
        script = f"echo '{clean_cmd}\r\n' > {self.node}; head -n 10 {self.node}"
        return self._run_adb(["shell", script])


def list_modem_ports():
    print(f"{CYAN}=== Available Serial & Modem Ports ==={RESET}")
    if not HAS_SERIAL:
        print(f"{RED}pyserial not installed.{RESET}")
        return
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print(f"{YELLOW}No COM/Serial ports detected.{RESET}")
        return
    for p in ports:
        desc = p.description or "Unknown"
        hwid = p.hwid or ""
        print(f"  {GREEN}[{p.device}]{RESET} - {desc} ({hwid})")


def run_diagnostics(client):
    presets = [
        ("Ping / Test", "AT"),
        ("Comprehensive Info (Samsung)", "AT+DEVCONINFO"),
        ("Software Version", "AT+VERSNAME=1,9,0"),
        ("Serial Number", "AT+SERIALNO=1,0"),
        ("IMEI Query", "AT+IMEITEST=1,0"),
        ("IMSI / SIM Identity", "AT+CIMI"),
        ("SIM Status", "AT+CPIN?"),
        ("Current CSC Region", "AT+PRECONFG=1,0"),
        ("Network Lock Query (PN)", 'AT+CLCK="PN",2'),
        ("Service Provider Lock (PP)", 'AT+CLCK="PP",2'),
        ("Signal Quality", "AT+CSQ"),
    ]

    print(f"\n{CYAN}=== Running Diagnostics Sequence ==={RESET}")
    for title, cmd in presets:
        print(f"{YELLOW}>> [{title}] Sending: {cmd}{RESET}")
        try:
            resp = client.send_cmd(cmd)
            if "OK" in resp:
                print(f"{GREEN}{resp}{RESET}\n")
            elif "ERROR" in resp:
                print(f"{RED}{resp}{RESET}\n")
            else:
                print(f"{resp}\n")
        except Exception as ex:
            print(f"{RED}Error: {ex}{RESET}\n")


def main():
    parser = argparse.ArgumentParser(description="GSM Tools AT Command Tester")
    parser.add_argument("-l", "--list", action="store_true", help="List available COM ports")
    parser.add_argument("-p", "--port", type=str, help="Serial port (e.g. COM3 or /dev/ttyACM0)")
    parser.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (default 115200)")
    parser.add_argument("-c", "--cmd", type=str, help="Single AT command to send (e.g. AT+DEVCONINFO)")
    parser.add_argument("-d", "--diag", action="store_true", help="Run full diagnostic suite")
    parser.add_argument("--csc", type=str, help="Change Samsung CSC (e.g. --csc XAA)")
    parser.add_argument("--adb", action="store_true", help="Use ADB fallback instead of Serial")

    args = parser.parse_args()

    if args.list:
        list_modem_ports()
        return

    if not args.port and not args.adb:
        print(f"{YELLOW}No port specified. Detecting available ports...{RESET}")
        list_modem_ports()
        print(f"\n{CYAN}Use --port COMx or --adb to execute commands.{RESET}")
        return

    if args.adb:
        client = AtAdbClient()
        print(f"{GREEN}Using ADB Fallback Client (/dev/umts_atc0){RESET}")
    else:
        client = AtSerialClient(port=args.port, baudrate=args.baud)
        try:
            client.open()
            print(f"{GREEN}Connected to {args.port} at {args.baud} bps{RESET}")
        except Exception as e:
            print(f"{RED}Failed to open port {args.port}: {e}{RESET}")
            return

    try:
        if args.csc:
            target = args.csc.strip().upper()
            print(f"{YELLOW}Initiating Samsung CSC change to {target}...{RESET}")
            seq = [
                "AT+SWATD=0",
                "AT+ACTIVATE=0,0,0",
                "AT+SWATD=1",
                f"AT+PRECONFG=2,{target}",
                "AT+PRECONFG=1,0",
                "AT+CFUN=1,1"
            ]
            for step in seq:
                print(f">> {step}")
                res = client.send_cmd(step)
                print(f"<< {res}")
            print(f"{GREEN}CSC change sequence complete.{RESET}")
            return

        if args.diag:
            run_diagnostics(client)
            return

        if args.cmd:
            res = client.send_cmd(args.cmd)
            print(res)
            return

        # Interactive loop
        print(f"{CYAN}Entering interactive AT terminal. Type 'exit' to quit.{RESET}")
        while True:
            try:
                user_cmd = input(f"{YELLOW}AT> {RESET}").strip()
                if not user_cmd:
                    continue
                if user_cmd.lower() in ["exit", "quit"]:
                    break
                resp = client.send_cmd(user_cmd)
                print(resp)
            except (KeyboardInterrupt, EOFError):
                break
    finally:
        if not args.adb:
            client.close()


if __name__ == "__main__":
    main()
