#!/usr/bin/env python3
#
# Based on ESP8266 ROM Bootloader Utility
# Bondarenko Andrey <pt200>
# https://github.com/pt200/8266PROG.git

import sys
import struct
import socket
import argparse

import time
import os
import math
import cmd


class ESPROG_AVR:
    # Commands
    _MAGIC_CMD_AVR_SET_RST = 0x32345677
    _MAGIC_CMD_AVR_STREAM_IO = 0x32345678
    _MAGIC_CMD_NONE = 0
    PROG_MAX_BLOCK_SIZE = ( 1024 - 12)

    devices__id__name_sname = {
         b'\x1E\x90\x07\xFF':( 'ATtiny13', 't13'),
         b'\x1E\x90\x06\xFF':( 'ATtiny15', 't15'),
         b'\x1E\x91\x08\xFF':( 'ATtiny25', 't25'),
         b'\x1E\x92\x06\xFF':( 'ATtiny45', 't45'),
         b'\x1E\x93\x0B\xFF':( 'ATtiny85', 't85')}
    devices__sname__fsize_esize_pagesize = { 't13':( 512, 64, 16) , 't25':( 1024, 128, 16), 't45':( 2048, 256, 32), 't85':( 4096, 512, 32)}

    def __init__(self):
        self.link = None

    """ Try connecting repeatedly until successful, or giving up """
    def connect(self, ip=None, port:int=1234):
        if( ip == None):
            raise Exception('Bad params')
        print('Connecting to AVR programmer...')
        for _ in range(4):
            self.link = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.link.settimeout(1)
            try:
                self.link.connect((ip, port))
                time.sleep(0.1)
                rx = self.link.recv(12)
                print('RX1: ' + str(rx))
                if (rx == b'8266_PROG_v1'):
                    self.link.send(b'AVR ')
                    time.sleep(0.1)
                    rx = self.link.recv(17)
                    print('RX2: ' + str(rx))
                    if (rx == b'AVR_PROG_v1'):
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
        print('Connect to AVR chip...')
        for _ in range( 5):
            self.prog_cmd(self._MAGIC_CMD_AVR_SET_RST, b'\x00') #PIN RESET = 0
            res = self.AVR_simple_cmd(cmd1=0xAC, cmd2=0x53)
            if( res[ 2] == 0x53): #check ECHO byte
                return True
            self.prog_cmd(self._MAGIC_CMD_AVR_SET_RST, b'\x01')  # PIN RESET = 1
            print('Try( ACK: %02x %02x %02x %02x ) ...' % ( res[ 0], res[ 1], res[ 2], res[ 3]))
        self.prog_cmd(self._MAGIC_CMD_AVR_SET_RST, b'\x00')  # PIN RESET = 0
        raise Exception('AVR CHIP not answer')
        return False

    """ Send a request and read the response to PROGRAMMER"""
    def prog_cmd(self, cmd, write_data, read_len=0):
        stub = struct.pack('<III', cmd, len(write_data), read_len) + write_data
        self.link.send(stub)
        data = self.read_exactly(read_len)
        return data

    """ Send a request and read the response to AVR"""
    def AVR_simple_cmd(self, cmd1, cmd2=0, cmd3=0, cmd4=0):
        stub = struct.pack('<IIIBBBB', self._MAGIC_CMD_AVR_STREAM_IO, 4, 4, cmd1, cmd2, cmd3, cmd4)
        self.link.send(stub)
        data = self.read_exactly(4)
        return data

    """ Send a request and read the response to AVR"""
    def AVR_burst_cmd(self, cmd):
        data = b''
        while( len( cmd) > 0):
            _cmd = cmd[ : ( min( len( cmd), self.PROG_MAX_BLOCK_SIZE))] # get partial data block
            cmd = cmd[ ( min( len( cmd), self.PROG_MAX_BLOCK_SIZE)):] # trim original data block  
            stub = struct.pack('<III', self._MAGIC_CMD_AVR_STREAM_IO, len( _cmd), len( _cmd)) + _cmd
            self.link.send(stub)
            data += self.read_exactly( len( _cmd))
        return data




    """ Read lock bits """
    def read_lock_bits(self):
        return self.AVR_simple_cmd( 0x58, 0x00)[ 3]

    """ Write lock bits """
    def write_lock_bits(self, lb):
        self.AVR_simple_cmd( 0xAC, 0xE0, 0x00, ( lb | 0xC0))
        return


    """ Read fuse bits """
    def read_fuses(self):
#        cmds_read_fuse = { 0:b'\x50\x00\x00\x00',  1:b'\x58\x08\x00\x00',  2:b'\x50\x08\x00\x00'}
        cmd = b'\x50\x00\x00\x00' + b'\x58\x08\x00\x00' + b'\x50\x08\x00\x00'
        data = self.AVR_burst_cmd( cmd)
#        print( data)
        return ( data[3], data[ 7], data[ 11])

    """ Write fuse bits """
    def write_fuses(self, fuse=None, hfuse=None, efuse=None):
#        cmds_write_fuse = { 0:b'\xAC\xA0\x00',  1:b'\xAC\xA8\x00',  2:b'\xAC\xA4\x00'}
        raise Exception('Safety exception') #remove it
        if( efuse != None):
            self.AVR_simple_cmd( 0xAC, 0xA4, 0x00, efuse)
            self.poll()
        if( hfuse != None):
            self.AVR_simple_cmd( 0xAC, 0xA8, 0x00, hfuse)
            self.poll()
        if( fuse != None):
            self.AVR_simple_cmd( 0xAC, 0xA0, 0x00, fuse)
            self.poll()
        return

    """ Read device id """
    def get_id(self):
        cmd = b'\x30\x00\x00\x00'+b'\x30\x00\x01\x00'+b'\x30\x00\x02\x00'+b'\x30\x00\x03\x00'
        data = self.AVR_burst_cmd( cmd) [3::4]
        return data

    """ Read EEPROM """
    def EEPROM_read(self, size):
        #ToDo: fragmentation
        cmd = b''
        for addr in range( size):
#            print( self.AVR_simple_cmd( 0xA0, ((addr>>8)&0xFF), ((addr>>0)&0xFF), 0))
            cmd += struct.pack('>BHx', 0xA0, addr)
        data = self.AVR_burst_cmd( cmd)
        return data[3::4]

    """ Read FLASH """
    def flash_read(self, size):
        #ToDo: fragmentation
        cmd = b''
        for addr in range( size):
            cmd += struct.pack('>BBBB', 0x20 | ( ( addr & 0x01) << 3), ( ( addr >> 9) & 0xFF), ( ( addr >> 1) & 0xFF), 0xFF) 
#        print( " ".join("{:02x}".format(c) for c in cmd))
        data = self.AVR_burst_cmd( cmd)
#        print( " ".join("{:02x}".format(c) for c in data[3::4]))
        return data[3::4]

    """ Write EEPROM """
    def eeprom_write(self, data):
        #ToDo: fragmentation
        for addr in range( len( data)):
            self.AVR_simple_cmd( 0xC0, ((addr>>8)&0xFF), ((addr>>0)&0xFF), data[ addr])
            self.poll()

        return

    """ Write block to flash """
    def flash_page_write(self, addr:int, data, check = True):
        #build page data + write cmd
        cmd = b''
        _addr = addr
        
        '''Load data to page buffer'''
        for _addr in range( len( data)):
            cmd += struct.pack('>BBBB', 0x40 | ( ( _addr & 0x01) << 3), ( ( _addr >> 9) & 0xFF), ( ( _addr >> 1) & 0xFF), data[ _addr]) 

        '''Page write CMD'''
        cmd += struct.pack('>BBBB', 0x4C, ( ( addr >> 9) & 0xFF), ( ( addr >> 1) & 0xFF), 0x00)
#        print( " ".join("{:02x}".format(c) for c in cmd))

#        raise Exception('Safety exception') #remove it
        self.AVR_burst_cmd( cmd)

        self.poll()
 
        #ToDo:  CHECK WRITED DATA
        return True

    """ Wait busy """
    def poll(self):
        while( self.isbusy()): # wait BUSY bit
#            print( 'Poll')
            time.sleep(0.001)
        return

    """ Get busy flag """
    def isbusy(self):
        return ( ( self.AVR_simple_cmd(0xF0, 0x00)[ 3])&0x01)

    """ Perform a chip erase of SPI flash """
    def chip_erase(self):
        self.AVR_simple_cmd( 0xAC, 0x80)
        self.poll()
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
    parser = argparse.ArgumentParser( prog='AVR.py', description='ESP8266 AVR programmer')

    parser.add_argument('--ip', '-ip',
        dest='ip',
        required=True,
        help='IP address')

    parser.add_argument('--port', '-p',
        dest='port',
        type = int,
        help='TCP port',
        default=1234)

    parser.add_argument(
        '-sl',
        help='Slow speed IO',
        dest='slow_spi',
        action='store_true')

    parser.add_argument( '--erase', '-e',
        help='Erase CHIP before write',
        dest='chip_erase',
        action='store_true')


    subparsers = parser.add_subparsers(dest='operation',
#        action='append',
        help='Run AVR {command} -h for additional help')

#    parser_get_id = subparsers.add_parser('id', help='Get ID')
    
    parser_fread_id = subparsers.add_parser('flash_read', help='Read FLASH')
    parser_fread_id.add_argument( 'fr_filename', help='File name for store data from FLASH')
    
    parser_fwrite_id = subparsers.add_parser('flash_write', help='Write FLASH')
    parser_fwrite_id.add_argument( 'fw_filename', help='File name data to write FLASH')

    parser_eread_id = subparsers.add_parser('eeprom_read', help='Read FLASH')
    parser_eread_id.add_argument( 'er_filename', help='File name for store data from EEPROM')
    
    parser_ewrite_id = subparsers.add_parser('eeprom_write', help='Write FLASH')
    parser_ewrite_id.add_argument( 'ew_filename', help='File name data to write EEPROM')

    parser_ewrite_id = subparsers.add_parser('fuse_read')

    parser_ewrite_id = subparsers.add_parser('calib_read')


    '''

    parser.add_argument(
        '-rlb',
        help='Read lock bits',
        dest='read_lock_bits',
        action='store_true')
    '''
    args = parser.parse_args()
    
    print( args)

    # Create the ESPROM connection object, if needed
    prg_AVR = None
    prg_AVR = ESPROG_AVR()
    prg_AVR.connect(args.ip, args.port)
    prg_AVR.sync()

    dev_id = prg_AVR.get_id()
    dev_names = prg_AVR.devices__id__name_sname.get( dev_id, ( '*', ''))
    dev_name = dev_names[ 0]
    dev_sname = dev_names[ 1]
    dev_info = prg_AVR.devices__sname__fsize_esize_pagesize.get( dev_sname)
    print("Device ID: " + " ".join("{:02x}".format(c) for c in dev_id) + ' --> ' + dev_name)
    print('  FLASH size %d words\n  EEPROM size %d bytes\n  FLASH page size %d words' % dev_info)
    chip_flash_size = dev_info[ 0]
    chip_flash_page_size = dev_info[ 2]
    chip_eeprom_size = dev_info[ 1]
 
    print("lock bits: %02x" % ( prg_AVR.read_lock_bits(), ))
    print("fuse_hfuse_efuse: %02x %02x %02x" % prg_AVR.read_fuses())




    if( args.chip_erase):
        t = time.time()
        print('Erasing ...', end=' ')
        prg_AVR.chip_erase()
        print('done')
        print('Time erase: %.2f seconds' % ( ( time.time() - t),))

    # Add vendor directory to module search path
    parent_dir = os.path.abspath(os.path.dirname(__file__))
    lib_dir = os.path.join(parent_dir, 'lib')
    sys.path.append(lib_dir)
    from lib.intelhex import IntelHex
    


    if ( args.operation == 'flash_write'):
        print( 'Read file: ' + args.fw_filename)
        ih = IntelHex( args.fw_filename)
        ih.padding = 0xFF
#        print("IH: " + " ".join("{:02x}".format(c) for c in ih.tobinarray( start = 0, size = 16, pad = 0xFF, ih.maxaddr())))
#        print( ih.addresses())
#        print( ih.segments())
#        print( ih.todict())

        data_size = ih.maxaddr()
        print( 'Data size: %d bytes' % (data_size, ))

        t = time.time()

        for addr in range( 0, data_size, chip_flash_page_size*2):
            print('\rWriting at 0x%08x...' % addr, end = ' ')
            prg_AVR.flash_page_write(addr, ih.tobinarray( start = addr, size = chip_flash_page_size*2))

        print('done')
        t = time.time() - t
        print('Written %d bytes in %.2f seconds (%.2f kbit/s)...' % (data_size, t, data_size / t * 8 / 1000))

    elif args.operation == 'eeprom_write':
        t = time.time()
        print( 'Read file: ' + args.ew_filename)
        ih = IntelHex( args.ew_filename)
        ih.padding = 0xFF
        data_size = min( chip_eeprom_size, ih.maxaddr())
        print( 'Data size: %d bytes' % (data_size, ))

#        print("IH: " + " ".join("{:02x}".format(c) for c in data))

        print('EEPROM write ...', end = ' ')
        prg_AVR.eeprom_write( ih.tobinarray( start = 0, size = data_size))
        print('done')
        t = time.time() - t
        print('Written %d bytes in %.2f seconds (%.2f kbit/s)...' % (data_size, t, data_size / t * 8 / 1000))

    elif args.operation == 'flash_read':
        t = time.time()
        print('FLASH reading ...', end=' ')

        ih = IntelHex()  # create empty object
        ih.puts( 0, prg_AVR.flash_read(chip_flash_size*2))
        ih.write_hex_file(args.fr_filename)

        print( 'done')
        t = time.time() - t
        print('Read %d bytes in %.2f seconds (%.2f kbit/s)...' % (chip_flash_size*2, t, chip_flash_size*2 / t * 8 / 1000))

    elif args.operation == 'eeprom_read':
        t = time.time()
        print('EEPROM reading ...', end=' ')

        ih = IntelHex()  # create empty object
        ih.puts( 0, prg_AVR.EEPROM_read(chip_eeprom_size))
        ih.write_hex_file(args.er_filename)
        
        print( 'done')
        t = time.time() - t
        print('Read %d bytes in %.2f seconds (%.2f kbit/s)...' % (chip_eeprom_size, t, chip_eeprom_size / t * 8 / 1000))


