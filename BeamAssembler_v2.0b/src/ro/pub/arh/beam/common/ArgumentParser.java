/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.common;

import java.util.Vector;

/**
 *
 * @author Echo
 */
public class ArgumentParser {

    private Vector<Pair> switches;
    private Vector<String> freeArgs;

public ArgumentParser(String[] _args) {
    switches = new Vector();
    freeArgs = new Vector();
    for(int i=0; i<_args.length; i++){
        if(_args[i].startsWith("--")){
            switches.add(new Pair(_args[i].substring(2), "true"));
        }else if(_args[i].startsWith("-")){
            try{
                switches.add(new Pair(_args[i].substring(1), _args[i + 1]));
            }catch(ArrayIndexOutOfBoundsException _aiobe){
                throw new RuntimeException("Missing value for switch " + _args[i]);
            }
            i++;
        }else{
            freeArgs.add(_args[i]);
        }
    }
}

public String getSwitch(String _switch){
    for(int i=0; i<switches.size(); i++){
        if(switches.elementAt(i).identifier.equals(_switch)){
            return switches.elementAt(i).value;
        }
    }

    return null;
}

public Vector<String> getFreeArgs(){
    return freeArgs;
}

class Pair{
    String identifier;
    String value;

    Pair(String _identifier, String _value){
        identifier = _identifier;
        value = _value;
    }
}


}
