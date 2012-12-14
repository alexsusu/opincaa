/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.periferals;

import ro.pub.arh.beam.hardware.MachineConstants;
import ro.pub.arh.beam.hardware.emulator.core.array.Line;

/**
 *
 * @author rhobincu
 */
public class ArrayInternalDMA implements Peripheral{

    private long baseAddress;
    private Memory memory;
    private Line[] arrayMemory;

    private int[] struct;

    private static final int OFFSET_EXTERNAL_MEMORY_ADDRESS = 0;
    private static final int OFFSET_IN_VECTOR_STRIDE = 4;
    private static final int OFFSET_INTER_VECTOR_STRIDE = 8;
    private static final int OFFSET_LOCAL_MEMORY_ADDRESS = 12;
    private static final int OFFSET_VECTOR_COUNT_AND_START = 16;

    private static final int READ_MASK = 0x00000000;
    private static final int WRITE_MASK = 0x40000000;

public ArrayInternalDMA(int _baseAddress, Memory _memory){
    baseAddress = (long)_baseAddress & 0xffffffffL;
    memory = _memory;
    struct = new int[5];
}

public void assignMemory(Line[] _arrayMemory){
    arrayMemory = _arrayMemory;
}


public int read(int _address) {
    long _offset = ((long)_address & 0xffffffffL) - baseAddress;
    if(_offset >= OFFSET_EXTERNAL_MEMORY_ADDRESS && _offset <= OFFSET_VECTOR_COUNT_AND_START){
        //System.err.println("WARNING: Attempting to READ from register in internal DMA controller!");
        //return struct[(int)(_offset >>> 2)];
        return 0;
    }


    throw new RuntimeException("Read from outside mapped DMA area!");
}

public void write(int _address, int _data) {
    long _offset = ((long)_address & 0xffffffffL) - baseAddress;
    if(_offset >= OFFSET_EXTERNAL_MEMORY_ADDRESS && _offset <= OFFSET_VECTOR_COUNT_AND_START){
        struct[(int)(_offset >>> 2)] = _data;
        if(_offset == OFFSET_VECTOR_COUNT_AND_START){
            startTransfer();
        }
        return;
    }

     throw new RuntimeException("Write to outside mapped internal DMA area!");
}

public boolean isMapped(int _address) {
    long _addressLong = (long)_address & 0xffffffffL;
    return _addressLong >= baseAddress && _addressLong <= baseAddress + OFFSET_VECTOR_COUNT_AND_START;
}

private void startTransfer() {
       int _count = struct[OFFSET_VECTOR_COUNT_AND_START >> 2] & (~WRITE_MASK);
       int _memoryBase = struct[OFFSET_EXTERNAL_MEMORY_ADDRESS >> 2];
       if((struct[OFFSET_VECTOR_COUNT_AND_START >> 2] & WRITE_MASK) == READ_MASK){
           for(int i=0; i<_count; i++){
                Line _destination = arrayMemory[(struct[OFFSET_LOCAL_MEMORY_ADDRESS >> 2] >>> 2) + i];
                for(int j=0; j<MachineConstants.ARRAY_LENGTH; j++){
                    short _data = (short)(((int)memory.readByte(_memoryBase + (i * MachineConstants.ARRAY_LENGTH + j) * 2) & 0xff) |
                            (int)memory.readByte(_memoryBase + (i * MachineConstants.ARRAY_LENGTH + j) * 2 + 1) << 8);
                    _destination.setCell(j, _data);
                }
                _destination.updateGUI();
           }
           System.out.println("Internal READ transfer: EXT_ADDR=" + Integer.toHexString(_memoryBase) + " to INT_ADDR=" +
                   Integer.toHexString(struct[OFFSET_LOCAL_MEMORY_ADDRESS >> 2] >>> 2) + " and count =" + _count);
       }else{
          for(int i=0; i<_count; i++){
                Line _source = arrayMemory[(struct[OFFSET_LOCAL_MEMORY_ADDRESS >> 2] >>> 2) + i];
                for(int j=0; j<MachineConstants.ARRAY_LENGTH; j++){
                    short _data = _source.getCell(j);
                    memory.writeByte(_memoryBase + (i * MachineConstants.ARRAY_LENGTH + j) * 2, (byte)_data);
                    memory.writeByte(_memoryBase + (i * MachineConstants.ARRAY_LENGTH + j) * 2 + 1, (byte)(_data >>> 8));
                }
           }
          System.out.println("Internal WRITE transfer: EXT_ADDR=" + Integer.toHexString(_memoryBase) + " from INT_ADDR=" +
                   Integer.toHexString(struct[OFFSET_LOCAL_MEMORY_ADDRESS >> 2] >>> 2) + " and count =" + _count);
       }

}

public boolean hasInterruptCapabilities() {
    return false;
}

public boolean interruptActive() {
    throw new RuntimeException("Array Internal DMA has no interrupt capabilities.");
}

}
