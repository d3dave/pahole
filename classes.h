#ifndef _PAHOLE_CLASSES_H_
#define _PAHOLE_CLASSES_H_ 1
/* 
  Copyright (C) 2006 Mandriva Conectiva S.A.
  Copyright (C) 2006 Arnaldo Carvalho de Melo <acme@mandriva.com>

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.
*/


#include <stdint.h>

#include "list.h"

struct cus {
	struct list_head cus;
	const char	 *filename;
};

struct cu {
	struct list_head node;
	struct list_head classes;
	struct list_head variables;
	const char	 *name;
	unsigned int	 id;
	unsigned long	 nr_inline_expansions;
	unsigned long	 size_inline_expansions;
	unsigned int	 nr_functions_changed;
	unsigned int	 nr_structures_changed;
	size_t		 max_len_changed_item;
	size_t		 function_bytes_added;
	size_t		 function_bytes_removed;
};

struct class {
	struct list_head node;
	struct list_head members;
	struct list_head inline_expansions;
	struct list_head variables;
	struct cu	 *cu;
	const char	 *name;
	uint64_t	 size;
	uint64_t	 id;
	uint64_t	 type;
	uint64_t	 low_pc;
	uint64_t	 high_pc;
	uint64_t	 nr_entries;	/* For arrays */
	unsigned int	 tag;		/* struct, union, base type, etc */
	const char	 *decl_file;
	unsigned int	 decl_line;
	unsigned short	 nr_members;
	unsigned short	 nr_holes;
	unsigned short	 nr_labels;
	unsigned short	 nr_variables;
	unsigned short	 padding;
	unsigned short	 inlined;
	unsigned short	 nr_inline_expansions;
	unsigned int	 refcnt;
	unsigned int	 size_inline_expansions;
	signed int	 diff;
	unsigned int	 cu_total_nr_inline_expansions;
	unsigned long	 cu_total_size_inline_expansions;
	struct class	 *class_to_diff;
};

struct class_member {
	struct list_head node;
	char		 *name;
	uint64_t	 type;
	uint64_t	 offset;
	unsigned int	 bit_size;
	unsigned int	 bit_offset;
	unsigned char	 visited:1;
	unsigned short	 hole;		/* If there is a hole before the next
					   one (or the end of the struct) */
};

struct variable {
	struct list_head cu_node;
	struct list_head class_node;
	char		 *name;
	uint64_t	 id;
	uint64_t	 type;
	uint64_t	 abstract_origin;
};

struct inline_expansion {
	struct list_head node;
	uint64_t	 type;
	uint64_t	 size;
};

#define DEFAULT_CACHELINE_SIZE 32

extern void class__find_holes(struct class *self, const struct cu *cu);
extern void class__print(struct class *self, const struct cu *cu);

extern struct cus   *cus__new(const char *filename);
extern int	    cus__load(struct cus *self);
extern struct cu    *cus__find_cu_by_name(struct cus *self, const char *name);
extern struct class *cu__find_class_by_id(const struct cu *cu,
					  const uint64_t type);
extern struct class *cu__find_class_by_name(struct cu *cu, const char *name);
extern int	    class__is_struct(const struct class *self,
				     struct cu *cu,
				     struct class **typedef_alias);
extern void	    cus__print_classes(struct cus *cus,
				       const unsigned int tag);
extern void	    class__print_inline_expansions(struct class *self,
						   const struct cu *cu);
extern void	    class__print_variables(struct class *self,
					   const struct cu *cu);
extern struct class *cus__find_class_by_name(struct cus *self, struct cu **cu,
					     const char *name);
extern void	    cu__account_inline_expansions(struct cu *self);
extern int	    cu__for_each_class(struct cu *cu,
				       int (*iterator)(struct cu *cu,
						       struct class *class,
						       void *cookie),
				       void *cookie);
extern void	    cus__for_each_cu(struct cus *self,
				     int (*iterator)(struct cu *cu,
						     void *cookie),
				      void *cookie);

static inline uint32_t class__function_size(const struct class *self)
{
	return self->high_pc - self->low_pc;
}

extern struct class_member *class__find_member_by_name(const struct class *self,
						       const char *name);

extern uint64_t class_member__names(const struct class_member *self,
				    const struct cu *cu,
				    char *class_name,
				    size_t class_name_size,
				    char *member_name,
				    size_t member_name_size);
extern unsigned int cacheline_size;

#endif /* _PAHOLE_CLASSES_H_ */
