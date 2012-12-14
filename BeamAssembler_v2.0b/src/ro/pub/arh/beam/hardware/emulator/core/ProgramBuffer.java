/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.core;

import ro.pub.arh.beam.hardware.MachineConstants;

/**
 *
 * @author Ares
 */
public class ProgramBuffer {

    public static final int BUFFER_SIZE = 32;       //words
    public static final int ADDRESS_MASK = ~0x7F;

    private BufferLine[] globalBuffer;

//------------------------------------------------------------------------------
public ProgramBuffer(){
    globalBuffer = new BufferLine[MachineConstants.THREAD_COUNT];
    for(int i=0; i<MachineConstants.THREAD_COUNT; i++){
        globalBuffer[i] = new BufferLine();
    }
}

//------------------------------------------------------------------------------
public int read(int _threadId, int _programCounter) throws OutsideOfBufferRangeException{
    if(((_programCounter ^ globalBuffer[_threadId].baseAddress) & ADDRESS_MASK) != 0){
        throw new OutsideOfBufferRangeException();
    }
    return globalBuffer[_threadId].buffer[(_programCounter >> 2) % BUFFER_SIZE];
}

//------------------------------------------------------------------------------
public void fill(int _threadId, int _programCounter, int[] _program){
    globalBuffer[_threadId].baseAddress = _programCounter & ADDRESS_MASK;
    globalBuffer[_threadId].buffer = _program;
}

//------------------------------------------------------------------------------
boolean outOfRange(int _threadId, int _programCounter) {
    return ((_programCounter ^ globalBuffer[_threadId].baseAddress) & ADDRESS_MASK) != 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class BufferLine{
    int baseAddress;
    int[] buffer;

BufferLine(){
    buffer = new int[BUFFER_SIZE];
}

}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------