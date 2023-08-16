#include <windows.h>

//I usually use NULL is i don't specifically care about a varible but is required by the computer or when i have to cancel/end something
#define LOG(x) OutputDebugStringA(x)

#define INTERNAL static   //Translation unit variable
#define LOCAL_PERSIST static
#define GLOBAL_VARIABLE static

//Variable to check if our window is running
GLOBAL_VARIABLE bool Running; 

GLOBAL_VARIABLE BITMAPINFO BitmapInfo;
GLOBAL_VARIABLE void *BitmapMemoryPtr; 	 //Area of memory where we can draw with our own renderer
GLOBAL_VARIABLE HBITMAP BitmapHandle;    //Handle to bitmap
GLOBAL_VARIABLE HDC BitmapDeviceContext;


//Stride = After how many bytes do we encounter the next element
//DIB = Device independent BITMAP
INTERNAL void W64ResizeDIBSection(int width, int height){
	//TODO: Try to not free bitmap memory first but after the a new bitmap has been created

	if(BitmapHandle != NULL)
	{
		DeleteObject(BitmapHandle);
	}
	if(BitmapDeviceContext == NULL)
	{
		BitmapDeviceContext = CreateCompatibleDC(0);
	}

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader); // So that BitmapInfo struct can skip that many bytes to get to colors
	BitmapInfo.bmiHeader.biWidth = width;
	BitmapInfo.bmiHeader.biHeight = height;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32; //Bits per pixel 32 = 4bytes per pixel


	 //Area of memory where we can draw with our own renderer
	BitmapHandle = CreateDIBSection(BitmapDeviceContext, &BitmapInfo, DIB_RGB_COLORS, &BitmapMemoryPtr, 0, 0);
}

//Put rectangle from one buffer to another buffer  
//Scaling as well as updating 
INTERNAL void W64UpdateScreen(HDC DeviceContext,int X, int Y, int Width, int Height){
	StretchDIBits(DeviceContext,
				  X, Y, Width, Height,
				  X, Y, Width, Height,
				  BitmapMemoryPtr,
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
			if(mode == BLACKNESS)
				mode = WHITENESS;
			else
				mode = BLACKNESS;
			W64UpdateScreen(DeviceContext, X, Y, Width, Height);
			
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
	{
		LOG("FAILED\n");
	}

	return 0;
}

 
