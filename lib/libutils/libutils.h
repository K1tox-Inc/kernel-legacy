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

#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <types.h>

void        ft_free_strs(char **tab);
char       *ft_itoa_base(unsigned long long nb, char *base);
char       *ft_itoa(int n);
char       *ft_join_and_free(char *s1, char *s2);
char      **ft_split(char const *str, char set);
char       *ft_strdup(const char *src);
char       *ft_strjoin(char const *s1, char const *s2);
char       *ft_strmapi(char const *s, char (*f)(unsigned int, char));
char       *ft_strtrim(char const *s, char const *charset);
char       *ft_substr(char const *s, unsigned int start, size_t len);
void       *ft_calloc(size_t count, size_t size);
const char *ft_strerror(int errcode);
