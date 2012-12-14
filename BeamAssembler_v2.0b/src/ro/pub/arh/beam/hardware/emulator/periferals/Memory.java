/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.periferals;

import java.util.Vector;
import ro.pub.arh.beam.execution.gui.ExecutionGui;
import ro.pub.arh.beam.structs.AddressDataPair;
import ro.pub.arh.beam.structs.MemoryLocationsList;

/**
 *
 * @author Ares
 */
public class Memory {


    public static final int RS232_ADDRESS = 0x24000000;
    public static final int DMA_ADDRESS = 0x22000000;
    public static final int IO_DMA_BASE_ADDRESS = 0x28000c20;
    public static final int RTC_ADDRESS = 0x2A000000;

    private MemoryLocationsList mem;
    private long size;
    private Vector<Peripheral> peripherals;

public Memory(long _size){
    size = _size;
    mem = new MemoryLocationsList();
    peripherals = new Vector();
    peripherals.add(new RS232(RS232_ADDRESS));
    peripherals.add(new ArrayInternalDMA(IO_DMA_BASE_ADDRESS, this));
    peripherals.add(new RTC(RTC_ADDRESS));
    //peripherals.add(new DMA(DMA_ADDRESS, this));
}

public RS232 getRS232(){
    for(int i=0; i<peripherals.size(); i++){
        if(peripherals.elementAt(i) instanceof RS232){
            return (RS232)peripherals.elementAt(i);
        }
    }
    return null;
}

public ArrayInternalDMA getIODMA(){
    for(int i=0; i<peripherals.size(); i++){
        if(peripherals.elementAt(i) instanceof ArrayInternalDMA){
            return (ArrayInternalDMA)peripherals.elementAt(i);
        }
    }
    return null;
}

public int read(int _address){

    for(int i=0; i<peripherals.size(); i++){
        if(peripherals.elementAt(i).isMapped(_address)){
            return peripherals.elementAt(i).read(_address);
        }
    }

    if(((long)_address & 0xFFFFFFFFL) >= size){
        throw new RuntimeException("Attempt to read from outside memory boundaries: " +
                Integer.toHexString(_address));
    }
    _address &= ~3;

    return mem.get(_address).data;
}

public void write(int _address, int _data){

   for(int i=0; i<peripherals.size(); i++){
        if(peripherals.elementAt(i).isMapped(_address)){
            peripherals.elementAt(i).write(_address, _data);
            ExecutionGui.lastGui.updateMemoryLocation(_address, _data);
            return;
        }
    }

    if(((long)_address & 0xFFFFFFFFL) >= size){
        throw new RuntimeException("Attempt to write outside memory boundaries: " +
                Integer.toHexString(_address));
    }
    _address &= ~3;

    mem.add(new AddressDataPair(_address, _data));
//    System.err.print("Updating location " + _address + "\n");
    ExecutionGui.lastGui.updateMemoryLocation(_address, _data);
}

public void writeByte(int _address, byte _data){
    int _oldData = read(_address);
    _oldData &= ~(0xFF << ((_address & 3) << 3));
    _oldData |= ((int)_data & 0xff) << ((_address & 3) << 3);
    write(_address, _oldData);
}

public byte readByte(int _address){
    int _data = read(_address);
    return (byte)(_data >>> ((_address & 3) << 3));
}


public int getSize() {
    return (int)(size >>> 2);
}

public int getByteSize() {
    return (int)size;
}

public void write(int _address, byte[] _buffer) {
    for(int i=0; i<_buffer.length; i++){
        writeByte(_address + i, _buffer[i]);
    }
}

public byte[] read(int _address, int _length) {
    byte[] _buffer = new byte[_length];
    for(int i=0; i<_length; i++){
        _buffer[i] = readByte(_address + i);
    }

    return _buffer;
}

public Vector<Peripheral> getPeripherals() {
    return peripherals;
}

public RTC getRTC() {
    for(int i=0; i<peripherals.size(); i++){
        if(peripherals.elementAt(i) instanceof RTC){
            return (RTC)peripherals.elementAt(i);
        }
    }
    return null;
}

}
