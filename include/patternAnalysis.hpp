/* There are several types of patterns that I want to look for.
 *
 * This first pattern that needs to be checked is to be able to identify regions
 * of memory that are changing. After that, we can attempt to align the values
 * to different types
 * So what we'll need is the history of a region's memory over time.
 *
 * After we find the addresses of the changing parts of memeory, we should copy
 * their values to different desired types, and then graph them over time.
 *
 * In the case that we encounter a memory map change, we'll have to do
 * everything all over again.
 *
 * Since we are graphing, we'll need to use sdl or some sort of graphics
 * library. Surely can't be that bad right?
 * */
