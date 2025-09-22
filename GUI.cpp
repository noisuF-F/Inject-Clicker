#include "GUI.h"
#include <thread>

static bool is_init{};
static bool do_draw{true};
static bool autoclicker_enabled{false};
static float autoclicker_cps{10.0f}; // Clicks per second

//
// Management functions
//

bool GUI::init(HWND wnd_handle)
{
	if (is_init)
		return false;

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(wnd_handle);
	ImGui_ImplOpenGL3_Init();

	is_init = true;

	return false;
}

void GUI::shutdown()
{
	if (!is_init)
		return;

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	is_init = false;
}

//
// Global functions
//

void GUI::draw()
{
	if (!do_draw)
		return;

	// Draw darkened background
	if (do_draw)
	{
		ImGui::GetBackgroundDrawList()->AddRectFilled(
			ImVec2(0, 0),
			ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y),
			IM_COL32(0, 0, 0, 128) // Semi-transparent black (alpha 128/255)
		);
	}

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Inject clicker v0.1");
	{
		ImGui::Separator();
		ImGui::Text("Autoclicker Settings");
		ImGui::Checkbox("Enable Autoclicker", &autoclicker_enabled);
		ImGui::SliderFloat("Clicks Per Second", &autoclicker_cps, 1.0f, 20.0f, "%.1f CPS");
	}
	ImGui::End();
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

//
// Get & Set functions
//

bool GUI::getIsInit()
{
	return is_init;
}

bool GUI::getDoDraw()
{
	return do_draw;
}

void GUI::setDoDraw(bool new_value)
{
	do_draw = new_value;
}

bool GUI::getAutoclickerEnabled()
{
	return autoclicker_enabled;
}

void GUI::setAutoclickerEnabled(bool new_value)
{
	autoclicker_enabled = new_value;
}

float GUI::getAutoclickerCPS()
{
	return autoclicker_cps;
}

void GUI::setAutoclickerCPS(float new_value)
{
	autoclicker_cps = new_value;
}