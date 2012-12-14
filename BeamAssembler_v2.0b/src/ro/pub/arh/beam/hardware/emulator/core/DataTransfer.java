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
public class DataTransfer {

    public static final int REG_COUNT_TO_SAVE = 16;

    private Memory memory;
    private BeamRegisterFile registerFile;
    private ThreadStatus[] threads;
    private ProgramTransfer programTransfer;

public DataTransfer(Memory _memory, BeamRegisterFile _registerFile, ThreadStatus[] _threads, ProgramTransfer _programTransfer){
    memory = _memory;
    registerFile = _registerFile;
    threads = _threads;
    programTransfer = _programTransfer;
}

//public void readRegisterFile(int _threadIndex, int _stackBase){
//int _baseAddress = memory.read(_stackBase) + 4;
//int _regsRead = REG_COUNT_TO_SAVE;
////--
//    registerFile.writeExt(_threadIndex, memory.read(_baseAddress));
//    _baseAddress += 4;
//    threads[_threadIndex].setProgramCounter(memory.read(_baseAddress));
//    _baseAddress += 4;
//    for(int i=_baseAddress; i< _baseAddress + 4 * REG_COUNT_TO_SAVE; i+=4){
//        registerFile.write(_threadIndex, --_regsRead, memory.read(i));
//    }
//
//    programTransfer.startTransfer(_threadIndex, threads[_threadIndex].getProgramCounter());
//}
//
//public void writeRegisterFile(int _threadIndex, int _memDumpAddress) {
//int _regsWritten = REG_COUNT_TO_SAVE;
////--
//    int _baseAddress = registerFile.read(_threadIndex, BeamRegisterFile.STACK_REG);
//    for(int i=_baseAddress; i> _baseAddress - 4 * REG_COUNT_TO_SAVE; i-=4){
//        memory.write(i, registerFile.read(_threadIndex, --_regsWritten));
//    }
//    _baseAddress -= REG_COUNT_TO_SAVE * 4;
//    memory.write(_baseAddress, threads[_threadIndex].getProgramCounter());
//    _baseAddress -= 4;
//    memory.write(_baseAddress, registerFile.readExt(_threadIndex));
//    memory.write(_memDumpAddress, _baseAddress - 4);
//}

 public void load(int _threadId, int _memoryAddress, int _destinationReg) {
    registerFile.write(_threadId, _destinationReg, memory.read(_memoryAddress));
 }

 public void store(int _threadId, int _memoryAddress, int _sourceRegister) {
    memory.write(_memoryAddress, registerFile.read(_threadId, _sourceRegister));
 }

public void loadByte(int _threadId, int _memoryAddress, byte _destinationReg) {
    registerFile.write(_threadId, _destinationReg, memory.readByte(_memoryAddress) << 24 >> 24);

}

public void storeByte(int _threadId, int _memoryAddress, int _sourceRegister) {
    int _data = registerFile.read(_threadId, _sourceRegister);
    memory.writeByte(_memoryAddress, (byte)_data);
}

public void loadShort(int _threadId, int _memoryAddress, byte _destinationReg) {
    int _data;
    _data = (memory.readByte(_memoryAddress + 1) << 24 >> 16) | (memory.readByte(_memoryAddress) & 0xff);
    registerFile.write(_threadId, _destinationReg, _data);
}

 public void storeShort(int _threadId, int _memoryAddress, int _sourceRegister) {
     int _data = registerFile.read(_threadId, _sourceRegister);
     memory.writeByte(_memoryAddress, (byte)_data);
     memory.writeByte(_memoryAddress + 1, (byte)(_data >> 8));
 }

}
