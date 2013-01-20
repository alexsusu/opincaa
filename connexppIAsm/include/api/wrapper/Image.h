/*
 * File:   Image.h
 *
 * This is the header file for a wrapper class for in image. 
 * Holds image data, size information and helper methods
 * 
 */

#ifndef IMAGE_H
#define IMAGE_H

enum{
    LUMA_BYTES_PER_PIXEL    =    1,
    RGB_BYTES_PER_PIXEL     =    3
};

class Image
{
    public:
        /*
        * Constructor for creating a new Image
        * 
        * @param width the width of the new Image
        * @param height the height of the new Image
        * @param image_data the image data for the image
        * @param bytes_per_pixel the number of bytes of data requried for each image pixel
        *       3 for RGB, 1 for luma only, default is RGB
        */ 
        Image(unsigned width, unsigned height, unsigned char* image_data, unsigned char bytes_per_pixel);
        
        /*
         * Destructor for class Image, releases the buffer associated with this Image
         */
        ~Image();
        
        /*
         * @return Returns the width of the image
         */
        unsigned getWidth();
        
        /*
         * @return Returns the height of the image
         */
        unsigned getHeight();
        
        /*
         * Writes the internal image buffer to the specified address
         * 
         * @param address The address where the image buffer should be written
         * 
         * @throws *string if the specified address is NULL
         */
        void writeTo(unsigned char * address);
        
        /*
         * Writes the pixel at the specified coordonates with the value
         * specified by pixel_data (where only the least significat bytes hold
         * useful information)
         * 
         * @param x the x coordonate of the pixel
         * @param x the y coordonate of the pixel
         * @param pixel_data the pixed data containing bytes_per_pixel useful
         *       information
         * 
         * @throws *string if the x or y values are outside the image boundaries
         */
        void setPixel(unsigned x, unsigned y, int pixel_data);
        
        /*
         * Return the pixel colour information specified by the coordonates 
         * 
         * @param x the x coordonate of the pixel
         * @param x the y coordonate of the pixel
         * @return the red, green, blue data stored in the return value's least 
         *      significat bytes
         * 
         * @throws *string if the x or y values are outside the image boundaries
         */
        int getPixel(unsigned x, unsigned y);
        
        /*
         * Return the image data at the specified line_number in the image 
         * 
         * @param line_number the image line number
         * @return a copy of the image data at the specified line
         * 
         * @throws *string if the line_number value is outside the image boundaries
         */
        unsigned char* getLine(unsigned line_number);
        
        /*
         * Sets the image data at the specified line_number in the image. 
         * 
         * @param line_number the image line number
         * @param line_buffer the buffer containing the line number
         * 
         * @throws *string if the line_number value is outside the image boundaries
         */
        void setLine(unsigned line_number, unsigned char * line_buffer);
        
       /*
        * Sets the image data at the specified line_number in the image, moving
        * all subsequent lines down. The internal image_data buffer will be 
        * altered and data will be moved. This is an expensive operation.
        * 
        * @param line_number the image line number
        * @param line_buffer the buffer containing the line number
        * 
        * @throws *string if the line_number value is outside the image boundaries
        */
       void insertLine(unsigned line_number, unsigned char * line_buffer);
       
       /*
        * Removes the specified line from the image. The internal image_data 
        * buffer will be altered and data will be moved. This is an expensive 
        * operation.
        * 
        * @param line_number the image line number
        * 
        * @throws *string if the line_number value is outside the image boundaries
        */
       void deleteLine(unsigned line_number);
       
       /*
        * Return the image data at the specified column_number in the image 
        * 
        * @param column_number the image column number
        * @return a copy of the image data at the specified column
        * 
        * @throws *string if the column_number value is outside the image boundaries
        */
       unsigned char* getColumn(unsigned column_number);
       
       /*
        * Sets the image data at the specified column_number in the image. 
        * 
        * @param column_number the image column number
        * @param column_buffer the buffer containing the column number
        * 
        * @throws *string if the column_number value is outside the image boundaries
        */
       void setColumn(unsigned column_number, unsigned char * column_buffer);
       
       /*
        * Sets the image data at the specified column_number in the image, moving
        * all subsequent columns right. The internal image_data buffer will be 
        * altered and data will be moved. This is an EXTREMELY expensive operation.
        * 
        * @param column_number the image column number
        * @param column_buffer the buffer containing the column number
        * 
        * @throws *string if the column_number value is outside the image boundaries
        */
       void insertColumn(unsigned column_number, unsigned char * column_buffer);
       
       /*
        * Removes the specified column from the image. The internal image_data 
        * buffer will be altered and data will be moved. This is an EXTREMELY 
        * expensive operation.
        * 
        * @param column_number the image line number
        * 
        * @throws *string if the column_number value is outside the image boundaries
        */
       void deleteColumn(unsigned column_number);
       
       /*
        * Pad the image by duplicating the first column the specified amount of times.
        * 
        * @param pad_size the amount of times the column should be duplicated
        */
       void padImageLeft(unsigned pad_size);
       
       /*
        * Pad the image by duplicating the last column the specified amount of times.
        * 
        * @param pad_size the amount of times the column should be duplicated
        */
       void padImageRight(unsigned pad_size);
       
       /*
        * Pad the image by duplicating the first line the specified amount of times.
        * 
        * @param pad_size the amount of times the line should be duplicated
        */
       void padImageTop(unsigned pad_size);
       
       /*
        * Pad the image by duplicating the last line the specified amount of times.
        * 
        * @param pad_size the amount of times the line should be duplicated
        */
       void padImageBottom(unsigned pad_size);
       
       /*
        * Pad the image by duplicating the first and last lines, and the first and
        * last columns the specified amount of times.
        * 
        * @param pad_size the amount of times the line/column should be duplicated
        */
       void padImage(unsigned pad_size);
       
       /*
        * Unpad the image by removing the first pad_size columns
        * 
        * @param pad_size the amount of columns that should be removed
        *
        * @throws *string if pad_size is larger than the actual image
        */
       void unpadImageLeft(unsigned pad_size);
       
        /*                                     *
        * Unpad the image by removing the last pad_size columns
        * 
        * @param pad_size the amount of columns that should be removed
        *
        * @throws *string if pad_size is larger than the actual image
        */
       void unpadImageRight(unsigned pad_size);
       
       /*
        * Unpad the image by removing the first pad_size lines.
        * 
        * @param pad_size the amount lines that should be removed
        * 
        * @throws *string if pad_size is larger than the actual image
        */
       void unpadImageTop(unsigned pad_size);
       
       /*
        * Unpad the image by removing the last pad_size lines.
        * 
        * @param pad_size the amount lines that should be removed
        * 
        * @throws *string if pad_size is larger than the actual image
        */
       void unpadImageBottom(unsigned pad_size);
       
       /*
        * Unpad the image by removing the first and last pad_size lines, and the first and
        * last pad_size columns 
        * 
        * @param pad_size the amount of lines/columns that should be removed
        * 
        * @throws *string if pad_size is larger than the actual image
        */
       void unpadImage(unsigned pad_size);
      
       /*
        * Returns a new Image object representing a piece of the image at the 
        * specified coordonates, and with the specified dimensions
        * 
        * @param x the x coordonate of top left pixel of the slice
        * @param x the y coordonate of top left pixel of the slice
        * @param slice_width the width of the required slice
        * @param slice_height the height of the required slice
        */
       Image* getSlice(unsigned x, unsigned y, unsigned slice_width, unsigned slice_height);
       
       /*
        * Sets an area of the current image with the specified slice
        * 
        * @param x the x coordonate of top left pixel of the slice
        * @param x the y coordonate of top left pixel of the slice
        * @param slice the new piece of the image
        */
       void setSlice(unsigned x, unsigned y, Image * slice);

       /*
        * Converts the current image from RGB 3 bytes/pixel representation to
        * Luma 1 byte/pixel representation. Croma information is lost;
        * 
        * @throws string if Image is already in Luma format
        */
       void convertRgbToLuma();
       
       /*
        * Dumps the image data to standard output stream
        */
       void dumpToConsole();
       
       
    private:
        /*
         * The width of the image
         */
        unsigned width;
        
        /*
         * The width of the image
         */
        unsigned height;
        
        /*
         * The buffer holding the image data
         */
        unsigned char *buffer;
        
        /*
         * The number of bytes in the buffer required for each pixel
         */
        unsigned int bytes_per_pixel;
        
        /*
         * The used size of the buffer holding the image data (the buffer itself 
         * may be larger)
         */
        unsigned int buffer_size;
        
};

#endif // IMAGE_H
