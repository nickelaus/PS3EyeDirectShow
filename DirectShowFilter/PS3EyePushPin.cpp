#include <streams.h>
#include <thread>
#include <strsafe.h>
#include "ps3eye.h"
#include "PS3EyeSourceFilter.h"
#include <atomic>
#include <iostream>
#include <tchar.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <string>


#define BUFSIZE 512
BOOL		bStop = FALSE;
HANDLE		gbl_hStopEvent =
CreateEvent
(NULL, // default security attribute
	TRUE, // manual reset event
	FALSE, // initial state = not signaled
	L";StopEvent"); // not a named event object

//static DWORD StaticThreadStart(LPVOID* param)
//{
//	if (param == NULL)
//	{
//		printf("\nERROR - Pipe Server Failure:\n");
//		printf("   InstanceThread got an unexpected NULL value in lpvParam.\n");
//		printf("   InstanceThread exitting.\n");
//		return (DWORD)-1;
//	}
//	PS3EyePushPin *myObj = (PS3EyePushPin*)param;
//	return myObj->InstanceThread();
//}
//std::atomic<bool> shutdown_requested(false);
void PS3EyePushPin::thread_func()
{
	OVERLAPPED	op;
	DWORD		dwThreadId = 0;
	//HANDLE		hPipe = INVALID_HANDLE_VALUE , 
		HANDLE				hThread = NULL, handleArray[2] ;

	LPCTSTR		lpszPipename = TEXT("\\\\.\\pipe\\my_pipe");


	memset(&op, 0, sizeof(op));
	handleArray[0] = op.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	handleArray[1] = gbl_hStopEvent;
	
	//HANDLE hPipe = INVALID_HANDLE_VALUE;
	

	//for (;;)
	while (bStop == FALSE)
	{

		std::cout << "Creating an instance of a named pipe.../n";
		// Create a pipe to send data
		//HANDLE pipe = CreateNamedPipe(
		//	L"\\\\.\\pipe\\my_pipe", // name of the pipe
		//	PIPE_ACCESS_OUTBOUND, // 1-way pipe -- send only
		//	PIPE_TYPE_BYTE, // send data as a byte stream
		//	1, // only allow 1 instance of this pipe
		//	0, // no outbound buffer
		//	0, // no inbound buffer
		//	0, // use default wait time
		//	NULL // use default security attributes
		//);
		hPipe = CreateNamedPipe(
			lpszPipename, // name of the pipe
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, // 2-way pipe 
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, // send data as a byte stream
			PIPE_UNLIMITED_INSTANCES, // only allow 1 instance of this pipe
			1024 * 16, // no outbound buffer
			1024 * 16, // no inbound buffer
			NMPWAIT_USE_DEFAULT_WAIT, // use default wait time
			NULL // use default security attributes
		);

		if (hPipe == NULL || hPipe == INVALID_HANDLE_VALUE) {
			std::cout << "Failed to create outbound pipe instance.\n";
			printf("CreateNamedPipe failed, GLE=%d.\n", GetLastError());
			// look up error code here using GetLastError()
			system("pause");
			return; // return 1;
		}
		std::cout << "Waiting for a client to connect to the pipe...n/";
		// This call blocks until a client process connects to the pipe
		BOOL result = ConnectNamedPipe(hPipe, &op);

		//std::thread threadObj;
		//if (result)
		//{

		DWORD dwRet = WaitForMultipleObjects(2, handleArray, FALSE, INFINITE);
		//switch (dwRet)
		if (dwRet == WAIT_OBJECT_0)
		{
			//case WAIT_OBJECT_0:
			std::cout << "Client connected, creating a processing thread.\n";
			//Create a thread for this client. 
		   //hThread = CreateThread(
		   //	NULL,              // no security attribute 
		   //	0,                 // default stack size 
		   //	InstanceThread,    // thread proc
		   //	(LPVOID)hPipe,    // thread parameter 
		   //	0,                 // not suspended 
		   //	&dwThreadId);      // returns thread ID 
		   //threadObj = std::thread(&PS3EyePushPin::InstanceThread, std::ref(*hPipe));
			//std::thread threadObj([&](PS3EyePushPin* pin) { pin->InstanceThread((LPVOID)hPipe); }, this);
			//threadObj.detach();
			InstanceThread((LPVOID)hPipe);
			//paramAdjustThread = std::thread(&PS3EyePushPin::thread_func, this); //t starts running

			//if (hThread == INVALID_HANDLE_VALUE)
			//{
			//	_tprintf(TEXT("CreateThread failed, GLE=%d.\n"), GetLastError());
			//	return; // return -1;
			//}
			//else CloseHandle(hThread);
			ResetEvent(handleArray[0]);
			//break;
		}
		else if (dwRet == WAIT_OBJECT_0 + 1)
		{

			//case WAIT_OBJECT_0 + 1:
			CloseHandle(hPipe);
			bStop = TRUE;
		}
		else 
		{
			//default:
			printf("Wait error: %d\n", GetLastError());
			break;
		}



	}

		//}

		//else {//if (!result) {
		//	cout << "Failed to make connection on named pipe." << endl;
		//	// look up error code here using GetLastError()
		//	CloseHandle(hPipe); // close the pipe
		//	system("pause");
		//	return 1;
		//}
	
	CloseHandle(handleArray[0]);
	return; //return 0;
}
PS3EyePushPin::PS3EyePushPin(HRESULT *phr, CSource *pFilter, ps3eye::PS3EYECam::PS3EYERef device) :
	CSourceStream(NAME("PS3 Eye Source"), phr, pFilter, L"Out"),
	_device(device)
{
	LPVOID refClock;
	HRESULT r = CoCreateInstance(CLSID_SystemClock, NULL, CLSCTX_INPROC_SERVER, IID_IReferenceClock, &refClock);
	if ((SUCCEEDED(r))) {
		_refClock = (IReferenceClock *)refClock;
	}
	else {
		OutputDebugString(L"PS3EyePushPin: failed to create reference clock\n");
		_refClock = NULL;
	}
}

PS3EyePushPin::~PS3EyePushPin() {
	if(_refClock != NULL) _refClock->Release();
}

HRESULT PS3EyePushPin::CheckMediaType(const CMediaType *pMediaType)
{
	CheckPointer(pMediaType, E_POINTER);

	if (pMediaType->IsValid() && *pMediaType->Type() == MEDIATYPE_Video &&
		pMediaType->Subtype() != NULL && *pMediaType->Subtype() == MEDIASUBTYPE_RGB32) {
		if (*pMediaType->FormatType() == FORMAT_VideoInfo &&
			pMediaType->Format() != NULL && pMediaType->FormatLength() > 0) {
			VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)pMediaType->Format();
			if ((pvi->bmiHeader.biWidth == 640 && pvi->bmiHeader.biHeight == 480) ||
				(pvi->bmiHeader.biWidth == 320 && pvi->bmiHeader.biHeight == 240)) {
				if (pvi->bmiHeader.biBitCount == 32 && pvi->bmiHeader.biCompression == BI_RGB
					&& pvi->bmiHeader.biPlanes == 1) {
					int minTime = 10000000 / 75;
					int maxTime = 10000000 / 2;
					if (pvi->AvgTimePerFrame >= minTime && pvi->AvgTimePerFrame <= maxTime) {
						return S_OK;
					}
				}
			}
		}
	}

	return E_FAIL;
}

HRESULT PS3EyePushPin::GetMediaType(int iPosition, CMediaType *pMediaType) {
	if (_currentMediaType.IsValid()) {
		// SetFormat from IAMStreamConfig has been called, only allow that format
		CheckPointer(pMediaType, E_POINTER);
		if (iPosition != 0) return E_UNEXPECTED;
		*pMediaType = _currentMediaType;
		return S_OK;
	}
	else {
		return _GetMediaType(iPosition, pMediaType);
	}
}

HRESULT PS3EyePushPin::_GetMediaType(int iPosition, CMediaType *pMediaType) {
	if (iPosition < 0 || iPosition >= 6) return E_UNEXPECTED;
	CheckPointer(pMediaType, E_POINTER);

	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)pMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
	if (pvi == 0)
		return(E_OUTOFMEMORY);
	ZeroMemory(pvi, pMediaType->cbFormat);


	int fps = 10;
	if (iPosition / 3 == 0) {
		// 640x480, {30, 60, 15} fps
		pvi->bmiHeader.biWidth = 640;
		pvi->bmiHeader.biHeight = 480;
		fps = iPosition == 2 ? 15 : 75 * (iPosition+1); // testing out .. seems to work at 75hz
		//fps = iPosition == 2 ? 15 : 75 * (iPosition + 1);
	}
	else  {
		// 320x240, {30, 60, 15} fps
		pvi->bmiHeader.biWidth = 320;
		pvi->bmiHeader.biHeight = 240;

		fps = iPosition == 5 ? 15 : 30 * (iPosition-2);
	}

	pvi->AvgTimePerFrame = 10000000 / fps;

	pvi->bmiHeader.biBitCount = 32;
	pvi->bmiHeader.biCompression = BI_RGB;
	pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pvi->bmiHeader.biPlanes = 1;
	pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
	pvi->bmiHeader.biClrImportant = 0;

	SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
	SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

	pMediaType->SetType(&MEDIATYPE_Video);
	pMediaType->SetFormatType(&FORMAT_VideoInfo);
	pMediaType->SetTemporalCompression(FALSE);

	// Work out the GUID for the subtype from the header info.
	const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
	pMediaType->SetSubtype(&SubTypeGUID);
	pMediaType->SetSampleSize(pvi->bmiHeader.biSizeImage);
	
	return NOERROR;
}

HRESULT PS3EyePushPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest)
{
	HRESULT hr;
	CAutoLock cAutoLock(m_pFilter->pStateLock());

	CheckPointer(pAlloc, E_POINTER);
	CheckPointer(pRequest, E_POINTER);

	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)m_mt.Format();

	// Ensure a minimum number of buffers
	if (pRequest->cBuffers == 0)
	{
		pRequest->cBuffers = 2;
	}
	pRequest->cbBuffer = pvi->bmiHeader.biSizeImage;

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pRequest, &Actual);
	if (FAILED(hr))
	{
		return hr;
	}

	// Is this allocator unsuitable?
	if (Actual.cbBuffer < pRequest->cbBuffer)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT PS3EyePushPin::OnThreadStartPlay()
{
	if (_refClock != NULL) _refClock->GetTime(&_startTime);
	return S_OK;
}

HRESULT PS3EyePushPin::OnThreadCreate()
{
	HRESULT hr;  // the return code from calls
	Command com;
	std::thread paramAdjustThread;

	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)m_mt.Format();
	int fps = 10000000 / ((int)pvi->AvgTimePerFrame);
	OutputDebugString(L"initiating device\n");
	if (_device.use_count() > 0) {
		bool didInit = _device->init(pvi->bmiHeader.biWidth, pvi->bmiHeader.biHeight, fps, ps3eye::PS3EYECam::EOutputFormat::BGRA);
		if (didInit) {
			OutputDebugString(L"starting device\n");
			_device->setFlip(false, true);
			_device->setAutogain(false);
			_device->setAutoWhiteBalance(false);
			//_device->setAutoWhiteBalance(true);
			_device->start();
			OutputDebugString(L"done\n");
			// create input processing thread (pipe)
			paramAdjustThread = std::thread(&PS3EyePushPin::thread_func, this); //t starts running
			std::cout << "main thread\n";
			//t.join(); // <- main thread wits for thread t to finish
			paramAdjustThread.detach();
			// Initialisation suceeded

			return S_OK;
		}
		else {
			OutputDebugString(L"failed to init device\n");
			return E_FAIL;
		}
	}
	else {
		// no device found but we'll render a blank frame so press on
		return S_OK;
	}
}

HRESULT PS3EyePushPin::OnThreadDestroy()
{
	if (_device.use_count() > 0) {
		_device->stop();
	}
	return S_OK;
}

HRESULT PS3EyePushPin::FillBuffer(IMediaSample *pSample)
{
	BYTE *pData;
	long cbData;

	CheckPointer(pSample, E_POINTER);

	pSample->GetPointer(&pData);
	cbData = pSample->GetSize();

	// Check that we're still using video
	ASSERT(m_mt.formattype == FORMAT_VideoInfo);

	if (_device.use_count() > 0) {
		_device->getFrame(pData);
	}
	else {
		// TODO: fill with error message image
		for (int i = 0; i < cbData; ++i) pData[i] = 0;
	}

	if (_refClock != NULL) {
		VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)m_mt.Format();
		REFERENCE_TIME t;
		_refClock->GetTime(&t);
		REFERENCE_TIME rtStart = t - _startTime - pvi->AvgTimePerFrame; // compensate for frame buffer in PS3EYECam
		//REFERENCE_TIME rtStart = t - _startTime; // compensate for frame buffer in PS3EYECam
		REFERENCE_TIME rtStop = rtStart + pvi->AvgTimePerFrame;

		pSample->SetTime(&rtStart, &rtStop);
		// Set TRUE on every sample for uncompressed frames
		pSample->SetSyncPoint(TRUE);
	}
	
	
	//if (count > 1200 && count < 10000)
	//{
	//	if (count % 100 == 0)
	//	{
	//		//if (_device.use_count() > 0) {
	//		//	_device->stop();
	//		//}
	//		_device->setAutogain(false);
	//		//_device->setAutoWhiteBalance(false);
	//		_device->setGain(10);
	//		_device->setExposure(0);
	//		_device->setBrightness(0);
	//		//_device->start();
	//		brightness += 20;
	//	}

	//}
	//++count;
	return S_OK;
}

HRESULT __stdcall PS3EyePushPin::Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData)
{
	if (guidPropSet == AMPROPSETID_Pin) {
		return E_PROP_ID_UNSUPPORTED;
	}
	else {
		return E_PROP_SET_UNSUPPORTED;
	}
}

HRESULT __stdcall PS3EyePushPin::Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD * pcbReturned)
{
	if (guidPropSet == AMPROPSETID_Pin) {
		if (dwPropID == AMPROPERTY_PIN_CATEGORY) {
			if (cbPropData >= sizeof(GUID)) {
				*((GUID *)pPropData) = PIN_CATEGORY_CAPTURE;
				return S_OK;
			}
			else {
				return E_POINTER;
			}
		}
		else {
			return E_PROP_ID_UNSUPPORTED;
		}
	}
	else {
		return E_PROP_SET_UNSUPPORTED;
	}
}

HRESULT __stdcall PS3EyePushPin::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD * pTypeSupport)
{
	if (guidPropSet == AMPROPSETID_Pin) {
		if (dwPropID == AMPROPERTY_PIN_CATEGORY) {
			*pTypeSupport = KSPROPERTY_SUPPORT_GET;
			return S_OK;
		}
		else {
			return E_PROP_ID_UNSUPPORTED;
		}
	}
	else {
		return E_PROP_SET_UNSUPPORTED;
	}
}

HRESULT __stdcall PS3EyePushPin::SetFormat(AM_MEDIA_TYPE * pmt)
{
	OutputDebugString(L"SETTING FORMAT\n");
	CheckPointer(pmt, E_POINTER)
	HRESULT hr = S_OK;
	CMediaType mt(*pmt, &hr);
	if (FAILED(hr)) return hr;

	if (SUCCEEDED(CheckMediaType(&mt))) {
		IPin *cpin;
		
		HRESULT hr2 = ConnectedTo(&cpin);
		if (SUCCEEDED(hr2)) {
			if (SUCCEEDED(cpin->QueryAccept(pmt))) {
				{
					CAutoLock cAutoLock(m_pFilter->pStateLock());
					CMediaType lastMT = _currentMediaType;
					_currentMediaType = mt;
				}
				
				hr = m_pFilter->GetFilterGraph()->Reconnect(cpin);
			}
			else {
				hr = E_FAIL;
			}
		}
		else {
			CAutoLock cAutoLock(m_pFilter->pStateLock());
			_currentMediaType = mt;
			hr = S_OK;
		}
		if (cpin != NULL) {
			cpin->Release();
		}
	}
	else {
		hr = E_FAIL;
	}
	return hr;
}

HRESULT __stdcall PS3EyePushPin::GetFormat(AM_MEDIA_TYPE ** ppmt)
{
	CheckPointer(ppmt, E_POINTER);
	*ppmt = NULL;
	if (!_currentMediaType.IsValid()) {
		CMediaType dmt;
		_GetMediaType(0, &dmt);
		*ppmt = CreateMediaType(&dmt);
	}
	else {
		*ppmt = CreateMediaType(&_currentMediaType);
	}
	if (*ppmt == NULL) {
		return E_FAIL;
	}
	else {
		return S_OK;
	}
}

HRESULT __stdcall PS3EyePushPin::GetNumberOfCapabilities(int * piCount, int * piSize)
{
	CheckPointer(piCount, E_POINTER);
	CheckPointer(piSize, E_POINTER);
	*piCount = 6;
	*piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
	return S_OK;
}

HRESULT __stdcall PS3EyePushPin::GetStreamCaps(int iIndex, AM_MEDIA_TYPE ** ppmt, BYTE * pSCC)
{
	CheckPointer(ppmt, E_POINTER);
	CheckPointer(pSCC, E_POINTER);
	CMediaType mt;
	HRESULT hr = _GetMediaType(iIndex, &mt);
	if (SUCCEEDED(hr)) {
		*ppmt = CreateMediaType(&mt);
		VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)mt.Format();

		const SIZE inputSize = { pvi->bmiHeader.biWidth, pvi->bmiHeader.biHeight };
		VIDEO_STREAM_CONFIG_CAPS *cc = (VIDEO_STREAM_CONFIG_CAPS *)pSCC;
		cc->guid = MEDIATYPE_Video;
		cc->VideoStandard = 0;
		cc->InputSize = inputSize;
		cc->MinCroppingSize = inputSize;
		cc->MaxCroppingSize = inputSize;
		cc->CropGranularityX = 4;
		cc->CropGranularityY = 4;
		cc->CropAlignX = 4;
		cc->CropAlignY = 4;
		cc->MinOutputSize = inputSize;
		cc->MaxOutputSize = inputSize;
		cc->OutputGranularityX = 4;
		cc->OutputGranularityY = 4;
		cc->StretchTapsX = 0;
		cc->StretchTapsY = 0;
		cc->ShrinkTapsX = 0;
		cc->ShrinkTapsY = 0;
		cc->MinFrameInterval = 10000000 / 60;
		cc->MaxFrameInterval = 10000000 / 2;
		cc->MinBitsPerSecond = 0;
		cc->MaxBitsPerSecond = (LONG)1000000000000;
	}
	return hr;
}

// following functions for pipe processing
//DWORD WINAPI PS3EyePushPin::InstanceThread(LPVOID lpvParam)
VOID WINAPI PS3EyePushPin::InstanceThread(LPVOID lpvParam)
// This routine is a thread processing function to read from and reply to a client
// via the open pipe connection passed from the main loop. Note this allows
// the main loop to continue executing, potentially creating more threads of
// of this procedure to run concurrently, depending on the number of incoming
// client connections.
{
	HANDLE hHeap = GetProcessHeap();
	TCHAR* pchRequest = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));
	TCHAR* pchReply = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));
	//TCHAR*   ReadBuffer[BUFSIZE];
	//TCHAR*   ReplyBuffer[BUFSIZE];

	DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;
	BOOL fSuccess = FALSE;
	HANDLE hPipe = NULL;
	//hPipe = NULL;

	// Do some extra error checking since the app will keep running even if this
	// thread fails.

	if (lpvParam == NULL)
	{
		printf("\nERROR - Pipe Server Failure:\n");
		printf("   InstanceThread got an unexpected NULL value in lpvParam.\n");
		printf("   InstanceThread exitting.\n");
		if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
		if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
		//return (DWORD)-1;
		return;
	}

	if (pchRequest == NULL)
	{
		printf("\nERROR - Pipe Server Failure:\n");
		printf("   InstanceThread got an unexpected NULL heap allocation.\n");
		printf("   InstanceThread exitting.\n");
		if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
		//return (DWORD)-1;
		return;
	}

	if (pchReply == NULL)
	{
		printf("\nERROR - Pipe Server Failure:\n");
		printf("   InstanceThread got an unexpected NULL heap allocation.\n");
		printf("   InstanceThread exitting.\n");
		if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
		//return (DWORD)-1;
		return;
	}
	// This should be for debugging only.
	printf("InstanceThread created, receiving and processing messages.\n");

	// The thread's parameter is a handle to a pipe object instance. 
	hPipe = (HANDLE)lpvParam;
	//local_hPipe = this->hPipe;//(HANDLE)lpvParam;

	while (1)
	{
		//wchar_t buffer[128];
		DWORD numBytesRead = 0;
		fSuccess = ReadFile(
			hPipe,
			pchRequest,// the data from the pipe will be put here
			BUFSIZE * sizeof(TCHAR), // size of buffer 
			//127 * sizeof(wchar_t), // number of bytes allocated
			&cbBytesRead, // number of bytes read 
			//&numBytesRead, // this will store number of bytes actually read
			NULL // not using overlapped IO
		);

		if (!fSuccess || cbBytesRead == 0) {

			if (GetLastError() == ERROR_BROKEN_PIPE)
			{
				_tprintf(TEXT("InstanceThread: client disconnected.\n"));
			}
			else
			{
				_tprintf(TEXT("InstanceThread ReadFile failed, GLE=%d.\n"), GetLastError());
			}
			break;
		}

		//buffer[numBytesRead / sizeof(wchar_t)] = '\0'; // null terminate the string
		pchRequest[cbBytesRead / sizeof(wchar_t)] = '\0'; // null terminate the string
		//ReadBuffer[cbBytesRead / sizeof(wchar_t)] = '\0'; // null terminate the string
		std::cout << "Number of bytes read: " << cbBytesRead << std::endl;
		std::wcout << "Message: " << pchRequest << std::endl;
		//std::wcout << "Message: " << ReadBuffer << std::endl;

		// Process the incoming message.
		GetAnswerToRequest(pchRequest, pchReply, &cbReplyBytes);
		//GetAnswerToRequest(ReadBuffer, ReplyBuffer, &cbReplyBytes);

		// Write the reply to the pipe. 
		fSuccess = WriteFile(
			hPipe,        // handle to pipe 
			pchReply,//ReplyBuffer, // buffer to write from 
			cbReplyBytes, // number of bytes to write 
			&cbWritten,   // number of bytes written 
			NULL);        // not overlapped I/O 

		if (!fSuccess || cbReplyBytes != cbWritten)
		{
			_tprintf(TEXT("InstanceThread WriteFile failed, GLE=%d.\n"), GetLastError());
			break;
		}
		
	}

	// Flush the pipe to allow the client to read the pipe's contents 
// before disconnecting. Then disconnect the pipe, and close the 
// handle to this pipe instance. 

	if (hPipe) {
		WaitForSingleObject(hPipe, INFINITE);
		FlushFileBuffers(hPipe);
		DisconnectNamedPipe(hPipe);
		CloseHandleSafe(hPipe);
		//CloseHandle(hPipe);
	}



	HeapFree(hHeap, 0, pchRequest);
	HeapFree(hHeap, 0, pchReply);
	printf("InstanceThread exiting.\n");
	//return 1;
	return;
}


VOID PS3EyePushPin::GetAnswerToRequest(LPTSTR pchRequest,
	LPTSTR pchReply,
	LPDWORD pchBytes)
	// This routine is a simple function to print the client request to the console
	// and populate the reply buffer with a default data string. This is where you
	// would put the actual client request processing code that runs in the context
	// of an instance thread. Keep in mind the main thread will continue to wait for
	// and receive other client connections while the instance thread is working.
	
{// update exposure, gain, brightness (doesn't seem to do anything), contrast, etc //
	double exposure = 0;
	double sharpness = 0; 
	double gain = 0; // gain, brightness, contrast, hue, blue balance, red balance; //
	double contrast = 0; // 
	double autoGain= 0; // 
	double autoWhiteBalance = 0; // 
	_tprintf(TEXT("Client Request String:\"%s\"\n"), pchRequest);
	
	std::wstring  answerFromServer = L"response from server:\n";// (pchRequest); // convert into string stream

	

	//wstring stringA = "foo";
	std::vector<std::wstring> paramValueString =
		tokenizeWstring(pchRequest, L";");
	for (auto i : paramValueString)
	{
		std::tuple<std::wstring, double> paramAndVal = getParamAndValue(i);
		if (compareWstrings(std::get<0>(paramAndVal), L"exposure"))
		{
			_tprintf(TEXT("match\n"));
			answerFromServer += L"match for exposure";
			exposure = std::get<1>(paramAndVal);
			_device->setExposure((int)(exposure));
		}
		if (compareWstrings(std::get<0>(paramAndVal), L"sharpness"))
		{
			_tprintf(TEXT("match\n"));
			answerFromServer += L"match for sharpness";
			sharpness = std::get<1>(paramAndVal);
			_device->setSharpness((int)(sharpness));
		}
		if (compareWstrings(std::get<0>(paramAndVal), L"gain"))
		{
			_tprintf(TEXT("match\n"));
			answerFromServer += L"match for gain";
			gain = std::get<1>(paramAndVal);
			_device->setGain((int)(gain));
		}
		if (compareWstrings(std::get<0>(paramAndVal), L"contrast"))
		{
			_tprintf(TEXT("match\n"));
			answerFromServer += L"match for contrast";
			contrast = std::get<1>(paramAndVal);
			_device->setContrast((int)(contrast));
		}

		//	setAutogain(autogain);
		if (compareWstrings(std::get<0>(paramAndVal), L"autogain"))
		{
			_tprintf(TEXT("match\n"));
			answerFromServer += L"match for autoGain";
			autoGain = std::get<1>(paramAndVal);
			_device->setAutogain((bool)(autoGain));
		}

		//setAutoWhiteBalance(awb);
		if (compareWstrings(std::get<0>(paramAndVal), L"awb"))
		{
			_tprintf(TEXT("match\n"));
			answerFromServer += L"match for autoWhiteBalance";
			autoWhiteBalance = std::get<1>(paramAndVal);
			_device->setAutoWhiteBalance((bool)(autoWhiteBalance));
		}
	}
	answerFromServer += L"new values:\n";// (pchRequest); // convert into string stream
//int exps = _device->getExposure();
	answerFromServer += L"Exposure: " + std::to_wstring(_device->getExposure()) + L"\n";
	answerFromServer += L"Gain: " + std::to_wstring(_device->getGain()) + L"\n";
	answerFromServer += L"Sharpness: " + std::to_wstring(_device->getSharpness()) + L"\n";
	answerFromServer += L"Brightness: " + std::to_wstring(_device->getBrightness()) + L"\n";
	answerFromServer += L"AutoGain: " + std::to_wstring(_device->getAutogain()) + L"\n";
	answerFromServer += L"AutoWhiteBalance: " + std::to_wstring(_device->getAutoWhiteBalance()) + L"\n";
	answerFromServer += L"Contrast: " + std::to_wstring(_device->getContrast()) + L"\n";
	//wstring stringB = L"exposure";
	//if (compareWstrings(pchRequest, stringB))
	//{
	//	_tprintf(TEXT("match\n"));
	//	// strings match
	//}
	//else
	//{
	//	_tprintf(TEXT("different\n"));
	//}

	// Check the outgoing message to make sure it's not too long for the buffer.
	//if (FAILED(StringCchCopy(pchReply, BUFSIZE, TEXT("default answer from server"))))
	if (FAILED(StringCchCopy(pchReply, BUFSIZE, answerFromServer.c_str())))
	{
		*pchBytes = 0;
		pchReply[0] = 0;
		printf("StringCchCopy failed, no outgoing message.\n");
		return;
	}
	*pchBytes = (lstrlen(pchReply) + 1) * sizeof(TCHAR);
}
std::vector <std::wstring> PS3EyePushPin::tokenizeWstring(std::wstring stringA, std::wstring delimeter)
{
	std::vector<std::wstring> tokens;
	std::wstring temp;
	std::wstringstream wss(stringA);
	const wchar_t* delim = delimeter.c_str();
	while (std::getline(wss, temp, *delim))
		tokens.push_back(temp);

	return (tokens);
}
std::tuple <std::wstring, double>  PS3EyePushPin::getParamAndValue(std::wstring stringA)
{
	std::vector <std::wstring> paramValueWstring = tokenizeWstring(stringA, L":");
	std::wstring param = L"";
	double value = 0;
	if (paramValueWstring.size() > 0)
	{
		param = paramValueWstring.at(0);
		std::wstringstream wss(paramValueWstring.at(1).c_str());
		double doubleValue;
		wss >> doubleValue;

		if (wss.eof() && !wss.fail())
			std::cout << "yay, success: " << doubleValue << std::endl;
		else
			std::cout << "that was not a double." << std::endl;
		value = doubleValue;
	}
	std::tuple<std::wstring, double> paramAndValue(param, value);


	return (paramAndValue);
}
bool PS3EyePushPin::compareWstrings(std::wstring stringA, std::wstring stringB)
{
	std::transform(stringA.begin(), stringA.end(), stringA.begin(), towupper);
	std::transform(stringB.begin(), stringB.end(), stringB.begin(), towupper);

	return (stringA == stringB);
}
