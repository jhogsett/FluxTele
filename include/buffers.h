#ifndef __BUFFERS_H__
#define __BUFFERS_H__

#define FSTRING_BUFFER 16  // Reduced from 31 to 16 (longest string is 8 chars + null)

// string buffer used to load string data from program memory (F() strings)
extern char fstring_buffer[FSTRING_BUFFER];

// Global display buffer for VFO and other display updates (max 12 chars + null)
extern char display_text_buffer[13];

#endif