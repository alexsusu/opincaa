/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.common;

/**
 *
 * @author rhobincu
 */
public class HexConverter {

    private HexConverter(){

    }

    public static String intToHex(int _value, int _wide){
        String _initialHex = Integer.toHexString(_value).toUpperCase();
        String _zeros = "";
        for(int i=0; i<_wide - _initialHex.length(); i++){
            _zeros += "0";
        }
        return _zeros + _initialHex;
    }

}
