/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.simulator;

import javax.swing.JOptionPane;
import ro.pub.arh.beam.hardware.emulator.core.Statistics;
import ro.pub.arh.beam.hardware.emulator.periferals.Peripheral;
import ro.pub.arh.beam.hardware.emulator.tools.LineSeeker;
import ro.pub.arh.beam.execution.gui.ExecutionGui;
import ro.pub.arh.beam.hardware.ExecutionEnvironment;
import ro.pub.arh.beam.hardware.emulator.periferals.InputOutputPeripheral;
import ro.pub.arh.beam.hardware.MachineConstants;
import ro.pub.arh.beam.hardware.emulator.core.ThreadStatus;
import ro.pub.arh.beam.hardware.emulator.core.array.Line;
import ro.pub.arh.beam.hardware.emulator.core.array.ArrayRegisterFile;
import ro.pub.arh.beam.hardware.emulator.core.array.ArrayCore;
import ro.pub.arh.beam.hardware.emulator.tools.Instruction;
import ro.pub.arh.beam.structs.Function;
import ro.pub.arh.beam.structs.Program;
import ubiCORE.jroot.common.utils.Convertor;

/**
 *
 * @author rhobincu
 */
public class Simulator implements ExecutionEnvironment{

    public static final int THREAD_COUNT = 4;
    public static final int REGISTER_COUNT = 16;

    public static final int BREAKPOINT_COUNT = 4;

    private ConnexBEAMSimulator wrapper;
    private Program program;
    private ExecutionGui executionGui;
    
    private int cycleCount;
    private long duration;
    private boolean resetRequested;
    private boolean simulationActive;

    private int[] breakpointsUsed;


public Simulator(){
        simulationActive = true;
        breakpointsUsed = new int[BREAKPOINT_COUNT];
        program = null;
}

public void init() {
    try{
        wrapper = new ConnexBEAMSimulator();
        for(int i=0; i<BREAKPOINT_COUNT; i++){
            wrapper.setBreakpoint(i, 0, 0);
        }
        simulationActive = true;
    }catch(Throwable _t){
        _t.printStackTrace();
        throw new RuntimeException("Unable to load the ConnexBEAMSimulator object file. Make sure LD_LIBRARY_PATH is set up correctly!");
    }
}

public void assignGui(ExecutionGui _executionGui) {
    executionGui = _executionGui;
    executionGui.assign(this);
}


public void loadProgram(Program _program) {
    for(int i=0; i<_program.size(); i++){
        Function _function = _program.elementAt(i);
        for(int j=0; j<_function.size(); j++){
            write(_function.elementAt(j).getAddress(), _function.elementAt(j).getInstruction());
        }
    }
    program = _program;
}

public Peripheral getPeripheral(String _id) {
    if(_id.equals("RS232")){
        return new RS232Sim();
    }else{
        throw new RuntimeException("Unknown peripheral ID.");
    }
}

public void writeByte(int _address, int _data) {
    byte[] _buffer = new byte[1];
    _buffer[0] = (byte)_data;
    if(_address >= 0 && _address < 0x20000000){
        wrapper.writeDDR(_buffer, _address, 1);
    }else if(_address >= 0x20000000){
        wrapper.writeFlash(_buffer, _address - 0x20000000, 1);
    }
}

public void write(int _address, int _data) {
    byte[] _buffer = Convertor.IntToBytes_v2(_data);
    if(_address >= 0 && _address < 0x20000000){
        wrapper.writeDDR(_buffer, _address, 4);
    }else if(_address >= 0x20000000){
        wrapper.writeFlash(_buffer, _address - 0x20000000, 4);
    }
}

public int read(int _address) {
    byte[] _buffer = new byte[4];
    if(_address >= 0 && _address < 0x20000000){
        wrapper.readDDR(_buffer, _address, 4);
    }else if(_address >= 0x20000000){
        wrapper.readFlash(_buffer, _address - 0x20000000, 4);
    }
    return Convertor.BytesToInt_v2(_buffer);
}

public synchronized void run(int _cycleCount) {
    cycleCount = _cycleCount;
    notify();
}

public void stopExecution() {
    wrapper.stop();
}

public int getPcForThread(int _threadId) {
    int[] _pc = new int[Simulator.THREAD_COUNT];
    wrapper.readPC(_pc);
    return _pc[_threadId];
}

public Statistics[] getBeamStatistics() {
    throw new UnsupportedOperationException("Not supported yet.");
}


@Override
public void run(){
    int _result = 0;
    synchronized(this){
        try {
            while(simulationActive){
                wait();
                if(!simulationActive){
                    break;
                }else if(resetRequested){
                    resetRequested = false;
                    resetCore();
                }else{
                    long _interval = System.currentTimeMillis();
                    _result = wrapper.run(cycleCount);

                    ExecutionGui.lastGui.updateCycleCount(wrapper.getCycles());
                    ExecutionGui.lastGui.updateRegisterFile(wrapper.readRegisterFile());
                    ExecutionGui.lastGui.updateThreadsStatus(getThreadStatus());
                    updateInstructions();
                    updateArray();
                    
                    duration += System.currentTimeMillis() - _interval;
                }
                executionGui.executionDone(_result, duration);
            }
        }catch (Exception _ex) {
            _ex.printStackTrace();
            executionGui.executionError();
            JOptionPane.showMessageDialog(executionGui,"Exception " + _ex.getClass().getName() + " thrown: " + _ex.getMessage(), "Exception thrown!", JOptionPane.ERROR_MESSAGE);
        }
    }
}

public void boot() {
    resetCore();
    (new Thread(this)).start();
}

public synchronized void release(){
    simulationActive = false;
    notify();
}


private void updateInstructions() {
    Instruction[] _instructions = new Instruction[THREAD_COUNT];
    int[] _pc = new int[THREAD_COUNT];
    wrapper.readPC(_pc);
    for(int i=0; i<THREAD_COUNT; i++){
        _instructions[i] = new Instruction(read(_pc[i]));
    }

    ExecutionGui.lastGui.updateInstruction(_instructions, _pc);
}

private ThreadStatus[] getThreadStatus(){
    ThreadStatus[] _threadStatus = new ThreadStatus[THREAD_COUNT];
    int[] _threadStatusFromSim = new int[THREAD_COUNT];
    wrapper.getThreadStatus(_threadStatusFromSim);
    for(int i=0; i<THREAD_COUNT; i++){
        _threadStatus[i] = new ThreadStatus();
        _threadStatus[i].setStatus(_threadStatusFromSim[i]);
    }

    return _threadStatus;
}

public void saveWaveform(boolean _enable) {
    if(_enable){
        wrapper.enableWaveform();
    }else{
        wrapper.disableWaveform();
    }
}

public synchronized void reset() {
    resetRequested = true;
    notify();
}

private void resetCore() {
    wrapper.resetAll(10);
    ExecutionGui.lastGui.updateThreadsStatus(getThreadStatus());
    ExecutionGui.lastGui.updateRegisterFile(wrapper.readRegisterFile());
    ExecutionGui.lastGui.updateCycleCount(wrapper.getCycles());
    updateInstructions();
    duration = 0;
}

private void updateArray(){
    updateArrayRegisters();
    updateArrayMemory();
    updateArrayFlags();
}

private void updateArrayRegisters() {
    short[] _data = new short[MachineConstants.ARRAY_LENGTH * ArrayRegisterFile.REGISTER_COUNT];
    wrapper.readArrayRegisters(_data);
    for(int i=0; i<ArrayRegisterFile.REGISTER_COUNT; i++){
        short[] _register = new short[MachineConstants.ARRAY_LENGTH];
        //System.arraycopy(_data, i * Line.LENGTH, _register, 0, Line.LENGTH);
		for(int j=0; j<MachineConstants.ARRAY_LENGTH; j++){
			  _register[j] = _data[ArrayRegisterFile.REGISTER_COUNT * j + i];
		}
        Line _line = new Line(_register);
        ExecutionGui.lastGui.updateArrayRegister(i, _line);
    }
}

private void updateArrayMemory() {
    short[] _data = new short[MachineConstants.ARRAY_LENGTH * ArrayCore.ARRAY_MEMORY_SIZE];
    wrapper.readArrayMemory(_data);
    for(int i=0; i<ArrayCore.ARRAY_MEMORY_SIZE; i++){
        short[] _memLine = new short[MachineConstants.ARRAY_LENGTH];
        //System.arraycopy(_data, i * Line.LENGTH, _memLine, 0, Line.LENGTH);
		for(int j=0; j<MachineConstants.ARRAY_LENGTH; j++){
			  _memLine[j] = _data[ArrayCore.ARRAY_MEMORY_SIZE * j + i];
		}
        Line _line = new Line(_memLine);
        ExecutionGui.lastGui.updateArrayMemory(i, _line);
    }
}

private void updateArrayFlags() {
    short[] _data = new short[MachineConstants.ARRAY_LENGTH];
    wrapper.readArrayFlags(_data);
    ExecutionGui.lastGui.updateArrayFlags(new Line(_data));
}

private boolean breakpointExists(int _pc){
    for(int i=0; i<breakpointsUsed.length; i++){
        if(breakpointsUsed[i] == _pc){
            return true;
        }
    }

    return false;
}

public boolean addBreakpoint(int _line){
    int _pc = program.getLineSeeker().getAddress(_line);
    if(_pc != -1 && !breakpointExists(_pc)){
        return addBreakpointOnPc(_pc);
    }

    System.out.println("Breakpoint failed to set on line " + _line + ". Invalid program line.");
    return false;
}

public boolean addBreakpointOnPc(int _pc) {
    for(int i=0; i<breakpointsUsed.length; i++){
        if(breakpointsUsed[i] == 0){
            breakpointsUsed[i] = _pc;
            wrapper.setBreakpoint(i, _pc, 1);
            System.out.println("Breakpoint " + i + " set on pc 0x" + Integer.toHexString(_pc));
            return true;
        }
    }

    System.out.println("No breakpoints available. Remove some before trying again.");
    return false;
}

public boolean removeBreakpoint(int _line) {
    int _pc = program.getLineSeeker().getAddress(_line);
    if(_pc != -1){
        return removeBreakpointFromPc(_pc);
    }

    System.out.println("Failed to remove breakpoint from line " + _line + ". Invalid program line.");
    return false;
}

public boolean removeBreakpointFromPc(int _pc) {
        for(int i=0; i<breakpointsUsed.length; i++){
        if(breakpointsUsed[i] == _pc){
            breakpointsUsed[i] = 0;
            wrapper.setBreakpoint(i, _pc, 0);
            System.out.println("Breakpoint " + i + " removed from pc 0x" + Integer.toHexString(_pc));
            return true;
        }
    }

    System.out.println("No active breakpoints at pc 0x" + Integer.toHexString(_pc));
    return false;
}

public int[] readBreakpoints() {
    return breakpointsUsed;
}

public LineSeeker getLineSeeker() {
    return program.getLineSeeker();
}

public void loadBinary(byte[] _buffer, int _address){
    wrapper.writeDDR(_buffer, _address, _buffer.length);
}

public byte[] saveBinary(int _address, int _length){
     byte[] _buffer = new byte[_length];
     wrapper.readDDR(_buffer, _address, _length);
     return _buffer;
}

//------------------------------------------------------------------------------
public class RS232Sim implements InputOutputPeripheral{

        public int read(int _address) {
            throw new RuntimeException("Invalid call to RS232Sim.read.");
        }

        public void write(int _address, int _data) {
            throw new RuntimeException("Invalid call to RS232Sim.write.");
        }

        public boolean isMapped(int _address) {
            throw new RuntimeException("Invalid call to RS232Sim.isMapped.");
        }

        public int externalRead() {
            byte[] _buffer = new byte[1];
            wrapper.readSerial(_buffer, 1);
            return _buffer[0];
        }

        public void externalWrite(int _data) {
            byte[] _buffer = new byte[1];
            _buffer[1] = (byte)_data;
            wrapper.writeSerial(_buffer, 1);
        }

        public int externalAvailable() {
            return wrapper.getSerialAvailable();
        }

        public boolean hasInterruptCapabilities() {
            return false;
        }

        public boolean interruptActive() {
            throw new UnsupportedOperationException("Not supported yet.");
        }

}

}
