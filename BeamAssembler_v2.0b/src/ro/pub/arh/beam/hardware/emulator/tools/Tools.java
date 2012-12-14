/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.tools;

/**
 *
 * @author rhobincu
 */
public class Tools {

private Tools(){

}

public static int rotate(int _value, int _shift, int _valueBitCount){
    return (_value >>> _shift) | (_value << (_valueBitCount - _shift));
}

}
