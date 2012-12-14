/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package ro.pub.arh.beam.hardware.fpga.network;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author rhobincu
 */
public class ServerLog {
    
    private FileOutputStream stream;
    
public ServerLog(File _file) throws IOException{
    stream = new FileOutputStream(_file);
    write("Server opened");
}

private synchronized void write(String _string) throws IOException{
    DateFormat dateFormat = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss:SSS");
    Date date = new Date();
    _string = "[" + dateFormat.format(date) + "]" + _string + "\n";
    stream.write(_string.getBytes());
    stream.flush();
}

public void clientConnect(Socket _socket) throws IOException {
    write("Connection attempt from " + _socket.getInetAddress().getCanonicalHostName() + " [" + _socket.getInetAddress().toString() + "]");
}

public void clientDenied(Socket _socket) throws IOException {
    write("Client " + _socket.getInetAddress().getCanonicalHostName() + " [" + _socket.getInetAddress().toString() + "] rejected. Server busy.");
}

public void messageReceived(Command _command, Socket _socket) throws IOException {
    write("Client " + _socket.getInetAddress().getCanonicalHostName() + " [" + _socket.getInetAddress().toString() + "] sent command " + _command.toString());
}

public void messageSent(Command _command, Socket _socket) throws IOException {
    write("Server replied to " + _socket.getInetAddress().getCanonicalHostName() + " [" + _socket.getInetAddress().toString() + "]: " + _command.toString());
}

public void connectionBroken(Socket _socket) {
    try {
        write("Connection error to" + _socket.getInetAddress().getCanonicalHostName() + " [" + _socket.getInetAddress().toString() + "]");
    } catch (IOException ex) {
        ex.printStackTrace();
    }
}

public void clientDisconnected(Socket _socket) {
    try {
        write("Client " + _socket.getInetAddress().getCanonicalHostName() + " [" + _socket.getInetAddress().toString() + "] disconnected. Server now available.");
    } catch (IOException ex) {
        ex.printStackTrace();
    }
}


}
