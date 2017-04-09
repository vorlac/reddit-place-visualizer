#include "PlaceVisualizerForm.h"

using namespace System;
using namespace System::Windows::Forms;

[STAThread]
void Main(array<String^>^ args)
{
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	place_gui::PlaceVisualizerForm form;
	Application::Run(%form);
}