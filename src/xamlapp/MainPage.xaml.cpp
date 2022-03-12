//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "inputmodel/InputModel.h"

using namespace xamlapp;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Documents;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage::MainPage()
{
	InitializeComponent();

	m_jsonInputModel = FACTORYCREATENS(Ribbon, JsonInputModel);

	//VirtualKeyPanel->InvokeScriptAsync();
}

void xamlapp::MainPage::webCtrl_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	EmulationText->Focus(Windows::UI::Xaml::FocusState::Keyboard);
}

void xamlapp::MainPage::webCtrl_ScriptNotify(Platform::Object^ sender, Windows::UI::Xaml::Controls::NotifyEventArgs^ e)
{
	auto jsonUtf8 = Ribbon::to_utf8(reinterpret_cast<const char16_t*>(e->Value->Data()));
	m_jsonInputModel->JsonCommand(jsonUtf8.c_str());
	EventPostProcess();
}


void xamlapp::MainPage::EmulationText_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
	e->Handled = true;
	json11::Json json = json11::Json::object{
		{ "command", "keydown" },
		{ "osKey", (int)e->Key },
	};
	m_jsonInputModel->JsonCommand(json.dump().c_str());
	EventPostProcess();
}


void xamlapp::MainPage::EmulationText_KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
	e->Handled = true;
	json11::Json json = json11::Json::object{
		{ "command", "keyup" },
		{ "osKey", (int)e->Key },
	};
	m_jsonInputModel->JsonCommand(json.dump().c_str());
	EventPostProcess();
}

void xamlapp::MainPage::EventPostProcess()
{
	{
		auto compositionState = m_jsonInputModel->CompositionState();

		std::string error;
		auto jsonData = json11::Json::parse(compositionState.c_str(), error);

		auto compositionText = jsonData["composition"][0]["display"].string_value();
		m_composition = reinterpret_cast<const wchar_t*>(Ribbon::to_utf16(compositionText).c_str());

		auto caretPos = jsonData["caret"].int_value();
		std::wstring preComposition = m_composition.substr(0, caretPos);
		std::wstring postComposition = m_composition.substr(caretPos, m_composition.length() - caretPos);

		std::wstring strDetermined = reinterpret_cast<const wchar_t*>(Ribbon::to_utf16(jsonData["commit"].string_value()).c_str());
		m_determined += strDetermined;

		std::wstring strTextContent;
		strTextContent = m_determined;
		strTextContent += L"[" + preComposition + L"|" + postComposition + L"]";
		EmulationText->Text = ref new Platform::String(strTextContent.c_str());
	}

	{
		auto candidateState = m_jsonInputModel->CandidateState();
		auto webCallArgs = ref new Platform::Collections::Vector<String^>();
		webCallArgs->Append(ref new Platform::String(reinterpret_cast<const wchar_t*>(Ribbon::to_utf16(candidateState).c_str())));
		webCtrl->InvokeScriptAsync(Platform::StringReference(L"MediatorUpdateCandidate"), webCallArgs);
	}

	//auto keyboardState = m_jsonInputModel->KeyboardState();

	if (m_jsonInputModel->GetRawInputModel()->KeyboardState() == Ribbon::ToKeyboard::Quit) {
		OutputDebugString(L"Quit Keyboard\n");
	}
}
