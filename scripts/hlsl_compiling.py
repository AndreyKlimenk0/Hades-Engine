import os
import sys
import enum
import subprocess

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT_DIR = os.path.join("data", "shaders")
PDB_FILES_DIR = os.path.join("build", "shader", "debug");
HLSH_DIR = "hlsl"
HLSL_VERSION = "5_0"


 # fxc /E vs_main /Od /Zi /T vs_5_0 /Fo PixelShader1.fxc demo.hlsl
 
 
class Compilation_Mode(enum.Enum):
    DEBUG = 0;
    RELEASE = 1;


class Shader_Type(enum.Enum):
    VERTEX_SHADER = 1
    COMPUTE_SHADER = 2
    DOMAIN_SHADER = 3
    GEOMETRY_SHADER = 4
    PIXEL_SHADER = 5
 
    
class Shader_File:
    def __init__(self, name, *args):
        self.name : str = name
        self.shader_types: [Shader_Type] = [item for item in args if isinstance(item, Shader_Type)]


class Compilation_Params:
    def __init__(self, compilation_mode : Compilation_Mode, shader_file_list : list[Shader_File]):
        self.compilation_mode = compilation_mode
        self.shader_file_list = shader_file_list


shader_files = [
    Shader_File("render_2d.hlsl", Shader_Type.VERTEX_SHADER, Shader_Type.PIXEL_SHADER),
    Shader_File("forward_light.hlsl", Shader_Type.VERTEX_SHADER, Shader_Type.PIXEL_SHADER),
    Shader_File("depth_map.hlsl", Shader_Type.VERTEX_SHADER),
    Shader_File("debug_cascaded_shadows.hlsl", Shader_Type.VERTEX_SHADER, Shader_Type.PIXEL_SHADER),
    Shader_File("draw_vertices.hlsl", Shader_Type.VERTEX_SHADER, Shader_Type.PIXEL_SHADER),
    Shader_File("silhouette.hlsl", Shader_Type.VERTEX_SHADER, Shader_Type.PIXEL_SHADER),
    Shader_File("outlining.hlsl", Shader_Type.COMPUTE_SHADER),
    Shader_File("voxelization.hlsl", Shader_Type.VERTEX_SHADER, Shader_Type.GEOMETRY_SHADER, Shader_Type.PIXEL_SHADER),
    Shader_File("draw_box.hlsl", Shader_Type.VERTEX_SHADER, Shader_Type.PIXEL_SHADER)
]


def get_entry_point(shader_type: Shader_Type) -> str:
    shader_entry_points = { 
        Shader_Type.VERTEX_SHADER : "vs_main",
        Shader_Type.COMPUTE_SHADER : "cs_main",
        Shader_Type.DOMAIN_SHADER : "ds_main",
        Shader_Type.GEOMETRY_SHADER : "gs_main",
        Shader_Type.PIXEL_SHADER : "ps_main",
    }

    return shader_entry_points[shader_type]

def get_profile(shader_type: Shader_Type) -> str:
    shader_entry_points = { 
        Shader_Type.VERTEX_SHADER : "vs_" + HLSL_VERSION,
        Shader_Type.COMPUTE_SHADER : "cs_" + HLSL_VERSION,
        Shader_Type.DOMAIN_SHADER : "ds_" + HLSL_VERSION,
        Shader_Type.GEOMETRY_SHADER : "gs_" + HLSL_VERSION,
        Shader_Type.PIXEL_SHADER : "ps_" + HLSL_VERSION,
    }

    return shader_entry_points[shader_type]


def get_output_file_name(shader_name: str, shader_type : Shader_Type) -> str:
    file_prefix = { 
        Shader_Type.VERTEX_SHADER : "_vs",
        Shader_Type.COMPUTE_SHADER : "_cs",
        Shader_Type.DOMAIN_SHADER : "_ds",
        Shader_Type.GEOMETRY_SHADER : "_gs",
        Shader_Type.PIXEL_SHADER : "_ps",
    }
    name = shader_name.split(".")[0]
    return name + file_prefix[shader_type] + ".cso"


def find_shader_file_in_list(shader_name : str):
    for shader_file in shader_files:
        if shader_name == shader_file.name:
            return shader_file
    return None


def build_fxc_command_line_params(shader_name: str, profile : str, entry_point : str, full_path_to_output_file : str, full_path_to_shader : str, compilation_mode = Compilation_Mode.DEBUG) -> (bool, str):
    row_major_oder_matrices = "/Zpr"
    if compilation_mode == Compilation_Mode.DEBUG:
        path_to_shader_pdb_file = os.path.join(PROJECT_DIR, PDB_FILES_DIR, shader_name.replace("hlsl", "pdb"))
        skip_optimization = "/Od"
        enable_debugging_information = "/Zi"
        strip_reflection = "/Qstrip_reflect"
        shader_debug_file = f'/Fd "{path_to_shader_pdb_file}"'
        command_line_params = f'"{full_path_to_shader}" /E {entry_point} /T {profile} /Fo "{full_path_to_output_file}" {skip_optimization} {row_major_oder_matrices} {enable_debugging_information} {strip_reflection} {shader_debug_file}'
        return (True, command_line_params)
    
    elif compilation_mode == Compilation_Mode.RELEASE:
        optimization = "/O0"
        command_line_params = f'"{full_path_to_shader}" /E {entry_point} /T {profile} /Fo "{full_path_to_output_file}" {row_major_oder_matrices} {optimization}'
        return (True, command_line_params)
    else:
        return (False, "")


def parse_command_line_args(command_line_args : list[str]) -> Compilation_Params:
    shader_file_list = []
    compilation_mode = Compilation_Mode.DEBUG
    for arg in command_line_args:
        if arg.endswith(".hlsl"):
            shader_file = find_shader_file_in_list(arg)
            if shader_file is not None:
                shader_file_list.append(shader_file)
            else:
                print("Error: Shader file '{}' was not found in shader file list.".format(arg))
        elif arg.lower() == "release":
            compilation_mode = Compilation_Mode.RELEASE
        else:
            print("Warning: '{}' is an unknown command line argument.".format(arg))
    if len(shader_file_list) == 0:
        shader_file_list = shader_files
    return Compilation_Params(compilation_mode, shader_file_list)
            

def compile_hlsl_shaders(compilation_params : Compilation_Params):
    for shader_file in compilation_params.shader_file_list:
        if len(shader_file.shader_types) == 0:
            print("Error: Type of shader file with name {} was not specified", shader_file.name)
            continue

        full_path_to_shader = os.path.join(PROJECT_DIR, HLSH_DIR, shader_file.name)
        
        for shader_type in shader_file.shader_types:
            profile = get_profile(shader_type)
            entiry_point = get_entry_point(shader_type)
            output_shader_file_name = get_output_file_name(shader_file.name, shader_type)     
            full_path_to_output_file = os.path.join(PROJECT_DIR, OUTPUT_DIR, output_shader_file_name)
            
            result, command_line_params = build_fxc_command_line_params(shader_file.name, profile, entiry_point, full_path_to_output_file, full_path_to_shader, compilation_params.compilation_mode)
            if result:
                result = subprocess.run(f'fxc {command_line_params}', shell=True, stdout=subprocess.PIPE)  
                if result.returncode != 0:
                    print(f"Shader: {shader_file.name} Type: {str(shader_type)} was not compiled.")
                else:
                    print(f"Shader: {shader_file.name} Type: {str(shader_type)} was succeeded compiled.")
            else:
                print(f"[Shader: {shader_file.name} Type: {str(shader_type)}] Failed to build command line params for fxc.exe.")


if __name__ == "__main__":
        compilation_params = parse_command_line_args(sys.argv[1:])
        compile_hlsl_shaders(compilation_params)
