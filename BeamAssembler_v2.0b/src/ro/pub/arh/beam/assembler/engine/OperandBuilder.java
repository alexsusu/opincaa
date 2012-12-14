//------------------------------------------------------------------------------
//
//		Interleaved Multithreading Processor
//			- OperandBuilder -
//
//------------------------------------------------------------------------------
//     $Id: OperandBuilder.java,java 1.1 2009/05/14 12:35:50 rhobincu Exp $
//     $Author: rhobincu $
//     $Revision: 1.0 $
//     $Locker:  $
//     $State: Exp $
//     $Date: 2009/05/14 12:35:50 $
//------------------------------------------------------------------------------
package ro.pub.arh.beam.assembler.engine;

//------------------------------------------------------------------------------
import ro.pub.arh.beam.assembler.Assembler;

//------------------------------------------------------------------------------
public class OperandBuilder{

//------------------------------------------------------------------------------
private OperandBuilder(){

}

//    public static final byte addTh    = (byte)0x84;
//    public static final byte remTh    = (byte)0x85;
//    public static final byte thBase   = (byte)0x87;


//------------------------------------------------------------------------------
public static String testLabel(String _label){

    if(_label.matches("\\w+(\\([\\w_\\.]+([\\+\\-][0-9]+)?\\))?")){
        return _label;
    }
    
    throw new RuntimeException("Invalid label for instruction: " + _label);
}

//------------------------------------------------------------------------------
public static int getRegisterNumber(String _regName){
int _regNumber;
//--
    if(_regName.matches("[rR]\\d{1,2}")){
            _regNumber = Integer.parseInt(_regName.substring(1));
            if(_regNumber >= 0 && _regNumber < Assembler.REGISTER_COUNT){
                return _regNumber;
            }
    }

    throw new RuntimeException("Invalid register ID: " + _regName);
}

//------------------------------------------------------------------------------
public static int getValue(String _stringNumber){
    try{
        if(_stringNumber.startsWith("0x")){
            return Integer.parseInt(_stringNumber.substring(2), 16);
        }else{
            return Integer.parseInt(_stringNumber);
        }
    }catch(NumberFormatException _nfe){
        throw new RuntimeException("Invalid value to load: " + _stringNumber);
    }
}

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
//      Change History:
//      $Log: OperandBuilder.java,java $
