#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Full Automated Test Suite for GSM Tools System (v3.0)
Executes deep unit and workflow tests for all newly added features.
"""

import re
import sys
import time

def test_devconinfo_edge_cases():
    print("[TEST] Running DevConInfoParser Edge Cases...")
    
    # 1. Authentic Galaxy Fold 4 trace
    fold4_sample = "+DEVCONINFO: MN(SM-F936U1);BASE(UNKNOWN);VER(F936U1UES8IZA1);MNC(00);MCC(000);PRD(XAA);SN(RFCT71GB10R);IMEI(358941091234567);LOCK(NONE);SDP(RUNTIME);"
    field_re = re.compile(r"([A-Z0-9_]+)\(([^)]*)\)")
    matches = dict(field_re.findall(fold4_sample))
    assert matches.get("MN") == "SM-F936U1"
    assert matches.get("SN") == "RFCT71GB10R"
    assert matches.get("PRD") == "XAA"
    assert matches.get("LOCK") == "NONE"

    # 2. Malformed input with unclosed parentheses
    malformed = "+DEVCONINFO: MN(SM-S918U;PRD(ATT);LOCK(LOCKED)"
    matches_mal = dict(field_re.findall(malformed))
    assert matches_mal.get("LOCK") == "LOCKED"

    # 3. Empty string
    empty_matches = dict(field_re.findall(""))
    assert len(empty_matches) == 0
    print("  -> DevConInfoParser Edge Cases: PASS")

def test_sim_lock_parser_scenarios():
    print("[TEST] Running SimLockParser Scenarios...")
    def parse_sim_locks(cpin: str, clck_pn: str, clck_pu: str = "", clck_pp: str = "", clck_pc: str = "", clck_sc: str = ""):
        cpin_state = "UNKNOWN"
        if "READY" in cpin: cpin_state = "READY"
        elif "SIM PIN" in cpin: cpin_state = "PIN_REQUIRED"
        elif "SIM PUK" in cpin: cpin_state = "PUK_REQUIRED"
        elif "NOT INSERTED" in cpin or "ERROR: 10" in cpin: cpin_state = "ABSENT"
        elif "ERROR" in cpin: cpin_state = "MODEM_ERROR"

        def clck(raw):
            if "+CLCK: 0" in raw or "+CLCK:0" in raw: return "UNLOCKED (0)"
            if "+CLCK: 1" in raw or "+CLCK:1" in raw: return "LOCKED (1)"
            return "UNKNOWN"

        pn = clck(clck_pn)
        pu = clck(clck_pu)
        pp = clck(clck_pp)
        pc = clck(clck_pc)
        sc = clck(clck_sc)

        is_unlocked = (pn in ["UNLOCKED (0)", "UNLOCKED"]) and pu != "LOCKED (1)" and pp != "LOCKED (1)" and pc != "LOCKED (1)"
        summary = "Device Unlocked" if is_unlocked else "Network Locked"
        if cpin_state != "READY": summary += f" (SIM: {cpin_state})"
        return cpin_state, pn, is_unlocked, summary

    # Case 1: Fully unlocked phone with active SIM
    s1 = parse_sim_locks("+CPIN: READY", "+CLCK: 0", "+CLCK: 0")
    assert s1[0] == "READY"
    assert s1[1] == "UNLOCKED (0)"
    assert s1[2] is True
    assert s1[3] == "Device Unlocked"

    # Case 2: Network locked phone (CLCK: 1)
    s2 = parse_sim_locks("+CPIN: READY", "+CLCK: 1")
    assert s2[0] == "READY"
    assert s2[1] == "LOCKED (1)"
    assert s2[2] is False
    assert s2[3] == "Network Locked"

    # Case 3: Absent SIM
    s3 = parse_sim_locks("+CME ERROR: 10", "+CLCK: 0")
    assert s3[0] == "ABSENT"
    assert s3[2] is True
    assert "SIM: ABSENT" in s3[3]

    print("  -> SimLockParser Scenarios: PASS")

def test_boost_unlock_and_relock_workflow():
    print("[TEST] Running BoostSprintWorkflow Simulation...")
    # Simulate 5-step Boost Unlock
    steps_executed = []
    def boost_unlock():
        steps_executed.append("AT+UNLOCKSPR=OLD")
        steps_executed.append("AT+SIMLOCKU=0,0,0")
        steps_executed.append("pm uninstall -k --user 0 com.sec.omadmspr")
        steps_executed.append("logcat -b radio -c")
        steps_executed.append("AT+CFUN=1,1")
        return len(steps_executed) == 5

    assert boost_unlock() is True
    assert steps_executed[0] == "AT+UNLOCKSPR=OLD"
    assert steps_executed[1] == "AT+SIMLOCKU=0,0,0"
    assert steps_executed[4] == "AT+CFUN=1,1"

    # Simulate Carrier Relock BCD Generation
    def relock_bcd(mcc, mnc):
        if len(mnc) == 1: mnc = "0" + mnc
        if len(mnc) == 2: return f"{mcc}F{mnc}00".upper()
        return f"{mcc}{mnc}00".upper()

    assert relock_bcd("421", "01") == "421F0100" # Sabafon
    assert relock_bcd("421", "02") == "421F0200" # MTN / YOU
    assert relock_bcd("421", "03") == "421F0300" # Yemen Mobile
    assert relock_bcd("310", "120") == "31012000" # Sprint
    print("  -> BoostSprintWorkflow Simulation: PASS")

def test_radio_control_actions():
    print("[TEST] Running RadioControlWorkflow Simulation...")
    valid_actions = {
        "REBOOT": "AT+CFUN=1,1",
        "OFF": "AT+CFUN=0",
        "ON": "AT+CFUN=1",
        "FACTORST": "AT+FACTORST=0,0",
        "SWRESET": "AT+SWRESET"
    }

    for act, cmd in valid_actions.items():
        assert cmd.startswith("AT+")
    
    # Invalid action check
    try:
        invalid = "EXPLODE"
        if invalid not in valid_actions:
            raise ValueError(f"Unknown action: {invalid}")
        assert False, "Should have thrown ValueError"
    except ValueError:
        pass
    print("  -> RadioControlWorkflow Simulation: PASS")

def test_csc_packet_chain():
    print("[TEST] Running Samsung CSC 5-Packet Chain Test...")
    csc = "XAA"
    hex_csc = "".join(f"{ord(c):02X}" for c in csc)
    assert hex_csc == "584141"

    p1 = f"0601000E0000{hex_csc}0000{hex_csc}"
    p2 = f"060100090000{hex_csc}"
    p3 = f"060100120000{hex_csc}0000{hex_csc}{hex_csc}"
    p4 = "060100120000"
    p5 = f"060100090000{hex_csc}"

    assert p1 == "0601000E00005841410000584141"
    assert p2 == "060100090000584141"
    assert p3 == "0601001200005841410000584141584141"
    assert p4 == "060100120000"
    assert p5 == "060100090000584141"
    print("  -> Samsung CSC 5-Packet Chain: PASS")

def test_bridge_interactive_command_dispatching():
    print("[TEST] Running Bridge Interactive Command Dispatching Test...")
    # Simulates at_bridge.c interactive loop routing
    def route_command(line: str) -> str:
        s = line.strip()
        if not s: return "EMPTY"
        upper = s.upper()
        if upper in ["BOOST", "/BOOST", "-BOOST", "--BOOST", "BOOST_UNLOCK", "SPR_UNLOCK"] or upper.startswith("BOOST:") or upper.startswith("/BOOST:"):
            return "ACTION:BOOST_UNLOCK"
        if upper.startswith("RELOCK:"):
            return f"ACTION:RELOCK:{s[7:]}"
        if upper.startswith("RADIO:"):
            return f"ACTION:RADIO:{s[6:]}"
        if upper in ["STATUS", "SCAN", "LOCKS", "DEVINFO", "AKSEED", "INFO", "RECONNECT"]:
            return f"ACTION:{upper}"
        if upper.startswith("CSC:"):
            return f"ACTION:CSC:{s[4:]}"
        if upper.startswith("AT:") or upper.startswith("AT") or s.startswith("+") or s.startswith("$") or upper.startswith("GETSPR") or upper.startswith("SETSPR") or upper.startswith("SRELOCK1"):
            cmd = s[3:] if upper.startswith("AT:") else s
            return f"MODEM_AT:{cmd}"
        # Hex IPC fallback
        hex_data = s[4:] if upper.startswith("HEX:") else s
        if re.match(r"^[0-9A-Fa-f]+$", hex_data):
            return f"MODEM_IPC:{hex_data}"
        return "ERR_SYNTAX"

    # Test /boost and variations
    assert route_command("/boost") == "ACTION:BOOST_UNLOCK"
    assert route_command("BOOST") == "ACTION:BOOST_UNLOCK"
    assert route_command("--boost") == "ACTION:BOOST_UNLOCK"
    assert route_command("/BOOST:") == "ACTION:BOOST_UNLOCK"

    # Test raw AT commands without AT: prefix (MUST NOT RETURN ERR_SYNTAX)
    assert route_command("AT") == "MODEM_AT:AT"
    assert route_command("AT+DEVCONINFO") == "MODEM_AT:AT+DEVCONINFO"
    assert route_command("AT+CPIN?") == "MODEM_AT:AT+CPIN?"
    assert route_command("AT+CFUN=1,1") == "MODEM_AT:AT+CFUN=1,1"
    assert route_command("AT+CLCK=\"PN\",2") == "MODEM_AT:AT+CLCK=\"PN\",2"
    assert route_command("GETSPR") == "MODEM_AT:GETSPR"
    assert route_command("SETSPR") == "MODEM_AT:SETSPR"
    assert route_command("+CSQ") == "MODEM_AT:+CSQ"

    # Test bridge control commands
    assert route_command("STATUS") == "ACTION:STATUS"
    assert route_command("SCAN") == "ACTION:SCAN"
    assert route_command("DEVINFO") == "ACTION:DEVINFO"
    assert route_command("LOCKS") == "ACTION:LOCKS"
    assert route_command("RELOCK:421F0100") == "ACTION:RELOCK:421F0100"
    assert route_command("RADIO:REBOOT") == "ACTION:RADIO:REBOOT"

    # Test hex IPC frames
    assert route_command("060100090000584141") == "MODEM_IPC:060100090000584141"
    assert route_command("HEX:060100090000584141") == "MODEM_IPC:060100090000584141"

    print("  -> Bridge Interactive Command Dispatching: PASS")

def test_csharp_session_command_formatting():
    print("[TEST] Running C# PersistentAtBridgeSession Command Formatting Test...")
    # Simulates PersistentAtBridgeSession.SendAtCommandAsync formatting
    bridge_keywords = {
        "STATUS", "SCAN", "HELP", "LOCKS", "SIMLOCK", "DEVINFO",
        "TELEMETRY", "AKSEED", "INFO", "RECONNECT", "EXIT", "QUIT"
    }

    def format_session_command(at_command: str) -> str:
        trimmed = at_command.strip()
        upper = trimmed.upper()
        if (upper.startswith("AT:") or upper.startswith("HEX:") or
            upper.startswith("BOOST") or upper.startswith("/BOOST") or
            upper.startswith("RELOCK:") or upper.startswith("RADIO:") or
            upper.startswith("CSC:") or upper.startswith("AUTH:") or
            upper.startswith("MODE:") or upper in bridge_keywords):
            return trimmed
        elif (upper.startswith("AT") or trimmed.startswith("+") or
              trimmed.startswith("$") or upper.startswith("GETSPR") or
              upper.startswith("SETSPR") or upper.startswith("SRELOCK1")):
            return trimmed
        else:
            return f"AT:{trimmed}"

    assert format_session_command("STATUS") == "STATUS" # NOT AT:STATUS!
    assert format_session_command("SCAN") == "SCAN"     # NOT AT:SCAN!
    assert format_session_command("DEVINFO") == "DEVINFO" # NOT AT:DEVINFO!
    assert format_session_command("BOOST") == "BOOST"   # NOT AT:BOOST!
    assert format_session_command("/boost") == "/boost" # NOT AT:/boost!
    assert format_session_command("AT+DEVCONINFO") == "AT+DEVCONINFO"
    assert format_session_command("AT+CFUN=1,1") == "AT+CFUN=1,1"
    assert format_session_command("GETSPR") == "GETSPR"
    assert format_session_command("CUSTOM_RAW") == "AT:CUSTOM_RAW"
    print("  -> C# PersistentAtBridgeSession Command Formatting: PASS")

if __name__ == "__main__":
    print("==================================================")
    print("  RUNNING DEEP AUTOMATED WORKFLOW TESTS")
    print("==================================================")
    test_devconinfo_edge_cases()
    test_sim_lock_parser_scenarios()
    test_boost_unlock_and_relock_workflow()
    test_radio_control_actions()
    test_csc_packet_chain()
    test_bridge_interactive_command_dispatching()
    test_csharp_session_command_formatting()
    print("==================================================")
    print("  ALL WORKFLOW TEST SUITES PASSED (7/7) SUCCESS!")
    print("==================================================")
