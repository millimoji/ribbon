//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "xamlpch.h"
#include "MainPage.g.h"

namespace Ribbon {
struct IJsonInputModel;
} // Ribbon

namespace xamlapp
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();

		// ObjectForScript
	private:
		void webCtrl_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void webCtrl_ScriptNotify(Platform::Object^ sender, Windows::UI::Xaml::Controls::NotifyEventArgs^ e);
		void EmulationText_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
		void EmulationText_KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
		void EventPostProcess();

		std::shared_ptr<Ribbon::IJsonInputModel> m_jsonInputModel;
		std::wstring m_determined;
		std::wstring m_composition;
	};
}
