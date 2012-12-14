/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.simulator;

/**
 *
 * @author rhobincu
 */
public class ConnexBEAMSimulator {
static {
    System.loadLibrary("ConnexBEAMSimulator");
}

//constructor calls initializeSimulator
public ConnexBEAMSimulator(){
    initializeSimulator();
}

//create the simulator object in native code
private native void initializeSimulator();

//TODO: merge write_ and read_ methods
//into unified memory map access

//writes flash with the contents of the data array
public native void writeFlash(byte[] data, int addr, int len);

//returns an array of the contents of flash
public native int readFlash(byte[] data, int addr, int len);

//writes ddr with the contents of the data array
public native void writeDDR(byte[] data, int addr, int len);

//returns an array of the contents of ddr
public native int readDDR(byte[] data, int addr, int len);

//run until timeout cycles have passed or breakpoint hit
public native int run(int timeout);

//keep simulator in reset for timeout cycles
public native void resetAll(int timeout);

//set the value of a given breakpoint
public native int setBreakpoint(int index, int value, int status);

//returns an array of short with the contents of the array register files
public native void readArrayRegisters(short[] data);

//returns an array of short with the contents of the array internal memory
public native void readArrayMemory(short[] data);

//returns an array of short with the contents of the array flags
public native void readArrayFlags(short[] data);

//returns an array of ints with the contents of the register files
public native void readRegisters(int[] data);

//returns an array of ints with the contents of the theads' PCs
public native void readPC(int[] data);

//returns number of cycles run so far (the simulation time)
public native long getCycles();

//returns how many bytes are available for reading from rs232 interface
public native int getSerialAvailable();

//returns an array of bytes from the RS232 interface
public native int readSerial(byte[] data, int len);

//transmits an array of bytes via the RS232 interface
public native int writeSerial(byte[] data, int len);

public native void getThreadStatus(int[] _status);

public int[][] readRegisterFile() {
    int[] _registersFromSim = new int[Simulator.THREAD_COUNT * Simulator.REGISTER_COUNT];
    readRegisters(_registersFromSim);
    int[][] _regFile = new int[Simulator.THREAD_COUNT][Simulator.REGISTER_COUNT];
    for(int i=0; i<Simulator.THREAD_COUNT; i++){
        for(int j=0; j<Simulator.REGISTER_COUNT; j++){
            _regFile[i][j] = _registersFromSim[i * Simulator.REGISTER_COUNT + j];
        }
    }

    return _regFile;
}

//enables waveform generation
public native void enableWaveform();

//disables waveform generation
public native void disableWaveform();

public native void stop();

}
