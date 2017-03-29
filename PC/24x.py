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


class ESPROG_I2C:
    # Commands

    _MAGIC_CMD_I2C_IO = 0x12345678
    _MAGIC_CMD_I2C_START = 0x12345668
    _MAGIC_CMD_I2C_STOP = 0x12345669
    _MAGIC_CMD_I2C_RESET = 0x12345667
    _MAGIC_CMD_NONE = 0


    def __init__(self):
        self.link = None

    """ Try connecting repeatedly until successful, or giving up """
    def connect(self, ip=None, port=1234):
        if(ip == None):
            raise Exception('Bad params')
        print('Connecting to _I2C programmer...')
        for _ in range(4):
            self.link = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.link.settimeout(1)
            try:
                self.link.connect((ip, port))
                time.sleep(0.1)
                rx = self.link.recv(12)
#                print('RX1: ' + str(rx))
                if (rx == b'8266_PROG_v1'):
                    self.link.send(b'_I2C ')
                    time.sleep(0.1)
                    rx = self.link.recv(17)
#                    print('RX2: ' + str(rx))
                    if (rx == b'I2C_PROG_v1'):
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
    def command(self, cmd: int=0, tx_data: bytes=b'', rx_len: int=0):
        if (cmd == None) or (rx_len == None):
            raise Exception('Bad params')
        #print( 'Read ' + str(size) + ' bytes')
        stub = struct.pack('<III', cmd, len(tx_data), rx_len)
        stub += tx_data
        #print("#CMD " + " ".join("{:02x}".format(c) for c in stub))
        self.link.sendall(stub)
        data = self.read_exactly(rx_len)
        return data

    """ Scan bus """
    def scan(self):
        dev_ids = []

        self.RESET()
        for addr in range(0, 256): # addrs 0, 1, ..  255
            stub = struct.pack('Bx', addr)
            self.START()
            ack = self.IO(stub, 2)
            self.STOP()

            if(ack[1] == 0):
                dev_ids.append(addr)

        return dev_ids
 
    """ Send a request and read the response """
    def get_type(self, addr: bytes=None, def_val: str=None):
        if (addr == None):
            raise Exception('Bad params')
        return self.devices_id.get(addr, def_val)

    """ Write block to flash """
    def read_exactly(self, size):
        buffer = b''
        while len(buffer) < size:
            data = self.link.recv(size - len(buffer))
            if not data:
                raise Exception('No receive data')
            buffer += data
        return buffer

    """ I2C_START """
    def START(self):
        self.command(self._MAGIC_CMD_I2C_START)
 
    """ I2C_STOP """
    def STOP(self):
        self.command(self._MAGIC_CMD_I2C_STOP)
 
    """ I2C_RESET """
    def RESET(self):
        self.command(self._MAGIC_CMD_I2C_RESET)
 
    """ I2C_IO """
    def IO(self, tx_data: bytes=b'', rx_len: int=0):
        return self.command(self._MAGIC_CMD_I2C_IO, tx_data, rx_len)
 




"""_I2C EEPROM 24x CLASS"""
class EEPROM_24X:

#    devices_id = { a:"24Cxx" for a in range(160, 175)}#, 0x00:"TEST"}

    """ Page size dict based on _I2C_EEProm_Reading_and_Programming.pdf_"""
    x24_page_sizes = { 0:1, 1:8, 2:8, 4:16, 8:16, 16:16, 32:64, 64:32, 128:64, 256:64, 512:128, 1024:128}
#    x24_page_sizes = { 0:1, 1:8, 2:8, 25:16, 4:16, 8:16, 16:16, 32:64, 64:32, 128:64, 256:64, 512:128, 1025:128}
    
    """ Init """
    def __init__(self, _i2c = None, x24_size: int = 2, p_size = None):
        if( _i2c == None):
            raise Exception('No _I2C interface')
        self.i2c = _i2c
        self.i2c.RESET()
        self.size = x24_size * 128
        
        if( self.size > 16):
            self.addr_16bit = True
        else:
            self.addr_16bit = False

        if( p_size != None):
            self.page_size = int( p_size)
        else:
            self.page_size = self.x24_page_sizes.get( x24_size, 1)
        
        print( 'Device info:') 
        if( self.addr_16bit):
            print( '  16 bit address')
        else:  
            print( '  8 bit address')
        print( '  Page size: ' + str( self.page_size) + ' bytes') 
#        if( self.page_size == 1):
#            print( 'Warning: single byte write operation') 

    """ READ byte(s) """
    def read_single_byte(self, addr: int=0):
        val = None
        valid = False

        # set addr
        if( self.addr_16bit):
            stub = struct.pack('BBBBBB', 0xA0 | (((addr >> 16) & 0x07) << 1), 0xFF, ( ( addr >> 8) & 0x0FF), 0xFF, (addr & 0x0FF), 0xFF) # dummy write
        else:
            stub = struct.pack('BBBB', 0xA0 | (((addr >> 8) & 0x07) << 1), 0xFF, (addr & 0x0FF), 0xFF) # dummy write
        self.i2c.START()
        ack = self.i2c.IO(stub,  len(stub))

        # read byte
        if(ack[1] == 0) and (ack[3] == 0):
            if( self.addr_16bit):
                stub = struct.pack('BBBB', 0xA1 | (((addr >> 16) & 0x07) << 1), 0xFF, 0xFF, 0xFF) # read
            else:
                stub = struct.pack('BBBB', 0xA1 | (((addr >> 8) & 0x07) << 1), 0xFF, 0xFF, 0xFF) # read
            self.i2c.START()
            ack = self.i2c.IO(stub, len(stub))
            val = ack[2]
            if(ack[1] == 0): # ToDo : correct chk
                valid = True

        self.i2c.STOP()

        return (val, valid)

    """ READ byte(s) """
    def read_bytes(self, addr: int=0, lenght: int = 1):
        val = None
        valid = False

        if( lenght < 1):
            return (val, valid)
        if( lenght > 512):
            lenght = 512

        # set addr
        if( self.addr_16bit):
            stub = struct.pack('BBBBBB', 0xA0 | (((addr >> 16) & 0x07) << 1), 0xFF, ( ( addr >> 8) & 0x0FF), 0xFF, (addr & 0x0FF), 0xFF) # dummy write
        else:
            stub = struct.pack('BBBB', 0xA0 | (((addr >> 8) & 0x07) << 1), 0xFF, (addr & 0x0FF), 0xFF) # dummy write
        self.i2c.START()
        ack = self.i2c.IO(stub,  len(stub))

        # read byte
        if(ack[1] == 0) and (ack[3] == 0):
            if( self.addr_16bit):
                stub = struct.pack('BB', 0xA1 | (((addr >> 16) & 0x07) << 1), 0xFF) # read
            else:
                stub = struct.pack('BB', 0xA1 | (((addr >> 8) & 0x07) << 1), 0xFF) # read
            stub += b'\xff\x00' * ( lenght - 1) # ACK - continue read
            stub += b'\xff\xff'                 # NACK - stop read
            self.i2c.START()
            ack = self.i2c.IO(stub, len(stub))
            val = ack[2::2]
            if(ack[1] == 0): # ToDo : correct chk
                valid = True
#                print( val)

        self.i2c.STOP()

        return (val, valid)

    """ READ byte(s) """
    def read_sinle_bytes(self, addr: int=0, size: int=1, progress_cb = None):
        val = b''
        
        for a in range(addr, (addr + size)):
            ret = self.read_single_byte(a)
            if(ret[1] == False):
                return (None, False)
            if( progress_cb != None):
                progress_cb( a)
            val += bytes( [ ret[0]])

        return (val, True)

    """ READ byte(s) """
    def read(self, addr: int=0, lenght: int=1, progress_cb = None):
        val = b''
        a = addr
        
        while( a < ( addr + lenght)):
            ret = self.read_bytes(a, ( addr + lenght - a))
            if(ret[1] == False):
                return (None, False)
            val += ret[ 0]
            a += len( ret[ 0])
            if( progress_cb != None):
                progress_cb( a)

        return (val, True)

    """ WRITE byte """
    def write_single_byte(self, addr: int=0, data=None):
        if(data == None):
            raise Exception('Empty data')
        # set addr + write
        if( self.addr_16bit):
            stub = struct.pack('BBBBBBBB', 0xA0 | (((addr >> 16) & 0x07) << 1), 0xFF, ( ( addr >> 8) & 0x0FF), 0xFF, (addr & 0x0FF), 0xFF, data, 0xFF) # write
        else:
            stub = struct.pack('BBBBBB', 0xA0 | (((addr >> 8) & 0x07) << 1), 0xFF, (addr & 0x0FF), 0xFF, data, 0xFF) # write
        
#       print("W " + " ".join("{:02x}".format(c) for c in stub))

        self.i2c.START()
        ack = self.i2c.IO(stub, len(stub))
        self.i2c.STOP()

        if(ack[1] != 0): # ToDo : correct chk
            return False
        
        #polling
        for _ in range(100):
            stub = b'\xA0\xFF'
            self.i2c.START()
            ack = self.i2c.IO(stub, len(stub))
            self.i2c.STOP()
            if( ack[ 1] == 0):
                return True
            
#            print("P " + " ".join("{:02x}".format(c) for c in stub) + " ---- " + " ".join("{:02x}".format(c) for c in ack))
        raise Exception('No ACK')

        return False

    """ WRITE bytes """
    def write_bytes(self, addr: int=0, data=None):
        if(data == None):
            raise Exception('Empty data')
        # set addr + write
        if( self.addr_16bit):
            stub = struct.pack('BBBBBB', 0xA0 | (((addr >> 16) & 0x07) << 1), 0xFF, ( ( addr >> 8) & 0x0FF), 0xFF, (addr & 0x0FF), 0xFF) # write
        else:
            stub = struct.pack('BBBB', 0xA0 | (((addr >> 8) & 0x07) << 1), 0xFF, (addr & 0x0FF), 0xFF) # write
        
        for b in data:
            stub += struct.pack('BB', b, 0xFF) # write
#       print("W " + " ".join("{:02x}".format(c) for c in stub))

        self.i2c.START()
        ack = self.i2c.IO(stub, len(stub))
        self.i2c.STOP()

        if(ack[1] != 0): # ToDo : correct chk
            return False
        
        #polling
        for _ in range(100):
            stub = b'\xA0\xFF'
            self.i2c.START()
            ack = self.i2c.IO(stub, len(stub))
            self.i2c.STOP()
            if( ack[ 1] == 0):
                return True
            
#            print("P " + " ".join("{:02x}".format(c) for c in stub) + " ---- " + " ".join("{:02x}".format(c) for c in ack))
        raise Exception('No ACK')

        return False
 
    """ WRITE byte(s) """
    def write_single_bytes(self, addr: int=0, data: bytes=None, progress_cb = None):
        valid = False
        caddr = addr

        if(data == None):
            raise Exception('Empty data')

        for c in data:
            self.write_single_byte(caddr, c)
            if( progress_cb != None):
                progress_cb( caddr)
            caddr += 1
#            if( ack[ 1] == 0): # ToDo : correct chk
#                valid = True

        self.i2c.STOP()

        return valid


    """ WRITE byte(s) """
    def write(self, addr: int=0, data: bytes=None, progress_cb = None):
        valid = False
        caddr = addr

        if(data == None):
            raise Exception('Empty data')

        for i in range( 0, len( data), self.page_size):
            self.write_bytes(caddr, data[ i: i+self.page_size])
            caddr += self.page_size
            if( progress_cb != None):
                progress_cb( caddr)
#            if( ack[ 1] == 0): # ToDo : correct chk
#                valid = True

        self.i2c.STOP()

        return valid




def progress_cb( addr):
    sys.stdout.write(' Addr: ' + str( addr) + '\r')
    sys.stdout.flush()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='ESP8266 _I2C programmer', prog='24x.py')

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
        help='EEPROM size ( 24x_SS_). Use "24x.py --ip x.x.x.x --port x -s _SS_ read [file]"',
        default=2)

    subparsers = parser.add_subparsers(dest='operation',
        help='Run 24x {command} -h for additional help')

    parser_read_id = subparsers.add_parser('read', help='Read EEPROM 24x')
    parser_read_id.add_argument( 'r_filename', help='File name for store data from EEPROM')
    
    parser_write_id = subparsers.add_parser('write', help='Write EEPROM 24x')
    parser_write_id.add_argument( 'w_filename', help='File name data to write EEPROM')


    args = parser.parse_args()

    # Create the ESPROM connection object, if needed
    prg_i2c = None
    prg_i2c = ESPROG_I2C()
    prg_i2c.connect(args.ip, args.port)
    prg_i2c.sync()

    e24c = EEPROM_24X(prg_i2c, args.size)

    if(args.operation == 'read'):
        with open( args.r_filename, "wb") as fo:
            rd_buf = e24c.read( 0, e24c.size, progress_cb)[ 0]
            if( rd_buf != None):
                print( "Readed " + str( len( rd_buf)) + " bytes")
                fo.write( rd_buf)

    if(args.operation == 'write'):
        with open( args.w_filename, "rb") as fi:
            wr_buf = fi.read()
            print( "Readed " + str( len( wr_buf)) + " bytes from file")
            print( "Write to EEPROM")
            if( len( wr_buf) > 0):
                ret = e24c.write( 0, wr_buf[ 0 : e24c.size], progress_cb)

    print("")
    print("Done")
