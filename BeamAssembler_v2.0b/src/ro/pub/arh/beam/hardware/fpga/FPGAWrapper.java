/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.fpga;

/**
 *
 * @author Echo
 */
public class FPGAWrapper {


static {
    System.loadLibrary("FPGAWrapper");
}

//constructor calls initializeSimulator
public FPGAWrapper(){

}

//writes ddr with the contents of the data array
public native int writeDDR(byte[] data, int addr, int len);

//returns an array of the contents of ddr
public native int readDDR(byte[] data, int addr, int len);

public native int reset();


}
