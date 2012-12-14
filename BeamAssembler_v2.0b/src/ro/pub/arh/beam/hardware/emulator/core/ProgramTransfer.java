/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.core;

import ro.pub.arh.beam.hardware.emulator.periferals.Memory;

/**
 *
 * @author Ares
 */
public class ProgramTransfer {

    private Memory memory;
    private ProgramBuffer programBuffer;
    private ThreadStatus[] threads;
    private Statistics[] statistics;

public ProgramTransfer(Memory _memory, ProgramBuffer _programBuffer, ThreadStatus[] _threads, Statistics[] _statistics){
    memory = _memory;
    programBuffer = _programBuffer;
    threads = _threads;
    statistics = _statistics;
}

public void startTransfer(int _threadId, int _programCounter){
int[] _program = new int[ProgramBuffer.BUFFER_SIZE];
//--
    _programCounter = _programCounter & ProgramBuffer.ADDRESS_MASK;
    for(int i=0; i<ProgramBuffer.BUFFER_SIZE; i++){
        _program[i] = memory.read(_programCounter + (i << 2));
    }

    programBuffer.fill(_threadId, _programCounter, _program);
    threads[_threadId].setStatus(ThreadStatus.THREAD_STATUS_READY);
    statistics[_threadId].newJumpOutsideBufferRange();
}

}
