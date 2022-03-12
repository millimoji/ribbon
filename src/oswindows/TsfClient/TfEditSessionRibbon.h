#include <wrl.h>
namespace wrl = Microsoft::WRL;

class TfEditSessionRibbon : public ITfEditSession
{
public:
	TfEditSessionRibbon(const std::function<HRESULT (TfEditCookie ec)>& handler) :
		m_handler(handler)
	{}

	virtual ~TfEditSessionRibbon() {}

	// IUnknown
	IFACEMETHOD(QueryInterface)(REFIID riid, _Outptr_ void **ppvObj) {
		if (riid == IID_IUnknown || riid == IID_ITfEditSession) {
			*ppvObj = static_cast<ITfEditSession*>(this);
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}
	IFACEMETHOD_(ULONG, AddRef)() {
		return (ULONG)InterlockedIncrement(&m_refCount);
	}
	IFACEMETHOD_(ULONG, Release)() {
		LONG refCount = InterlockedDecrement(&m_refCount);
		if (refCount == 0) {
			delete this;
		}
		return refCount;
	}
	// ITfEditSession
	IFACEMETHOD(DoEditSession)(TfEditCookie ec) {
		return m_handler(ec);
	}

private:
	const std::function<HRESULT(TfEditCookie ec)> m_handler;
    LONG m_refCount = 0;

//
	TfEditSessionRibbon& operator = (const TfEditSessionRibbon&) = delete;
};

HRESULT _MyInjectText(_In_ ITfContext* tctx, TfEditCookie ec, const char16_t* text)
{
    wrl::ComPtr<ITfInsertAtSelection> tfIas;
    RETURN_IF_FAILED(tctx->QueryInterface(IID_PPV_ARGS(&tfIas)));

    wrl::ComPtr<ITfRange> tfRange;
	RETURN_IF_FAILED(tfIas->InsertTextAtSelection(ec, TF_IAS_NO_DEFAULT_COMPOSITION,
		reinterpret_cast<const wchar_t*>(text), static_cast<LONG>(Ribbon::textlen(text)), &tfRange));

	return S_OK;
}

HRESULT _MyCommonSetAttributes(_In_ ITfContext* tctx, TfEditCookie ec, ITfRange* tfRange, TfGuidAtom tfGa)
{
	wrl::ComPtr<ITfProperty> langaugeProperty;
	RETURN_IF_FAILED(tctx->GetProperty(GUID_PROP_LANGID, &langaugeProperty));

	VARIANT var;
	var.vt = VT_I4;
	var.lVal = TEXTSERVICE_LANGID;
	RETURN_IF_FAILED(langaugeProperty->SetValue(ec, tfRange, &var));

	wrl::ComPtr<ITfProperty> dispAttrProperty;
	RETURN_IF_FAILED(tctx->GetProperty(GUID_PROP_ATTRIBUTE, &dispAttrProperty));
	var.vt = VT_I4;
	var.lVal = tfGa;
	RETURN_IF_FAILED(dispAttrProperty->SetValue(ec, tfRange, &var));

	wrl::ComPtr<ITfRange> rangeSelection;
	RETURN_IF_FAILED(tfRange->Clone(&rangeSelection));

	rangeSelection->Collapse(ec, TF_ANCHOR_END);

	TF_SELECTION sel;
	sel.range = rangeSelection.Get();
	sel.style.ase = TF_AE_NONE;
	sel.style.fInterimChar = FALSE;
	RETURN_IF_FAILED(tctx->SetSelection(ec, 1, &sel));

	return S_OK;
}

HRESULT _MyStartComposition(_In_ ITfContext* tctx, TfEditCookie ec, _In_ ITfCompositionSink* compositionSink, _In_ const char16_t* text, TfGuidAtom tfGa, _COM_Outptr_ ITfComposition** tfComposition)
{
    wrl::ComPtr<ITfInsertAtSelection> tfIas;
    RETURN_IF_FAILED(tctx->QueryInterface(IID_PPV_ARGS(&tfIas)));

    wrl::ComPtr<ITfRange> tfRange;
	RETURN_IF_FAILED(tfIas->InsertTextAtSelection(ec, /*TF_IAS_QUERYONLY*/ 0,
		reinterpret_cast<const wchar_t*>(text), static_cast<LONG>(Ribbon::textlen(text)), &tfRange));

	wrl::ComPtr<ITfContextComposition> contextComposition;
	RETURN_IF_FAILED(tctx->QueryInterface(IID_PPV_ARGS(&contextComposition)));
	RETURN_IF_FAILED(contextComposition->StartComposition(ec, tfRange.Get(), compositionSink, tfComposition));

	return _MyCommonSetAttributes(tctx, ec, tfRange.Get(), tfGa);
}

HRESULT _MyUpdateComposition(_In_ ITfContext* tctx, TfEditCookie ec, _In_ ITfComposition* tfComposition, _In_ const char16_t* text, TfGuidAtom tfGa)
{
	wrl::ComPtr<ITfRange> tfRange;
	RETURN_IF_FAILED(tfComposition->GetRange(&tfRange));

	RETURN_IF_FAILED(tfRange->SetText(ec, 0, reinterpret_cast<const wchar_t*>(text), static_cast<LONG>(Ribbon::textlen(text))));

	return _MyCommonSetAttributes(tctx, ec, tfRange.Get(), tfGa);
}

HRESULT _MyEndComposition(_In_ ITfContext* tctx, TfEditCookie ec, _In_ ITfComposition* tfComposition, _In_ const char16_t* text)
{
	{
		wrl::ComPtr<ITfRange> range;
		RETURN_IF_FAILED(tfComposition->GetRange(&range));

		if (text == nullptr || *text == 0) {
			RETURN_IF_FAILED(range->SetText(ec, 0, nullptr, 0));
		} else {
			RETURN_IF_FAILED(range->SetText(ec, 0, reinterpret_cast<const wchar_t*>(text), static_cast<LONG>(Ribbon::textlen(text))));
		}

		wrl::ComPtr<ITfProperty> tfProperty;
		if (SUCCEEDED(tctx->GetProperty(GUID_PROP_ATTRIBUTE, &tfProperty))) {
			tfProperty->Clear(ec, range.Get());
		}

		range->Collapse(ec, TF_ANCHOR_END);
		RETURN_IF_FAILED(tfComposition->EndComposition(ec));

		ULONG fetched = 0;
		TF_SELECTION tfSelection;
		if (SUCCEEDED(tctx->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &fetched)) && fetched == 1) {
			tfSelection.range->Release();
			tfSelection.range = range.Get();
			tctx->SetSelection(ec, 1, &tfSelection);
		}
	}
	return S_OK;
}


bool s_candidateRangeInitialized = false;
CCandidateRange s_candidateRange;
CRibbonImeArray<CCandidateListItem> s_candidateList;

bool _MyIsCandidateDispalyed(wrl::ComPtr<CCandidateListUIPresenter>& candidateListUIPresenter)
{
	if (!candidateListUIPresenter) {
		return false;
	}
	BOOL isShowing = FALSE;
	candidateListUIPresenter->IsShown(&isShowing);
	return !!isShowing;
}

HRESULT _MyOpenCandidate(_In_ ITfContext* tctx, TfClientId tcid, ITfComposition* tfComposition, TfEditCookie ec,
	wrl::ComPtr<CCandidateListUIPresenter>& candidateListUIPresenter, CRibbonIME *pTextService)
{
	if (!s_candidateRangeInitialized) {
	    s_candidateRangeInitialized = true;
	    for (DWORD i = 1; i <= 10; i++) {
        	DWORD* pNewIndexRange = nullptr;
        	pNewIndexRange = s_candidateRange.Append();
	        if (pNewIndexRange != nullptr) {
				*pNewIndexRange = (i % 10);
			}
        }
    }

	if (!candidateListUIPresenter) {
		candidateListUIPresenter = new CCandidateListUIPresenter(
			pTextService,
			Global::AtomCandidateWindow,
			CATEGORY_CANDIDATE,
			&s_candidateRange,
			FALSE);
	}

    // call _Start*Line for CCandidateListUIPresenter or CReadingLine
    // we don't cache the document manager object so get it from pContext.
    {	wrl::ComPtr<ITfDocumentMgr> pDocumentMgr;
	    if (SUCCEEDED(tctx->GetDocumentMgr(&pDocumentMgr)) && pDocumentMgr) {
	        wrl::ComPtr<ITfRange> pRange;
			if (tfComposition != nullptr) {
				tfComposition->GetRange(&pRange); // TODO: error check
			}
			else {
				TF_SELECTION tfSelection;
				ULONG fetched = 0;
				tctx->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &fetched);
				pRange.Attach(tfSelection.range);
			}
	        if (pRange) {
				candidateListUIPresenter->_StartCandidateList(tcid, pDocumentMgr.Get(), tctx, ec, pRange.Get(), CAND_WIDTH);
			}
		}
	}

	return S_OK;
}

HRESULT _MyCloseCandidate(wrl::ComPtr<CCandidateListUIPresenter>& candidateListUIPresenter)
{
	if (candidateListUIPresenter) {
		candidateListUIPresenter->_EndCandidateList();
		candidateListUIPresenter->Show(FALSE);
	}
	return S_OK;
}

HRESULT _MyUpdateCandidate(wrl::ComPtr<CCandidateListUIPresenter>& candidateListUIPresenter, const std::shared_ptr<Ribbon::IPhraseList>& phraseList)
{
	// convert structure
	int phraseCount = phraseList->PhraseCount();

	s_candidateList.Clear();
	s_candidateList.reserve(static_cast<size_t>(phraseCount));

	for (int phraseIndex = 0; phraseIndex < phraseCount; ++phraseIndex) {
		auto phrase = phraseList->Phrase(phraseIndex);

		CCandidateListItem* candListItem = s_candidateList.Append();
		candListItem->_ItemString.Set(reinterpret_cast<const WCHAR*>(phrase->Display().u16ptr()), phrase->Display().length());
		candListItem->_FindKeyCode.Clear();
	}

	candidateListUIPresenter->_ClearList();
	candidateListUIPresenter->_SetText(&s_candidateList, FALSE);
	return S_OK;
}

// TODO: consider later
wrl::ComPtr<ITfComposition> m_tfComposition;
wrl::ComPtr<CCandidateListUIPresenter> m_tfCandidateListUIPresenter;

HRESULT UpdateEditControl(ITfContext* _tctx, TfClientId tcid, _In_ CRibbonIME* _this, const std::shared_ptr<Ribbon::IInputModel>& _inputModel, TfGuidAtom tfGa) {
	std::shared_ptr<Ribbon::IInputModel> inputModel { _inputModel };
	wrl::ComPtr<ITfContext> tctx { _tctx };
	wrl::ComPtr<CRibbonIME> ribbonIME{ _this };

	wrl::ComPtr<ITfEditSession> tes = new TfEditSessionRibbon([tctx, tcid, ribbonIME, inputModel, tfGa](TfEditCookie ec) -> HRESULT {
		bool hasComposition = !!m_tfComposition;

		auto insertingText = inputModel->InsertingText();
		if (insertingText.length() > 0) {
			if (hasComposition) {
				RETURN_IF_FAILED(_MyEndComposition(tctx.Get(), ec, m_tfComposition.Get(), insertingText.u16ptr()));
				m_tfComposition.Reset();
				hasComposition = false;
			} else {
				RETURN_IF_FAILED(_MyInjectText(tctx.Get(), ec, insertingText.u16ptr()));
			}
		}

		if (inputModel->CompositionText()) {
			auto compositionText = inputModel->CompositionText()->Display();
			if (compositionText.length() > 0) {
				if (hasComposition) {
					RETURN_IF_FAILED(_MyUpdateComposition(tctx.Get(), ec, m_tfComposition.Get(), compositionText.u16ptr(), tfGa));
				} else {
					RETURN_IF_FAILED(_MyStartComposition(tctx.Get(), ec, ribbonIME.Get(), compositionText.u16ptr(), tfGa, &m_tfComposition));
				}
			} else {
				if (hasComposition) {
					RETURN_IF_FAILED(_MyEndComposition(tctx.Get(), ec, m_tfComposition.Get(), u""));
					m_tfComposition.Reset();
					hasComposition = false;
				}
			}
		}

		// Candidate Window
		const auto& phraseList = inputModel->CandidateList();
		bool closeCandidate = !phraseList || (phraseList->PhraseCount() == 0) || !!(phraseList->BitFlags() & Ribbon::PHRASELIST_BIT_NEXTWORD);
		if (closeCandidate) {
			if (_MyIsCandidateDispalyed(m_tfCandidateListUIPresenter)) {
				_MyCloseCandidate(m_tfCandidateListUIPresenter);
			}
		}
		if (phraseList && phraseList->PhraseCount() > 0) {
			if (!_MyIsCandidateDispalyed(m_tfCandidateListUIPresenter)) {
				_MyOpenCandidate(tctx.Get(), tcid, m_tfComposition.Get(), ec, m_tfCandidateListUIPresenter, ribbonIME.Get());
			}
			_MyUpdateCandidate(m_tfCandidateListUIPresenter, inputModel->CandidateList());
		}
		return S_OK;
	});

	HRESULT hr = _tctx->RequestEditSession(tcid, tes.Get(), TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hr);
	return hr;
}

HRESULT GetTextContext(ITfContext* tctx, TfClientId tcid, std::wstring& contextString) {
	wrl::ComPtr<ITfEditSession> tes = new TfEditSessionRibbon([&](TfEditCookie ec) -> HRESULT {
		TF_SELECTION tfSelection;
		ULONG fetchedCount;
		RETURN_IF_FAILED(tctx->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &fetchedCount));

		wrl::ComPtr<ITfRange> selectionRange;
		selectionRange.Attach(tfSelection.range);

		wrl::ComPtr<ITfRange> contextRange;
		RETURN_IF_FAILED(selectionRange->Clone(&contextRange));

		LONG movedCount = 0;
		RETURN_IF_FAILED(contextRange->ShiftStart(ec, -120, &movedCount, nullptr));

		wchar_t contextBuf[122];
		contextBuf[0] = L' ';
		ULONG contextCount = 0;
		RETURN_IF_FAILED(contextRange->GetText(ec, 0, contextBuf + 1, static_cast<ULONG>(ARRAYSIZE(contextBuf) - 1), &contextCount));

		if (contextCount == 0) {
			contextString = L"";
		}
		else if (contextCount < static_cast<ULONG>(ARRAYSIZE(contextBuf) - 1)) {
			contextString = std::wstring(contextBuf, contextCount + 1);
		}
		else {
			contextString = std::wstring(contextBuf + 1, contextCount);
		}
		return S_OK;
	});
	HRESULT hr = tctx->RequestEditSession(tcid, tes.Get(), TF_ES_SYNC | TF_ES_READ, &hr);
	return hr;
}

