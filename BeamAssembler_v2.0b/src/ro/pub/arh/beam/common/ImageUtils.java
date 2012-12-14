/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package ro.pub.arh.beam.common;

import java.awt.image.BufferedImage;
import java.io.File;
import javax.imageio.ImageIO;

/**
 *
 * @author rhobincu
 */
public class ImageUtils {
    
    public static final int DEFAULT_IMAGE_SIZE = 128;
    public static final int DEFAULT_IMAGE_WIDTH = 800;
    public static final int DEFAULT_IMAGE_HEIGHT = 600;
    
    
    
public static byte[] loadBitmap(File _selectedFile) throws Exception{
    BufferedImage _image = null;
    _image = ImageIO.read(_selectedFile);
    int _width = _image.getWidth();
    int _height = _image.getHeight();
    if(_image.getWidth() != _image.getHeight() && _image.getHeight() != DEFAULT_IMAGE_SIZE){
        System.out.println("Warning: Invalid image size. Required 128x128.");
    }
    int[] _buffer = _image.getRGB(0, 0, _width, _height, null, 0, _width);
    byte[] _byteFixedBuffer = new byte[_width * _height * 2];

    for(int i=0; i<_buffer.length; i++){
        _byteFixedBuffer[2 * i] = getLuma(_buffer[i]);
    }

    return _byteFixedBuffer;
}

public static void saveBitmap(File _selectedFile, byte[] _bitmap) throws Exception{

    int[] _buffer = new int[DEFAULT_IMAGE_SIZE * DEFAULT_IMAGE_SIZE];

    for(int i=0; i<_buffer.length; i++){
        int _component = (int)_bitmap[2 * i] & 0xff;
        _buffer[i] = (_component << 16) | (_component << 8) | _component;
    }

    BufferedImage _renderedImage = new BufferedImage(DEFAULT_IMAGE_SIZE, DEFAULT_IMAGE_SIZE, BufferedImage.TYPE_INT_RGB);
    _renderedImage.setRGB(0, 0, DEFAULT_IMAGE_SIZE, DEFAULT_IMAGE_SIZE, _buffer, 0, DEFAULT_IMAGE_SIZE);

    if(!_selectedFile.getName().endsWith(".png")){
        _selectedFile = new File(_selectedFile.getCanonicalPath() + ".png");
    }

    ImageIO.write(_renderedImage, "png", _selectedFile);

}

private static byte getLuma(int _rgb){
    double _luma = 0.212 * ((_rgb >>> 16) & 0xff) + 0.701 * ((_rgb >>> 8) & 0xff) + 0.087 * (_rgb & 0xff);
    return _luma >= 255 ? (byte)255 : (byte)_luma;
}

}
