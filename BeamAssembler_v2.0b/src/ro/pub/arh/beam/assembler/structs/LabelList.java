//------------------------------------------------------------------------------
//
//		Interleaved Multithreading Processor
//			- LabelList -
//
//------------------------------------------------------------------------------
//     $Id: LabelList.java,java 1.1 2009/05/14 12:35:50 rhobincu Exp $
//     $Author: rhobincu $
//     $Revision: 1.0 $
//     $Locker:  $
//     $State: Exp $
//     $Date: 2009/05/14 12:35:50 $
//------------------------------------------------------------------------------
package ro.pub.arh.beam.assembler.structs;

//------------------------------------------------------------------------------
import java.util.Vector;

//------------------------------------------------------------------------------
public class LabelList extends Vector{

//------------------------------------------------------------------------------
public LabelList(){
    super();
}

//------------------------------------------------------------------------------
@Override
public JumpLabel elementAt(int _index){
    return (JumpLabel)super.elementAt(_index);
}

//------------------------------------------------------------------------------
public int findAddressOfLabel(String _label){
    for(int i=0; i<size(); i++){
        if(elementAt(i).getLabel().equals(_label)){
            return elementAt(i).getAddress();
        }
    }

    throw new RuntimeException("No such label defined: " + _label);
}

//------------------------------------------------------------------------------
@Override
public void addElement(Object _element){
JumpLabel _label = (JumpLabel)_element;
//--
    for(int i=0; i<size(); i++){
        if(_label.getLabel().equals(elementAt(i).getLabel())){
            throw new RuntimeException("Label '" + _label.getLabel() + "' redefined. (first definiton at address 0x" + Integer.toHexString(elementAt(i).getAddress()) +
                    ")");
        }
    }

    super.addElement(_label);
}

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
//      Change History:
//      $Log: LabelList.java,java $
