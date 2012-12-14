//------------------------------------------------------------------------------
//
//		Interleaved Multithreading Processor
//			- JumpLabel -
//
//------------------------------------------------------------------------------
//     $Id: JumpLabel.java,java 1.1 2009/05/14 12:35:50 rhobincu Exp $
//     $Author: rhobincu $
//     $Revision: 1.0 $
//     $Locker:  $
//     $State: Exp $
//     $Date: 2009/05/14 12:35:50 $
//------------------------------------------------------------------------------
package ro.pub.arh.beam.assembler.structs;

//------------------------------------------------------------------------------
public class JumpLabel{

    private int address;
    private String label;

//------------------------------------------------------------------------------
public JumpLabel(int _address, String _label){
    address = _address;
    label = _label;
}

//------------------------------------------------------------------------------
public String getLabel(){
    return label;
}

//------------------------------------------------------------------------------
public int getAddress(){
    return address;
}

public void offsetAddress(int _offset) {
    address += _offset;
}

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
//      Change History:
//      $Log: JumpLabel.java,java $
