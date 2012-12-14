/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.fpga.network;

import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.net.Socket;
import ro.pub.arh.beam.hardware.fpga.FPGAWrapper;

/**
 *
 * @author Echo
 */
public class ClientPeer extends Thread {

    private FPGAConnectionServer server;
    private Socket socket;
    private ObjectOutputStream outputStream;
    private ServerLog log;

public ClientPeer(FPGAConnectionServer _server, Socket _socket, ServerLog _log) {
    server = _server;
    socket = _socket;
    log = _log;
}

@Override
public void run() {
    try{
        ObjectInputStream _inputStream = new ObjectInputStream(socket.getInputStream());
        outputStream = new ObjectOutputStream(socket.getOutputStream());
        Command _command = new Command(Command.RESPONSE_OK);
        outputStream.writeObject(_command);
        log.messageSent(_command, socket);
        while(true){
            if(!parseCommand((Command)_inputStream.readObject())){
                break;
            }
        }
    }catch(Exception _e){
        //server.clientDisconnected();
        log.connectionBroken(socket);
        _e.printStackTrace();
    }

    server.clientDisconnected();
    log.clientDisconnected(socket);
}

private boolean parseCommand(Command _command) throws IOException {
    int _code = 0;
    log.messageReceived(_command, socket);
    switch(_command.getCode()){
        case Command.COMMAND_READ:
            byte[] _buffer = new byte[_command.getLength()];
            _code = (new FPGAWrapper()).readDDR(_buffer, _command.getAddress(), _command.getLength());
            if(_code == 0){
                _command = new Command(Command.RESPONSE_OK, _command.getAddress(), _buffer);
            }else{
                _command = new Command(Command.RESPONSE_ERROR, "FPGAWrapper returned non-zero error code on read operation.");
            }
            outputStream.writeObject(_command);
            log.messageSent(_command, socket);
            break;
        case Command.COMMAND_RESET:
            _code = (new FPGAWrapper()).reset();
            if(_code == 0){
                _command = new Command(Command.RESPONSE_OK);
            }else{
                _command = new Command(Command.RESPONSE_ERROR, "FPGAWrapper returned non-zero error code on reset operation.");
            }
            outputStream.writeObject(_command);
            log.messageSent(_command, socket);
            break;
        case Command.COMMAND_WRITE:
            _code = (new FPGAWrapper()).writeDDR(_command.getBuffer(), _command.getAddress(), _command.getBuffer().length);
            if(_code == 0){
                _command = new Command(Command.RESPONSE_OK);
            }else{
                _command = new Command(Command.RESPONSE_ERROR, "FPGAWrapper returned non-zero error code on write operation.");
            }
            outputStream.writeObject(_command);
            log.messageSent(_command, socket);
            break;
        case Command.COMMAND_DISCONNECT:
            _code = -1;
            break;
        default:
            _command = new Command(Command.RESPONSE_ERROR, "Invalid command code: " + _command.getCode());
            outputStream.writeObject(_command);
            log.messageSent(_command, socket);
            _code = -1;
            break;
    }
    
    return _code == 0;
}

}
