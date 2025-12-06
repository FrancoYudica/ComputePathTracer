#include "register_types.h"

#include <gdextension_interface.h>

#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/godot.hpp>

#include "pt_analytical_geometry.h"
#include "pt_renderer_settings.h"
#include "pt_renderer.h"
#include "pt_renderer_stats.h"
#include "pt_camera.h"

using namespace godot;

void initialize_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    GDREGISTER_RUNTIME_CLASS(PTAnalyticalGeometry);
    GDREGISTER_RUNTIME_CLASS(PTRendererSettings);
    GDREGISTER_RUNTIME_CLASS(PTRendererStats);
    GDREGISTER_RUNTIME_CLASS(PTRenderer);
    GDREGISTER_RUNTIME_CLASS(PTCamera);

    // Register PTRenderer as a singleton
    PTRenderer* singleton = memnew(PTRenderer);
    // singleton->init();
    Engine::get_singleton()->register_singleton("PTRenderer", singleton);
}

void uninitialize_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT
example_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
                     const GDExtensionClassLibraryPtr p_library,
                     GDExtensionInitialization* r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address,
                                                   p_library, r_initialization);

    init_obj.register_initializer(initialize_module);
    init_obj.register_terminator(uninitialize_module);
    init_obj.set_minimum_library_initialization_level(
        MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}