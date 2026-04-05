#pragma once

/*
 * Exit code layout :
 * 	|  exit code  | signal + flags |
 *  |  bits 15-8  |   bits 7-0     |
 */

#define WEXITSTATUS(status) (((status) >> 8) & 0xff)
#define WIFEXITED(status)   (((status) & 0x7f) == 0)
#define WIFSIGNALED(status) (((status) & 0x7f) != 0)
#define WTERMSIG(status)    ((status) & 0x7f)
