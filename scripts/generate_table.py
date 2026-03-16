import os
import sys

script_dir   = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.dirname(script_dir)

syscall_dir     = os.path.join(project_root, "src", "syscall")
tbl_path        = os.path.join(syscall_dir, "syscall.tbl")
kernel_generated = os.path.join(project_root, "src", "generated")
user_headers    = os.path.join(project_root, "include", "uapi")

def main():
    try:
        os.makedirs(kernel_generated, exist_ok=True)
        os.makedirs(user_headers, exist_ok=True)
        
        syscalls = {}
        with open(tbl_path, "r") as file:
            content = file.readlines()
            for line in content:
                clean_line = line.strip()
                if clean_line and not clean_line.startswith("#"): # Optionnel: ignorer les commentaires
                    parts = clean_line.split()
                    if len(parts) != 3:
                        raise ValueError(f'Bad format in syscall.tbl: {clean_line}')
                    
                    syscall_id, name, sys_entry = parts

                    if not syscall_id.isdigit():
                        raise ValueError(f"ID must be an integer: {syscall_id}")
                    
                    id_int = int(syscall_id)
                    if id_int > 200:
                        raise ValueError(f"ID must be < 200: {id_int}")

                    if not name.isidentifier() or not sys_entry.isidentifier():
                        raise ValueError(f"Invalid C identifier: {name} or {sys_entry}")
                    
                    syscalls[id_int] = {
                        'name': name,
                        'sys_entry': sys_entry
                    }
        
        with open(f"{kernel_generated}/syscall_table.c", "w") as sys_table, \
             open(f"{user_headers}/syscall.h", "w") as sys_user_header:
            
            sys_table.write("/* auto-generated FILE - do not edit */\n\n")
            sys_table.write("#include <arch/trap_frame.h>\n")
            sys_table.write("#include <syscall/syscall.h>\n\n")
            # sys_table.write("#define MAX_SYSCALL 200 \n\n")

            for syscall_id in sorted(syscalls.keys()):
                sys_entry = syscalls[syscall_id]['sys_entry']
                sys_table.write(f"extern long {sys_entry}();\n")
            
            sys_table.write("\n")

            sys_user_header.write("/* auto-generated FILE - do not edit */\n\n")
            sys_user_header.write("#pragma once\n\n")
            
            for syscall_id in sorted(syscalls.keys()):
                sys_name = syscalls[syscall_id]['name']
                sys_user_header.write(f"#define __NR_{sys_name} {syscall_id}\n")

            sys_table.write("const syscallHandler syscall_table[MAX_SYSCALL] = {\n")
            
            for syscall_id in sorted(syscalls.keys()):
                sys_entry = syscalls[syscall_id]['sys_entry']
                sys_table.write(f"    [{syscall_id}] = (syscallHandler){sys_entry},\n")

            sys_table.write("};\n")
        sys.exit(0)
    except FileNotFoundError as e:
        sys.stderr.write(f"Error: Configuration file not found: {e}\n")
    except ValueError as e:
        sys.stderr.write(f"Error: Invalid data in syscall.tbl: {e}\n")
    except PermissionError as e:
        sys.stderr.write(f"Error: Permission denied during file generation: {e}\n")
    except Exception as e:
        sys.stderr.write(f"Error: Unexpected failure: {type(e).__name__} - {e}\n")
    
    sys.exit(1)


if __name__ == "__main__":
    main()