//------------------------------------------------------------------------------
//
//		Interleaved Multithreading Processor
//			- ConsolePrintStream -
//
//------------------------------------------------------------------------------
//     $Id: ConsolePrintStream.java,java 1.1 2009/05/14 12:35:50 rhobincu Exp $
//     $Author: rhobincu $
//     $Revision: 1.0 $
//     $Locker:  $
//     $State: Exp $
//     $Date: 2009/05/14 12:35:50 $
//------------------------------------------------------------------------------
package ro.pub.arh.beam.utils.gui;

//------------------------------------------------------------------------------
import java.io.IOException;
import java.io.OutputStream;
import javax.swing.JTextPane;

//------------------------------------------------------------------------------
public class ConsolePrintStream extends OutputStream{

    private static final int BUFFER_SIZE = 102400;

    private JTextPane console;
    private char[] buffer;
    private int marker;

//------------------------------------------------------------------------------
public ConsolePrintStream(JTextPane _console){
    console = _console;
    buffer = new char[BUFFER_SIZE];
    marker = 0;
}

//------------------------------------------------------------------------------
@Override
public synchronized void write(int _byte) throws IOException{
    char _char = (char)_byte;
    buffer[marker++] = _char;
    if(_char == '\n' || marker == BUFFER_SIZE){
        flush();
    }
}

    @Override
public synchronized void flush(){
    console.setText(console.getText() + new String(buffer, 0, marker).replace('\r', ' '));
    marker = 0;
}

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
//      Change History:
//      $Log: ConsolePrintStream.java,java $
