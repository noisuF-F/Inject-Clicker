#include <MinHook.h>
#include <gl/GL.h>
#include <chrono>

#include "Hook.h"
#include "GUI.h"

typedef BOOL(__stdcall* TWglSwapBuffers) (HDC hDc);

static bool is_init{};
static HWND wnd_handle{};
static WNDPROC origin_wndproc{};
void* p_swap_buffers{};
TWglSwapBuffers origin_wglSwapBuffers{};

static LRESULT __stdcall WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static bool __stdcall wglSwapBuffers(HDC hDc);

//
// Management functions
//

bool Hook::init()
{
	if (is_init)
		return false;

	/* WndProc */
	{
		// Older versions of Minecraft
		wnd_handle = FindWindowA("LWJGL", nullptr);

		/* If failed to get window handle, attempt using window class used by more recent versions of Minecraft */
		if (!wnd_handle)
		{
			wnd_handle = FindWindowA("GLFW30", nullptr);

			/* At this point cannot find Minecraft window */
			if (!wnd_handle)
			{
				printf("[-] Failed to get window handle\n");
				return true;
			}
		}

		origin_wndproc = (WNDPROC)SetWindowLongPtrW(wnd_handle, GWLP_WNDPROC, (LONG_PTR)WndProc);
	}

	MH_Initialize();

	/* wglSwapBuffers */
	{
		p_swap_buffers = (void*)GetProcAddress(GetModuleHandleA("opengl32.dll"), "wglSwapBuffers");

		if (p_swap_buffers == nullptr)
		{
			printf("[-] Failed to get wglSwapBuffers' address\n");
			return true;
		}

		MH_CreateHook(p_swap_buffers, &wglSwapBuffers, (LPVOID*)&origin_wglSwapBuffers);
	}

	MH_EnableHook(MH_ALL_HOOKS);

	is_init = true;

	return false;
}

void Hook::shutdown()
{
	if (!is_init)
		return;

	SetWindowLongPtrW(wnd_handle, GWLP_WNDPROC, (LONG_PTR)origin_wndproc);
	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);

	is_init = false;
}

//
// Get functions
//

bool Hook::getIsInit()
{
	return is_init;
}

//
// Hooks
//

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (GUI::getIsInit())
	{
		/* If VK_INSERT, change menu draw state */
		if (msg == WM_KEYDOWN && wParam == VK_INSERT)
		{
			GUI::setDoDraw(!GUI::getDoDraw());
			return 0; // Block further processing
		}

		/* If VK_F6, toggle autoclicker */
		if (msg == WM_KEYDOWN && wParam == VK_F6)
		{
			GUI::setAutoclickerEnabled(!GUI::getAutoclickerEnabled());
			return 0; // Block further processing
		}

		/* Handle ImGui input when menu is open */
		if (GUI::getDoDraw())
		{
			if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
				return true; // Let ImGui handle the message
			// Block keyboard input when menu is open, except VK_INSERT and VK_F6
			if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP)
			{
				return 0; // Block keyboard events from reaching the game
			}
			// Block mouse input and movement when menu is open
			if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP ||
				msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP || msg == WM_MOUSEMOVE || msg == WM_INPUT)
			{
				return 0; // Block mouse events, movement, and raw input from reaching the game
			}
		}
		else
		{
			// Pass all messages to the game when menu is closed
			return CallWindowProcA(origin_wndproc, hWnd, msg, wParam, lParam);
		}
	}

	return CallWindowProcA(origin_wndproc, hWnd, msg, wParam, lParam);
}

bool __stdcall wglSwapBuffers(HDC hDc)
{
	HGLRC origin_context{ wglGetCurrentContext() };
	static HGLRC new_context{};

	if (static bool was_init{}; was_init == false)
	{
		new_context = wglCreateContext(hDc);
		wglMakeCurrent(hDc, new_context);

		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		glViewport(0, 0, viewport[2], viewport[3]);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, viewport[2], viewport[3], 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glDisable(GL_DEPTH_TEST);

		GUI::init(wnd_handle);

		was_init = true;
	}
	else
	{
		wglMakeCurrent(hDc, new_context);
		GUI::draw();

		/* Autoclicker logic */
		static auto last_click = std::chrono::steady_clock::now();
		if (GUI::getAutoclickerEnabled() && (GetAsyncKeyState(VK_LBUTTON) & 0x8000)) // Check if LMB is held
		{
			auto now = std::chrono::steady_clock::now();
			int delay = static_cast<int>(1000.0f / GUI::getAutoclickerCPS()); // Delay in milliseconds
			if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_click).count() >= delay)
			{
				PostMessage(wnd_handle, WM_LBUTTONDOWN, MK_LBUTTON, 0);
				PostMessage(wnd_handle, WM_LBUTTONUP, 0, 0);
				last_click = now;
			}
		}
	}

	wglMakeCurrent(hDc, origin_context);

	return origin_wglSwapBuffers(hDc);
}