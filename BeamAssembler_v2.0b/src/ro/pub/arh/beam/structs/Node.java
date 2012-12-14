/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.structs;

/**
 *
 * @author rhobincu
 */
public class Node<Type> {

    private Node nextNode;
    private Node previousNode;

    private Type data;

    public Node(Type _pair) {
        nextNode = null;
        previousNode = null;
        data = _pair;
    }

    public Node getNextNode() {
        return nextNode;
    }

    public void setNextNode(Node nextNode) {
        this.nextNode = nextNode;
    }

    public Node getPreviousNode() {
        return previousNode;
    }

    public void setPreviousNode(Node previousNode) {
        this.previousNode = previousNode;
    }

    public Type getData() {
        return data;
    }

    public void setData(Type data) {
        this.data = data;
    }



}
