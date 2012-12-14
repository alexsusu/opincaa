/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package ro.pub.arh.beam.hardware.emulator;

import java.io.IOException;
import ro.pub.arh.beam.hardware.emulator.tools.LineSeeker;
import javax.swing.JOptionPane;
import ro.pub.arh.beam.hardware.emulator.core.Core;
import ro.pub.arh.beam.hardware.emulator.core.Statistics;
import ro.pub.arh.beam.execution.gui.ExecutionGui;
import ro.pub.arh.beam.hardware.emulator.periferals.Memory;
import ro.pub.arh.beam.hardware.emulator.periferals.Peripheral;
import ro.pub.arh.beam.hardware.ExecutionEnvironment;
import ro.pub.arh.beam.structs.Function;
import ro.pub.arh.beam.structs.Program;

/**
 *
 * @author Ares
 */
public class Emulator implements ExecutionEnvironment{

    private Core core;
    private Memory memory;
    private ExecutionGui emulatorGui;
    private static final int DEFAULT_MEM_SPACE = 0x27ffffff;
    private int instructionCount;
    private boolean askedForReset;
    private Program program;

public Emulator(){
    this(DEFAULT_MEM_SPACE);
}

public Emulator(int _memorySpace){
    memory = new Memory(_memorySpace);
    core = new Core(memory);
    askedForReset = false;
    program = null;
}

public void assignGui(ExecutionGui _emulatorGui){
    emulatorGui = _emulatorGui;
    emulatorGui.assign(this);
}

public void writeByte(int _address, int _data){
    memory.writeByte(_address, (byte)_data);
}

public int read(int _address){
    return memory.read(_address);
}

public Peripheral getPeripheral(String _id){
    if(_id.equals("RS232")){
        return memory.getRS232();
    }else if(_id.equals("IODMA")){
        return memory.getIODMA();
    }else{
        throw new RuntimeException("Unknown peripheral id.");
    }
}

public void boot(){
    core.boot();
    (new Thread(this)).start();
}

public synchronized void run(int _instructionCount){
    instructionCount = _instructionCount;
    notify();
}

@Override
public void run(){
    synchronized(this){
        try {
            while(true){
                wait();
                if(askedForReset){
                    askedForReset = false;
                    break;
                }
                int _status = core.start(instructionCount);
                emulatorGui.executionDone(_status);
            }
        }catch (Exception _ex) {
            _ex.printStackTrace();
            emulatorGui.executionError();
            JOptionPane.showMessageDialog(emulatorGui,"Exception " + _ex.getClass().getName() + " thrown: " + _ex.getMessage(), "Exception thrown!", JOptionPane.ERROR_MESSAGE);
        }
        notify();
    }
}

public Statistics[] getBeamStatistics(){
    return core.getBeamStatistics();
}

public LineSeeker getLineSeeker(){
    return program.getLineSeeker();
}

public int getPcForThread(int _threadId){
    return core.getPcForThread(_threadId);
}

public void stopExecution(){
    core.stop();
}

public void loadProgram(Program _program) {
    for(int i=0; i<_program.size(); i++){
        Function _function = _program.elementAt(i);
        for(int j=0; j<_function.size(); j++){
            memory.write(_function.elementAt(j).getAddress(), _function.elementAt(j).getInstruction());
        }
    }
    program = _program;
}

public void saveWaveform(boolean _enable) {
}

public synchronized void reset() {
    askedForReset = true;
    notify();
    try {
        wait();
    } catch (InterruptedException ex) {
       ex.printStackTrace();
    }
    boot();
    emulatorGui.executionDone(core.STATUS_TIMEOUT);
}

public synchronized void release(){
    askedForReset = true;
    notify();
}

public boolean addBreakpoint(int _line) {
    return core.addBreakpoint(getLineSeeker().getAddress(_line));
}

public boolean removeBreakpoint(int _line) {
    return core.removeBreakpoint(getLineSeeker().getAddress(_line));
}

public int[] readBreakpoints() {
    return core.readBreakpoints();
}

public void loadBinary(byte[] _buffer, int _address) throws IOException{
    memory.write(_address, _buffer);
}

public byte[] saveBinary(int _address, int _length){
    return memory.read(_address, _length);
}

public void init() {
    //do nothing here
}


}
