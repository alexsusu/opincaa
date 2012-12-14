/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.assembler.gui;

import java.awt.Point;
import java.awt.Rectangle;
import java.io.*;
import javax.swing.JFrame;

/**
 *
 * @author Ares
 */
public class GuiConfig implements Serializable{



    public File DIRECTORY_PATH;
    public String[] HISTORY;
    public String[] OPENED_FILES;
    public boolean highLightEnabled;
    public int state;
    public Point position;
    public Rectangle size;
    public int stackSize;

    public static GuiConfig config;
    public static final String CONFIG_FILE = "gui_config.conf";
    public String GCC_PATH;
    public int GCC_OPTIMIZATION_LEVEL;
    public int dividerLocation;
    public String INCLUDE_PATH;
    public String OUTPUT_PATH;

    public boolean LINK_MALLOC;
    public boolean LINK_PTHREAD;
    public boolean LINK_STDIO;

    public String LIBS_PATH;
    public String BIN_PATH;

    public int SERVER_PORT;
    public String SERVER_ADDRESS;


public GuiConfig() throws IOException{
    HISTORY = new String[0];
    OPENED_FILES = new String[0];
    GCC_PATH = null;
    INCLUDE_PATH = null;
    GCC_OPTIMIZATION_LEVEL = 0;
    state = JFrame.MAXIMIZED_BOTH;
    size = new Rectangle(0, 0, 500, 400);
    dividerLocation = 250;
    stackSize = 128 * 1024;
    DIRECTORY_PATH = new File(".");
    LINK_STDIO = true;
    LINK_PTHREAD = true;
    LINK_MALLOC = true;
    OUTPUT_PATH = (new File(".")).getCanonicalPath();
    BIN_PATH = (new File(".")).getCanonicalPath();

    SERVER_ADDRESS = "phd2.arh.pub.ro";
    SERVER_PORT = 9001;
}

public static void loadConfig() {
    try {
        ObjectInputStream _guiConfigReader = new ObjectInputStream(new FileInputStream(CONFIG_FILE));
        config = (GuiConfig)_guiConfigReader.readObject();
    }catch (Exception ex) {
        System.err.println("Error reading config file. Creating new...");
        try{
            config = new GuiConfig();
        } catch (IOException ioe) {
          ex.printStackTrace();
        }
    }
}

public static void saveConfig() {
    try {
        ObjectOutputStream _guiConfigWriter = new ObjectOutputStream(new FileOutputStream(CONFIG_FILE));
        _guiConfigWriter.writeObject(config);
    }catch (IOException ex) {
       ex.printStackTrace();
    }
}

public String[] getLibs(){
    int _libsCount = (LINK_STDIO ? 1 : 0) +
            (LINK_PTHREAD ? 1 : 0) +
            (LINK_MALLOC ? 1 : 0);
    String[] _libs = new String[_libsCount];
    _libsCount = 0;
    if(LINK_STDIO){
        _libs[_libsCount++] = LIBS_PATH + "/stdio.s";
    }
    if(LINK_PTHREAD){
        _libs[_libsCount++] = LIBS_PATH + "/pthread.s";
    }
    if(LINK_MALLOC){
        _libs[_libsCount++] = LIBS_PATH + "/malloc.s";
    }

    return _libs;
}

}
