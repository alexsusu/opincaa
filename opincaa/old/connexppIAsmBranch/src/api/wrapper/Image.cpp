/*
 * File:   Image.cpp
 *
 * This is the source file for a wrapper class for in image. 
 * Holds image data, size information and helper methods
 * 
 */

#include "../../../include/api/wrapper/Image.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h>

using namespace std;
/*
* Constructor for creating a new Image
* 
* @param width the width of the new Image
* @param height the height of the new Image
* @param image_data the image data for the image, 24 bits/ pixes
* @param bytes_per_pixel the number of bytes of data requried for each image pixel
*       3 for RGB, 1 for luma only, default is RGB
* 
*/ 
Image::Image(unsigned width, unsigned height, unsigned char* image_data, unsigned char bytes_per_pixel = RGB_BYTES_PER_PIXEL)
{
    this->width = width;
    this->height = height;
    this->buffer = image_data;
    this->bytes_per_pixel = bytes_per_pixel;
    this->buffer_size = width * height * bytes_per_pixel;
}

/*
 * Destructor for class Image, releases the buffer associated with this Image
 */
Image::~Image()
{
    delete buffer;
}

/*
* @return Returns the width of the image
*/
unsigned Image::getWidth()
{
    return width;
}

/*
* @return Returns the height of the image
*/
unsigned Image::getHeight()
{
    return height;
}

/*
 * Writes the internal image buffer to the specified address
 * 
 * @param address The address where the image buffer should be written
 * 
 * @throws string if the specified address is NULL
 */
void Image::writeTo(unsigned char * address)
{
    if(address == NULL)
    {
        throw string("Null address in Image::writeTo");
    }
    memcpy(address, buffer, buffer_size);
}

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
* @throws string if the x or y values are outside the image boundaries
*/
void Image::setPixel(unsigned x, unsigned y, int pixel_data){
    if(x >= width || y >= height)
    {
        throw string("Pixel coordonates out of range in Image::setPixel");
    }
    
    for(unsigned i=0; i<bytes_per_pixel; i++)
    {
        buffer[(y * width + x) * bytes_per_pixel + i] = (unsigned char)(pixel_data >> (8 * i));
    }
}

/*
* Return the pixel colour information specified by the coordonates 
* 
* @param x the x coordonate of the pixel
* @param x the y coordonate of the pixel
* @return the pixel information data stored in the return value's least 
*      significat bytes
* 
* @throws string if the x or y values are outside the image boundaries
*/
int Image::getPixel(unsigned x, unsigned y)
{
    if(x >= width || y >= height)
    {
        throw string("Pixel coordonates out of range in Image:getPixel");
    }
    
    int data = 0;
    for(unsigned i=0; i<bytes_per_pixel; i++)
    {
        data &= (int)buffer[(y * width + x) * bytes_per_pixel + i] << (8 * i);
    }
    
    return data;
}

/*
* Return the image data at the specified line_number in the image 
* 
* @param line_number the image line number
* @return a copy of the image data at the specified line
* 
* @throws string if the line_number value is outside the image boundaries
*/
unsigned char* Image::getLine(unsigned line_number)
{
    if(line_number >= height)
    {
        throw string("Line index out of range in Image:getLine");
    }
    
    unsigned line_size = width * bytes_per_pixel;
    unsigned char * line_buffer = new unsigned char[line_size];
    memcpy(line_buffer, buffer + line_number * line_size, line_size);
    return line_buffer;
}

/*
* Sets the image data at the specified line_number in the image. 
* 
* @param line_number the image line number
* @param line_buffer the buffer containing the line number
* 
* @throws string if the line_number value is outside the image boundaries
*/
void Image::setLine(unsigned line_number, unsigned char * line_buffer)
{
    if(line_number >= height)
    {
        throw string("Line index out of range in Image:setLine");
    }
    
    unsigned line_size = width * bytes_per_pixel;
    memcpy(buffer + line_number * line_size, line_buffer, line_size);
}

/*
* Sets the image data at the specified line_number in the image, moving
* all subsequent lines down. The internal image_data buffer will be 
* altered and data will be moved. This is an expensive operation.
* 
* @param line_number the image line number
* @param line_buffer the buffer containing the line number
* 
* @throws string if the line_number value is outside the image boundaries
*/
void Image::insertLine(unsigned line_number, unsigned char * line_buffer)
{
    if(line_number > height)
    {
        throw string("Line index out of range in Image:insertLine");
    }
    
    /* Allocate new buffer */
    unsigned line_size = width * bytes_per_pixel;
    unsigned char * new_buffer = new unsigned char[(height + 1) * line_size];
    
    unsigned first_chunk_size  = line_number * line_size;
    
    /* Copy the first part of the image */
    memcpy(new_buffer, buffer, first_chunk_size);
    
    /* Copy the new line */
    memcpy(new_buffer + first_chunk_size, line_buffer, line_size);
    
    /* Copy the last part of the image */
    memcpy(new_buffer + first_chunk_size + line_size /*dest*/, 
           buffer + first_chunk_size /*source*/, 
           buffer_size - first_chunk_size /*size*/);
    
    /* Update fields */
    height++;
    buffer_size += line_size;
    delete buffer;
    buffer = new_buffer;
}

/*
* Removes the specified line from the image. The internal image_data 
* buffer will be altered and data will be moved. This is an expensive 
* operation.
* 
* @param line_number the image line number
* 
* @throws String if the line_number value is outside the image boundaries
*/
void Image::deleteLine(unsigned line_number)
{
    if(line_number >= height)
    {
        throw string("Line index out of range in Image:deleteLine");
    }
    /* We will simply move all subsequent lines up and update 
     * the buffer_size value, not copying to a new buffer.
     * TODO: add another field to the class called used_buffer_size to
     *      be used if lines are removed and then added, when allocating
     *      a new buffer is not required.
     */
    unsigned line_size = width * bytes_per_pixel;
    memmove(buffer + line_number * line_size, 
            buffer + (line_number + 1) * line_size,
            (height - line_number - 1) * line_size);
    
    height--;
    buffer_size -= line_size;
}

/*
 * Return the image data at the specified column_number in the image 
 * 
 * @param column_number the image column number
 * @return a copy of the image data at the specified column
 * 
 * @throws string if the column_number value is outside the image boundaries
 */
unsigned char* Image::getColumn(unsigned column_number)
{
    if(column_number >= width)
    {
        throw string("Column index out of range in Image:getColumn");
    }
    
    unsigned column_size = height * bytes_per_pixel;
    unsigned char * column_buffer = new unsigned char[column_size];
    for(unsigned line=0; line<height; line++)
    {
        memcpy(column_buffer + line * bytes_per_pixel, 
               buffer + (line * width + column_number) * bytes_per_pixel, 
               bytes_per_pixel);
    }
    return column_buffer;
}

/*
 * Sets the image data at the specified column_number in the image. 
 * 
 * @param column_number the image column number
 * @param column_buffer the buffer containing the column number
 * 
 * @throws string if the column_number value is outside the image boundaries
 */
void Image::setColumn(unsigned column_number, unsigned char * column_buffer)
{
    if(column_number >= width)
    {
        throw string("Column index out of range in Image:getColumn");
    }
    
    for(unsigned line=0; line<height; line++)
    {
        memcpy(buffer + (line * width + column_number) * bytes_per_pixel, 
               column_buffer + line * bytes_per_pixel, 
               bytes_per_pixel);
    }
}

/*
* Sets the image data at the specified column_number in the image, moving
* all subsequent columns right. The internal image_data buffer will be 
* altered and data will be moved. This is an EXTREMELY expensive operation.
* 
* @param column_number the image column number
* @param column_buffer the buffer containing the column number
* 
* @throws string if the column_number value is outside the image boundaries
*/
void Image::insertColumn(unsigned column_number, unsigned char * column_buffer)
{
    /*
     * TODO: Optimize
     */
    if(column_number > width)
    {
        throw string("Column index out of range in Image:insertColumn");
    }
    
    /* Allocate new buffer */
    unsigned column_size = height * bytes_per_pixel;
    unsigned char * new_buffer = new unsigned char[(width + 1) * column_size];
    
    /* Move the last piece of the last line to right,
     * outside of the main loop, since it doesn't match the 
     * loop template
     */
    memcpy(new_buffer + ((height - 1) * (width + 1) + column_number + 1) * bytes_per_pixel, 
           buffer + ((height - 1) * width + column_number) * bytes_per_pixel,
           (width - column_number) * bytes_per_pixel);
    /* Copy the last pixel of the column */
    memcpy(new_buffer + ((height - 1) * (width + 1) + column_number) * bytes_per_pixel,
           column_buffer + (height - 1) * bytes_per_pixel,
           bytes_per_pixel);
    
    for(int line = (int)height - 2; line >=0; line--)
    {
        memcpy(new_buffer + (line * (width + 1) + column_number + 1) * bytes_per_pixel, 
               buffer + (line * width + column_number) * bytes_per_pixel,
               width * bytes_per_pixel);
        /* Copy the pixel of the column */
        memcpy(new_buffer + (line * (width + 1) + column_number) * bytes_per_pixel,
               column_buffer + line * bytes_per_pixel,
               bytes_per_pixel);
    }
    
    /* Copy the first part of the first line */
    memcpy(new_buffer,
           buffer,
           column_number * bytes_per_pixel);
        
    width++;
    buffer_size += column_size;
    delete buffer;
    buffer = new_buffer;
}

/*
* Removes the specified column from the image. The internal image_data 
* buffer will be altered and data will be moved. This is an EXTREMELY 
* expensive operation.
* 
* @param column_number the image line number
* 
* @throws string if the column_number value is outside the image boundaries
*/
void Image::deleteColumn(unsigned column_number)
{
    /* TODO: optimize */
    if(column_number >= width)
    {
        throw string("Column index out of range in Image:deleteColumn");
    }
    
    
    int size_left = buffer_size - column_number * bytes_per_pixel;
    for(unsigned line=0; line < height; line++)
    {
        memmove(buffer + (line * (width - 1) + column_number) * bytes_per_pixel, 
                buffer + (line * width + column_number - 1) * bytes_per_pixel,
                size_left - bytes_per_pixel 
        );
        size_left -= width * bytes_per_pixel;
    }
    width--;
    buffer_size -= height * bytes_per_pixel;
}

/*
* Pad the image by duplicating the first column the specified amount of times.
* 
* @param pad_size the amount of times the column should be duplicated
*/
void Image::padImageLeft(unsigned pad_size)
{
    unsigned char *first_column = getColumn(0);
    for(unsigned i=0; i<pad_size; i++)
    {
        insertColumn(0, first_column);
    }
    delete first_column;
}

/*
* Pad the image by duplicating the last column the specified amount of times.
* 
* @param pad_size the amount of times the column should be duplicated
*/
void Image::padImageRight(unsigned pad_size)
{
    unsigned char *last_column = getColumn(width - 1);
    for(unsigned i=0; i<pad_size; i++)
    {
        insertColumn(width, last_column);
    }
    delete last_column;
}

/*
* Pad the image by duplicating the first line the specified amount of times.
* 
* @param pad_size the amount of times the line should be duplicated
*/
void Image::padImageTop(unsigned pad_size)
{
    unsigned char *first_line = getLine(0);
    for(unsigned i=0; i<pad_size; i++)
    {
        insertLine(0, first_line);
    }
    delete first_line;
}

/*
* Pad the image by duplicating the last line the specified amount of times.
* 
* @param pad_size the amount of times the line should be duplicated
*/
void Image::padImageBottom(unsigned pad_size)
{
    unsigned char *last_line = getLine(height - 1);
    for(unsigned i=0; i<pad_size; i++)
    {
        insertLine(height, last_line);
    }
    delete last_line;
}

/*
* Pad the image by duplicating the first and last lines, and the first and
* last columns the specified amount of times.
* 
* @param pad_size the amount of times the line/column should be duplicated
*/
void Image::padImage(unsigned pad_size)
{
    padImageLeft(pad_size);
    padImageRight(pad_size);
    padImageTop(pad_size);
    padImageBottom(pad_size);
}

/*
 * Unpad the image by removing the first pad_size columns
 * 
 * @param pad_size the amount of columns that should be removed
 *
 * @throws string if pad_size is larger than the actual image
 */
void Image::unpadImageLeft(unsigned pad_size)
{
    if(pad_size > width)
    {
        throw string("pad_size larger that image width in Image::unpadImageLeft");
    }
    
    for(unsigned i=0; i<pad_size; i++)
    {
        deleteColumn(0);
    }
}

/*                                     *
 * Unpad the image by removing the last pad_size columns
 * 
 * @param pad_size the amount of columns that should be removed
 *
 * @throws string if pad_size is larger than the actual image
 */
void Image::unpadImageRight(unsigned pad_size)
{
    if(pad_size > width)
    {
        throw string("pad_size larger that image width in Image::unpadImageRight");
    }
    
    for(unsigned i=0; i<pad_size; i++)
    {
        deleteColumn(width - 1);
    }
}

/*
 * Unpad the image by removing the first pad_size lines.
 * 
 * @param pad_size the amount lines that should be removed
 * 
 * @throws string if pad_size is larger than the actual image
 */
void Image::unpadImageTop(unsigned pad_size)
{
    if(pad_size > height)
    {
        throw string("pad_size larger that image height in Image::unpadImageTop");
    }
    
    for(unsigned i=0; i<pad_size; i++)
    {
        deleteLine(0);
    }
}

/*
 * Unpad the image by removing the last pad_size lines.
 * 
 * @param pad_size the amount lines that should be removed
 * 
 * @throws string if pad_size is larger than the actual image
 */
void Image::unpadImageBottom(unsigned pad_size)
{
    if(pad_size > height)
    {
        throw string("pad_size larger that image height in Image::unpadImageBottom");
    }
    
    for(unsigned i=0; i<pad_size; i++)
    {
        deleteLine(height - 1);
    }
}

/*
 * Unpad the image by removing the first and last pad_size lines, and the first and
 * last pad_size columns 
 * 
 * @param pad_size the amount of lines/columns that should be removed
 * 
 * @throws string if pad_size is larger than the actual image
 */
void Image::unpadImage(unsigned pad_size)
{
    if(pad_size * 2 > height)
    {
        throw string("pad_size * 2 larger that image height in Image::unpadImage");
    }
    
    if(pad_size * 2 > width)
    {
        throw string("pad_size * 2 larger that image width in Image::unpadImage");
    }
    
    unpadImageLeft(pad_size);
    unpadImageRight(pad_size);
    unpadImageTop(pad_size);
    unpadImageBottom(pad_size);
}

/*
* Returns a new Image object representing a piece of the image at the 
* specified coordonates, and with the specified dimensions
* 
* @param x the x coordonate of top left pixel of the slice
* @param y the y coordonate of top left pixel of the slice
* @param slice_width the width of the required slice
* @param slice_height the height of the required slice
*/
Image* Image::getSlice(unsigned x, unsigned y, unsigned slice_width, unsigned slice_height)
{
    /* If the slice dimesions are outside of the image,
     * copy only the relevant data, then pad to actual required size
     */
    unsigned copy_slice_width = x + slice_width < width ? slice_width : width - x;
    unsigned copy_slice_height = y + slice_height < height ? slice_height : height - y;
    unsigned char * new_buffer = new unsigned char[copy_slice_height * copy_slice_width * bytes_per_pixel];
    /* Copy the slice */
    for(unsigned line = 0; line < copy_slice_height; line++)
    {
        memcpy(new_buffer + line * copy_slice_width * bytes_per_pixel,
               buffer + ((y + line) * width + x) * bytes_per_pixel,
               copy_slice_width * bytes_per_pixel);
    }
    
    Image* slice = new Image(copy_slice_width, copy_slice_height, new_buffer, bytes_per_pixel);
    slice->padImageBottom(slice_height - copy_slice_height);
    slice->padImageRight(slice_width - copy_slice_width);
    return slice;
}

/*
* Sets an area of the current image with the specified slice
* 
* @param x the x coordonate of top left pixel of the slice
* @param x the y coordonate of top left pixel of the slice
* @param slice the new piece of the image
*/
void Image::setSlice(unsigned x, unsigned y, Image * slice)
{
    unsigned available_width = width - x;
    unsigned available_height = height - y;
    
    if(available_height < slice->height)
    {
        slice->unpadImageBottom(slice->height - available_height);
    }
    if(available_width < slice->width)
    {
        slice->unpadImageRight(slice->width - available_width);
    }
    
    /* Copy the slice */
    for(unsigned line = 0; line < available_height; line++)
    {
        memcpy(buffer + ((y + line) * width + x) * bytes_per_pixel,
               slice->buffer + line * available_width * bytes_per_pixel,
               available_width * bytes_per_pixel);
    }
}

/*
 * Converts the current image from RGB 3 bytes/pixel representation to
 * Luma 1 byte/pixel representation. Croma information is lost;
 * 
 * @throws string if Image is already in Luma format
 */
void Image::convertRgbToLuma()
{
    if(bytes_per_pixel == LUMA_BYTES_PER_PIXEL)
    {
        throw string("Image format is already in LUMA_BYTES_PER_PIXEL Image::convertRgbToLuma");
    }
    
    for(unsigned i=0; i<width * height * bytes_per_pixel; i+=3)
    {
        unsigned char red   = buffer[i + 0];
        unsigned char green = buffer[i + 1];
        unsigned char blue  = buffer[i + 2];
        double luma = 0.299 * red + 0.587 * green + 0.114 * blue;
        buffer[i] = luma > 255 ? 255 : (unsigned char)luma;
    }
    
    bytes_per_pixel = LUMA_BYTES_PER_PIXEL;
    buffer_size = width * height * bytes_per_pixel;
}

/*
 * Dumps the image data to standard output stream
 */
void Image::dumpToConsole()
{
    for(unsigned y=0; y<height; y++)
    {
        for(unsigned x=0; x<width; x++)
        {
            printf("{%3d", buffer[(y * width + x) * bytes_per_pixel]);
            for(unsigned i=1; i<bytes_per_pixel; i++)
            {
                printf(",%3d", buffer[(y * width + x + i) * bytes_per_pixel]);
            }
            printf("} ");
        }
        printf("\n");
    }    
}
