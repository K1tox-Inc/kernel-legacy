/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_split.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jbettini <jbettini@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/10/20 18:27:01 by jbettini          #+#    #+#             */
/*   Updated: 2024/04/16 17:27:54 by jbettini         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libutils.h"

static size_t ft_strlen(const char *s)
{
	size_t i;

	i = 0;
	while (s[i])
		i++;
	return (i);
}

static char *ft_strndup(char const *s, size_t n)
{
	char  *dest;
	size_t i;

	i    = 0;
	dest = (char *)kmalloc(n + 1, GFP_KERNEL);
	if (dest == NULL)
		return (NULL);
	while (i < n) {
		dest[i] = s[i];
		i++;
	}
	dest[i] = '\0';
	return (dest);
}

static int ft_countwords(char *str, char set)
{
	int i;
	int len;

	len = 0;
	i   = 0;
	if (ft_strlen(str) == 0)
		return (0);
	if (str[0] != set)
		len++;
	while (str[i]) {
		if (str[i] == set) {
			if (str[i] == set && (str[i + 1] != set && str[i + 1] != '\0'))
				len++;
		}
		i++;
	}
	return (len);
}

char **ft_split(char const *str, char set)
{
	char **tab;
	int    i;
	int    m_tab;
	int    len_word;

	m_tab = 0;
	i     = -1;
	if (!str)
		return (NULL);
	tab = (char **)kmalloc(sizeof(tab) * ft_countwords((char *)str, set) + 1, GFP_KERNEL);
	if (!tab)
		return (NULL);

	while (str[++i]) {
		len_word = 0;
		if (str[i] != set) {
			while (str[i + len_word] != set && str[i + len_word] != '\0')
				len_word++;
			tab[m_tab++] = ft_strndup(str + i, len_word);
			i            = i + len_word - 1;
		}
	}

	tab[m_tab] = NULL;

	return (tab);
}
