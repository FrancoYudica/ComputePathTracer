#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

env.Append(CPPPATH=["src/"])

# Internal build name (doesn't change)
lib_base_name = "libpathtracer" 
sources = Glob("src/*.cpp")

# Define the build output path based on platform
if env["platform"] == "macos":
    target_path = "bin/{}.{}.{}.framework/{}".format(
        lib_base_name, env["platform"], env["target"], lib_base_name
    )
    library = env.SharedLibrary(target_path, source=sources)
elif env["platform"] == "ios":
    suffix = ".simulator.a" if env["ios_simulator"] else ".a"
    target_path = "bin/{}.{}.{}{}".format(lib_base_name, env["platform"], env["target"], suffix)
    library = env.StaticLibrary(target_path, source=sources)
else:
    # This handles Windows (.dll) and Linux (.so)
    target_path = "bin/{}{}{}".format(lib_base_name, env["suffix"], env["SHLIBSUFFIX"])
    library = env.SharedLibrary(target_path, source=sources)


# Define multiple install targets
# Demo project path
demo_path = "cpt/addons/pathtracer/bin/"

# Distribution addon folder
dist_path = "addon_files/addons/pathtracer/bin/"

# Instruct SCons to "Install" (copy) the library to both places
install_targets = [
    env.Install(demo_path, library),
    env.Install(dist_path, library)
]

# Grab all files in the shader directory and filter the .import files
all_shader_files = Glob("cpt/addons/pathtracer/shaders/*")
shader_sources = [f for f in all_shader_files if not str(f).endswith(".import")]
shader_dest = "addon_files/addons/pathtracer/shaders/"
install_shaders = env.Install(shader_dest, shader_sources)

# Make these the default action when you run 'scons'
Default(library)
Default(install_targets)
Default(install_shaders)