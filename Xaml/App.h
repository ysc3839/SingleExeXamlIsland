#pragma once
#include "App.g.h"
#include "App.base.h"

namespace winrt::Xaml::implementation
{
	struct App : AppT2<App>
	{
		App();
	};
}

namespace winrt::Xaml::factory_implementation
{
	class App : public AppT<App, implementation::App>
	{
	};
}
