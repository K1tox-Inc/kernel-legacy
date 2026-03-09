syscall_dir = "../src/syscall"

def main():
    try:
        with open(f"{syscall_dir}/syscall_table.c", "w") as sys_header:
            sys_header.write("/* auto-generated FILE - do not edit */\n\n")
            sys_header.write("void *syscall_table[] = {\n")
            file = open(f'{syscall_dir}/syscall.tbl')
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
                    
                    if not name.isidentifier():
                        raise ValueError("Bad value: syscall.tbl: name id must be a valid identifier in C")
                        
                    if not sys_entry.isidentifier():
                        raise ValueError("Bad value: syscall.tbl: sys_entry id must be a valid identifier in C")

                    sys_header.write(f"    [{syscall_id}] = {sys_entry},\n")
            sys_header.write("};\n")
            
    except FileNotFoundError as e:
        print(f"FileNotFoundError: {str(e)}")
    except TypeError as e:
        print(f"TypeError: {str(e)}")
    except BaseException as e:
        print(f"An exception has been caught: {type(e).__name__} - {str(e)}")


if __name__ == "__main__":
    main()