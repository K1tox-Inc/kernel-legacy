#include <utils/error.h>

const char *ft_strerror(int errcode)
{
	switch (-errcode) {
	case SUCCESS:
		return "Success";
	case EPERM:
		return "Operation not permitted";
	case ENOENT:
		return "No such file or directory";
	case ESRCH:
		return "No such process";
	case EINTR:
		return "Interrupted system call";
	case EIO:
		return "I/O error";
	case ENXIO:
		return "No such device or address";
	case E2BIG:
		return "Argument list too long";
	case ENOEXEC:
		return "Exec format error";
	case EBADF:
		return "Bad file number";
	case ECHILD:
		return "No child processes";
	case EAGAIN:
		return "Try again / Resource temporarily unavailable";
	case ENOMEM:
		return "Out of memory";
	case EACCES:
		return "Permission denied";
	case EFAULT:
		return "Bad address";
	case ENOTBLK:
		return "Block device required";
	case EBUSY:
		return "Device or resource busy";
	case EEXIST:
		return "File exists";
	case EXDEV:
		return "Cross-device link";
	case ENODEV:
		return "No such device";
	case ENOTDIR:
		return "Not a directory";
	case EISDIR:
		return "Is a directory";
	case EINVAL:
		return "Invalid argument";
	case ENFILE:
		return "File table overflow";
	case EMFILE:
		return "Too many open files";
	case ENOTTY:
		return "Not a typewriter (Inappropriate ioctl)";
	case ETXTBSY:
		return "Text file busy";
	case EFBIG:
		return "File too large";
	case ENOSPC:
		return "No space left on device";
	case ESPIPE:
		return "Illegal seek";
	case EROFS:
		return "Read-only file system";
	case EMLINK:
		return "Too many links";
	case EPIPE:
		return "Broken pipe";
	case EDOM:
		return "Math argument out of domain of func";
	case ERANGE:
		return "Math result not representable";
	case EDEADLK:
		return "Resource deadlock would occur";
	case ENAMETOOLONG:
		return "File name too long";
	case ENOLCK:
		return "No record locks available";
	case ENOSYS:
		return "Invalid system call number";
	case ENOTEMPTY:
		return "Directory not empty";
	case ELOOP:
		return "Too many symbolic links encountered";
	case ERESTART:
		return "Interrupted system call should be restarted";
	case ENOSR:
		return "Out of streams resources";
	case EUNREACH:
		return "Code is unreachable";
	default:
		return "Unknown error";
	}
}
