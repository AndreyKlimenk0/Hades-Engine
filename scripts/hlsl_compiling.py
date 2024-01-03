import os
import sys
import enum
import subprocess

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT_DIR = os.path.join("data", "shaders")
HLSH_DIR = "hlsl"


 # fxc /E vs_main /Od /Zi /T vs_5_0 /Fo PixelShader1.fxc demo.hlsl

class Shader_Type(enum.Enum):
    VERTEX_SHADER = 1
    COMPUTER_SHADER = 2
    DOMAIN_SHADER = 3
    GEOMETRY_SHADER = 4
    PIXEL_SHADER = 5
    HEADER_FILE = 6


def get_entry_point(shader_type: Shader_Type) -> str:
    shader_entry_points = { 
        Shader_Type.VERTEX_SHADER : "vs_main",
        Shader_Type.COMPUTER_SHADER : "cs_main",
        Shader_Type.DOMAIN_SHADER : "ds_main",
        Shader_Type.GEOMETRY_SHADER : "gs_main",
        Shader_Type.PIXEL_SHADER : "ps_main",
        Shader_Type.HEADER_FILE : ""
    }

    return shader_entry_points[shader_type]

def get_profile(shader_type: Shader_Type) -> str:
    shader_entry_points = { 
        Shader_Type.VERTEX_SHADER : "vs_5_0",
        Shader_Type.COMPUTER_SHADER : "cs_5_0",
        Shader_Type.DOMAIN_SHADER : "ds_5_0",
        Shader_Type.GEOMETRY_SHADER : "gs_5_0",
        Shader_Type.PIXEL_SHADER : "ps_5_0",
        Shader_Type.HEADER_FILE : "hs_5_0"
    }

    return shader_entry_points[shader_type]


def get_output_file_name(shader_name: str, shader_type : Shader_Type) -> str:
    file_prefix = { 
        Shader_Type.VERTEX_SHADER : "_vs",
        Shader_Type.COMPUTER_SHADER : "_cs",
        Shader_Type.DOMAIN_SHADER : "_ds",
        Shader_Type.GEOMETRY_SHADER : "_gs",
        Shader_Type.PIXEL_SHADER : "_ps",
        Shader_Type.HEADER_FILE : "_hs"
    }
    name = shader_name.split(".")[0]
    return name + file_prefix[shader_type] + ".cso"


class Shader_File:
    def __init__(self, name, *args):
        self.name : str = name
        self.shader_types: [Shader_Type] = [item for item in args if isinstance(item, Shader_Type)]



shader_files = [
    Shader_File("globals.hlsl", Shader_Type.HEADER_FILE),
    Shader_File("vertex.hlsl", Shader_Type.HEADER_FILE),
    Shader_File("utils.hlsl", Shader_Type.HEADER_FILE),
    Shader_File("render_2d.hlsl", Shader_Type.VERTEX_SHADER, Shader_Type.PIXEL_SHADER),
    Shader_File("outlining.hlsl", Shader_Type.VERTEX_SHADER, Shader_Type.PIXEL_SHADER),
    Shader_File("forward_light.hlsl", Shader_Type.VERTEX_SHADER, Shader_Type.PIXEL_SHADER),
    Shader_File("draw_lines.hlsl", Shader_Type.VERTEX_SHADER, Shader_Type.PIXEL_SHADER),
    Shader_File("depth_map.hlsl", Shader_Type.VERTEX_SHADER),
    Shader_File("debug_cascaded_shadows.hlsl", Shader_Type.VERTEX_SHADER, Shader_Type.PIXEL_SHADER),
    Shader_File("draw_vertices.hlsl", Shader_Type.VERTEX_SHADER, Shader_Type.PIXEL_SHADER)
]


def find_shader_file_in_list(shader_name : str):
    for shader_file in shader_files:
        if shader_name == shader_file.name:
            return shader_file
    return None


def compile_hlsl_shaders():
    compiled_shaders : str = []
    
    for shader_file in shader_files:
        
        if len(shader_file.shader_types) == 0:
            print("[tool.py] Warning: Type of shader file with name {} was not specified", shader_file.name)
            continue

        full_path_to_shader = os.path.join(PROJECT_DIR, HLSH_DIR, shader_file.name)

        for shader_type in shader_file.shader_types:

            if shader_type == Shader_Type.HEADER_FILE:
                continue
            
            profile = get_profile(shader_type)
            entiry_point = get_entry_point(shader_type)
            output_shader_file_name = get_output_file_name(shader_file.name, shader_type)
            
            full_path_to_output_file = os.path.join(PROJECT_DIR, OUTPUT_DIR, output_shader_file_name)

            result = subprocess.run(f'fxc /E {entiry_point} /Od /Zi /Zpr /T {profile} /Fo "{full_path_to_output_file}" "{full_path_to_shader}"', shell=True, stdout=subprocess.PIPE)
            
            if result.returncode != 0:
                print(f"Shader: {shader_file.name}")
                exit()
            
            print(f"Shader: {shader_file.name} Type: {str(shader_type)} was succeeded compiled.")

        compiled_shaders.append(shader_file.name)

    
    all_shader_files = [file for file in os.listdir(os.path.join(PROJECT_DIR, HLSH_DIR)) if os.path.isfile(os.path.join(PROJECT_DIR, HLSH_DIR, file))]
    
    not_compiled_shaders = set(all_shader_files) - set(compiled_shaders)

    if len(not_compiled_shaders) > 0:
        print("\n")
        
        for not_compiled_shader in not_compiled_shaders:
            print(f"Shader: {not_compiled_shader} was not compiled.")


def compile_specific_shaders(shader_list : list[str]):

    for shader_file_name in shader_list:

        shader_file = find_shader_file_in_list(shader_file_name)

        if shader_file is None:
            print(f"Error: shader with name {shader_file_name} was not found in file shader list.")
            continue

        full_path_to_shader = os.path.join(PROJECT_DIR, HLSH_DIR, shader_file.name)
        
        for shader_type in shader_file.shader_types:

            if shader_type == Shader_Type.HEADER_FILE:
                continue
                    
            profile = get_profile(shader_type)
            entiry_point = get_entry_point(shader_type)
            output_shader_file_name = get_output_file_name(shader_file.name, shader_type)
                    
            full_path_to_output_file = os.path.join(PROJECT_DIR, OUTPUT_DIR, output_shader_file_name)

            result = subprocess.run(f'fxc /E {entiry_point} /Od /Zi /Zpr /T {profile} /Fo "{full_path_to_output_file}" "{full_path_to_shader}"', shell=True, stdout=subprocess.PIPE)
                    
            if result.returncode != 0:
                print(f"Shader: {shader_file.name}")
                exit()
                    
            print(f"Shader: {shader_file.name} Type: {str(shader_type)} was succeeded compiled.")


if __name__ == "__main__":
    
    if (len(sys.argv) > 1):
        compile_specific_shaders(sys.argv[1:])
    else:
        compile_hlsl_shaders()