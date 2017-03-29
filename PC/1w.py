#!/usr/bin/env python3
#
# Based on ESP8266 ROM Bootloader Utility
# Bondarenko Andrey <pt200>
# https://github.com/pt200/8266PROG.git

import struct
import socket
import argparse
import time


class ESPROG_1W:
    # Commands
    _MAGIC_CMD_RESET = 0x12345671
    _MAGIC_CMD_FIND_ID = 0x12345672
    _MAGIC_CMD_IO = 0x12345673
    _MAGIC_CMD_IO_POWER = 0x12345674
    _MAGIC_CMD_NONE = 0

    devices_id = { 0x01:"DS1990", 0x10:"18S20", 0x28:"18B20"}

    def __init__(self):
        self.link = None

    """ Try connecting repeatedly until successful, or giving up """

    def connect(self, ip=None, port=1234):
        if( ip == None):
            raise Exception('Bad params')
        print('Connecting to 1Ware programmer...')
        for _ in range(4):
            self.link = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.link.settimeout(1)
            try:
                self.link.connect((ip, port))
                time.sleep(0.1)
                rx = self.link.recv(12)
#                print('RX1: ' + str(rx))
                if (rx == b'8266_PROG_v1'):
                    self.link.send(b'1w  ')
                    time.sleep(0.1)
                    rx = self.link.recv(17)
#                    print('RX2: ' + str(rx))
                    if (rx == b'1w_PROG_v1'):
                        self.link.settimeout(5)
                        self.link.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                        return
            except Exception as e:
                print(e)
                print('Try...')
            self.link.close()
        raise Exception('Failed to connect')

    """ Perform a connection test """
    def sync(self):
        return

    """ Send a request and read the response """
    def command(self, cmd: int = None, tx_data: bytes = b'', rx_len: int = 0):
        if (cmd == None) or (rx_len == None):
            raise Exception('Bad params')
        #        print( 'Read ' + str(size) + ' bytes')
        stub = struct.pack('<III', cmd, len(tx_data), rx_len)
        stub += tx_data
#        print("#CMD " + " ".join("{:02x}".format(c) for c in stub))
        self.link.send(stub)
        data = self.read_exactly( rx_len)
        return data

    """ Reset bus """
    def reset(self):
        return self.command( ESPROG_1W._MAGIC_CMD_RESET)

    """ Send a request and read the response """
    def io(self, addr: bytes = None, tx_data: bytes = b'', rx_len: int = 0):
        if (addr == None):
            raise Exception('Bad params')
        #        print( 'Read ' + str(size) + ' bytes')
        return self.command( self._MAGIC_CMD_IO, b'\x55' + addr + tx_data, rx_len)

    """ Send a request and read the response """
    def io_power(self, addr: bytes = None, tx_data: bytes = b'', rx_len: int = 0):
        if (addr == None):
            raise Exception('Bad params')
        #        print( 'Read ' + str(size) + ' bytes')
        return self.command( self._MAGIC_CMD_IO_POWER, b'\x55' + addr + tx_data, rx_len)

    """ Send a request and read the response """
    def get_type(self, addr: bytes = None, def_val : str = None):
        if (addr == None):
            raise Exception('Bad params')
        return self.devices_id.get( addr[ 0], def_val)

    """ Scan 1W bus """
    def scan(self):
        dev_ids = []
        reset_search = b'\x01'

        while( 1):
            rx_buf = self.command( self._MAGIC_CMD_FIND_ID, reset_search, 12)
            reset_search = b'\x00'
            if( rx_buf == None):
                break
            if( rx_buf[ 0] != 0):
                dev_ids.append( rx_buf[ 4:12])
            else:
                break
        return dev_ids


    """ Write block to flash """
    def read_exactly(self, size):
        buffer = b''
        while len(buffer) < size:
            data = self.link.recv(size - len(buffer))
            if not data:
                raise Exception('No receive data')
            buffer += data
        return buffer



if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='ESP8266 1Ware programmer', prog='p')

    parser.add_argument('--ip', '-ip',
        dest='ip',
        required=True,
        help='IP address')

    parser.add_argument('--port', '-p',
        dest='port',
        type = int,
        help='TCP port',
        default=1234)

    subparsers = parser.add_subparsers(
        dest='operation',
        help='Run 1w {command} -h for additional help')

    parser_flash_id = subparsers.add_parser(
        'scan',
        help='Read 1w devices ID')
    parser_flash_id = subparsers.add_parser(
        'scan_short',
        help='Read 1w devices ID')


    args = parser.parse_args()

    # Create the ESPROM connection object, if needed
    prg_1w = None
    prg_1w = ESPROG_1W()
    prg_1w.connect(args.ip, args.port)
    prg_1w.sync()

    if( args.operation == 'scan') or ( args.operation == 'scan_short'):
        devices = prg_1w.scan()
        if( len( devices) > 0):
            for addr in devices:
                if( addr[ 0] == 0x10) or ( addr[ 0] == 0x28): # 1W TERMOMETR
                    prg_1w.reset()
                    prg_1w.io_power(addr, b'\x44')
                    time.sleep( 0.8)
                    prg_1w.reset()
                    scrachpad = prg_1w.io(addr, b'\xBE', 9)

                    ( Ti16,) = struct.unpack( "<h", scrachpad[0:2])
                    
                    if( addr[ 0] == 0x28): # 18B20
                        T = float( Ti16) / 16.0;
                    elif( addr[ 0] == 0x10): # 18S20
                        T = float( Ti16) / 2.0;
                    else:
                        raise Exception( 'Unknown DECODER')

                    if( args.operation == 'scan'):
                        print( "Found device: " + prg_1w.get_type( addr, "***") +
                            "  ID: " + " ".join("{:02x}".format(c) for c in addr) +
                            "  Scratchpad: " + " ".join("{:02x}".format(c) for c in scrachpad) +
                            "  Temp: {:.2f} *C".format( T))
                    else:
                        print( "# " + prg_1w.get_type( addr, "***") +
                            ", " + "".join("{:02x}".format(c) for c in addr) +
                            ", " + "".join("{:02x}".format(c) for c in scrachpad) +
                            ", {:.2f}".format( T))
                else:
                    if( args.operation == 'scan'):
                        print( "Found device: " + prg_1w.get_type( addr, "***") +
                            "  ID: " + " ".join("{:02x}".format(c) for c in addr))
                    else:
                        print( "# " + prg_1w.get_type( addr, "***") +
                            ", " + "".join("{:02x}".format(c) for c in addr))
        else:
            if( args.operation == 'scan'):
                print("No Found device")

    print("Done")
