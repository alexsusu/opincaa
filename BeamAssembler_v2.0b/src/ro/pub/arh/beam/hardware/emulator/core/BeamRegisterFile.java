/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.core;

import ro.pub.arh.beam.execution.gui.ExecutionGui;
import ro.pub.arh.beam.hardware.MachineConstants;

/**
 *
 * @author Ares
 */
public class BeamRegisterFile {

    public static final int REGISTER_BIT_SIZE = 32;
    public static final int REGISTER_COUNT = 16;
    public static final int STACK_REG = 14;
    //public static final int PC_REG_INDEX = 0;

    private int[][] registerFile;
    private int[] extRegs;


//------------------------------------------------------------------------------
public BeamRegisterFile(){
    registerFile = new int[MachineConstants.THREAD_COUNT][REGISTER_COUNT];
    extRegs = new int[MachineConstants.THREAD_COUNT];
}

//------------------------------------------------------------------------------
public void write(int _threadId, int _address, int _data){
    if(_address < 0 && _address >= REGISTER_COUNT){
        throw new RuntimeException("Attempt of writing outside regfile boudaries.");
    }

    if(_threadId < 0 && _threadId >= MachineConstants.THREAD_COUNT){
        throw new RuntimeException("Attempt of writing with an invalid thread id.");
    }

    registerFile[_threadId][_address] = _data;
    ExecutionGui.lastGui.updateRegisterFile(_threadId, _data, _address);
}

//------------------------------------------------------------------------------
public int read(int _threadId, int _address){
    if(_address < 0 || _address >= REGISTER_COUNT){
        throw new RuntimeException("Attempt of reading from outside regfile boudaries.");
    }

    if(_threadId < 0 || _threadId >= MachineConstants.THREAD_COUNT){
        throw new RuntimeException("Attempt of reading with an invalid thread id.");
    }

    return registerFile[_threadId][_address];
}

//------------------------------------------------------------------------------
public int[][] copy(){
   /*int[][] _newArray = new int[Core.THREAD_COUNT][];
    for(int i=0; i<Core.THREAD_COUNT; i++){
        _newArray[i] = new int[REGISTER_COUNT];
        System.arraycopy(registerFile[i], 0, _newArray[i], 0, REGISTER_COUNT);
    }
    return _newArray;
    */
    return registerFile;
}

public int getStackBase(int _thread) {
    return read(_thread, STACK_REG);
}

public void writeExt(int _threadIndex, int _value){
    extRegs[_threadIndex] = _value;
}

public int readExt(int _threadIndex){
    return extRegs[_threadIndex];
}

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------