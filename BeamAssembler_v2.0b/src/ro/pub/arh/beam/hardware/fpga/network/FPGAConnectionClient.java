/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.fpga.network;

import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.net.Socket;
import ro.pub.arh.beam.execution.gui.ExecutionGui;
import ro.pub.arh.beam.hardware.ExecutionEnvironment;
import ro.pub.arh.beam.hardware.emulator.periferals.Peripheral;
import ro.pub.arh.beam.hardware.emulator.core.Statistics;
import ro.pub.arh.beam.hardware.emulator.tools.LineSeeker;
import ro.pub.arh.beam.structs.Program;

/**
 *
 * @author Echo
 */
public class FPGAConnectionClient implements ExecutionEnvironment{

    private Socket socket;
    private ObjectOutputStream outputStream;
    private ObjectInputStream inputStream;


public FPGAConnectionClient(String _host, int _port) throws IOException, ClassNotFoundException{
    socket = new Socket(_host, _port);
    outputStream = new ObjectOutputStream(socket.getOutputStream());
    inputStream = new ObjectInputStream(socket.getInputStream());
    Command _command = (Command)inputStream.readObject();
    if(_command.getCode() == Command.RESPONSE_ERROR){
        throw new IOException(_command.getMessage());
    }
}

public void assignGui(ExecutionGui _emulatorGui) {
    throw new UnsupportedOperationException("Not supported yet.");
}

public void loadProgram(Program _program) {
    throw new UnsupportedOperationException("Not supported yet.");
}

public void boot() {
    throw new UnsupportedOperationException("Not supported yet.");
}

public Peripheral getPeripheral(String _id) {
    throw new UnsupportedOperationException("Not supported yet.");
}

public void writeByte(int _address, int _data) {
    throw new UnsupportedOperationException("Not supported yet.");
}

public int read(int _address) {
    throw new UnsupportedOperationException("Not supported yet.");
}

public void run(int cycleCount) {
    throw new UnsupportedOperationException("Not supported yet.");
}

public void stopExecution() {
    throw new UnsupportedOperationException("Not supported yet.");
}

public int getPcForThread(int _threadId) {
    throw new UnsupportedOperationException("Not supported yet.");
}

public void saveWaveform(boolean _enable) {
    throw new UnsupportedOperationException("Not supported yet.");
}

public void run() {
    throw new UnsupportedOperationException("Not supported yet.");
}

public void release() {
    throw new UnsupportedOperationException("Not supported yet.");
}

public boolean addBreakpoint(int _line) {
    throw new UnsupportedOperationException("Not supported yet.");
}

public boolean removeBreakpoint(int _line) {
    throw new UnsupportedOperationException("Not supported yet.");
}

public int[] readBreakpoints() {
    throw new UnsupportedOperationException("Not supported yet.");
}

public Statistics[] getBeamStatistics() {
    throw new UnsupportedOperationException("Not supported yet.");
}

public LineSeeker getLineSeeker() {
    throw new UnsupportedOperationException("Not supported yet.");
}

public void loadBinary(byte[] _buffer, int _address) throws IOException {
    Command _command = new Command(Command.COMMAND_WRITE, _buffer, _address);
    outputStream.writeObject(_command);
    try{
        _command = (Command)inputStream.readObject();
        if(_command.getCode() != Command.RESPONSE_OK){
            throw new IOException(_command.getMessage());
        }
    }catch(ClassNotFoundException _cnfe){
        throw new IOException("Invalid Command object received. Update, recompile and restart the server.");
    }
}

public byte[] saveBinary(int _address, int _length) throws IOException {
    Command _command = new Command(Command.COMMAND_READ, _address, _length);
    outputStream.writeObject(_command);
    try{
        _command = (Command)inputStream.readObject();
        if(_command.getCode() != Command.RESPONSE_OK){
            throw new IOException(_command.getMessage());
        }else{
            return _command.getBuffer();
        }
    }catch(ClassNotFoundException _cnfe){
        throw new IOException("Invalid Command object received. Update, recompile and restart the server.");
    }
}

public void reset() throws IOException{
    Command _command = new Command(Command.COMMAND_RESET);
    outputStream.writeObject(_command);
    try{
        _command = (Command)inputStream.readObject();
        if(_command.getCode() != Command.RESPONSE_OK){
            throw new IOException(_command.getMessage());
        }
    }catch(ClassNotFoundException _cnfe){
        throw new IOException("Invalid Command object received. Update, recompile and restart the server.");
    }
}

    public void disconnect() throws IOException{
        Command _command = new Command(Command.COMMAND_DISCONNECT);
        outputStream.writeObject(_command);
        socket.close();
    }

    public void init() {
        //do nothing here
    }

}
