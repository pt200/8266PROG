#!/usr/bin/env python3
#
# Based on ESP8266 ROM Bootloader Utility
# Bondarenko Andrey <pt200>
# https://github.com/pt200/8266PROG.git

import struct
import socket
import argparse
import math
import time
import sys


class ESPROG_25X:
    # Commands
    _MAGIC_CMD_IO = 0x12345678
    _MAGIC_CMD_SERIAL_WRITE = 0x12345679
    _MAGIC_CMD_PAGE_WRITE = 0x12345668  # write & verify native ( ret[ 0] -> error ( 0 = OK))
    _MAGIC_CMD_NONE = 0
    PAGE_SIZE = 256
    _RX_DATA_SIZE = 4096

    def __init__(self):
        self.link = None

    """ Try connecting repeatedly until successful, or giving up """
    def connect(self, ip=None, port=1234):
        if( ip == None):
            raise Exception('Bad params')
        print('Connecting to 25x programmer...' + '\x20' + 32 * '\x55')
        for _ in range(4):
            self.link = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.link.settimeout(1)
            try:
                self.link.connect((ip, port))
                time.sleep(0.1)
                rx = self.link.recv(12)
                print('RX1: ' + str(rx))
                if (rx == b'8266_PROG_v1'):
                    self.link.send(b'25x ')
                    time.sleep(0.1)
                    rx = self.link.recv(17)
                    print('RX2: ' + str(rx))
                    if (rx == b'25x_PROG_v1'):
                        self.link.settimeout(0.5)
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
    def command(self, cmd, write_data, read_len = 0):
#        print('Read ' + str(size) + ' bytes')
        # Create a custom stub
        stub = struct.pack('<III', cmd, len( write_data), read_len) + write_data
        self.link.send(stub)
        data = self.read_exactly(read_len)
        return data
    #        return val, body

    """ Read SPI flash manufacturer and device id """
    def flash_id(self):
        data = self.command( self._MAGIC_CMD_IO, b'\x9F', 32)
        return data

    """ Read SPI flash """
    def flash_read(self, size):
        # typedef  struct{
        #   uint32_t MAGIC_CMD;
        #   uint32_t WR_LEN;
        #   uint32_t RD_LEN;
        #   uint8_t  data[0];
        # }PROG_CMD;
        # Create a custom stub
        stub = struct.pack('<IIIBBBBx', self._MAGIC_CMD_IO, 5, size, 0x0B, 0, 0, 0)
        self.link.send(stub)
        data = self.read_exactly( size)
        #   sys.stdout.write( '\rReceve ' + str(i * self._RX_DATA_SIZE) + ' bytes')
        return data

    """ Verify block """
    def verify_block(self, addr:int, data):
        # Create a custom stub
        stub = struct.pack('<IIIBBBBx', self._MAGIC_CMD_IO, 5, len( data), 0x0B, ( ( addr >> 16) & 0xFF), ( ( addr >> 8) & 0xFF), ( addr & 0xFF))
        self.link.send(stub)
        r_data = self.read_exactly( len( data))
        if( data == r_data):
            return True
        return False

    """ Write block to flash """
    def page_write(self, addr:int, data):
        self.command( self._MAGIC_CMD_IO, b'\x06') # Write ENABLE
        if( not self.status1_is_set( 0x02)): # Check WEL bit
            print( 'ERROR page WRITE: Not set WEL bit')
            return False
        # Create a custom stub
        packt = struct.pack('<bbbb', 0x02, ( ( addr >> 16) & 0xFF), (( addr >> 8) & 0xFF), (addr&0xFF))
        packt += data[ 0 : 256]
        self.command( self._MAGIC_CMD_IO, packt)
        while( self.status1_is_set( 0x01)): # wait BUSY bit
            time.sleep(0.001)
        #ToDo:  CHECK WRITED DATA
        return True

    """ Native write block to flash """
    def page_write_and_verify_native(self, addr:int, data):
        # Create a custom stub
        packt = struct.pack('<I', addr) + data[ 0 : 256]
        ret = self.command( self._MAGIC_CMD_PAGE_WRITE, packt, 1)
        return ret[ 0]   #error code

    """ Get status1 register """
    def get_status1(self):
        return self.command( self._MAGIC_CMD_IO, b'\x05', 1)[0] # Get Status1 Register

    def status1_is_set(self, bit):
        return bool( ( self.get_status1() and bit) == bit)

    """ Perform a chip erase of SPI flash """
    def chip_erase(self):
        self.command( self._MAGIC_CMD_IO, b'\x06') # Write ENABLE
        if( not self.status1_is_set( 0x02)): # Check WEL bit
            print( 'ERROR chip ERASE: Not set WEL bit')
            return
        self.command( self._MAGIC_CMD_IO, b'\xC7') # Chip ERASE
        while( self.status1_is_set( 0x01)): # wait BUSY bit
            time.sleep(0.1)
        return

    #
    def read_exactly(self, size):
        buffer = b''
#        i = 0
        while len(buffer) < size:
            data = self.link.recv(size - len(buffer))
#            i += 1
            if not data:
                raise Exception('No receive data')
            buffer += data
#        print('Iterations ' + str(i))
        return buffer



if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='ESP8266 programmer', prog='p')

    parser.add_argument('--ip', '-ip',
        dest='ip',
        required=True,
        help='IP address')

    parser.add_argument('--port', '-p',
        dest='port',
        type = int,
        help='TCP port',
        default=1234)

    parser.add_argument('--size', '-s',
        dest='size',
        type = int,
        required=True,
        help='FLASH size Kbit. Use "25x.py --ip x.x.x.x --port x -s _SS_ read [file]"')

    parser.add_argument( '--erase', '-e',
        help='Erase CHIP before write',
        dest='erase_chip',
        action='store_true')

#    parser.add_argument( '--dontverify', '-V',
#        help='Disable automatic verify check when write data.',
#        dest='dontverify',
#        action='store_true')


    subparsers = parser.add_subparsers(dest='operation',
        help='Run 24x {command} -h for additional help')

    parser_read_id = subparsers.add_parser('read', help='Read FLASH')
    parser_read_id.add_argument( 'r_filename', required=True, help='File name for store data from EEPROM')
    
    parser_write_id = subparsers.add_parser('write', help='Write FLASH')
    parser_write_id.add_argument( 'w_filename', required=True, help='File name data to write EEPROM')

    parser_ewrite_id = subparsers.add_parser('erase_write', help='Erase and write FLASH')
    parser_ewrite_id.add_argument( 'w_filename', required=True, help='File name data to write EEPROM')


    args = parser.parse_args()

    # Create the ESPROM connection object, if needed
    prg_25x = None
    prg_25x = ESPROG_25X()
    #    prg_25x.connect(args.port, args.baud)
    prg_25x.connect(args.ip, args.port)
    prg_25x.sync()
    print("Device ID( CMD 0x9F): " + " ".join("{:02x}".format(c) for c in prg_25x.flash_id()))
    print("Device ID( CMD 0x15): " + " ".join("{:02x}".format(c) for c in prg_25x.command(prg_25x._MAGIC_CMD_IO, b'\x15', 32)))

#   if (True):
    if ((args.operation == 'write') or (args.operation == 'erase_write')):
        if (( args.operation == 'erase_write') | ( args.erase_chip == True)):
            sys.stdout.write('Erasing ...')
            sys.stdout.flush()
            prg_25x.chip_erase()
            print( ' done')

        image = open(args.w_filename, 'rb').read()

        blocks = math.ceil(len(image) / float(prg_25x.PAGE_SIZE))
        t = time.time()
        addr = 0
        while len(image) > 0:
            sys.stdout.write('Writing at 0x%08x...' % addr + '\r')
            sys.stdout.flush()

            block = image[0:prg_25x.PAGE_SIZE]
            ret = prg_25x.page_write_and_verify_native( addr, block)
            if( ret != 0):
                print('\nWrite error at addr 0x%08x, error code %d' % ( addr, ret))
#            if( args.dontverify == False):
#                if( prg_25x.verify_block( addr, block) == False):
#                    print('\nVerify error addr 0x%08x' % addr)
            image = image[prg_25x.PAGE_SIZE:]
            addr += len(block)
        t = time.time() - t
        written = addr
        print('\nWritten %d bytes in %.2f seconds (%.2f kbit/s)...' % (written, t, written / t * 8 / 1000))
        print("\nLeaving...")

    elif args.operation == 'flash_id':
        flash_id = prg_25x.flash_id()
        print('Manufacturer: %02x' % (flash_id & 0xff))
        print('Device: %02x%02x' % ((flash_id >> 8) & 0xff, (flash_id >> 16) & 0xff))

    elif args.operation == 'read':
        print('Please wait...')
        t = time.time()
        open(args.r_filename, 'wb').write(prg_25x.flash_read( int( args.size * 1024 / 8)))
        t = time.time() - t
        print('Read %d bytes in %.2f seconds (%.2f kbit/s)...' % ( int( args.size * 1024 / 8), t, int( args.size * 1024 / 8) / t * 8 / 1000))

    elif args.operation == 'erase':
        prg_25x.chip_erase()
