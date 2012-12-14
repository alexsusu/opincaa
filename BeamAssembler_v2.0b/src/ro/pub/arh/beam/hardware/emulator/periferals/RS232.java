/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.periferals;

/**
 *
 * @author rhobincu
 */
public class RS232 implements InputOutputPeripheral{

    public static final int BUFFER_SIZE = 2048;

    public static final int OFFSET_DIVS = 0;
    public static final int OFFSET_TX_D = 4;
    public static final int OFFSET_RX_D = 8;
    public static final int OFFSET_TX_C = 12;
    public static final int OFFSET_RX_C = 16;

    private int div;

    private long baseAddress;
    
    private byte[] inBuffer;
    private int available;
    private int inReadOffset;
    private int inWriteOffset;
    
    private byte[] outBuffer;
    private int filled;
    private int outReadOffset;
    private int outWriteOffset;

public RS232(int _baseAddress){
    baseAddress = (long)_baseAddress & 0xffffffffL;
    inBuffer = new byte[BUFFER_SIZE];
    outBuffer = new byte[BUFFER_SIZE];
}



public synchronized int read(int _address) {
    long _offset = ((long)_address & 0xffffffffL) - baseAddress;
    if(_offset == OFFSET_TX_D){
        System.err.println("WARNING: Attempting to READ from TS_D register in RS232 controller!");
        return 0;
    }

    if(_offset == OFFSET_DIVS){
        return div;
    }else if(_offset == OFFSET_RX_D){
        available--;
        int _data = inBuffer[inReadOffset++];
        inReadOffset %= BUFFER_SIZE;
        System.err.println("reading from index " + (inReadOffset - 1));
        return _data & 0xff;
    }else if(_offset == OFFSET_RX_C){
        return available;
    }else if(_offset == OFFSET_TX_C){
        return filled;
    }else{
        throw new RuntimeException("Reading from outside register range in RS232");
    }
}

public synchronized void write(int _address, int _data) {
    long _offset = ((long)_address & 0xffffffffL) - baseAddress;
    if(_offset != OFFSET_DIVS && _offset != OFFSET_TX_D){
        System.err.println("WARNING: Attempting to write " + Integer.toHexString(_data) + "to readonly registers in RS232 controller!");
        return;
    }

    if(_offset == OFFSET_DIVS){
        div = _data;
    }else if(_offset == OFFSET_TX_D){
        outBuffer[outWriteOffset++] = (byte)_data;
        outWriteOffset %= BUFFER_SIZE;
        filled++;
    }
}

public synchronized void externalWrite(int _data) {
    inBuffer[inWriteOffset++] = (byte)_data;
    inWriteOffset %= BUFFER_SIZE;
    available++;
}

public synchronized int externalRead(){
    filled--;
    int _data = outBuffer[outReadOffset++];
    outReadOffset %= BUFFER_SIZE;
    return _data;
}

public synchronized int externalAvailable(){
    return filled;
}

public synchronized int remoteFillled(){
    return available;
}

public boolean isMapped(int _address) {
    long _addressLong = (long)_address & 0xffffffffL;
    return _addressLong >= baseAddress && _addressLong <= baseAddress + OFFSET_RX_C;
}

public boolean hasInterruptCapabilities() {
    return false;
}

public boolean interruptActive() {
    throw new RuntimeException("RS232 has no interrupt capabilities.");
}

}
