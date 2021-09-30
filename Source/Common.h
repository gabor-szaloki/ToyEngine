#pragma once

#pragma warning(disable:4267) // warning C4267: 'argument': conversion from 'size_t' to 'int', possible loss of data
#pragma warning(disable:4244) // warning C4244: 'initializing': conversion from 'double' to 'float', possible loss of data

#include <DirectXMath.h>
#include <3rdParty/plog/Log.h>

extern class IDriver* drv;
extern class ThreadPool* tp;
extern class AssetManager* am;
extern class WorldRenderer* wr;
extern class IFullscreenExperiment* fe;
extern void create_driver_d3d11();
extern void* get_hwnd();
extern void exit_program();
extern const char* get_log_file_path();

// TODO: move to some kind of utils
wchar_t* utf8_to_wcs(const char* utf8_str, wchar_t* wcs_buf, int wcs_buf_len);
char* wcs_to_utf8(const wchar_t* wcs_str, char* utf8_buf, int utf8_buf_len);
float random_01();
float random_range(float min, float max);

#define M_PI 3.14159265358979323846

#define RAD_TO_DEG 57.2957795f;	
#define DEG_TO_RAD 0.0174532925f;
inline float to_deg(float rad) { return rad * RAD_TO_DEG; }
inline float to_rad(float deg) { return deg * DEG_TO_RAD; }

#define SAFE_DELETE(p) { if (p != nullptr) { delete p; p = nullptr; } }
#define SAFE_DELETE_ARRAY(arr) { if (arr != nullptr) { delete arr; arr = nullptr; } }

enum class RenderPass
{
	FORWARD,
	DEPTH,
	_COUNT // Not an actual render pass, only used for enumeration, or declaring arrays
};

struct ProfileScopeHelper
{
	ProfileScopeHelper(const char* label);
	~ProfileScopeHelper();
};

#define PROFILE_MARKERS_ENABLED _DEBUG
#if PROFILE_MARKERS_ENABLED
	#define TOY_PS_CC0(a, b) a##b
	#define TOY_PS_CC1(a, b) TOY_PS_CC0(a, b)
	#define PROFILE_SCOPE(label) ProfileScopeHelper TOY_PS_CC1(profileScope, __LINE__)(label)
#else
	#define PROFILE_SCOPE(label) (void)0;
#endif

// For DirectXMath
using namespace DirectX;