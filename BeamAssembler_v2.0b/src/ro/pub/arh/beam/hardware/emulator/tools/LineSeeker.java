//------------------------------------------------------------------------------
//
//		Interleaved Multithreading Processor
//			- LineSeeker -
//
//------------------------------------------------------------------------------
//     $Id: LineSeeker.java,java 1.1 2009/05/14 12:35:50 rhobincu Exp $
//     $Author: rhobincu $
//     $Revision: 1.0 $
//     $Locker:  $
//     $State: Exp $
//     $Date: 2009/05/14 12:35:50 $
//------------------------------------------------------------------------------
package ro.pub.arh.beam.hardware.emulator.tools;

//------------------------------------------------------------------------------
import java.util.Vector;
import ro.pub.arh.beam.structs.Function;

//-----------------------------------------------------------------------------
public class LineSeeker extends Vector{

//-----------------------------------------------------------------------------
public LineSeeker(){
    super();
}

//-----------------------------------------------------------------------------
private LineInstruction getElementAt(int _index){
    return (LineInstruction)super.elementAt(_index);
}

//-----------------------------------------------------------------------------
public int getLineNumber(int _address){
    for(int i=0; i<size(); i++){
        if(getElementAt(i).address == _address){
            return getElementAt(i).lineNumber;
        }
    }

    return - 1;
}

//-----------------------------------------------------------------------------
public int getAddress(int _lineNumber){
    for(int i=0; i<size(); i++){
        if(getElementAt(i).lineNumber == _lineNumber){
            return getElementAt(i).address;
        }
    }

    return -1;
}

//-----------------------------------------------------------------------------
private void processMemoryLine(ro.pub.arh.beam.structs.Instruction _instruction){
    addElement(new LineInstruction(_instruction.getAddress(), _instruction.getLineNumber()));
}

public void processMemoryLines(Function _function) {
    for(int i=0; i<_function.size(); i++){
        processMemoryLine(_function.elementAt(i));
    }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class LineInstruction{
    int address;
    int lineNumber;

//-----------------------------------------------------------------------------
LineInstruction(int _address, int _lineNumber){
    address = _address;
    lineNumber = _lineNumber;
}

}
//-----------------------------------------------------------------------------
//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
//      Change History:
//      $Log: LineSeeker.java,java $
