/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   libk.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: noalexan <noalexan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/01 04:35:08 by noalexan          #+#    #+#             */
/*   Updated: 2025/01/13 15:24:19 by noalexan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <types.h>

extern int    ft_isalpha(int c);
extern int    ft_isdigit(int c);
extern int    ft_isalnum(int c);
extern int    ft_isascii(int c);
extern int    ft_isprint(int c);
extern size_t ft_strlen(const char *s);
extern void  *ft_memset(void *s, int c, size_t n);
extern void   ft_bzero(void *s, size_t n);
extern void  *ft_memcpy(void *dest, const void *src, size_t n);
extern void  *ft_memmove(void *dest, const void *src, size_t n);
extern size_t ft_strlcpy(char *dst, const char *src, size_t dsize);
extern int    ft_toupper(int c);
extern int    ft_tolower(int c);
extern char  *ft_strchr(const char *s, int c);
extern char  *ft_strrchr(const char *s, int c);
extern int    ft_strncmp(const char *s1, const char *s2, size_t n);
extern void  *ft_memchr(const void *s, int c, size_t n);
extern int    ft_memcmp(const void *s1, const void *s2, size_t n);
extern size_t ft_strlcat(char *dst, const char *src, size_t dsize);
extern char  *ft_strnstr(const char *str, const char *seg, size_t len);
extern int    ft_atoi(const char *nptr);
extern size_t ft_strslen(char **str);
extern void   ft_striteri(char *s, void (*f)(unsigned int, char *));
extern bool   ft_strequ(const char *s1, const char *s2);
