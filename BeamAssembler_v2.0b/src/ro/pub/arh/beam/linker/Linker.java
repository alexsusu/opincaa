/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.linker;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Vector;
import ro.pub.arh.beam.assembler.Assembler;
import ro.pub.arh.beam.common.ArgumentParser;
import ro.pub.arh.beam.hardware.MachineConstants;
import ro.pub.arh.beam.structs.Program;
import ubiCORE.jroot.common.utils.ErrorCode;

/**
 *
 * @author Echo
 */
public class Linker {

    private Assembler assembler;
    private String filename;
    private int stackSize;
    
    public static final String MACHINE_SPECIFIC_FUNCTIONS_FILE = "../beam/libs/machine_specific.s";

public Linker(int _stackSize){
    stackSize = _stackSize;
}

public Linker(){
    this(MachineConstants.STACK_SIZE);
}

public static void main(String[] _args){
    try{
        ArgumentParser _argumentParser = new ArgumentParser(_args);
        if(_args.length == 0 || "true".equals(_argumentParser.getSwitch("help"))){
            displayHelpMessage();
            return;
        }
        Vector<String> _filenames = _argumentParser.getFreeArgs();
        Vector<File> _files = new Vector();
        for(int i=0; i<_filenames.size(); i++){
            _files.add(new File(_filenames.elementAt(i)));
        }
        Linker _linker = null;
        if(_argumentParser.getSwitch("stack") != null){
            _linker = new Linker(Long.decode(_argumentParser.getSwitch("stack")).intValue());
        }else{
            _linker = new Linker();
        }

        _linker.link(_files, new File(_argumentParser.getSwitch("o")));
        _linker.generateFiles();
    }catch(Exception _e){
        displayHelpMessage();
        _e.printStackTrace();
    }
}

private static void displayHelpMessage() {
    System.out.print("Usage:\n");
    System.out.print("\t./link.sh file0.s file1.s ... file3.s -o outputFile [-stack stack_size]\n");
}

public Program link(Vector<File> _files, File _outputFile) throws IOException, ErrorCode{

    FileOutputStream _stream = new FileOutputStream(_outputFile.getCanonicalPath() + ".s");

    //always link with the machine specific assembly functions
    addToStream(_stream, new File(MACHINE_SPECIFIC_FUNCTIONS_FILE));
    
    //put the rest of the code in the text area
//    _stream.write(("(" + stackSize + ")\n").getBytes());

    //add requested files to link
    for(int i=0; i<_files.size(); i++){
        addToStream(_stream, _files.elementAt(i));
    }

    _stream.close();

    assembler = new Assembler(_outputFile.getCanonicalPath() + ".s");

    assembler.run();
    filename = _outputFile.getCanonicalPath();
    return assembler.getProgram();
}

public void generateFiles() throws IOException, ErrorCode{
    if(assembler == null){
        return;
    }
    assembler.writeBinFile(filename + ".bin");
   //assembler.writeHexFile(filename + ".hex");
    assembler.writeMemFile(filename + ".mem");
}

private void addToStream(FileOutputStream _stream, File _input) throws IOException{
    FileInputStream _inputStream = new FileInputStream(_input);
    byte[] _buffer = new byte[(int)_input.length()];
    _stream.write(".newfile\n".getBytes());
    _stream.write(("; " + _input.getCanonicalPath() + "\n").getBytes());
    _inputStream.read(_buffer);
    _inputStream.close();
    _stream.write(_buffer);
    _stream.write("\n".getBytes());
}

}
