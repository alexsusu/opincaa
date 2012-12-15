//------------------------------------------------------------------------------
//
//		Interleaved Multithreading Processor
//			- Assembler -
//
//------------------------------------------------------------------------------
//     $Id: Assembler.java,java 1.1 2009/05/14 12:35:50 rhobincu Exp $
//     $Author: rhobincu $
//     $Revision: 1.0 $
//     $Locker:  $
//     $State: Exp $
//     $Date: 2009/05/14 12:35:50 $
//------------------------------------------------------------------------------
package ro.pub.arh.beam.assembler;

//------------------------------------------------------------------------------
import java.io.*;
import java.util.Vector;
import ro.pub.arh.beam.assembler.structs.*;
import ro.pub.arh.beam.common.HexConverter;
import ro.pub.arh.beam.structs.*;
import ubiCORE.jroot.common.utils.ErrorCode;
//------------------------------------------------------------------------------
public class Assembler{

    public static final int THREADS_COUNT = 4;
    public static final int REGISTER_COUNT = 32;
    public static final String INSTRUCTION_SEPARATOR = "\\|";
    public static final String OPERAND_SEPARATOR = "\\s+|\\s*\\,\\s*";
    public static final int VERILOG_OFFSET = 0;
    public static final String LOCAL_LABEL_REGEX = "LC?\\d+";
    
    private static PrintStream dumpStream = System.out;


    private String fileName;

    private int currentAddress;
    private int currentLine;

    private Program program;
    private Function function;
    private int entityIndex;

//------------------------------------------------------------------------------
public static void main(String[] _args){
    try{
//        Instruction.setEndianess(Instruction.ENDIAN_BIG);
        new Assembler(_args[0]).run();
    }catch(ArrayIndexOutOfBoundsException _aiobe){
        //_e.printStackTrace();
        out("File is empty or no file specified.");
    }catch(Exception _e){
        _e.printStackTrace();
        out("Assembly error: " + _e.getMessage());
    }
}



//------------------------------------------------------------------------------
public Assembler(String _fileName){
    fileName = _fileName;
    currentAddress = 0;
    currentLine = 1;
}

//------------------------------------------------------------------------------
public void run() throws IOException, ErrorCode{
BufferedReader _reader   = new BufferedReader(new FileReader(fileName));
String _line;
Vector<Instruction> _instructions = null;
boolean _atLeastOneError = false;
//--

    entityIndex = 0;
    program = new Program(fileName);
    do{
        _line = _reader.readLine();
        if(_line == null){
            program.add(function);
            break;
        }
        _line = trimWhiteSpaces(_line);
        try{
            _instructions = processLine(_line);

            if(_instructions != null){
                if(function == null){
                    System.out.println("WARNING! No .globl tag found. Creating default function \"main\"!");
                    function = new Function("main", currentAddress);
                }
                function.addAll(_instructions);
                currentAddress += Instruction.INSTRUCTION_LOCATION_COUNT;
            }
        }catch(RuntimeException _re){
            out("Line " + currentLine + ": " + _re.getMessage());
            //_re.printStackTrace();
            _atLeastOneError = true;
        }
        
        
        currentLine++;
    }while(true);

    if(_atLeastOneError){
        out("Finished with errors.");
        throw new RuntimeException("Assembly failed.");
    }
    
    out("No syntax errors.");
    //placeIRQHandler("irq_handler", MachineConstants.IRQ_HANDLER_ADDR);
    replaceJumpLabels();
    buildLineSeeker();

    out("Assembly succesfull!");

    program.sort();

}

public Program getProgram(){
    return program;
}

//------------------------------------------------------------------------------
private Vector<Instruction> processLine(String _line){
String[] _lineElements;
Instruction _instruction;
String _label;
Vector _words = new Vector<Instruction>();
//--
    //comment
    if(_line.length() == 0){
        return null;
    }

    //label
    if(_line.endsWith(":")){
        _label = _line.substring(0, _line.length() - 1);
        if(!_label.matches("[\\w\\.]+") || Character.isDigit(_label.charAt(0))){
            throw new RuntimeException("Invalid jump label format");
        }
        if(_label.matches(LOCAL_LABEL_REGEX)){
            function.getLabelList().addElement(new JumpLabel(currentAddress, "File" + "_" + entityIndex + "_" + _label));
        }else{
            function.getLabelList().addElement(new JumpLabel(currentAddress, _label));
        }
        
        return null;
    }

    //new address
    if(_line.startsWith("(") && _line.endsWith(")")){
        currentAddress = getValueOfString(_line.substring(1, _line.length() - 1));
        return null;
    }

    //direct value
    if(_line.startsWith(".word") || _line.startsWith(".long")){
        try{
            _line = _line.split(" ")[1];
            Object _data = decodeWord(_line);
            if(_data instanceof String){
                _words.add(new Data((String)_data, currentAddress, currentLine));
            }else{
                _words.add(new Data(((Long)_data).intValue(), currentAddress, currentLine));
            }
            
            return _words;
        }catch(Exception e){
            throw new RuntimeException("Invalid data format: "+ _line);
        }
    }

    if(_line.startsWith(".short")){
        try{
            _line = _line.split(" ")[1];
            Object _data = decodeWord(_line);
            int _value = ((Long)_data).shortValue();
            _words.add(new Data(_value << ((currentAddress & 2) << 3), currentAddress, currentLine, 2));
            currentAddress -= 2;
            return _words;
        }catch(Exception e){
            throw new RuntimeException("Invalid data format: "+ _line);
        }
    }

    if(_line.startsWith(".byte")){
        try{
            _line = _line.split(" ")[1];
            Object _data = decodeWord(_line);
            int _value = ((Long)_data).byteValue();
            _words.add(new Data(_value << ((currentAddress & 1) << 3), currentAddress, currentLine, 1));
            currentAddress -= 3;
            return _words;
        }catch(Exception e){
            throw new RuntimeException("Invalid data format: "+ _line);
        }
    }

    //direct value
/*    if(_line.startsWith(".long")){
        try{
            _line = _line.split(" ")[1];
            Object _data = decodeWord(_line);
            if(_data instanceof Long){
                _words.add(new Data(((Long)_data).intValue(), currentAddress, currentLine));
                _words.add(new Data((int)(((Long)_data).longValue() >>> 32), currentAddress + 4, currentLine));
                currentAddress += 4;
            }else{
                throw new RuntimeException();
            }

            return _words;
        }catch(Exception e){
            throw new RuntimeException("Invalid data format: "+ _line);
        }
    }
*/
     if(_line.startsWith(".newfile")){
        entityIndex++;
        return null;
     }

    if(_line.startsWith(".globl")){
        if(function != null){
            program.add(function);
        }
        function = new Function(_line.split("\\ +")[1],currentAddress);
        return null;
    }

    //global (ignored), text (ignored), data
    if(_line.startsWith(".text") ||
        _line.startsWith(".data")){
        return null;
    }

    //asciiz
    if(_line.startsWith(".ascii")){
        try{
            _line = _line.split("\"")[1];
            _line = _line.replaceAll("\\\\t", "\t");
            _line = _line.replaceAll("\\\\012", "\n");
            _line = _line.replaceAll("\\\\r", "\r");
            _line = _line.replaceAll("\\\\000", "\000");

            int _word = 0;
            int i;
            int _wordAddress = currentAddress;
            for(i=0; i<_line.length(); i++){
                _word |= (((int)_line.charAt(i) & 0xff) << ((currentAddress % 4) * 8));
                currentAddress++;
                if(currentAddress % 4 == 0){
                    _words.add(new Data(_word, _wordAddress, currentLine, currentAddress - _wordAddress));
                    _wordAddress = currentAddress;
                    _word = 0;
                }

            }
            if(currentAddress % 4 != 0){
                _words.add(new Data(_word, _wordAddress, currentLine, currentAddress - _wordAddress));
            }
            currentAddress -= Instruction.INSTRUCTION_LOCATION_COUNT;
            
            return _words;
        }catch(Exception e){
            //e.printStackTrace();
            throw new RuntimeException("Invalid data format.");
        }
    }

    if(_line.startsWith(".skip")){
         try{
            _line = _line.split(" ")[1];
            int _skip = Long.decode(_line).intValue();
            currentAddress += _skip;
            return null;
        }catch(Exception e){
            //e.printStackTrace();
            throw new RuntimeException("Invalid data format.");
        }
    }

    //align
    if(_line.startsWith(".align")){
        try{
            _line = _line.split(" ")[1];
            int _align = Long.decode(_line).intValue();
            currentAddress += (currentAddress % _align != 0) ? (_align - (currentAddress % _align)) : 0;
            return null;
        }catch(Exception e){
            //e.printStackTrace();
            throw new RuntimeException("Invalid data format.");
        }
    }

    //instruction;
    String[] _instructionPair = _line.split(INSTRUCTION_SEPARATOR);
    _lineElements = _instructionPair[0].trim().split(OPERAND_SEPARATOR);

    _instruction = new Instruction(_lineElements, currentAddress, currentLine, entityIndex);
    if(_instructionPair.length != 1){
        throw new RuntimeException("Only 2 instructions per pair accepted.");
    }

    
    _words.add(_instruction);
    return _words;
}

//------------------------------------------------------------------------------
private String trimWhiteSpaces(String _line){
    _line = _line.substring(0, _line.indexOf("//") == -1 ? _line.length() : _line.indexOf("//"));
    _line = _line.substring(0, _line.indexOf(";") == -1 ? _line.length() : _line.indexOf(";"));
    _line = _line.trim();
    _line = _line.replaceAll("\\s+", " ");
    return _line;
}

//------------------------------------------------------------------------------
private int getValueOfString(String _number){
int _value;
//--
    try{
        if(_number.startsWith("0x")){
            _value = Integer.parseInt(_number.substring(2), 16);
        }else{
            _value = Integer.parseInt(_number);
        }
    }catch(NumberFormatException _nfe){
        throw new RuntimeException("Invalid new code address");
    }

    if(_value % 4 != 0){
        throw new RuntimeException("Invalid new code address " + currentLine + ". Address must divide by 4.");
    }
    return _value;
}

//------------------------------------------------------------------------------
private void replaceJumpLabels(){
int _address;
Instruction _instruction;
//--
    for(int i=0; i<program.size(); i++){
        Function _function = program.elementAt(i);
        for(int j=0; j<program.elementAt(i).size(); j++){
            _instruction = _function.elementAt(j);
            if(_instruction.getLabel() != null){
                String _label = extractLabel(_instruction.getLabel());
                if(_instruction.isCall()){
                    _address = program.findAddressOfFunction(_label);
                }else if(_instruction.isAbsoluteJump()){
                    _address = program.findAddressOfLabel(_label);
                }else{
                    _address = program.findAddressOfLabel(_label) - _instruction.getAddress();
                }

                _instruction.setLabelAddress(offsetAddress(_address, _instruction.getLabel()));
            }
        }
    }
}

//------------------------------------------------------------------------------
//public void writeHexFile(String _filename) throws ErrorCode{
//int _startIndex = 0;
//byte[] _buffer;
//HexFileReadWrite _hexWriter;
//int _offset = 0;//program.elementAt(0).getAddress();
////--
//    _hexWriter = new HexFileReadWrite(_filename, true);
//    for(int i=1; i<program.size(); i++){
//        if((program.elementAt(i).getAddress() != program.elementAt(i - 1).getAddress() + Instruction.INSTRUCTION_LOCATION_COUNT) ||
//                i-_startIndex >= 4){
//            if(program.elementAt(i).getAddress() < program.elementAt(i - 1).getAddress() + Instruction.INSTRUCTION_LOCATION_COUNT){
//                throw new RuntimeException("Address spaces overlap or they are not defined in order.");
//            }
//            _buffer = new byte[Instruction.INSTRUCTION_LOCATION_COUNT * (i - _startIndex)];
//            for(int j=_startIndex; j<i; j++){
//                System.arraycopy(program.elementAt(j).getBytesReversed(), 0, _buffer, Instruction.INSTRUCTION_LOCATION_COUNT * (j - _startIndex), Instruction.INSTRUCTION_LOCATION_COUNT);
//            }
//            _hexWriter.addLine(program.elementAt(_startIndex).getAddress()-_offset, _buffer);
//            _startIndex = i;
//        }
//    }
//
//    _buffer = new byte[Instruction.INSTRUCTION_LOCATION_COUNT * (program.size() - _startIndex)];
//    for(int j=_startIndex; j<program.size(); j++){
//        System.arraycopy(program.elementAt(j).getBytesReversed(), 0, _buffer, Instruction.INSTRUCTION_LOCATION_COUNT * (j - _startIndex), Instruction.INSTRUCTION_LOCATION_COUNT);
//    }
//    _hexWriter.addLine(program.elementAt(_startIndex).getAddress()-_offset, _buffer);
//    _hexWriter.addEndLine();
//    _hexWriter.saveData();
//}

//------------------------------------------------------------------------------
public void writeMemFile(String _filename) throws IOException, ErrorCode{
//String _filename = outputFile.substring(0, outputFile.lastIndexOf('.') == -1 ? outputFile.length() : outputFile.lastIndexOf('.')) + ".mem";
FileOutputStream _file = new FileOutputStream(_filename);
Instruction _instruction;
int _offset = 0;//program.elementAt(0).getAddress();
//--
    System.out.print("Writing file [" + _filename +"]");
    for(int j=0; j<program.size(); j++){
        for(int i=0; i<program.elementAt(j).size(); i++){
            _instruction = program.elementAt(j).elementAt(i);
            _file.write(("@" + Integer.toHexString(_instruction.getAddress()-_offset) + " " +
                    HexConverter.intToHex(_instruction.getInstruction() & 0xff, 2) + " " +
                    HexConverter.intToHex((_instruction.getInstruction() >> 8) & 0xff, 2) + " " +
                    HexConverter.intToHex((_instruction.getInstruction() >> 16) & 0xff, 2) + " " +
                    HexConverter.intToHex((_instruction.getInstruction() >> 24) & 0xff, 2) + " " +
                    "//line number: " + _instruction.getLineNumber() + "\r\n").getBytes());
        }
    }
    _file.close();
    System.out.println("...done.");
}

public void writeBinFile(String _filename) throws IOException{
        byte[] _map = getMemoryMap(0);
        System.out.print("Writing file [" + _filename +"]");
        FileOutputStream _out = new FileOutputStream(_filename);
        _out.write(_map);
        _out.close();
        System.out.println("...done. Binary fille written! Buffer size is " + _map.length);
}

//------------------------------------------------------------------------------
private static void out(String _message){
    dumpStream.println(_message);
}

//------------------------------------------------------------------------------
public static void setOutput(PrintStream _stream){
    dumpStream = _stream;
}

public byte[] getMemoryMap(int _offset) {
    return program.getByteMap(_offset);
}

private Object decodeWord(String _line) {
    try{
        return Long.decode(_line);
    }catch(NumberFormatException _nfe){
        return _line;
    }
}

//private void placeIRQHandler(String _handlerName, int _handlerAddress) {
//    program.sort();
//    Function _irqHandler = program.findFunction(_handlerName);
//    _irqHandler.replaceOpcode(Opcodes.ajmp, Opcodes.irqret);
//    if(_irqHandler.getAddress() == _handlerAddress){
//        return;
//    }
//
//    int _offset = _irqHandler.moveTo(_handlerAddress);
//    int _newIndexOfHandler = program.findNewIndex(_offset < 0, _handlerAddress);
//
//    _offset = _irqHandler.getSize() + _irqHandler.getAddress() - program.elementAt(_newIndexOfHandler).getAddress();
//    for(int i=_newIndexOfHandler; i<program.size(); i++){
//        if(!program.elementAt(i).getFunctionName().equals(_handlerName)){
//            _offset = program.elementAt(i).moveTo(program.elementAt(i).getAddress() + _offset);
//            //System.out.print("Function " + program.elementAt(i).getFunctionName() + " was offseted with " + _offset + " bytes.\n");
//        }
//    }
//}

private void buildLineSeeker() {
    for(int i=0; i<program.size(); i++){
        program.getLineSeeker().processMemoryLines(program.elementAt(i));
    }
}

private String extractLabel(String _label) {
    _label = _label.split("[\\+\\-]")[0];
    return _label;
}

private int offsetAddress(int _address, String _label) {
    int _index = _label.indexOf("+") + 1;
    if(_index <= 0){
        _index = _label.indexOf("-");
    }
    if(_index == -1){
        return _address;
    }

    return _address + Integer.parseInt(_label.substring(_index));
}


//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
//      Change History:
//      $Log: Assembler.java,java $
