/**	
 * |----------------------------------------------------------------------
 * | Copyright (c) 2017 Tilen Majerle
 * |  
 * | Permission is hereby granted, free of charge, to any person
 * | obtaining a copy of this software and associated documentation
 * | files (the "Software"), to deal in the Software without restriction,
 * | including without limitation the rights to use, copy, modify, merge,
 * | publish, distribute, sublicense, and/or sell copies of the Software, 
 * | and to permit persons to whom the Software is furnished to do so, 
 * | subject to the following conditions:
 * | 
 * | The above copyright notice and this permission notice shall be
 * | included in all copies or substantial portions of the Software.
 * | 
 * | THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * | EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * | OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * | AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * | HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * | WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * | FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * | OTHER DEALINGS IN THE SOFTWARE.
 * |----------------------------------------------------------------------
 */
#define GUI_INTERNAL
#include "gui_buffer.h"

uint8_t GUI_BUFFER_Init(GUI_BUFFER_t* Buffer, uint32_t Size, void* BufferPtr) {
	if (Buffer == NULL) {									/* Check buffer structure */
		return 1;
	}
	memset(Buffer, 0, sizeof(*Buffer));        		        /* Set buffer values to all zeros */
    
	Buffer->Size = Size;                        			/* Set default values */
	Buffer->Buffer = BufferPtr;
	Buffer->StringDelimiter = '\n';
	
	if (!Buffer->Buffer) {                      			/* Check if malloc should be used */
		Buffer->Buffer = __GUI_MEMALLOC(Size * sizeof(uint8_t));    /* Try to allocate memory for buffer */
		if (!Buffer->Buffer) {                  			/* Check if allocated */    
			Buffer->Size = 0;                   			/* Reset size */
			return 1;                           			/* Return error */
		} else {
			Buffer->Flags |= GUI_BUFFER_MALLOC;     		/* Set flag for malloc */
		}
	}
	Buffer->Flags |= GUI_BUFFER_INITIALIZED;				/* We are initialized */
	
	return 0;												/* Initialized OK */
}

void GUI_BUFFER_Free(GUI_BUFFER_t* Buffer) {
	if (Buffer == NULL) {									/* Check buffer structure */
		return;
	}
	if (Buffer->Flags & GUI_BUFFER_MALLOC) {			    /* If malloc was used for allocation */
		__GUI_MEMFREE(Buffer->Buffer);						/* Free memory */
	}
	Buffer->Flags = 0;
	Buffer->Size = 0;
}

uint32_t GUI_BUFFER_Write(GUI_BUFFER_t* Buffer, const void* Data, uint32_t count) {
	uint32_t i = 0;
	uint32_t free;
    const uint8_t* d = (const uint8_t *)Data;
#if GUI_BUFFER_FAST
	uint32_t tocopy;
#endif

	if (Buffer == NULL || count == 0) {						/* Check buffer structure */
		return 0;
	}
	if (Buffer->In >= Buffer->Size) {						/* Check input pointer */
		Buffer->In = 0;
	}
	free = GUI_BUFFER_GetFree(Buffer);							/* Get free memory */
	if (free < count) {										/* Check available memory */	
		if (free == 0) {									/* If no memory, stop execution */
			return 0;
		}
		count = free;										/* Set values for write */
	}

	/* We have calculated memory for write */
#if GUI_BUFFER_FAST
	tocopy = Buffer->Size - Buffer->In;						/* Calculate number of elements we can put at the end of buffer */
	if (tocopy > count) {									/* Check for copy count */
		tocopy = count;
	}
	memcpy(&Buffer->Buffer[Buffer->In], d, tocopy);		    /* Copy content to buffer */
	i += tocopy;											/* Increase number of bytes we copied already */
	Buffer->In += tocopy;	
	count -= tocopy;
	if (count > 0) {										/* Check if anything to write */	
		memcpy(Buffer->Buffer, (void *)&d[i], count);		/* Copy content */
		Buffer->In = count;									/* Set input pointer */
	}
	if (Buffer->In >= Buffer->Size) {						/* Check input overflow */
		Buffer->In = 0;
	}
	return (i + count);										/* Return number of elements stored in memory */
#else
	while (count--) {										/* Go through all elements */
		Buffer->Buffer[Buffer->In++] = *d++;				/* Add to buffer */
		i++;												/* Increase number of written elements */
		if (Buffer->In >= Buffer->Size) {					/* Check input overflow */
			Buffer->In = 0;
		}
	}
	return i;												/* Return number of elements written */
#endif
}

uint32_t GUI_BUFFER_WriteToTop(GUI_BUFFER_t* Buffer, const void* Data, uint32_t count) {
	uint32_t i = 0;
	uint32_t free;
    uint8_t *d = (uint8_t *)Data;

	if (Buffer == NULL || count == 0) {						/* Check buffer structure */
		return 0;
	}
	if (Buffer->In >= Buffer->Size) {						/* Check input pointer */
		Buffer->In = 0;
	}
	if (Buffer->Out >= Buffer->Size) {						/* Check output pointer */
		Buffer->Out = 0;
	}
	free = GUI_BUFFER_GetFree(Buffer);							/* Get free memory */
	if (free < count) {										/* Check available memory */
		if (free == 0) {									/* If no memory, stop execution */
			return 0;
		}
		count = free;										/* Set values for write */
	}
	d += count - 1;										    /* Start on bottom */
	while (count--) {										/* Go through all elements */
		if (Buffer->Out == 0) {								/* Check output pointer */
			Buffer->Out = Buffer->Size - 1;
		} else {
			Buffer->Out--;
		}
		Buffer->Buffer[Buffer->Out] = *d--;				    /* Add to buffer */
		i++;												/* Increase pointers */
	}
	return i;												/* Return number of elements written */
}

uint32_t GUI_BUFFER_Read(GUI_BUFFER_t* Buffer, void* Data, uint32_t count) {
	uint32_t i = 0, full;
    uint8_t *d = (uint8_t *)Data;
#if GUI_BUFFER_FAST
	uint32_t tocopy;
#endif
	
	if (Buffer == NULL || count == 0) {						/* Check buffer structure */
		return 0;
	}
	if (Buffer->Out >= Buffer->Size) {						/* Check output pointer */
		Buffer->Out = 0;
	}
	full = GUI_BUFFER_GetFull(Buffer);							/* Get free memory */
	if (full < count) {										/* Check available memory */
		if (full == 0) {									/* If no memory, stop execution */
			return 0;
		}
		count = full;										/* Set values for write */
	}
#if GUI_BUFFER_FAST
	tocopy = Buffer->Size - Buffer->Out;					/* Calculate number of elements we can read from end of buffer */
	if (tocopy > count) {									/* Check for copy count */
		tocopy = count;
	}
	memcpy(d, &Buffer->Buffer[Buffer->Out], tocopy);		/* Copy content from buffer */
	i += tocopy;											/* Increase number of bytes we copied already */
	Buffer->Out += tocopy;
	count -= tocopy;
	if (count > 0) {										/* Check if anything to read */
		memcpy(&d[i], Buffer->Buffer, count);			    /* Copy content */
		Buffer->Out = count;								/* Set input pointer */
	}
	if (Buffer->Out >= Buffer->Size) {						/* Check output overflow */
		Buffer->Out = 0;
	}
	return (i + count);										/* Return number of elements stored in memory */
#else
	while (count--) {										/* Go through all elements */
		*d++ = Buffer->Buffer[Buffer->Out++];			    /* Read from buffer */
		i++;												/* Increase pointers */
		if (Buffer->Out >= Buffer->Size) {					/* Check output overflow */
			Buffer->Out = 0;
		}
	}
	return i;												/* Return number of elements stored in memory */
#endif
}

uint32_t GUI_BUFFER_GetFree(GUI_BUFFER_t* Buffer) {
	uint32_t size = 0, in, out;
	
	if (Buffer == NULL) {									/* Check buffer structure */
		return 0;
	}
	in = Buffer->In;										/* Save values */
	out = Buffer->Out;
	if (in == out) {										/* Check if the same */
		size = Buffer->Size;
	} else if (out > in) {									/* Check normal mode */
		size = out - in;
	} else {												/* Check if overflow mode */
		size = Buffer->Size - (in - out);
	}
	return size - 1;										/* Return free memory */
}

uint32_t GUI_BUFFER_GetFull(GUI_BUFFER_t* Buffer) {
	uint32_t in, out, size;
	
	if (Buffer == NULL) {									/* Check buffer structure */
		return 0;
	}
	in = Buffer->In;										/* Save values */
	out = Buffer->Out;
	if (in == out) {										/* Pointer are same? */
		size = 0;
	} else if (in > out) {									/* Buffer is not in overflow mode */
		size = in - out;
	} else {												/* Buffer is in overflow mode */
		size = Buffer->Size - (out - in);
	}
	return size;											/* Return number of elements in buffer */
}

uint32_t GUI_BUFFER_GetFullFast(GUI_BUFFER_t* Buffer) {
	uint32_t in, out;
	
	if (Buffer == NULL) {									/* Check buffer structure */
		return 0;
	}
	in = Buffer->In;										/* Save values */
	out = Buffer->Out;
	return (Buffer->Size + in - out) % Buffer->Size;
}

void GUI_BUFFER_Reset(GUI_BUFFER_t* Buffer) {
	if (Buffer == NULL) {									/* Check buffer structure */
		return;
	}
	Buffer->In = 0;											/* Reset values */
	Buffer->Out = 0;
}

int32_t GUI_BUFFER_FindElement(GUI_BUFFER_t* Buffer, uint8_t Element) {
	uint32_t Num, Out, retval = 0;
	
	if (Buffer == NULL) {									/* Check buffer structure */
		return -1;
	}
	
	Num = GUI_BUFFER_GetFull(Buffer);							/* Create temporary variables */
	Out = Buffer->Out;
	while (Num > 0) {										/* Go through input elements */
		if (Out >= Buffer->Size) {							/* Check output overflow */
			Out = 0;
		}
		if ((uint8_t)Buffer->Buffer[Out] == (uint8_t)Element) {	/* Check for element */
			return retval;									/* Element found, return position in buffer */
		}
		Out++;												/* Set new variables */
		Num--;
		retval++;
	}
	return -1;												/* Element is not in buffer */
}

int32_t GUI_BUFFER_Find(GUI_BUFFER_t* Buffer, const void* Data, uint32_t Size) {
	uint32_t Num, Out, i, retval = 0;
	uint8_t found = 0;
    uint8_t* d = (uint8_t *)Data;

	if (Buffer == NULL || (Num = GUI_BUFFER_GetFull(Buffer)) < Size) {	/* Check buffer structure and number of elements in buffer */
		return -1;
	}
	Out = Buffer->Out;										/* Create temporary variables */
	while (Num > 0) {										/* Go through input elements in buffer */
		if (Out >= Buffer->Size) {							/* Check output overflow */
			Out = 0;
		}
		if ((uint8_t)Buffer->Buffer[Out] == d[0]) {	        /* Check if current element in buffer matches first element in data array */
			found = 1;
		}
		
		Out++;												/* Set new variables */
		Num--;
		retval++;
		if (found) {										/* We have found first element */
			i = 1;											/* First character found */
			while (i < Size && Num > 0) {					/* Check others */	
				if (Out >= Buffer->Size) {					/* Check output overflow */
					Out = 0;
				}
				if ((uint8_t)Buffer->Buffer[Out] != d[i]) {	/* Check if current character in buffer matches character in string */
					retval += i - 1;
					break;
				}
				Out++;										/* Set new variables */
				Num--;
				i++;
			}
			if (i == Size) {								/* We have found data sequence in buffer */
				return retval - 1;
			}
		}
	}
	return -1;												/* Data sequence is not in buffer */
}

uint32_t GUI_BUFFER_WriteString(GUI_BUFFER_t* Buffer, const char* buff) {
	return GUI_BUFFER_Write(Buffer, (uint8_t *)buff, strlen(buff));	/* Write string to buffer */
}

uint32_t GUI_BUFFER_ReadString(GUI_BUFFER_t* Buffer, char* buff, uint32_t buffsize) {
	uint32_t i = 0, freeMem, fullMem;
	uint8_t ch;
	if (Buffer == NULL) {
		return 0;											/* Check value buffer */
	}
	
	freeMem = GUI_BUFFER_GetFree(Buffer);					/* Get free memory */
	fullMem = GUI_BUFFER_GetFull(Buffer);				    /* Get full memory */
	if (													/* Check for any data in buffer */
		fullMem == 0 ||                                 	/* Buffer empty */
		(
			GUI_BUFFER_FindElement(Buffer, Buffer->StringDelimiter) < 0 && 	/* String delimiter is not in buffer */
			freeMem != 0 &&                                 /* Buffer is not full */
			fullMem < buffsize                              /* User buffer size is larger than number of elements in buffer */
		)
	) {
		return 0;											/* Return with no elements read */
	}
	while (i < (buffsize - 1)) {							/* If available buffer size is more than 0 characters */
		GUI_BUFFER_Read(Buffer, &ch, 1);						/* We have available data */
		buff[i] = (char)ch;									/* Save character */
		if ((char)buff[i] == (char)Buffer->StringDelimiter) {	/* Check for end of string */
			break;											/* Done */
		}
		i++;												/* Increase */
	}
	if (i == (buffsize - 1)) {								/* Add zero to the end of string */
		buff[i] = 0;
	} else {
		buff[++i] = 0;
	}
	return i;												/* Return number of characters in buffer */
}

int8_t GUI_BUFFER_CheckElement(GUI_BUFFER_t* Buffer, uint32_t pos, uint8_t* element) {
	uint32_t In, Out, i = 0;
	if (Buffer == NULL) {									/* Check value buffer */
		return 0;
	}
	
	In = Buffer->In;										/* Read current values */
	Out = Buffer->Out;
	while (i < pos && (In != Out)) {						/* Set pointers to right location */
		Out++;												/* Increase output pointer */
		i++;
		if (Out >= Buffer->Size) {							/* Check overflow */
			Out = 0;
		}
	}
	if (i == pos) {											/* If positions match */	
		*element = Buffer->Buffer[Out];						/* Save element */	
		return 1;											/* Return OK */
	}
	return 0;												/* Return zero */
}
