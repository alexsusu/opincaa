/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.fpga.network;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.ObjectOutputStream;
import java.net.ServerSocket;
import java.net.Socket;

/**
 *
 * @author Echo
 */
public class FPGAConnectionServer {

    public static final int DEFAULT_PORT = 9001;

    private int port;
    private boolean serverBusy;
    private ServerLog log;
    private static final File DEFAULT_LOG_NAME = new File("fpga_server.log");

public FPGAConnectionServer() throws IOException{
    this(DEFAULT_PORT);
}

public FPGAConnectionServer(int _port) throws IOException{
    port = _port;
    log = new ServerLog(DEFAULT_LOG_NAME);
}


public static void main(String[] _args){
    try{
        int _port = DEFAULT_PORT;
        if(_args.length > 0){
            _port = Integer.parseInt(_args[0]);
        }

        (new FPGAConnectionServer(_port)).start();
    }catch(NumberFormatException _nfe){
        System.out.println("Invalid port number as first argument.");
    }catch(Exception _e){
        _e.printStackTrace();
    }
}

public void start() throws IOException{
    ServerSocket _server = new ServerSocket(port);
    serverBusy = false;
    while(true){
        Socket _socket = _server.accept();
        log.clientConnect(_socket);
        if(!serverBusy){
            serverBusy = true;
            (new ClientPeer(this, _socket, log)).start();
        }else{
            denyAccess(_socket);
        }
    }
}

private void denyAccess(Socket _socket){
    try{
        ObjectOutputStream _stream = new ObjectOutputStream(_socket.getOutputStream());
        Command _command = new Command(Command.RESPONSE_ERROR, "Server already in use. Try again later.");
        _stream.writeObject(_command);
        log.messageSent(_command, _socket);
        _socket.close();
    }catch(Exception _e){
        _e.printStackTrace();
    }
}

public void clientDisconnected() {
    serverBusy = false;
}

}
