
#include "BluetoothSerial.h"
#include <string.h>

#define LOG(args...)   // std::printf(args)

BluetoothSerial::BluetoothSerial(PinName tx, PinName rx) : _serial(tx, rx)
{   

}

void BluetoothSerial::setup()
{
     _serial.baud(BLUETOOTH_SERIAL_DEFAULT_BAUD);
}


void BluetoothSerial::master(const char *name, uint8_t autoc)
{
    _serial.puts("\r\n+STWMOD=1\r\n");
    _serial.printf("\r\n+STNA=%s\r\n", name);
    _serial.printf("\r\n+STAUTO=%d\r\n", autoc ? 1 : 0);
}


void BluetoothSerial::slave(const char *name, uint8_t autoc, uint8_t oaut)
{
    _serial.puts("\r\n+STWMOD=0\r\n");
    _serial.printf("\r\n+STNA=%s\r\n", name);
    _serial.printf("\r\n+STOAUT=%d\r\n", oaut ? 1 : 0);
    _serial.printf("\r\n+STAUTO=%d\r\n", autoc ? 1 : 0);
}

int BluetoothSerial::connect()
{
    clear();
    _serial.puts("\r\n+INQ=1\r\n"); // Make the bluetooth module inquirable
    LOG("BT: INQUIRING\r\n");
    
    const char *prefix = "CONNECT:";
    uint8_t prefix_len = sizeof("CONNECT:") - 1;
    for (uint8_t i = 0; i < 12; i++) {
        int len = readline(_buf, sizeof(_buf));
        if (len > 0) {
            LOG("%s\r\n", _buf);
            if (!memcmp(_buf, prefix, prefix_len)) {    // check prefix
                const char *suffix = "OK";
                uint8_t suffix_len = sizeof("OK") - 1;
                
                if (!memcmp(_buf + prefix_len, suffix, suffix_len)) { // check suffix
                    LOG("BT: CONNECTED\r\n");
                    return 1;
                }
                
                suffix = "FAIL";
                suffix_len = sizeof("FAIL") - 1;
                
                if (!memcmp(_buf + prefix_len, suffix, suffix_len)) { // check suffix
                    return 0;
                }
            }
        }
    }
    
    return 0;
}

int BluetoothSerial::connect(const char *name)
{
    char *mac;
    int name_len = strlen(name);
    
    clear();
    _serial.puts("\r\n+INQ=1\r\n");
    LOG("BT: INQUERING\r\n");
    while (1) {
        int len = readline(_buf, sizeof(_buf));     // +RTINQ=XX,XX,X,X,X,X;DEVICE_NAME
        if (len > 0) {
            LOG("%s\r\n", _buf);
            if (!memcmp(_buf, "+RTINQ=", sizeof("+RTINQ=") - 1)) {  // check prefix
                
                if (!memcmp(_buf + len - name_len, name, name_len)) { // check suffix
                    _buf[len - name_len - 1] = '\0';
                    mac = (char*)_buf + sizeof("+RTINQ=") - 1;
                    LOG("Connecting device: %s\r\n", mac);
                    
                    break;
                }
            }

        }
        
    }
    
    LOG("BT: CONNECTING\r\n");
    _serial.printf("\r\n+CONN=%s\r\n", mac);
    
    const char *prefix = "CONNECT:";
    int prefix_len = sizeof("CONNECT:") - 1;
    for (uint8_t i = 0; i < 6; i++) {
        int len = readline(_buf, sizeof(_buf), 0);
        if (len >= 0) {
            LOG("%s\r\n", _buf);
            if (!memcmp(_buf, prefix, prefix_len)) {    // check prefix
                const char *suffix = "OK";
                uint8_t suffix_len = sizeof("OK") - 1;
                
                if (!memcmp(_buf + prefix_len, suffix, suffix_len)) { // check suffix
                    LOG("BT: CONNECTED\r\n");
                    return 1;
                }
                
                suffix = "FAIL";
                suffix_len = sizeof("FAIL") - 1;
                
                if (!memcmp(_buf + prefix_len, suffix, suffix_len)) { // check suffix
                    LOG("TB: CONNECTION FAILED\r\n");
                    return 0;
                }
            }
        }
    }
    
    return 0;
}
    

int BluetoothSerial::_getc()
{
    return _serial.getc();
}

int BluetoothSerial::_putc(int c)
{
    return _serial.putc(c);
}

int BluetoothSerial::readline(uint8_t *buf, int len, uint32_t timeout)
{
    int get = 0;
    int count = timeout;
    while (count >= 0) {
        if (_serial.readable()) {
            char c = _serial.getc();
            buf[get] = c;
            if (c == '\n' && get && buf[get - 1] == '\r') {
                buf[get - 1] = '\0';
                return get - 1;
            }
            get++;
            if (get >= len) {
                LOG("Too long line, the buffer is not enough\r\n");
                return -(get + 1);
            }
            
            count = timeout;
        }
        
        if (timeout != 0) {
            count--;
        }
    }

    return -(get + 1);
}

void BluetoothSerial::clear()
{
    int count = 0;
    
    LOG("Clear previous command output\r\n");
    do {
        count++;
        if (_serial.readable()) {
            int get = _serial.getc();
            count = 0;
            
            LOG("%c", get);
        }
    } while (count < BLUETOOTH_SERIAL_TIMEOUT);
    LOG("done\r\n");
}


   
