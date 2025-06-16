//==============================================================================
// Zenith Engine - Client Application Header
// 
// This header is meant to be included **only** in client applications using the Zenith Engine.
// 
// **DO NOT** include this file in the engine's internal codebase.
//
// Including this file provides access to core engine functionality.
//
//==============================================================================

#pragma once

#include "Zenith/Core/Application.hpp"
#include "Zenith/Core/Log.hpp"
#include "Zenith/Core/Input.hpp"
#include "Zenith/Core/TimeStep.hpp"
#include "Zenith/Core/Timer.hpp"
#include "Zenith/Core/Platform.hpp"
#include "Zenith/Core/Version.hpp"

#include "Zenith/Events/Event.hpp"
#include "Zenith/Events/ApplicationEvent.hpp"
#include "Zenith/Events/KeyEvent.hpp"
#include "Zenith/Events/MouseEvent.hpp"

#include <imgui.h>

// ==== Zenith Renderer API ==============================
#include "Zenith/Renderer/Renderer.hpp"
// =======================================================