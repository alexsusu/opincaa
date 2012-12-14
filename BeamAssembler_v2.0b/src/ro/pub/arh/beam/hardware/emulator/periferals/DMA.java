/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.periferals;

/**
 *
 * @author rhobincu
 */
public class DMA implements Peripheral{

    public static final int BUFFER_SIZE = 1024 * 8;

    public static final int OFFSET_DONE = 0;
    public static final int OFFSET_STATUS = 4;
    public static final int OFFSET_ADDRESS = 8;
    public static final int OFFSET_COMMAND = 12;

    private int baseAddress;
    
    private byte[] buffer;
    private int done;
    private int address;
    private int command;
    private DecodedCommand decodedCommand;
    private int status;

    private Memory memory;
    
public DMA(int _baseAddress, Memory _memory){
    baseAddress = _baseAddress;
    memory = _memory;
    buffer = new byte[BUFFER_SIZE];
}



public int read(int _address) {
    int _offset = _address - baseAddress;
    if(_offset == OFFSET_ADDRESS){
        System.err.println("WARNING: Attempting to READ from ADDRESS register in DMA controller!");
        return address;
    }

    if(_offset == OFFSET_COMMAND){
        System.err.println("WARNING: Attempting to READ from COMMAND register in DMA controller!");
        return command;
    }

    if(_offset == OFFSET_STATUS){
        return status;
    }else if(_offset == OFFSET_DONE){
        int _done = done;
        done = 0;
        return _done;
    }

    throw new RuntimeException("Read from outside mapped DMA area!");
}

public void write(int _address, int _data) {
    int _offset = _address - baseAddress;
    if(_offset == OFFSET_DONE || _offset == OFFSET_STATUS){
        System.err.println("WARNING: Attempting to write " + Integer.toHexString(_data) + " to readonly registers in DMA controller!");
        return;
    }

    if(_offset == OFFSET_ADDRESS){
        address = _data;
    }else if(_offset == OFFSET_COMMAND){
        command = _data;
        decodedCommand = new DecodedCommand(_data);
        startTransfer();
    }
}

public boolean isMapped(int _address) {
    return _address >= baseAddress && _address <= baseAddress + OFFSET_COMMAND;
}

private void startTransfer() {
    int _length = decodedCommand.length;

    if(decodedCommand.src == DecodedCommand.LOOP_CHANNEL && decodedCommand.dest == DecodedCommand.MEM_CHANNEL){
        for(int i=0; i<_length; i++){
            memory.writeByte(address + i, buffer[i]);
        }
        done = 1;
    }else if(decodedCommand.src == DecodedCommand.MEM_CHANNEL && decodedCommand.dest == DecodedCommand.LOOP_CHANNEL){
        for(int i=0; i<_length; i++){
            buffer[i] = memory.readByte(address + i);
        }
        done = 1;
    }else{
        throw new RuntimeException("Invalid src - dest combination in DMA command. SRC = " + decodedCommand.src + "; DEST = " + decodedCommand.dest);
    }
   
}

public boolean hasInterruptCapabilities() {
    return false;
}

public boolean interruptActive() {
    throw new RuntimeException("DMA has no interrupt capabilities.");
}

class DecodedCommand{

    int length;
    int src;
    int dest;
    int scatter;
    int gather;

    static final int MEM_CHANNEL = 1;
    static final int LOOP_CHANNEL = 2;
    static final int PCI_CHANNEL = 4;

    DecodedCommand(int _command){
        length = _command & 0xFFFF;
        dest = (_command >>> 16) & 0x7F;
        src = (_command >>> 23) & 0x7F;
        gather = (_command >>> 30) & 1;
        scatter = _command >>> 31;
    }
}

}
