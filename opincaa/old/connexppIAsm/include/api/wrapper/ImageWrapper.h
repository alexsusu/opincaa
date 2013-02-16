/*
 * File:   ImageWrapper.h
 *
 * Header file for the ImageWrapper, used for slicing and/ or assambling
 * an image file for being processed in the Connex array
 * 
 * This class can perform two operations:
 *  - slice an original image, specified by an original_width, original_height,
 * and image_data into slices of slice_width, slice_height, slice_pad_x, 
 * slice_pad_y
 * 
 *  - merge a set of slices into in original image (the inverse operation)
 * 
 * These operations will be done on separate threads so it doesn't slow down the 
 * IO and Connex instruction threads.
 */

#ifndef IMAGEWRAPPER_H
#define IMAGEWRAPPER_H

#include <stdio.h>
#include <stdlib.h>
#include "Image.h"

enum{
    /*
     * This is the state of an ImageWrapper object that
     * hasn't been issued any slice or merge commands
     */
    STATE_IDLE = 0,
    
    /*
     * This is the state of an ImageWrapper object that
     * has been issued a slice command 
     */
    STATE_SLICING,
    
    /*
     * This is the state of an ImageWrapper object that
     * has been issued a merge command 
     */
    STATE_MERGING,
    
    /*
     * This is the state of an ImageWrapper object that
     * has been issued a slice command and the work has
     * finished
     */
    STATE_SLICE_DONE,
    
    /*
     * This is the state of an ImageWrapper object that
     * has been issued a merge command and the work has
     * finished
     */
    STATE_MERGE_DONE
};

class ImageWrapper
{
    public:
        ImageWrapper(Image *image, unsigned pad);
        
        vector<Image*>* getSlices();
        void merge(vector<Image*>* slices);
    protected:
    private:
        /*
         * This is the operation type, it can hold a value
         * of the enum above
         */
        int operation_type;
};

#endif // IMAGEWRAPPER_H
