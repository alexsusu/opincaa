/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware;

import ro.pub.arh.beam.hardware.emulator.periferals.Peripheral;
import java.io.File;
import java.io.IOException;
import ro.pub.arh.beam.hardware.emulator.core.Statistics;
import ro.pub.arh.beam.execution.gui.ExecutionGui;
import ro.pub.arh.beam.hardware.emulator.tools.LineSeeker;
import ro.pub.arh.beam.structs.Program;

/**
 *
 * @author rhobincu
 */
public interface ExecutionEnvironment extends Runnable{

    public void assignGui(ExecutionGui _emulatorGui);
    public void init();
    public void loadProgram(Program _program);
    public void boot();
    public Peripheral getPeripheral(String _id);
    public void writeByte(int _address, int _data);
    public int read(int _address);
    public void run(int cycleCount);
    public void stopExecution();
    public int getPcForThread(int _threadId);
    public void saveWaveform(boolean _enable);
    public void reset() throws IOException;
    public void release();
    public boolean addBreakpoint(int _line);
    public boolean removeBreakpoint(int _line);
    public int[] readBreakpoints();

    public Statistics[] getBeamStatistics();
    public LineSeeker getLineSeeker();

    public void loadBinary(byte[] _buffer, int _address) throws IOException;
    public byte[] saveBinary(int _address, int _length) throws IOException;

}
