#pragma once

// Filter name strings
#define g_ps3PS3EyeSource    L"PS3 Eye Universal"


class PS3EyePushPin;

class PS3EyePushPin : public CSourceStream, public IKsPropertySet, public IAMStreamConfig
{
protected:
	ps3eye::PS3EYECam::PS3EYERef _device;
	CMediaType _currentMediaType;
	HRESULT _GetMediaType(int iPosition, CMediaType *pMediaType);
	REFERENCE_TIME _startTime;
	IReferenceClock *_refClock;
	int count = 0;
	int brightness = 0;

	static DWORD static_entry(LPVOID* param);

public:
	PS3EyePushPin(HRESULT *phr, CSource *pFilter, ps3eye::PS3EYECam::PS3EYERef device);
	BOOL CloseHandleSafe(HANDLE& h);

	~PS3EyePushPin();



	DECLARE_IUNKNOWN

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
	{
		if (riid == IID_IKsPropertySet)
		{
			return GetInterface((IKsPropertySet*)this, ppv);
		}
		else if (riid == IID_IAMStreamConfig) {
			return GetInterface((IAMStreamConfig*)this, ppv);
		}
		return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
	}
	void thread_func();
	//DWORD WINAPI InstanceThread(LPVOID);
	void WINAPI InstanceThread(LPVOID);
	//HANDLE hPipe = INVALID_HANDLE_VALUE;
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	VOID GetAnswerToRequest(LPTSTR, LPTSTR, LPDWORD);

	std::vector <std::wstring> tokenizeWstring(std::wstring stringA, std::wstring delimeter);
	std::tuple <std::wstring, double> getParamAndValue(std::wstring stringA);
	bool compareWstrings(std::wstring stringA, std::wstring stringB);

	HRESULT CheckMediaType(const CMediaType *);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest);
	
	HRESULT OnThreadStartPlay();
	HRESULT OnThreadCreate();
	HRESULT OnThreadDestroy();
	HRESULT FillBuffer(IMediaSample *pSample);
	void FillError(BYTE *pData);

	// Quality control
	// Not implemented because we aren't going in real time.
	// If the file-writing filter slows the graph down, we just do nothing, which means
	// wait until we're unblocked. No frames are ever dropped.
	STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q)
	{
		return E_FAIL;
	}


	// Inherited via IKsPropertySet
	virtual HRESULT __stdcall Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData) override;

	virtual HRESULT __stdcall Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD * pcbReturned) override;

	virtual HRESULT __stdcall QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD * pTypeSupport) override;


	// Inherited via IAMStreamConfig
	virtual HRESULT __stdcall SetFormat(AM_MEDIA_TYPE * pmt) override;

	virtual HRESULT __stdcall GetFormat(AM_MEDIA_TYPE ** ppmt) override;

	virtual HRESULT __stdcall GetNumberOfCapabilities(int * piCount, int * piSize) override;

	virtual HRESULT __stdcall GetStreamCaps(int iIndex, AM_MEDIA_TYPE ** ppmt, BYTE * pSCC) override;

};
inline BOOL PS3EyePushPin::CloseHandleSafe(HANDLE& h)
{
	HANDLE tmp = h; h = NULL;
	return tmp && tmp != INVALID_HANDLE_VALUE ? CloseHandle(tmp) : TRUE;
}

class PS3EyeSource : public CSource
{
private:
	PS3EyeSource(IUnknown *pUnk, HRESULT *phr);
	~PS3EyeSource();

	PS3EyePushPin *_pin;

public:
	static CUnknown * WINAPI CreateInstance(IUnknown *pUnk, HRESULT *phr);
};