//------------------------------------------------------------------------------
// File: Source.cpp
//
// Desc: DirectShow  base classes - implements CSource, which is a Quartz
//       source filter 'template.'
//
// Copyright (c) 1992-2001 Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


// Locking Strategy.
//
// Hold the filter critical section (m_pFilter->pStateLock()) to serialise
// access to functions. Note that, in general, this lock may be held
// by a function when the worker thread may want to hold it. Therefore
// if you wish to access shared state from the worker thread you will
// need to add another critical section object. The execption is during
// the threads processing loop, when it is safe to get the filter critical
// section from within FillBuffer().

#include <streams.h>
#include <thread>
#include <iostream>
#include <atomic>
#include <tchar.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <strsafe.h>
#include <string>

////////#define BUFSIZE 512
////////
////////DWORD WINAPI InstanceThread(LPVOID);
////////VOID GetAnswerToRequest(LPTSTR, LPTSTR, LPDWORD);
////////
////////std::vector <std::wstring> tokenizeWstring(std::wstring stringA, std::wstring delimeter);
////////std::tuple <std::wstring, double> getParamAndValue(std::wstring stringA);
////////bool compareWstrings(std::wstring stringA, std::wstring stringB);
////////BOOL		bStop = FALSE;
////////HANDLE		gbl_hStopEvent =
////////CreateEvent
////////(NULL, // default security attribute
////////	TRUE, // manual reset event
////////	FALSE, // initial state = not signaled
////////	L";StopEvent"); // not a named event object

//std::atomic<bool> shutdown_requested(false);
////////void thread_func()
////////{
////////	OVERLAPPED	op;
////////	DWORD		dwThreadId = 0;
////////	HANDLE		hPipe = INVALID_HANDLE_VALUE, hThread = NULL, handleArray[2];
////////
////////	LPCTSTR		lpszPipename = TEXT("\\\\.\\pipe\\my_pipe");
////////
////////
////////	memset(&op, 0, sizeof(op));
////////	handleArray[0] = op.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
////////	handleArray[1] = gbl_hStopEvent;
////////
////////
////////	//for (;;)
////////	while (bStop == FALSE)
////////	{
////////
////////		std::cout << "Creating an instance of a named pipe.../n";
////////		// Create a pipe to send data
////////		//HANDLE pipe = CreateNamedPipe(
////////		//	L"\\\\.\\pipe\\my_pipe", // name of the pipe
////////		//	PIPE_ACCESS_OUTBOUND, // 1-way pipe -- send only
////////		//	PIPE_TYPE_BYTE, // send data as a byte stream
////////		//	1, // only allow 1 instance of this pipe
////////		//	0, // no outbound buffer
////////		//	0, // no inbound buffer
////////		//	0, // use default wait time
////////		//	NULL // use default security attributes
////////		//);
////////		hPipe = CreateNamedPipe(
////////			lpszPipename, // name of the pipe
////////			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, // 2-way pipe 
////////			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, // send data as a byte stream
////////			PIPE_UNLIMITED_INSTANCES, // only allow 1 instance of this pipe
////////			1024 * 16, // no outbound buffer
////////			1024 * 16, // no inbound buffer
////////			NMPWAIT_USE_DEFAULT_WAIT, // use default wait time
////////			NULL // use default security attributes
////////		);
////////
////////		if (hPipe == NULL || hPipe == INVALID_HANDLE_VALUE) {
////////			std::cout << "Failed to create outbound pipe instance.\n";
////////			printf("CreateNamedPipe failed, GLE=%d.\n", GetLastError());
////////			// look up error code here using GetLastError()
////////			system("pause");
////////			return; // return 1;
////////		}
////////		std::cout << "Waiting for a client to connect to the pipe...n/";
////////		// This call blocks until a client process connects to the pipe
////////		BOOL result = ConnectNamedPipe(hPipe, &op);
////////
////////
////////		//if (result)
////////		//{
////////		switch (WaitForMultipleObjects(2, handleArray, FALSE, INFINITE))
////////		{
////////			case WAIT_OBJECT_0:
////////				std::cout << "Client connected, creating a processing thread.\n";
////////				// Create a thread for this client. 
////////				hThread = CreateThread(
////////					NULL,              // no security attribute 
////////					0,                 // default stack size 
////////					InstanceThread,    // thread proc
////////					(LPVOID)hPipe,    // thread parameter 
////////					0,                 // not suspended 
////////					&dwThreadId);      // returns thread ID 
////////
////////				if (hThread == INVALID_HANDLE_VALUE)
////////				{
////////					_tprintf(TEXT("CreateThread failed, GLE=%d.\n"), GetLastError());
////////					return; // return -1;
////////				}
////////				else CloseHandle(hThread);
////////				ResetEvent(handleArray[0]);
////////				break;
////////			case WAIT_OBJECT_0 + 1:
////////				CloseHandle(hPipe);
////////				bStop = TRUE;
////////
////////			default:
////////				break;
////////		}
////////
////////		//}
////////
////////		//else {//if (!result) {
////////		//	cout << "Failed to make connection on named pipe." << endl;
////////		//	// look up error code here using GetLastError()
////////		//	CloseHandle(hPipe); // close the pipe
////////		//	system("pause");
////////		//	return 1;
////////		//}
////////	}
////////	CloseHandle(handleArray[0]);
////////	return; //return 0;
////////}
//
// CSource::Constructor
//
// Initialise the pin count for the filter. The user will create the pins in
// the derived class.
CSource::CSource(__in_opt LPCTSTR pName, __inout_opt LPUNKNOWN lpunk, CLSID clsid)
	: CBaseFilter(pName, lpunk, &m_cStateLock, clsid),
	m_iPins(0),
	m_paStreams(NULL)
{
}

CSource::CSource(__in_opt LPCTSTR pName, __inout_opt LPUNKNOWN lpunk, CLSID clsid, __inout HRESULT *phr)
	: CBaseFilter(pName, lpunk, &m_cStateLock, clsid),
	m_iPins(0),
	m_paStreams(NULL)
{
	UNREFERENCED_PARAMETER(phr);
}

#ifdef UNICODE
CSource::CSource(__in_opt LPCSTR pName, __inout_opt LPUNKNOWN lpunk, CLSID clsid)
	: CBaseFilter(pName, lpunk, &m_cStateLock, clsid),
	m_iPins(0),
	m_paStreams(NULL)
{
}

CSource::CSource(__in_opt LPCSTR pName, __inout_opt LPUNKNOWN lpunk, CLSID clsid, __inout HRESULT *phr)
	: CBaseFilter(pName, lpunk, &m_cStateLock, clsid),
	m_iPins(0),
	m_paStreams(NULL)
{
	UNREFERENCED_PARAMETER(phr);
}
#endif

//
// CSource::Destructor
//
CSource::~CSource()
{
	/*  Free our pins and pin array */
	while (m_iPins != 0) {
		// deleting the pins causes them to be removed from the array...
		delete m_paStreams[m_iPins - 1];
	}

	ASSERT(m_paStreams == NULL);
}


//
//  Add a new pin
//
HRESULT CSource::AddPin(__in CSourceStream *pStream)
{
	CAutoLock lock(&m_cStateLock);

	/*  Allocate space for this pin and the old ones */
	CSourceStream **paStreams = new CSourceStream *[m_iPins + 1];
	if (paStreams == NULL) {
		return E_OUTOFMEMORY;
	}
	if (m_paStreams != NULL) {
		CopyMemory((PVOID)paStreams, (PVOID)m_paStreams,
			m_iPins * sizeof(m_paStreams[0]));
		paStreams[m_iPins] = pStream;
		delete[] m_paStreams;
	}
	m_paStreams = paStreams;
	m_paStreams[m_iPins] = pStream;
	m_iPins++;
	return S_OK;
}

//
//  Remove a pin - pStream is NOT deleted
//
HRESULT CSource::RemovePin(__in CSourceStream *pStream)
{
	int i;
	for (i = 0; i < m_iPins; i++) {
		if (m_paStreams[i] == pStream) {
			if (m_iPins == 1) {
				delete[] m_paStreams;
				m_paStreams = NULL;
			}
			else {
				/*  no need to reallocate */
				while (++i < m_iPins)
					m_paStreams[i - 1] = m_paStreams[i];
			}
			m_iPins--;
			return S_OK;
		}
	}
	return S_FALSE;
}

//
// FindPin
//
// Set *ppPin to the IPin* that has the id Id.
// or to NULL if the Id cannot be matched.
STDMETHODIMP CSource::FindPin(LPCWSTR Id, __deref_out IPin **ppPin)
{
	CheckPointer(ppPin, E_POINTER);
	ValidateReadWritePtr(ppPin, sizeof(IPin *));
	// The -1 undoes the +1 in QueryId and ensures that totally invalid
	// strings (for which WstrToInt delivers 0) give a deliver a NULL pin.
	int i = WstrToInt(Id) - 1;
	*ppPin = GetPin(i);
	if (*ppPin != NULL) {
		(*ppPin)->AddRef();
		return NOERROR;
	}
	else {
		return VFW_E_NOT_FOUND;
	}
}

//
// FindPinNumber
//
// return the number of the pin with this IPin* or -1 if none
int CSource::FindPinNumber(__in IPin *iPin) {
	int i;
	for (i = 0; i < m_iPins; ++i) {
		if ((IPin *)(m_paStreams[i]) == iPin) {
			return i;
		}
	}
	return -1;
}

//
// GetPinCount
//
// Returns the number of pins this filter has
int CSource::GetPinCount(void) {

	CAutoLock lock(&m_cStateLock);
	return m_iPins;
}


//
// GetPin
//
// Return a non-addref'd pointer to pin n
// needed by CBaseFilter
CBasePin *CSource::GetPin(int n) {

	CAutoLock lock(&m_cStateLock);

	// n must be in the range 0..m_iPins-1
	// if m_iPins>n  && n>=0 it follows that m_iPins>0
	// which is what used to be checked (i.e. checking that we have a pin)
	if ((n >= 0) && (n < m_iPins)) {

		ASSERT(m_paStreams[n]);
		return m_paStreams[n];
	}
	return NULL;
}


//


// *
// * --- CSourceStream ----
// *

//
// Set Id to point to a CoTaskMemAlloc'd
STDMETHODIMP CSourceStream::QueryId(__deref_out LPWSTR *Id) {
	CheckPointer(Id, E_POINTER);
	ValidateReadWritePtr(Id, sizeof(LPWSTR));

	// We give the pins id's which are 1,2,...
	// FindPinNumber returns -1 for an invalid pin
	int i = 1 + m_pFilter->FindPinNumber(this);
	if (i < 1) return VFW_E_NOT_FOUND;
	*Id = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR) * 12);
	if (*Id == NULL) {
		return E_OUTOFMEMORY;
	}
	IntToWstr(i, *Id);
	return NOERROR;
}



//
// CSourceStream::Constructor
//
// increments the number of pins present on the filter
CSourceStream::CSourceStream(
	__in_opt LPCTSTR pObjectName,
	__inout HRESULT *phr,
	__inout CSource *ps,
	__in_opt LPCWSTR pPinName)
	: CBaseOutputPin(pObjectName, ps, ps->pStateLock(), phr, pPinName),
	m_pFilter(ps) {

	*phr = m_pFilter->AddPin(this);
}

#ifdef UNICODE
CSourceStream::CSourceStream(
	__in_opt LPCSTR pObjectName,
	__inout HRESULT *phr,
	__inout CSource *ps,
	__in_opt LPCWSTR pPinName)
	: CBaseOutputPin(pObjectName, ps, ps->pStateLock(), phr, pPinName),
	m_pFilter(ps) {

	*phr = m_pFilter->AddPin(this);
}
#endif
//
// CSourceStream::Destructor
//
// Decrements the number of pins on this filter
CSourceStream::~CSourceStream(void) {

	m_pFilter->RemovePin(this);
}


//
// CheckMediaType
//
// Do we support this type? Provides the default support for 1 type.
HRESULT CSourceStream::CheckMediaType(const CMediaType *pMediaType) {

	CAutoLock lock(m_pFilter->pStateLock());

	CMediaType mt;
	GetMediaType(&mt);

	if (mt == *pMediaType) {
		return NOERROR;
	}

	return E_FAIL;
}


//
// GetMediaType/3
//
// By default we support only one type
// iPosition indexes are 0-n
HRESULT CSourceStream::GetMediaType(int iPosition, __inout CMediaType *pMediaType) {

	CAutoLock lock(m_pFilter->pStateLock());

	if (iPosition < 0) {
		return E_INVALIDARG;
	}
	if (iPosition > 0) {
		return VFW_S_NO_MORE_ITEMS;
	}
	return GetMediaType(pMediaType);
}


//
// Active
//
// The pin is active - start up the worker thread
HRESULT CSourceStream::Active(void) {

	CAutoLock lock(m_pFilter->pStateLock());

	HRESULT hr;

	if (m_pFilter->IsActive()) {
		return S_FALSE;	// succeeded, but did not allocate resources (they already exist...)
	}

	// do nothing if not connected - its ok not to connect to
	// all pins of a source filter
	if (!IsConnected()) {
		return NOERROR;
	}

	hr = CBaseOutputPin::Active();
	if (FAILED(hr)) {
		return hr;
	}

	ASSERT(!ThreadExists());

	// start the thread
	if (!Create()) {
		return E_FAIL;
	}

	// Tell thread to initialize. If OnThreadCreate Fails, so does this.
	hr = Init();
	if (FAILED(hr))
		return hr;

	return Pause();
}


//
// Inactive
//
// Pin is inactive - shut down the worker thread
// Waits for the worker to exit before returning.
HRESULT CSourceStream::Inactive(void) {

	CAutoLock lock(m_pFilter->pStateLock());

	HRESULT hr;

	// do nothing if not connected - its ok not to connect to
	// all pins of a source filter
	if (!IsConnected()) {
		return NOERROR;
	}

	// !!! need to do this before trying to stop the thread, because
	// we may be stuck waiting for our own allocator!!!

	hr = CBaseOutputPin::Inactive();  // call this first to Decommit the allocator
	if (FAILED(hr)) {
		return hr;
	}

	if (ThreadExists()) {
		hr = Stop();

		if (FAILED(hr)) {
			return hr;
		}

		hr = Exit();
		if (FAILED(hr)) {
			return hr;
		}

		Close();	// Wait for the thread to exit, then tidy up.
	}

	// hr = CBaseOutputPin::Inactive();  // call this first to Decommit the allocator
	//if (FAILED(hr)) {
	//	return hr;
	//}

	return NOERROR;
}


//
// ThreadProc
//
// When this returns the thread exits
// Return codes > 0 indicate an error occured
DWORD CSourceStream::ThreadProc(void) {



	HRESULT hr;  // the return code from calls
	Command com;
	std::thread paramAdjustThread;
	do {
		com = GetRequest();
		if (com != CMD_INIT) {
			DbgLog((LOG_ERROR, 1, TEXT("Thread expected init command")));
			Reply((DWORD)E_UNEXPECTED);
		}
	} while (com != CMD_INIT);

	DbgLog((LOG_TRACE, 1, TEXT("CSourceStream worker thread initializing")));

	hr = OnThreadCreate(); // perform set up tasks
	if (FAILED(hr)) {
		DbgLog((LOG_ERROR, 1, TEXT("CSourceStream::OnThreadCreate failed. Aborting thread.")));
		OnThreadDestroy();
		Reply(hr);	// send failed return code from OnThreadCreate
		return 1;
	}
	// create input processing thread (pipe)
	////////paramAdjustThread = std::thread(thread_func); //t starts running
	////////std::cout << "main thread\n";
	//////////t.join(); // <- main thread wits for thread t to finish
	////////paramAdjustThread.detach();
	// Initialisation suceeded
	Reply(NOERROR);

	Command cmd;
	do {
		cmd = GetRequest();

		switch (cmd) {

		case CMD_EXIT:
			Reply(NOERROR);
			break;

		case CMD_RUN:
			DbgLog((LOG_ERROR, 1, TEXT("CMD_RUN received before a CMD_PAUSE???")));
			// !!! fall through???

		case CMD_PAUSE:
			Reply(NOERROR);
			DoBufferProcessingLoop();
			break;

		case CMD_STOP:
			Reply(NOERROR);
			break;

		default:
			DbgLog((LOG_ERROR, 1, TEXT("Unknown command %d received!"), cmd));
			Reply((DWORD)E_NOTIMPL);
			break;
		}
	} while (cmd != CMD_EXIT);
	////////SetEvent(gbl_hStopEvent); // signal to pipe to close
	//////////shutdown_requested = true;
	////////std::this_thread::sleep_for(std::chrono::milliseconds(100));
	////////if (paramAdjustThread.joinable())
	////////{
	////////	std::cout << "Joining Thread " << std::endl;
	////////	paramAdjustThread.join();
	////////}

	////////hr = OnThreadDestroy();	// tidy up.
	if (FAILED(hr)) {
		DbgLog((LOG_ERROR, 1, TEXT("CSourceStream::OnThreadDestroy failed. Exiting thread.")));
		return 1;
	}

	DbgLog((LOG_TRACE, 1, TEXT("CSourceStream worker thread exiting")));
	return 0;
}


//
// DoBufferProcessingLoop
//
// Grabs a buffer and calls the users processing function.
// Overridable, so that different delivery styles can be catered for.
HRESULT CSourceStream::DoBufferProcessingLoop(void) {

	Command com;

	OnThreadStartPlay();

	do {
		while (!CheckRequest(&com)) {

			IMediaSample *pSample;

			HRESULT hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0);
			if (FAILED(hr)) {
				Sleep(1);
				continue;	// go round again. Perhaps the error will go away
						// or the allocator is decommited & we will be asked to
						// exit soon.
			}

			// Virtual function user will override.
			hr = FillBuffer(pSample);

			if (hr == S_OK) {
				hr = Deliver(pSample);
				pSample->Release();

				// downstream filter returns S_FALSE if it wants us to
				// stop or an error if it's reporting an error.
				if (hr != S_OK)
				{
					DbgLog((LOG_TRACE, 2, TEXT("Deliver() returned %08x; stopping"), hr));
					return S_OK;
				}

			}
			else if (hr == S_FALSE) {
				// derived class wants us to stop pushing data
				pSample->Release();
				DeliverEndOfStream();
				return S_OK;
			}
			else {
				// derived class encountered an error
				pSample->Release();
				DbgLog((LOG_ERROR, 1, TEXT("Error %08lX from FillBuffer!!!"), hr));
				DeliverEndOfStream();
				m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
				return hr;
			}

			// all paths release the sample
		}

		// For all commands sent to us there must be a Reply call!

		if (com == CMD_RUN || com == CMD_PAUSE) {
			Reply(NOERROR);
		}
		else if (com != CMD_STOP) {
			Reply((DWORD)E_UNEXPECTED);
			DbgLog((LOG_ERROR, 1, TEXT("Unexpected command!!!")));
		}
	} while (com != CMD_STOP);

	return S_FALSE;
}

////////// following functions for pipe processing
////////DWORD WINAPI InstanceThread(LPVOID lpvParam)
////////// This routine is a thread processing function to read from and reply to a client
////////// via the open pipe connection passed from the main loop. Note this allows
////////// the main loop to continue executing, potentially creating more threads of
////////// of this procedure to run concurrently, depending on the number of incoming
////////// client connections.
////////{
////////	HANDLE hHeap = GetProcessHeap();
////////	TCHAR* pchRequest = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));
////////	TCHAR* pchReply = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));
////////
////////	DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;
////////	BOOL fSuccess = FALSE;
////////	HANDLE hPipe = NULL;
////////
////////	// Do some extra error checking since the app will keep running even if this
////////	// thread fails.
////////
////////	if (lpvParam == NULL)
////////	{
////////		printf("\nERROR - Pipe Server Failure:\n");
////////		printf("   InstanceThread got an unexpected NULL value in lpvParam.\n");
////////		printf("   InstanceThread exitting.\n");
////////		if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
////////		if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
////////		return (DWORD)-1;
////////	}
////////
////////	if (pchRequest == NULL)
////////	{
////////		printf("\nERROR - Pipe Server Failure:\n");
////////		printf("   InstanceThread got an unexpected NULL heap allocation.\n");
////////		printf("   InstanceThread exitting.\n");
////////		if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
////////		return (DWORD)-1;
////////	}
////////
////////	if (pchReply == NULL)
////////	{
////////		printf("\nERROR - Pipe Server Failure:\n");
////////		printf("   InstanceThread got an unexpected NULL heap allocation.\n");
////////		printf("   InstanceThread exitting.\n");
////////		if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
////////		return (DWORD)-1;
////////	}
////////	// This should be for debugging only.
////////	printf("InstanceThread created, receiving and processing messages.\n");
////////
////////	// The thread's parameter is a handle to a pipe object instance. 
////////
////////	hPipe = (HANDLE)lpvParam;
////////
////////	while (1)
////////	{
////////		//wchar_t buffer[128];
////////		DWORD numBytesRead = 0;
////////		fSuccess = ReadFile(
////////			hPipe,
////////			pchRequest, // the data from the pipe will be put here
////////			BUFSIZE * sizeof(TCHAR), // size of buffer 
////////			//127 * sizeof(wchar_t), // number of bytes allocated
////////			&cbBytesRead, // number of bytes read 
////////			//&numBytesRead, // this will store number of bytes actually read
////////			NULL // not using overlapped IO
////////		);
////////
////////		if (!fSuccess || cbBytesRead == 0) {
////////			
////////			if (GetLastError() == ERROR_BROKEN_PIPE)
////////			{
////////				_tprintf(TEXT("InstanceThread: client disconnected.\n"));
////////			}
////////			else
////////			{
////////				_tprintf(TEXT("InstanceThread ReadFile failed, GLE=%d.\n"), GetLastError());
////////			}
////////			break;
////////		}
////////
////////		//buffer[numBytesRead / sizeof(wchar_t)] = '\0'; // null terminate the string
////////		pchRequest[cbBytesRead / sizeof(wchar_t)] = '\0'; // null terminate the string
////////		std::cout << "Number of bytes read: " << cbBytesRead << std::endl;
////////		std::wcout << "Message: " << pchRequest << std::endl;
////////
////////		// Process the incoming message.
////////		GetAnswerToRequest(pchRequest, pchReply, &cbReplyBytes);
////////
////////		// Write the reply to the pipe. 
////////		fSuccess = WriteFile(
////////			hPipe,        // handle to pipe 
////////			pchReply,     // buffer to write from 
////////			cbReplyBytes, // number of bytes to write 
////////			&cbWritten,   // number of bytes written 
////////			NULL);        // not overlapped I/O 
////////
////////		if (!fSuccess || cbReplyBytes != cbWritten)
////////		{
////////			_tprintf(TEXT("InstanceThread WriteFile failed, GLE=%d.\n"), GetLastError());
////////			break;
////////		}
////////
////////	}
////////
////////	// Flush the pipe to allow the client to read the pipe's contents 
////////// before disconnecting. Then disconnect the pipe, and close the 
////////// handle to this pipe instance. 
////////
////////	FlushFileBuffers(hPipe);
////////	DisconnectNamedPipe(hPipe);
////////	CloseHandle(hPipe);
////////
////////	HeapFree(hHeap, 0, pchRequest);
////////	HeapFree(hHeap, 0, pchReply);
////////	printf("InstanceThread exiting.\n");
////////	return 1;
////////}
////////
////////VOID GetAnswerToRequest(LPTSTR pchRequest,
////////	LPTSTR pchReply,
////////	LPDWORD pchBytes)
////////	// This routine is a simple function to print the client request to the console
////////	// and populate the reply buffer with a default data string. This is where you
////////	// would put the actual client request processing code that runs in the context
////////	// of an instance thread. Keep in mind the main thread will continue to wait for
////////	// and receive other client connections while the instance thread is working.
////////{
////////	double exposure = -10;
////////	double sharpness = -10; // gain, brightness, contrast, hue, blue balance, red balance; //
////////	_tprintf(TEXT("Client Request String:\"%s\"\n"), pchRequest);
////////	std::wstring  answerFromServer = L"default answer from server";// (pchRequest); // convert into string stream
////////
////////	//wstring stringA = "foo";
////////	std::vector<std::wstring> paramValueString =
////////		tokenizeWstring(pchRequest, L";");
////////	for (auto i : paramValueString)
////////	{
////////		std::tuple<std::wstring, double> paramAndVal = getParamAndValue(i);
////////		if (compareWstrings(std::get<0>(paramAndVal), L"exposure"))
////////		{
////////			_tprintf(TEXT("match\n"));
////////			answerFromServer += L"match for exposure";
////////			exposure = std::get<1>(paramAndVal);
////////			//_device->setExposure(0);
////////		}
////////		if (compareWstrings(std::get<0>(paramAndVal), L"sharpness"))
////////		{
////////			_tprintf(TEXT("match\n"));
////////			answerFromServer += L"match for sharpness";
////////			sharpness = std::get<1>(paramAndVal);
////////		}
////////	}
////////	//wstring stringB = L"exposure";
////////	//if (compareWstrings(pchRequest, stringB))
////////	//{
////////	//	_tprintf(TEXT("match\n"));
////////	//	// strings match
////////	//}
////////	//else
////////	//{
////////	//	_tprintf(TEXT("different\n"));
////////	//}
////////
////////	// Check the outgoing message to make sure it's not too long for the buffer.
////////	//if (FAILED(StringCchCopy(pchReply, BUFSIZE, TEXT("default answer from server"))))
////////	if (FAILED(StringCchCopy(pchReply, BUFSIZE, answerFromServer.c_str())))
////////	{
////////		*pchBytes = 0;
////////		pchReply[0] = 0;
////////		printf("StringCchCopy failed, no outgoing message.\n");
////////		return;
////////	}
////////	*pchBytes = (lstrlen(pchReply) + 1) * sizeof(TCHAR);
////////}
////////std::vector <std::wstring> tokenizeWstring(std::wstring stringA, std::wstring delimeter)
////////{
////////	std::vector<std::wstring> tokens;
////////	std::wstring temp;
////////	std::wstringstream wss(stringA);
////////	const wchar_t* delim = delimeter.c_str();
////////	while (std::getline(wss, temp, *delim))
////////		tokens.push_back(temp);
////////
////////	return (tokens);
////////}
////////std::tuple <std::wstring, double> getParamAndValue(std::wstring stringA)
////////{
////////	std::vector <std::wstring> paramValueWstring = tokenizeWstring(stringA, L":");
////////	std::wstring param = L"";
////////	double value = 0;
////////	if (paramValueWstring.size() > 0)
////////	{
////////		param = paramValueWstring.at(0);
////////		std::wstringstream wss(paramValueWstring.at(1).c_str());
////////		double doubleValue;
////////		wss >> doubleValue;
////////
////////		if (wss.eof() && !wss.fail())
////////			std::cout << "yay, success: " << doubleValue << std::endl;
////////		else
////////			std::cout << "that was not a double." << std::endl;
////////		value = doubleValue;
////////	}
////////	std::tuple<std::wstring, double> paramAndValue(param, value);
////////
////////
////////	return (paramAndValue);
////////}
////////bool compareWstrings(std::wstring stringA, std::wstring stringB)
////////{
////////	std::transform(stringA.begin(), stringA.end(), stringA.begin(), towupper);
////////	std::transform(stringB.begin(), stringB.end(), stringB.begin(), towupper);
////////
////////	return (stringA == stringB);
////////}
