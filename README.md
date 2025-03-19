** nickelaus note - original branch documentation follows:
# PS3EyeDirectShow
Windows DirectShow source filter for the PS3 Eye Camera via WinUSB (32 / 64 bit)

This package is an extension of [https://github.com/inspirit/PS3EYEDriver/](https://github.com/inspirit/PS3EYEDriver/), which is itself a port of the PS3 Eye Linux driver to Windows. This project wraps that code into a DirectShow source filter so that it behaves like a USB camera with a native Windows driver. Any application that goes through the standard DirectShow API to connect to a webcam should (in theory) be able to use this driver. This code has only had limited testing so there may be compatibility issues with some apps.

## User Space WinUSB Based Driver
This project makes use of the generic WinUSB driver to communicate with the camera. This has several advantages and disadvantages compared to a native camera driver. The biggest advantage is all of the code runs in user space instead of kernel space. This means any flaws in this driver will only affect the program that's using the camera instead of crashing the entire operating system. The part that runs in kernel space is the WinUSB driver and it's a stable component provided by Microsoft. It provides a generic way for user space programs to communicate with the device.

Another big advantage of a user space driver is it's much easier to directly make use of other libraries such as libusb and the github project that this driver is based on.

On the other hand native camera drivers have slightly better performance. Also Windows will automatically make those cameras available via the DirectShow API (both 32 and 64 bit) as well as Windows Media Foundation. There isn't any need to provide a custom DirectShow or WMF source for a native camera.

## 32 / 64 bit
This project provides both a 32 bit and 64 bit source filter. Every application that uses a DirectShow camera must load its source filter library (usually a wrapper to a native camera provided by Microsoft). 32 bit applications require 32 bit libraries, and 64 bit applications require 64 bit libraries. Thus if the source filter is only provided as a 32 bit library the camera can only be used by 32 bit applications.

There's another PS3 Eye driver by [Code Laboratories](https://codelaboratories.com/) that seems to take a similar approach to this driver in that it provides a custom DirectShow source filter instead of using the default wrapper. That driver is more mature and well tested than this one, but it only provides a 32 bit source filter and won't work with 64 bit apps.

Some examples of 64 bit DirectShow applications:
- [ViewTracker](https://store.steampowered.com/app/929270/ViewTracker/)
- [OBS Studio](https://obsproject.com/)
- [VLC media player](https://www.videolan.org/vlc/)

## Installing
(** Installation instructions for the original branch below ( details for this fork will follow soon)): 
The easiest way to get started with this driver is to head to the [releases section](https://github.com/jkevin/PS3EyeDirectShow/releases) and download the installer. It will install the DirectShow filters as well as a generic WinUSB driver. If it detects that the Code Laboratories driver is already installed it will only install a 64 bit DirectShow filter. The driver component provided by Code Laboratories is WinUSB compatible so there isn't any need to install another one.

## Compiling From Source

Requirements:
- Visual Studio 2017
- Windows 10 SDK
- WIX

First clone the repo and init the submodules:
```
$ git clone https://github.com/jkevin/PS3EyeDirectShow.git
$ cd PS3EyeDirectShow
$ git submodule init
$ git submodule update
```

Open `libusb/msvc/libusb_2017.sln` in Visual Studio 2017. If you don't have the Windows 8.1 SDK, change the SDK target to Windows 10 in all of the libusb project properties. Build the solution with all desired configurations. Next open PS3EyeDriverMSVC.sln and build the desired configs again.

If you would like to make the compiled DirectShow filters available to other applications, run `c:\windows\system32\regsvr32.exe <path to filter dll>` from an administrator command prompt. If you have run the installer make sure to uninstall the package before doing this. Use `c:\windows\system32\regsvr32.exe /u <path to filter dll>` to unregister the filter.

If the DLL was built with a debug configuration it's possible to use WinDBG to set a breakpoint in the filter code which will be hit when using the camera with any DirectShow application.

To build the installer, first build the 32 and 64 bit release configurations of the solution. Then right click on the PS3EyeInstaller project in the solution explorer and click build.

## TODO
- Manual exposure and white balance controls
- Windows Media Foundation source?

## nickelaus fork differences
 - Camera adjustements (gain, contrast, sharpness, auto white balance) can be made by named pipe.
   Example code for setting contrast to max (255) shown below:

  		int contrast = 255;
		parameterNameValue += L"contrast: ";
		parameterNameValue += std::to_wstring(contrast);
		const wchar_t* data = parameterNameValue.c_str();

		std::cout << "Connecting to pipe..." << std::endl;
		//calibrate camera params
		DWORD cbRead;
		TCHAR  chBuf[BUFSIZE];

		LPCTSTR lpszPipename = TEXT("\\\\.\\pipe\\my_pipe");
		while (true)
		{
			pipe = CreateFile(
				lpszPipename,
				GENERIC_READ | GENERIC_WRITE,
				0,
				NULL,
				OPEN_EXISTING,
				0,
				NULL
			);
			// break if  the pipe handle is valid
			if (pipe != INVALID_HANDLE_VALUE) {
				break;
			}
			// Exit if an error other than ERROR_PIPE_BUSY occurs. 

			if (GetLastError() != ERROR_PIPE_BUSY)
			{
				printf(("Could not open pipe. GLE=%d\n"), GetLastError());
				return -1;
			}

			// All pipe instances are busy, so wait for 20 seconds. 

			if (!WaitNamedPipe(lpszPipename, 20000))
			{
				printf("Could not open pipe: 20 second wait timed out.");
				return -1;
			}

		}
		std::cout << "outside while loop" << std::endl;
		DWORD numBytesWritten = 0;

		BOOL result = WriteFile(
			pipe, // handle to our outbound pipe
			data, // data to send
			wcslen(data) * sizeof(wchar_t), // length of data to send (bytes)
			&numBytesWritten, // will store actual amount of data sent
			NULL // not using overlapped IO
		);

		if (result) {
			std::wcout << "Number of bytes sent: " << numBytesWritten << std::endl;
		}
		else {
			std::wcout << "Failed to send data." << std::endl;
			printf("WriteFile to pipe failed. GLE=%d\n", GetLastError());
			// look up error code here using GetLastError()
		}

		std::wcout << "Reading data from pipe..." << std::endl;
		// The read operation will block until there is data to read
		wchar_t buffer[128];
		DWORD numBytesRead = 0;
		result;
		do
		{
			result = ReadFile(
				pipe,
				chBuf, // the data from the pipe will be put here
				//127 * sizeof(wchar_t), // number of bytes allocated
				BUFSIZE * sizeof(TCHAR),  // size of buffer 
				&cbRead, // this will store number of bytes actually read
				NULL // not using overlapped IO
			);
			if (!result && GetLastError() != ERROR_MORE_DATA) {
				break;
			}
			_tprintf(TEXT("\"%s\"\n"), chBuf);
		} while (!result); //repeat loop if ERROR_MORE_DATA 
		if (!result) //|| numBytesRead == 0) {
		{
			//printf("SetNamedPipeHandleState failed. GLE=%d\n", GetLastError());
			_tprintf(TEXT("ReadFile from pipe failed. GLE=%d\n"), GetLastError());
			//CloseHandle(pipe);
			return -1;
		}
		printf("\n<End of message/n");// , press ENTER to terminate connection and exit > / n");
		//system("pause");
		// Close our pipe handle
		
		CloseHandle(pipe);
		pipeClosed = true;
		std::wcout << "Done." << std::endl;
