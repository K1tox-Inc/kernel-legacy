import os

script_dir   = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.dirname(script_dir)

syscall_dir     = os.path.join(project_root, "src", "syscall")
tbl_path        = os.path.join(syscall_dir, "syscall.tbl")
kernel_generated = os.path.join(project_root, "src", "generated")
user_headers    = os.path.join(project_root, "lib", "includes", "generated")

def main():
    try:
        os.makedirs(kernel_generated, exist_ok=True)
        os.makedirs(user_headers, exist_ok=True)
        
        syscalls = {}
        file = open(tbl_path)
        content = file.readlines()
        for line in content:
            clean_line = line.strip()
            if clean_line:
                parts = clean_line.split()
                if len(parts) != 3:
                    raise ValueError('Bad format: syscall.tbl')
                syscall_id, name, sys_entry = parts

                if not syscall_id.isdigit():
                    raise ValueError("Bad value: syscall.tbl: syscall id must be an integer")
                elif int(syscall_id) > 200:
                    raise ValueError("Bad value: syscall.tbl: syscall id must be lower than 200")

                if not name.isidentifier():
                    raise ValueError("Bad value: syscall.tbl: name id must be a valid identifier in C")
                    
                if not sys_entry.isidentifier():
                    raise ValueError("Bad value: syscall.tbl: sys_entry id must be a valid identifier in C")
                
                syscalls[int(syscall_id)] = {
                    'name': name,
                    'sys_entry': sys_entry
                }
        
        with open(f"{kernel_generated}/syscall_table.c", "w") as sys_table, \
             open(f"{user_headers}/syscall.h", "w") as sys_user_header:
            
            sys_table.write("/* auto-generated FILE - do not edit */\n\n")
            sys_table.write("#include <arch/trap_frame.h> \n")
            sys_table.write("#include <syscall/syscall.h> \n\n")
            # sys_table.write("#define MAX_SYSCALL 200 \n\n")

            for syscall_id in sorted(syscalls.keys()):
                sys_entry = syscalls[syscall_id]['sys_entry']
                sys_table.write(f"extern long {sys_entry}(struct trap_frame *tf);\n")
            
            sys_table.write("\n")

            sys_user_header.write("/* auto-generated FILE - do not edit */\n\n")
            sys_user_header.write("#pragma once\n\n")
            
            for syscall_id in sorted(syscalls.keys()):
                sys_entry = syscalls[syscall_id]['sys_entry']
                sys_user_header.write(f"#define {sys_entry} {syscall_id}\n")

            sys_table.write("const void *syscall_table[MAX_SYSCALL + 1] = {\n")
            
            for syscall_id in sorted(syscalls.keys()):
                sys_entry = syscalls[syscall_id]['sys_entry']
                sys_table.write(f"    [{syscall_id}] = {sys_entry},\n")

            sys_table.write("};\n")
            
    except FileNotFoundError as e:
        print(f"FileNotFoundError: {str(e)}")
    except TypeError as e:
        print(f"TypeError: {str(e)}")
    except BaseException as e:
        print(f"An exception has been caught: {type(e).__name__} - {str(e)}")


if __name__ == "__main__":
    main()