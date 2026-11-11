/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_free_split.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jbettini <jbettini@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/12/16 16:39:41 by jbettini          #+#    #+#             */
/*   Updated: 2024/03/22 22:51:47 by jbettini         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libutils.h"

void ft_free_strs(char **tab)
{
	size_t i;

	i = -1;
	while (tab[++i])
		kfree(tab[i]);
	kfree(tab);
	tab = NULL;
}
