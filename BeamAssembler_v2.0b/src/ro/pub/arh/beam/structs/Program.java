//------------------------------------------------------------------------------
//
//		Interleaved Multithreading Processor
//			- Program -
//
//------------------------------------------------------------------------------
//     $Id: Program.java,java 1.1 2009/05/14 12:35:50 rhobincu Exp $
//     $Author: rhobincu $
//     $Revision: 1.0 $
//     $Locker:  $
//     $State: Exp $
//     $Date: 2009/05/14 12:35:50 $
//------------------------------------------------------------------------------
package ro.pub.arh.beam.structs;

//------------------------------------------------------------------------------
import java.util.Collections;
import java.util.Vector;
import ro.pub.arh.beam.hardware.emulator.tools.LineSeeker;
import ubiCORE.jroot.common.utils.Convertor;

//------------------------------------------------------------------------------
public class Program extends Vector<Function>{

    private LineSeeker lineSeeker;
    private String sourceFileName;

//------------------------------------------------------------------------------
public Program(String _sourceFileName){
    super();
    sourceFileName = _sourceFileName;
    lineSeeker = new LineSeeker();
}

public String getSourceFileName(){
    return sourceFileName;
}

//------------------------------------------------------------------------------
public void sort(){
    Collections.sort(this, new FunctionComparator());
}

//------------------------------------------------------------------------------
public int[] getMemoryMap(int _offset){
    sort();
    int[] _memory = new int[1024];
    int i = 0;
    try{
        for(int j=0; j<size(); j++){
            for(i=0; i<elementAt(j).size(); i++){
                _memory[(elementAt(i).elementAt(j).getAddress() - _offset) >>> 2] = elementAt(i).elementAt(j).getInstruction();
            }
        }
    }catch(ArrayIndexOutOfBoundsException _aiobe){
        throw new RuntimeException("Invalid index in memory for address " + elementAt(i).getAddress() + " with offset " + _offset);
    }

    return _memory;
}

public byte[] getByteMap(int _offset) {
    sort();
    byte[] _memory = new byte[(lastElement().lastElement()).getAddress() + 4 - _offset];
    int i = 0;
    int j = 0;
    try{
        for(j=0; j<size(); j++){
            for(i=0; i<elementAt(j).size(); i++){
                System.arraycopy(Convertor.IntToBytes_v2(elementAt(j).elementAt(i).getInstruction()), 0, _memory, elementAt(j).elementAt(i).getAddress() - _offset, 4);
            }
        }
    }catch(ArrayIndexOutOfBoundsException _aiobe){
        throw new RuntimeException("(getByteMap) Invalid index in memory for address " + elementAt(j).elementAt(i).getAddress() + " with offset " + _offset);
    }

    return _memory;
}

//------------------------------------------------------------------------------
public LineSeeker getLineSeeker(){
    return lineSeeker;
}

public int findAddressOfFunction(String _name){
    return findFunction(_name).getAddress();
}

public int findAddressOfLabel(String _label) {
    for(int i=0; i<size(); i++){
        try{
           return elementAt(i).getLabelList().findAddressOfLabel(_label);
        }catch(Exception _e){

        }
    }

    throw new RuntimeException("Can't find label " + _label + " anywhere.");
}

public Function findFunction(String _functionName) {
    for(int i=0; i<size(); i++){
        if(elementAt(i).getFunctionName().equals(_functionName)){
            return elementAt(i);
        }
    }

    throw new RuntimeException("No function " + _functionName + " found. Make sure you're linking against required libraries.");
}

public int findNewIndex(boolean _moveUp, int _handlerAddress) {
    for(int i=0; i<size(); i++){
        if(_handlerAddress >= elementAt(i).getAddress() && _handlerAddress < elementAt(i).getAddress() + elementAt(i).getSize()){
            return i;
        }
    }

    return 0;
}

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
//      Change History:
//      $Log: Program.java,java $
