#!/usr/bin/env python3
"""
ASCOM Alpaca Discovery Test Script

This script tests the ASCOM Alpaca Discovery protocol by sending
UDP broadcast packets and listening for responses.

Usage:
    python test_discovery.py [--broadcast-address 255.255.255.255] [--timeout 5]
"""

import socket
import json
import argparse
import sys
import time

DISCOVERY_PORT = 32227
DISCOVERY_MESSAGE = b"alpacadiscovery1"

def discover_alpaca_devices(broadcast_address='255.255.255.255', timeout=5):
    """
    Send discovery broadcast and collect responses.
    
    Args:
        broadcast_address: The broadcast address to send to
        timeout: How long to wait for responses (seconds)
    
    Returns:
        List of discovered devices with their information
    """
    devices = []
    
    try:
        # Create UDP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        sock.settimeout(timeout)
        
        print(f"Sending Alpaca discovery broadcast to {broadcast_address}:{DISCOVERY_PORT}")
        print(f"Message: {DISCOVERY_MESSAGE.decode()}")
        print(f"Waiting {timeout} seconds for responses...\n")
        
        # Send discovery request
        sock.sendto(DISCOVERY_MESSAGE, (broadcast_address, DISCOVERY_PORT))
        
        # Collect responses
        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                data, addr = sock.recvfrom(1024)
                
                try:
                    response = json.loads(data.decode())
                    device_info = {
                        'ip_address': addr[0],
                        'port': addr[1],
                        'alpaca_port': response.get('AlpacaPort', 'Unknown')
                    }
                    devices.append(device_info)
                    
                    print(f"✓ Found Alpaca device at {addr[0]}")
                    print(f"  Response Port: {addr[1]}")
                    print(f"  Alpaca API Port: {response.get('AlpacaPort', 'Unknown')}")
                    print(f"  Raw Response: {data.decode()}\n")
                    
                except json.JSONDecodeError as e:
                    print(f"⚠ Received invalid JSON from {addr[0]}: {data.decode()}")
                    print(f"  Error: {e}\n")
                    
            except socket.timeout:
                # Timeout waiting for next packet - this is expected
                break
                
    except Exception as e:
        print(f"✗ Error during discovery: {e}", file=sys.stderr)
        return devices
        
    finally:
        sock.close()
    
    return devices

def main():
    parser = argparse.ArgumentParser(
        description='Test ASCOM Alpaca Discovery protocol',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Discover devices on local network
  python test_discovery.py
  
  # Use specific broadcast address
  python test_discovery.py --broadcast-address 192.168.1.255
  
  # Wait longer for responses
  python test_discovery.py --timeout 10
        """
    )
    parser.add_argument(
        '--broadcast-address',
        default='255.255.255.255',
        help='Broadcast address to send discovery to (default: 255.255.255.255)'
    )
    parser.add_argument(
        '--timeout',
        type=int,
        default=5,
        help='Timeout in seconds to wait for responses (default: 5)'
    )
    
    args = parser.parse_args()
    
    print("=" * 60)
    print("ASCOM Alpaca Discovery Test")
    print("=" * 60)
    print()
    
    devices = discover_alpaca_devices(
        broadcast_address=args.broadcast_address,
        timeout=args.timeout
    )
    
    print("=" * 60)
    print(f"Discovery complete. Found {len(devices)} device(s).")
    print("=" * 60)
    
    if devices:
        print("\nSummary of discovered devices:")
        for i, device in enumerate(devices, 1):
            print(f"\n{i}. Device at {device['ip_address']}")
            print(f"   Alpaca API: http://{device['ip_address']}:{device['alpaca_port']}/")
            print(f"   Management API: http://{device['ip_address']}:{device['alpaca_port']}/management/apiversions")
            print(f"   Configured Devices: http://{device['ip_address']}:{device['alpaca_port']}/management/v1/configureddevices")
    else:
        print("\n⚠ No Alpaca devices found on the network.")
        print("\nTroubleshooting:")
        print("  1. Ensure the device is connected and running")
        print("  2. Check that UDP port 32227 is not blocked by firewall")
        print("  3. Verify you're on the same network as the device")
        print("  4. Try using a specific broadcast address (e.g., 192.168.1.255)")
        print("  5. Check the device's serial output for errors")
    
    return 0 if devices else 1

if __name__ == '__main__':
    sys.exit(main())
