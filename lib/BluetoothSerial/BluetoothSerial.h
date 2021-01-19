/**
 * The library is for Bluetooth Shield from Seeed Studio
 */

#ifndef __BLUETOOTH_SERIAL_H__
#define __BLUETOOTH_SERIAL_H__

#include "mbed.h"

#define BLUETOOTH_SERIAL_DEFAULT_BAUD       38400
#define BLUETOOTH_SERIAL_TIMEOUT            10000
#define BLUETOOTH_SERIAL_EOL                "\r\n"


class BluetoothSerial : public Stream {
public:
    BluetoothSerial(PinName tx, PinName rx);
    
    /**
     * Setup bluetooth module(serial port baud rate)
     */
    void setup();

    /**
     * Set bluetooth module as a master
     * \param   name    device name
     * \param   autoc   1: auto-connection, 0 not
     */
    void master(const char *name, uint8_t autoc = 0);

    /**
     * Set bluetooth module as a slave
     * \param   name    device name
     * \param   autoc   1: auto-connection, 0 not
     * \param   oaut    1: permit paired device to connect, 0: not
     */
    void slave(const char *name, uint8_t autoc = 0, uint8_t oaut = 1);

    /**
     * Inquire bluetooth devices and connect the specified device
     */
    int connect(const char *name);

    /**
     * Make the bluetooth module inquirable and available to connect, used in slave mode
     */
    int connect();
    
    int readable() {
        return _serial.readable();
    }
    
    int writeable() {
        return _serial.writeable();
    }

    
protected:
    virtual int _getc();
    virtual int _putc(int c);

    void clear();
    int readline(uint8_t *buf, int len, uint32_t timeout = 0);
    
    Serial     _serial;
    uint8_t    _buf[64];  
};

#endif // __BLUETOOTH_SERIAL_H__
