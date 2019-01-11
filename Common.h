#pragma once

#define SAFE_RELEASE(resource) { if (resource != nullptr) { resource->Release(); } }