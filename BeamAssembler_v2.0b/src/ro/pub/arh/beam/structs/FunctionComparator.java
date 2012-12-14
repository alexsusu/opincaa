//------------------------------------------------------------------------------
//
//		Interleaved Multithreading Processor
//			- FunctionComparator -
//
//------------------------------------------------------------------------------
//     $Id: FunctionComparator.java,java 1.1 2009/05/14 12:35:50 rhobincu Exp $
//     $Author: rhobincu $
//     $Revision: 1.0 $
//     $Locker:  $
//     $State: Exp $
//     $Date: 2009/05/14 12:35:50 $
//------------------------------------------------------------------------------
package ro.pub.arh.beam.structs;

//------------------------------------------------------------------------------

import java.util.Comparator;

class FunctionComparator implements Comparator{

//------------------------------------------------------------------------------
public FunctionComparator(){
}

//------------------------------------------------------------------------------
public int compare(Object _object1, Object _object2){
    try{
        Function _function1 = (Function)_object1;
        Function _function2 = (Function)_object2;
        return _function1.getAddress() - _function2.getAddress();
    }catch(Exception _e){
        throw new RuntimeException("Invalid object in program array.");
    }
}

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
//      Change History:
//      $Log: FunctionComparator.java,java $
