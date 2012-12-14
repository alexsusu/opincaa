/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.utils.gui;

import java.awt.Color;
import java.util.regex.*;
import javax.swing.JTextArea;
import javax.swing.text.*;

/**
 *
 * @author Ares
 */
public class HighlightedTextArea extends JTextArea{

public HighlightedTextArea(){
    super();
    setBackground(Color.LIGHT_GRAY);
}

public void highlight(String _regex, Color _color) {
    try {
        Pattern _pattern = Pattern.compile(_regex);
        Matcher _matcher = _pattern.matcher(getText());

        // Search for _pattern
        Highlighter _highlighter = getHighlighter();
        while (_matcher.find()){
            _highlighter.addHighlight(_matcher.start(), _matcher.end(), new MyHighlightPainter(_color));
        }
    } catch (BadLocationException e) {
    }
}

private void highlight(int _start, int _end, Color _color) {
    try {
        Highlighter _highlighter = getHighlighter();
        _highlighter.addHighlight(_start, _end, new MyHighlightPainter(_color));
    } catch (BadLocationException e) {
    }
}

public void highlight(int _lineNumber, Color _color){
    int _currentPosition = 0;
     if(_lineNumber > 0){
            do{
                if(_lineNumber == 1){
                    setCaretPosition(_currentPosition + 1);
                    highlight(_currentPosition, getText().indexOf("\n", _currentPosition + 1), _color);
                    break;
                }
                 _currentPosition = getText().indexOf("\n", _currentPosition + 1);
                _lineNumber--;
            }while(true);
        }
}

// Removes only our private highlights
public void removeHighlights() {
    Highlighter _highlighter = getHighlighter();
    Highlighter.Highlight[] _highlights = _highlighter.getHighlights();

    for (int i=0; i<_highlights.length; i++) {
        if (_highlights[i].getPainter() instanceof MyHighlightPainter) {
            _highlighter.removeHighlight(_highlights[i]);
        }
    }
}

    @Override
public int getRowHeight(){
    return super.getRowHeight();
}

// A private subclass of the default highlight painter
class MyHighlightPainter extends DefaultHighlighter.DefaultHighlightPainter {
    public MyHighlightPainter(Color _color) {
        super(_color);
    }
}


}
