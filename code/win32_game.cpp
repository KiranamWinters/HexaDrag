/*
_________________________________________________
Author : Kiranam Dewangan						 
Date Started : 15 August, 2023
Status : Ongoing
_________________________________________________
*/

/*
NOTES TO SELF:
---> We are using Hungarian Notation which means data type is appended to variable names.
---> NULL = We don't care what happens to it.
---> Windows is LITTLE ENDIAN or so i've been told
*/

//WParam --> Used to be a 16 bit Word but now is 32 byte int 
//LParam --> Used to be a 32 bit Double_Word but now it is 64 bit long

#include <windows.h>
#include <stdint.h>
#include <math.h>

//I usually use NULL when i don't specifically care about a varible, but the required is by the computer or when i have to cancel/end something
#define LOG(x) OutputDebugStringA(x)

#define INTERNAL static   			//Translation unit variable
#define LOCAL_PERSIST static		//Persist a Local varible
#define GLOBAL_VARIABLE static		//Global Variables

//Different types of integers based on size
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;


struct W64_offscreen_pixel_buffer{
	BITMAPINFO info;	 //Contains Information about Bitmap	
	void *memory; 	 //Ptr to Area of memory where we can draw with our own renderer
	int width;		 //Width of Bitmap
	int height;		 //Height of Bitmap
	int BytesPerPixel;  
	int pitch;
};  

struct WindowsDimension
{
	int width;
	int height;
};

//Variable to check if our window is running
GLOBAL_VARIABLE bool Running;

GLOBAL_VARIABLE W64_offscreen_pixel_buffer G_BACKBUFFER;

WindowsDimension GetWindowDimension(HWND winHandle){
	RECT screen;
	GetClientRect(winHandle, &screen);
	int width = screen.right - screen.left;
	int height = screen.bottom - screen.top;
	WindowsDimension result = {width, height};
	return result;
}


/*
--- Stride: After how many bytes do we encounter the next element
--- DIB: Device independent BITMAP
--- Function Job : To resize the Bitmap 
*/


void RenderResetingGradient(W64_offscreen_pixel_buffer Buffer, int XOffset, int YOffset){
	uint8 *Row = (uint8*)(Buffer.memory); //This basically makes it so that we read 8bits of memory | Offset = 8 ---------> PointerArithmetic

	//Loop iterating through all pixels in display buffer
	for(int Y = 0; Y < Buffer.height; Y++)
	{
		uint32 *Pixel = (uint32*) Row;      //makes it so each increment is of [4 BYTES]
		for(int X = 0; X < Buffer.width; X++)
		{
			//One byte is used for color in memory
			//This is how is setup in Memory
			//This is set up as Little Endian Which means the values are reverse BGR
			//-----------------------------------------------------------------------
			//			  PADDING		BLUE		GREEN		  RED
			//Offset:   (Pixel + 0)  (Pixel + 1)  (Pixel + 2)  (Pixel + 3)
			//Hex: 			00 			 00 		  00 		   00
			//-----------------------------------------------------------------------
			//Little Endian:  0xBBGGRRXX --------> 32 bit Address in hexadecimal
			//Big Endian:     0xXXRRGGBB --------> 32 bit Address in hexadecimal

			//(Green << 8) = 0x0000GG00
			//((Green << 8) | Blue) = 0x0000GGBB
			//Note OR and + have same effects on bits
			uint8 green = (uint8)((X ^ Y)+ 3 + XOffset );
			//uint8 blue = (uint8)(Y + YOffset);
			*Pixel++ = ((green << 8) ); //blue);

		}
		Row = Row + Buffer.pitch;
	}
}
void W64Fill(W64_offscreen_pixel_buffer Buffer, uint8 RED, uint8 BLUE, uint8 GREEN){
	int width = Buffer.width;
	int height = Buffer.height;
	int pitch = width * Buffer.BytesPerPixel;
	uint8 *Row = (uint8*)(Buffer.memory); //This basically makes it so that we read 8bits of memory | Offset = 8 ---------> PointerArithmetic

	for(int Y = 0; Y < height; Y++)
	{
		uint8 *Pixel = (uint8*)Row;      //makes it so each increment is of [4 BYTES]
		for(int X = 0; X < width; X++)
		{
			
			*Pixel = (uint8)BLUE;
			Pixel++;

			*Pixel = (uint8)GREEN;
			Pixel++;

			*Pixel = (uint8)RED;
			Pixel++;

			 //Padding: Don't touch
			*Pixel = (uint8)0;
			Pixel++;

		}
		Row = Row + pitch;
	}
}

//This function sets our global variable G_BACKBUFFER
//Stride = After how many bytes do we encounter the next element
//DIB = Device independent BITMAP
INTERNAL void W64ResizeDIBSection(W64_offscreen_pixel_buffer *Buffer, int width, int height){
	//TODO: Try to not free bitmap memory first but after the a new bitmap has been created

	Buffer->width = width;
	Buffer->height = height;
	if(Buffer->memory){
		VirtualFree(Buffer->memory,
					NULL, 				  //Knows how we allocated ,frees itself
					MEM_RELEASE);
	}
	Buffer->BytesPerPixel = 4;
	Buffer->info.bmiHeader.biSize = sizeof(Buffer->info.bmiHeader); 	// So that BitmapInfo struct can skip that many bytes to get to colors
	Buffer->info.bmiHeader.biWidth = Buffer->width;
	Buffer->info.bmiHeader.biHeight = -Buffer->height; 					// negative value means that our bitmap is layed top to bottom origin at the upper left
	Buffer->info.bmiHeader.biPlanes = 1;	
	Buffer->info.bmiHeader.biBitCount = 32; 							//Bits per pixel 32 = 4bytes per pixel
	Buffer->info.bmiHeader.biCompression = BI_RGB;

	//Allocating our own memory 
	int BitmapMemorySize = (Buffer->width * Buffer->height) * Buffer->BytesPerPixel;		//Total number of pixels * Bytes per pixel

	//BitmapMemory is a void pointer
	//BitmapMemory ---> Start address to area where our pixel buffer lives
	Buffer->memory = VirtualAlloc(
				 NULL,  			//Don't care where its allocated
				 BitmapMemorySize, //Size of allocated memory
				 MEM_COMMIT, 	  //Commit mode
				 PAGE_READWRITE  //Access
	);

	Buffer->pitch = Buffer->width * Buffer->BytesPerPixel;
}

//Put rectangle from one buffer to another buffer  
//Scaling as well as updating 
INTERNAL void W64DisplayBufferToWindowBuffer(W64_offscreen_pixel_buffer Buffer,
											HDC DeviceContext,
											int WindowWidth, int WindowHeight,
											int X,
											int Y,
											int Width,
											int Height)
{
	StretchDIBits(DeviceContext,
				  0, 0, WindowWidth, WindowHeight,
				  0, 0, Buffer.width, Buffer.height,
				  Buffer.memory,
				  &Buffer.info, 
				  DIB_RGB_COLORS,
				  SRCCOPY
	);
}

//CallBack --->  Times when windows calls us | Good times??
LRESULT CALLBACK
W64MainWindowCallback( 
	HWND Window, 
	UINT Message, 
	WPARAM wParam, 
	LPARAM lParam )
{
	LRESULT result = NULL;
	switch (Message){
		case(WM_PAINT):
		{
			LOG("WM_PAINT\n");
			PAINTSTRUCT Paint;
			RECT Rectangle = {};
			// Rectangle.left = 100;
			// Rectangle.right = 1000;
			// Rectangle.bottom = 1000;
			// Rectangle.top = 100;

			HDC DeviceContext = BeginPaint(Window, &Paint);
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;
			LOCAL_PERSIST DWORD mode = BLACKNESS;
			PatBlt(DeviceContext, X, Y, Width, Height, mode);

			WindowsDimension Dimension = GetWindowDimension(Window);

			W64DisplayBufferToWindowBuffer(G_BACKBUFFER,
											DeviceContext,
											Dimension.width, Dimension.height,
											0, 
											0, 
											Width, 
											Height);

			//DrawFocusRect(DeviceContext, &Rectangle);
			EndPaint(Window, &Paint);
 
		}	break;

		case (WM_SIZE):
		{
			LOG("WM_SIZE\n");
		}	break;

		case (WM_ACTIVATEAPP):
		{	
			LOG("WM_ACTIVATEAPP\n");
		}	break;

		case (WM_CLOSE):
		{
			PostQuitMessage(0);
			LOG("WM_ClOSE\n");
			Running = FALSE;
		}	break;

		case (WM_DESTROY):
		{
			LOG("WM_DESTROY\n");
			Running = FALSE;
		}	break;

		default:
		{
			//LOG("DEFAULT\n");
			//DefWindowProc handles all the messages we don't handle
			result = DefWindowProc(Window,Message,wParam,lParam);
			
		}	break;
	}
	return result;
}



/*================================================ENTRY POINT TO PROGRAM======================================================================================*/
int CALLBACK 
WinMain(
	HINSTANCE Instance,			//Current Instance
	HINSTANCE PrevInstance,		//Previous instance if the application was opened before
	LPSTR CommandLine,			//Command line interaction
	int ShowCommand				//Size of the Window
){
	//Window Class C
	WNDCLASS windowClass = {};
	//WindowsDimension Dimension = GetWindowDimension(Window);
	W64ResizeDIBSection(&G_BACKBUFFER,1280,720);
	windowClass.lpfnWndProc = W64MainWindowCallback;
	windowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	windowClass.hInstance = Instance;
	windowClass.lpszClassName = "GameWindowClass";

	//Registration of window class
	if( RegisterClass(&windowClass) != NULL ){
		//Creation of WindowHandle
		HWND Window = CreateWindowExA(
		NULL,
		windowClass.lpszClassName, 
  		"HexaDrag",
    	WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT,
    	CW_USEDEFAULT,
    	CW_USEDEFAULT,
    	CW_USEDEFAULT,
   		NULL,
   		NULL,
   		Instance,
		NULL);

		if(Window != NULL){
			Running = TRUE;

			uint8 XOffset = 0;
			uint8 YOffset = 0;

			while(Running){
				MSG Message;

				while (PeekMessage(&Message,NULL,NULL,NULL,PM_REMOVE)){
					TranslateMessage(&Message);
					DispatchMessage(&Message);
					if (Message.message == WM_QUIT){
						Running = FALSE;
					}
				}

				RenderResetingGradient(G_BACKBUFFER,XOffset,YOffset);

				HDC DeviceContext = GetDC(Window);
				WindowsDimension Dimension = GetWindowDimension(Window);
				
				W64DisplayBufferToWindowBuffer(G_BACKBUFFER,DeviceContext,
												Dimension.width,Dimension.height,
												0,0,
												Dimension.width,Dimension.height);
				ReleaseDC(Window, DeviceContext); 
				
				YOffset = YOffset + 2;
				XOffset = XOffset +  2;
			}
		}
	}
	else
		LOG("FAILED\n");

	return EXIT_SUCCESS;
}

 
