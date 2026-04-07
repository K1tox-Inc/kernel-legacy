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

int    ft_isalpha(int c);
int    ft_isdigit(int c);
int    ft_isalnum(int c);
int    ft_isascii(int c);
int    ft_isprint(int c);
size_t ft_strlen(const char *s);
void  *ft_memset(void *s, int c, size_t n);
void   ft_bzero(void *s, size_t n);
void  *ft_memcpy(void *dest, const void *src, size_t n);
void  *ft_memmove(void *dest, const void *src, size_t n);
size_t ft_strlcpy(char *dst, const char *src, size_t dsize);
int    ft_toupper(int c);
int    ft_tolower(int c);
char  *ft_strchr(const char *s, int c);
char  *ft_strrchr(const char *s, int c);
int    ft_strncmp(const char *s1, const char *s2, size_t n);
void  *ft_memchr(const void *s, int c, size_t n);
int    ft_memcmp(const void *s1, const void *s2, size_t n);
size_t ft_strlcat(char *dst, const char *src, size_t dsize);
char  *ft_strnstr(const char *str, const char *seg, size_t len);
int    ft_atoi(const char *nptr);
size_t ft_strslen(char **str);
void   ft_striteri(char *s, void (*f)(unsigned int, char *));
bool   ft_strequ(const char *s1, const char *s2);
