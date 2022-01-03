#include "pch.h"

#include "App.h"
#include "App.g.cpp"

namespace winrt::Xaml::implementation
{
	App::App()
	{
		AddRef(); // Fix crash on exit
	}
}
