#include <windows.h>
#include <stdint.h>
//EndTime = 40:30/ 1:34:02
//Handmade Hero 004
//I usually use NULL is i don't specifically care about a varible but is required by the computer or when i have to cancel/end something
#define LOG(x) OutputDebugStringA(x)

#define INTERNAL static   //Translation unit variable
#define LOCAL_PERSIST static
#define GLOBAL_VARIABLE static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;



//Variable to check if our window is running
GLOBAL_VARIABLE bool Running; 

GLOBAL_VARIABLE BITMAPINFO BitmapInfo;
GLOBAL_VARIABLE void *BitmapMemory; 	 //Ptr to Area of memory where we can draw with our own renderer
GLOBAL_VARIABLE int BitmapWidth;
GLOBAL_VARIABLE int BitmapHeight;


//Stride = After how many bytes do we encounter the next element
//DIB = Device independent BITMAP
INTERNAL void W64ResizeDIBSection(int width, int height){
	//TODO: Try to not free bitmap memory first but after the a new bitmap has been created

	if(BitmapMemory){
		VirtualFree(BitmapMemory,NULL, //Knows how we allocated frees itself
					MEM_RELEASE);
	}

	BitmapWidth = width;
	BitmapHeight = height;

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader); // So that BitmapInfo struct can skip that many bytes to get to colors
	BitmapInfo.bmiHeader.biWidth = BitmapWidth;
	BitmapInfo.bmiHeader.biHeight = -BitmapHeight; // negative value means that our bitmap is layed top to bottom origin at the upper left
	BitmapInfo.bmiHeader.biPlanes = 1;	
	BitmapInfo.bmiHeader.biBitCount = 32; //Bits per pixel 32 = 4bytes per pixel
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	//Allocating our own memory
	int BytesPerPixel = 4;
	int BitmapMemorySize = (width * height) * BytesPerPixel;	//Total number of pixels * Bytes per pixel 
	BitmapMemory = VirtualAlloc(
				 NULL,  //Don't care where its allocated
				 BitmapMemorySize, //Size of allocated memory
				 MEM_COMMIT, 
				 PAGE_READWRITE //Access
				);
	int Pitch = width * BytesPerPixel;
	uint8 *Row = (uint8*)(BitmapMemory); //This basically makes it so that we read 8bits of memory | Offset = 8 ---------> PointerArithmetic
	for(int Y = 0; Y < BitmapHeight; Y++)
	{
		uint8 *Pixel = (uint8*) Row;      //makes it so each increment is of [4 BYTES]
		for(int X = 0; X < BitmapWidth; X++)
		{
			//One byte is used for color in memory
			//This is how is setup in Memory
			//This is set up as Little Endian Which means the values are reverse BGR
			//-----------------------------------------------------------------------
			//			  PADDING		BLUE		GREEN		  RED
			//Offset:   (Pixel + 0)  (Pixel + 1)  (Pixel + 2)  (Pixel + 3)
			//Hex: 			00 			 00 		  00 		   00
			//-----------------------------------------------------------------------
			//LittleEndian:  0xXXBBGGRR --------> 32 bit Address in hexadecimal

			*Pixel = (uint8)X*X + Y*Y;
			Pixel++;

			*Pixel = (uint8)Y;
			Pixel++;

			*Pixel = (uint8)X*X;
			Pixel++;

			*Pixel = (uint8)0;
			Pixel++;

		}
		Row = Row + Pitch;
	}
}

//Put rectangle from one buffer to another buffer  
//Scaling as well as updating 
INTERNAL void W64UpdateScreen(HDC DeviceContext,RECT *WindowRect, int X, int Y, int Width, int Height){
	int WindowWidth = WindowRect->right - WindowRect->left;
	int WindowHeight = WindowRect->bottom - WindowRect->top;
	StretchDIBits(DeviceContext,
				//  X, Y, Width, Height,
				//  X, Y, Width, Height,
				  0,0,BitmapWidth,BitmapHeight,
				  0,0,WindowWidth,WindowHeight,
				  BitmapMemory,
				  &BitmapInfo, 
				  DIB_RGB_COLORS,
				  SRCCOPY
	);
}

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

			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			W64UpdateScreen(DeviceContext, &ClientRect, X, Y, Width, Height);

			//DrawFocusRect(DeviceContext, &Rectangle);
			EndPaint(Window, &Paint);
 
		}	break;

		case (WM_SIZE):
		{
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			int Width = ClientRect.right - ClientRect.left;
			int Height = ClientRect.bottom - ClientRect.top;
			W64ResizeDIBSection(Width, Height);
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
			result = DefWindowProc(Window,Message,wParam,lParam);
			
		}	break;
	}
	return result;
}

int CALLBACK 
WinMain(
	HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR CommandLine,
	int ShowCommand
){
	//Window Class C
	WNDCLASS windowClass = {};
	windowClass.lpfnWndProc = W64MainWindowCallback;
	windowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	windowClass.hInstance = Instance;
	windowClass.lpszClassName = "GameWindowClass";

	//Registration of window class
	if( RegisterClass(&windowClass) != NULL ){
		//Creation of WindowHandle
		HWND WinHandle = CreateWindowEx(
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

		if(WinHandle != NULL){
			Running = TRUE;
			MSG Message;
			BOOL MessageResult  =  GetMessageA(&Message,NULL,NULL,NULL);
			while(Running){
				MessageResult  =  GetMessageA(&Message,NULL,NULL,NULL);
				if (MessageResult > 0){
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}
				else {
					break;
				}
			}
		}
	}
	else
		LOG("FAILED\n");

	return 0;
}

 
