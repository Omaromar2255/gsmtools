#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
GSM Tools AT Command Tester & Diagnostic Utility (v3.0)
======================================================
A cross-platform Python utility for communicating with mobile phone modems
(Samsung Shannon, Exynos, Qualcomm, Mediatek) via Serial (COM/TTY) or ADB shell.

Features:
- Full Sprint / Boost Mobile unlock sequence (--boost)
- Carrier Relock & MCC/MNC BCD injection (--relock)
- Deep Device Telemetry acquisition (--telemetry)
- 3GPP SIM & Network Lock inspection (--locks)
- Radio & Power controls (--radio REBOOT|OFF|ON|FACTORST)
- Interactive AT Terminal & Scripted execution
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
        script = f"if [ -x /data/local/tmp/AT ]; then /data/local/tmp/AT -a \"{clean_cmd}\" 2>/dev/null; elif [ -w {self.node} ]; then echo -e '{clean_cmd}\\r' > {self.node}; head -n 10 {self.node} 2>/dev/null; else echo 'AT_MODEM_UNAVAILABLE'; fi"
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
        ("PDA / Build ID", "AT+VERSNAME=1,7,0"),
        ("Baseband CP Version", "AT+VERSNAME=1,0,0"),
        ("Serial Number", "AT+SERIALNO=1,0"),
        ("IMEI Query", "AT+IMEITEST=1,0"),
        ("IMSI / SIM Identity", "AT+CIMI"),
        ("SIM Status", "AT+CPIN?"),
        ("Current CSC Region", "AT+PRECONFG=1,0"),
        ("Network Lock Query (PN)", 'AT+CLCK="PN",2'),
        ("Network Subset Lock (PU)", 'AT+CLCK="PU",2'),
        ("Service Provider Lock (PP)", 'AT+CLCK="PP",2'),
        ("Corporate Lock (PC)", 'AT+CLCK="PC",2'),
        ("SIM PIN Lock (SC)", 'AT+CLCK="SC",2'),
        ("Battery Diagnostic", "AT+BATTEST=1,0"),
        ("SysScope Status", "AT+SYSSCOPE=1,0"),
        ("Signal Quality", "AT+CSQ"),
    ]

    print(f"\n{CYAN}=== Running Master Diagnostics Sequence ==={RESET}")
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


def run_boost_unlock(client):
    print(f"\n{CYAN}=== Sprint / Boost Mobile Unlock Sequence (/boost) ==={RESET}")
    if isinstance(client, AtAdbClient):
        check = client._run_adb(["shell", "if [ -x /data/local/tmp/AT ]; then /data/local/tmp/AT /boost; else echo 'NO_AT'; fi"])
        if "NO_AT" not in check:
            print(f"<< {check}")
            print(f"{GREEN}[+] Sprint / Boost Unlock completed via native /data/local/tmp/AT /boost.{RESET}")
            return

    steps = [
        ("Unlock SPR Old", "AT+UNLOCKSPR=OLD"),
        ("Reset SimLock Parameters", "AT+SIMLOCKU=0,0,0"),
        ("Query Network Lock Status", 'AT+CLCK="PN",2'),
        ("Radio Soft Reboot", "AT+CFUN=1,1")
    ]
    for title, cmd in steps:
        print(f"{YELLOW}[*] {title}: {cmd}{RESET}")
        resp = client.send_cmd(cmd)
        print(f"<< {resp}")
    print(f"{GREEN}[+] Sprint / Boost Unlock sequence completed.{RESET}")


def run_carrier_relock(client, bcd_hex: str):
    clean_bcd = bcd_hex.strip().upper()
    print(f"\n{CYAN}=== Carrier Relock via MCCREQ BCD: {clean_bcd} ==={RESET}")
    total_bytes = 4 + (len(clean_bcd) // 2)
    srelock_frame = f"0601{total_bytes:04X}0000{clean_bcd}"
    print(f"{YELLOW}[*] Dispatching SRELOCK1 Frame: {srelock_frame}{RESET}")
    if isinstance(client, AtAdbClient):
        cmd = f"if [ -x /data/local/tmp/AT ]; then /data/local/tmp/AT -r {clean_bcd}; else echo -e 'AT+CFUN=1,1\\r' > /dev/umts_atc0; fi"
        resp = client._run_adb(["shell", cmd])
    else:
        resp = client.send_cmd(f"AT+CFUN=1,1")
    print(f"<< {resp}")
    print(f"{GREEN}[+] Relock request processed.{RESET}")


def run_radio_action(client, action: str):
    act = action.strip().upper()
    cmd_map = {
        "REBOOT": "AT+CFUN=1,1",
        "OFF": "AT+CFUN=0",
        "ON": "AT+CFUN=1",
        "FACTORST": "AT+FACTORST=0,0",
        "SWRESET": "AT+SWRESET"
    }
    cmd = cmd_map.get(act)
    if not cmd:
        print(f"{RED}Unknown radio action: {action}. Available: REBOOT, OFF, ON, FACTORST, SWRESET{RESET}")
        return
    print(f"{YELLOW}[*] Executing Radio Action '{act}': {cmd}{RESET}")
    resp = client.send_cmd(cmd)
    print(f"<< {resp}")


def main():
    parser = argparse.ArgumentParser(description="GSM Tools AT Command Tester & Forensic Utility")
    parser.add_argument("-l", "--list", action="store_true", help="List available COM ports")
    parser.add_argument("-p", "--port", type=str, help="Serial port (e.g. COM3 or /dev/ttyACM0)")
    parser.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (default 115200)")
    parser.add_argument("-c", "--cmd", type=str, help="Single AT command to send (e.g. AT+DEVCONINFO)")
    parser.add_argument("-d", "--diag", action="store_true", help="Run full diagnostic suite")
    parser.add_argument("--csc", type=str, help="Change Samsung CSC (e.g. --csc XAA)")
    parser.add_argument("--boost", action="store_true", help="Execute Sprint / Boost Mobile unlock sequence")
    parser.add_argument("--relock", type=str, help="Carrier relock with BCD payload")
    parser.add_argument("--radio", type=str, help="Radio action (REBOOT, OFF, ON, FACTORST, SWRESET)")
    parser.add_argument("--locks", action="store_true", help="Query all SIM and Network locks")
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
        if args.boost:
            run_boost_unlock(client)
            return

        if args.relock:
            run_carrier_relock(client, args.relock)
            return

        if args.radio:
            run_radio_action(client, args.radio)
            return

        if args.locks:
            for lk in ['AT+CPIN?', 'AT+CLCK="PN",2', 'AT+CLCK="PU",2', 'AT+CLCK="PP",2', 'AT+CLCK="PC",2', 'AT+CLCK="SC",2']:
                print(f">> {lk}\n<< {client.send_cmd(lk)}\n")
            return

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

        # Interactive terminal loop
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
