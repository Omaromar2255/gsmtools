#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
GSM Tools Unified AuthKey Generator
====================================
Generates the 512-hex response key from the 64-character token displayed
by the device bridge agent during the 'Enter AuthKey:' authentication challenge.
"""

import sys
import hashlib
import hmac

MASTER_SECRET_KEY = b"PHOENIX_GSM_MASTER_KEY_2026_SECURITY_TOKEN_VERIFICATION_V1"

def generate_auth_key(token: str) -> str:
    token_clean = token.strip().upper()
    if not token_clean:
        raise ValueError("Token cannot be empty")
    
    token_bytes = token_clean.encode('utf-8')
    
    # Generate 256 bytes (512 hex characters) using iterative HMAC-SHA256
    blocks = []
    for i in range(8):
        block_counter = i.to_bytes(4, byteorder='big')
        h = hmac.new(MASTER_SECRET_KEY, token_bytes + block_counter, hashlib.sha256)
        blocks.append(h.digest())
    
    full_key = b"".join(blocks)
    return full_key.hex().upper()

def main():
    if len(sys.argv) < 2:
        print("Usage: python auth_keygen.py <TOKEN_FROM_PHONE>")
        print("Example: python auth_keygen.py CB8033856156C55F9753936511DF30EA00DF81A9BF17D4CA30A46797708049A1")
        sys.exit(1)
        
    token = sys.argv[1]
    try:
        auth_key = generate_auth_key(token)
        print("\n=== GSM Tools AuthKey Generator ===")
        print(f"Input Token : {token}")
        print(f"Generated Key (512 hex chars):")
        print(auth_key)
        print("\nSend this key to the terminal when prompted: 'Enter AuthKey:'\n")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
