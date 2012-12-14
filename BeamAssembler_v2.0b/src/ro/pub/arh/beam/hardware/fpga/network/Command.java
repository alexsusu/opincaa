/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.fpga.network;

import java.io.Serializable;

/**
 *
 * @author Echo
 */
public class Command implements Serializable{

    public static final int COMMAND_NONE = 0;
    public static final int COMMAND_READ = 1;
    public static final int COMMAND_WRITE = 2;
    public static final int COMMAND_RESET = 3;
    public static final int COMMAND_DISCONNECT = 4;
    
    public static final int RESPONSE_OK = 5;
    public static final int RESPONSE_ERROR = 6;
    
    public static final int UNKNOWN = 7;

    public static final String[] COMMAND_NAMES = {
        "COMMAND_NONE (0)",
        "COMMAND_READ (1)",
        "COMMAND_WRITE (2)",
        "COMMAND_RESET (3)",
        "COMMAND_DISCONNECT (4)",
        "RESPONSE_OK (5)",
        "RESPONSE_ERROR (6)",
        "UNKNOWN"
    };

    private int code;
    private String message;
    private byte[] buffer;
    private int address;
    private int length;

public Command(int _code){
    code = _code;
}

public Command(int _code, String _message){
    this(_code);
    message = _message;
}

public Command(int _code, int _address, int _length){
    this(_code);
    length = _length;
    address = _address;
}

public Command(int _code, int _address, byte[] _buffer){
    this(_code);
    buffer = _buffer;
    address = _address;
}

public Command(int _code, byte[] _buffer, int _address){
    this(_code);
    buffer = _buffer;
    address = _address;
}

public int getCode() {
    return code;
}

public String getMessage() {
    return message;
}

public byte[] getBuffer() {
    return buffer;
}

public int getAddress() {
    return address;
}

public int getLength() {
    return length;
}

@Override
public String toString(){
    switch(code){
        case COMMAND_NONE: return COMMAND_NAMES[code];
        case COMMAND_READ: return COMMAND_NAMES[code] + " from 0x" + Integer.toHexString(address) + ", " + length + " bytes";
        case COMMAND_WRITE: return COMMAND_NAMES[code] + " to 0x" + Integer.toHexString(address) + ", " + (buffer != null ? buffer.length : "!!NULL BUFFER!!") +" bytes";
        case COMMAND_RESET: return COMMAND_NAMES[code];
        case COMMAND_DISCONNECT: return COMMAND_NAMES[code];
        case RESPONSE_ERROR: return COMMAND_NAMES[code] + ": " + message;
        case RESPONSE_OK: return COMMAND_NAMES[code];    
        default: return COMMAND_NAMES[UNKNOWN];        
    }
}

}
