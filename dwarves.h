#ifndef _DWARVES_H_
#define _DWARVES_H_ 1
/* 
  Copyright (C) 2006 Mandriva Conectiva S.A.
  Copyright (C) 2006 Arnaldo Carvalho de Melo <acme@mandriva.com>

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.
*/


#include <stdint.h>
#include <stdio.h>
#include <dwarf.h>
#include <elfutils/libdw.h>

#include "list.h"
#include "strings.h"

extern struct strings *strings;

struct cus {
	struct list_head      cus;
};

struct cus *cus__new(void);

#define HASHTAGS__BITS 8
#define HASHTAGS__SIZE (1UL << HASHTAGS__BITS)

struct cu {
	struct list_head node;
	struct list_head tags;
	struct list_head hash_tags[HASHTAGS__SIZE];
	struct list_head tool_list;	/* To be used by tools such as ctracer */
	const char	 *name;
	uint8_t		 addr_size;
	uint16_t	 language;
	unsigned long	 nr_inline_expansions;
	size_t		 size_inline_expansions;
	uint32_t	 nr_functions_changed;
	uint32_t	 nr_structures_changed;
	size_t		 max_len_changed_item;
	size_t		 function_bytes_added;
	size_t		 function_bytes_removed;
	int		 build_id_len;
	unsigned char	 build_id[0];
};

struct tag;

void cu__hash(struct cu *cu, struct tag *tag);

struct tag {
	struct list_head node;
	struct list_head hash_node;
	Dwarf_Off	 type;
	Dwarf_Off	 id;
	strings_t	 decl_file;
	uint16_t	 decl_line;
	uint16_t	 tag;
	uint16_t	 visited:1;
	uint16_t	 recursivity_level:15;
};

static inline int tag__is_enumeration(const struct tag *self)
{
	return self->tag == DW_TAG_enumeration_type;
}

static inline int tag__is_namespace(const struct tag *self)
{
	return self->tag == DW_TAG_namespace;
}

static inline int tag__is_struct(const struct tag *self)
{
	return self->tag == DW_TAG_structure_type ||
	       self->tag == DW_TAG_class_type;
}

static inline int tag__is_typedef(const struct tag *self)
{
	return self->tag == DW_TAG_typedef;
}

static inline int tag__is_union(const struct tag *self)
{
	return self->tag == DW_TAG_union_type;
}

static inline bool tag__has_namespace(const struct tag *self)
{
	return tag__is_struct(self) ||
	       tag__is_union(self) ||
	       tag__is_namespace(self) ||
	       tag__is_enumeration(self);
}

/**
 * tag__is_tag_type - is this tag derived from the 'type' class?
 * @tag - tag queried
 */
static inline int tag__is_type(const struct tag *self)
{
	return tag__is_union(self)   ||
	       tag__is_struct(self)  ||
	       tag__is_typedef(self) ||
	       tag__is_enumeration(self);
}
 
/**
 * tag__is_tag_type - is this one of the possible types for a tag?
 * @tag - tag queried
 */
static inline int tag__is_tag_type(const struct tag *self)
{
	return tag__is_type(self) ||
	       tag__is_enumeration(self) ||
	       self->tag == DW_TAG_array_type ||
	       self->tag == DW_TAG_base_type ||
	       self->tag == DW_TAG_const_type ||
	       self->tag == DW_TAG_pointer_type ||
	       self->tag == DW_TAG_reference_type ||
	       self->tag == DW_TAG_subroutine_type ||
	       self->tag == DW_TAG_volatile_type;
}

static inline const char *tag__decl_file(const struct tag *self)
{
	return strings__ptr(strings, self->decl_file);
}
 
struct ptr_to_member_type {
	struct tag tag;
	Dwarf_Off  containing_type;
};

static inline struct ptr_to_member_type *
		tag__ptr_to_member_type(const struct tag *self)
{
	return (struct ptr_to_member_type *)self;
}
 
struct namespace {
	struct tag	 tag;
	strings_t	 name;
	uint16_t	 nr_tags;
	struct list_head tags;
};

static inline struct namespace *tag__namespace(const struct tag *self)
{
	return (struct namespace *)self;
}

/**
 * namespace__for_each_tag - iterate thru all the tags
 * @self: struct namespace instance to iterate
 * @pos: struct tag iterator
 */
#define namespace__for_each_tag(self, pos) \
	list_for_each_entry(pos, &(self)->tags, node)

/**
 * namespace__for_each_tag_safe - safely iterate thru all the tags
 * @self: struct namespace instance to iterate
 * @pos: struct tag iterator
 * @n: struct class_member temp iterator
 */
#define namespace__for_each_tag_safe(self, pos, n) \
	list_for_each_entry_safe(pos, n, &(self)->tags, node)

/**
 * struct type - base type for enumerations, structs and unions
 *
 * @nr_members: number of DW_TAG_member entries
 * @nr_tags: number of tags
 */
struct type {
	struct namespace namespace;
	struct list_head node;
	Dwarf_Off	 specification;
	size_t		 size;
	size_t		 size_diff;
	uint16_t	 nr_members;
	uint8_t		 declaration; /* only one bit used */
	uint8_t		 definition_emitted:1;
	uint8_t		 fwd_decl_emitted:1;
	uint8_t		 resized:1;
};

static inline struct class *type__class(const struct type *self)
{
	return (struct class *)self;
}

/** 
 * type__for_each_tag - iterate thru all the tags
 * @self: struct type instance to iterate
 * @pos: struct tag iterator
 */
#define type__for_each_tag(self, pos) \
	list_for_each_entry(pos, &(self)->namespace.tags, node)

/** 
 * type__for_each_enumerator - iterate thru the enumerator entries
 * @self: struct type instance to iterate
 * @pos: struct enumerator iterator
 */
#define type__for_each_enumerator(self, pos) \
	list_for_each_entry(pos, &(self)->namespace.tags, tag.node)

/** 
 * type__for_each_member - iterate thru the entries that use space
 *                         (data members and inheritance entries)
 * @self: struct type instance to iterate
 * @pos: struct class_member iterator
 */
#define type__for_each_member(self, pos) \
	list_for_each_entry(pos, &(self)->namespace.tags, tag.node) \
		if (!(pos->tag.tag == DW_TAG_member || \
		      pos->tag.tag == DW_TAG_inheritance)) \
			continue; \
		else

/** 
 * type__for_each_data_member - iterate thru the data member entries
 * @self: struct type instance to iterate
 * @pos: struct class_member iterator
 */
#define type__for_each_data_member(self, pos) \
	list_for_each_entry(pos, &(self)->namespace.tags, tag.node) \
		if (pos->tag.tag != DW_TAG_member) \
			continue; \
		else

/** 
 * type__for_each_member_safe - safely iterate thru the entries that use space
 *                              (data members and inheritance entries)
 * @self: struct type instance to iterate
 * @pos: struct class_member iterator
 * @n: struct class_member temp iterator
 */
#define type__for_each_member_safe(self, pos, n) \
	list_for_each_entry_safe(pos, n, &(self)->namespace.tags, tag.node) \
		if (pos->tag.tag != DW_TAG_member) \
			continue; \
		else

/** 
 * type__for_each_data_member_safe - safely iterate thru the data member entries
 * @self: struct type instance to iterate
 * @pos: struct class_member iterator
 * @n: struct class_member temp iterator
 */
#define type__for_each_data_member_safe(self, pos, n) \
	list_for_each_entry_safe(pos, n, &(self)->namespace.tags, tag.node) \
		if (pos->tag.tag != DW_TAG_member) \
			continue; \
		else

static inline struct type *tag__type(const struct tag *self)
{
	return (struct type *)self;
}

struct class {
	struct type	 type;
	struct list_head vtable;
	uint16_t	 nr_vtable_entries;
	uint8_t		 nr_holes;
	uint8_t		 nr_bit_holes;
	uint16_t	 padding;
	uint8_t		 bit_padding;
	void		 *priv;
};

static inline struct class *tag__class(const struct tag *self)
{
	return (struct class *)self;
}

static inline struct tag *class__tag(const struct class *self)
{
	return (struct tag *)self;
}

extern struct class *class__clone(const struct class *from,
				  const char *new_class_name);
extern void class__delete(struct class *self);

static inline struct list_head *class__tags(struct class *self)
{
	return &self->type.namespace.tags;
}

extern const char *type__name(struct type *self, const struct cu *cu);

static inline const char *class__name(struct class *self,
				      const struct cu *cu)
{
	return type__name(&self->type, cu);
}

static inline int class__is_struct(const struct class *self)
{
	return tag__is_struct(&self->type.namespace.tag);
}

struct base_type {
	struct tag	tag;
	strings_t	name;
	size_t		bit_size;
};

static inline struct base_type *tag__base_type(const struct tag *self)
{
	return (struct base_type *)self;
}

static inline size_t base_type__size(const struct tag *self)
{
	return tag__base_type(self)->bit_size / 8;
}

static inline const char *base_type__name(const struct base_type *self)
{
	return strings__ptr(strings, self->name);
}

struct array_type {
	struct tag	tag;
	uint32_t	*nr_entries;
	uint8_t		dimensions;
};

static inline struct array_type *tag__array_type(const struct tag *self)
{
	return (struct array_type *)self;
}

struct class_member {
	struct tag	 tag;
	strings_t	 name;
	uint32_t	 offset;
	uint8_t		 bit_offset;
	uint8_t		 bit_size;
	uint8_t		 bit_hole;	/* If there is a bit hole before the next
					   one (or the end of the struct) */
	uint8_t		 bitfield_end:1; /* Is this the last entry in a bitfield? */
	uint8_t		 visited:1;
	uint8_t		 accessibility:2; /* DW_ACCESS_{public,protected,private} */
	uint8_t		 virtuality:2; /* DW_VIRTUALITY_{none,virtual,pure_virtual} */
	uint16_t	 hole;		/* If there is a hole before the next
					   one (or the end of the struct) */
};

static inline struct class_member *tag__class_member(const struct tag *self)
{
	return (struct class_member *)self;
}

extern size_t class_member__size(const struct class_member *self,
				 const struct cu *cu);

static inline const char *class_member__name(const struct class_member *self)
{
	return strings__ptr(strings, self->name);
}

extern void class_member__delete(struct class_member *self);

struct lexblock {
	struct tag	 tag;
	struct list_head tags;
	Dwarf_Addr	 low_pc;
	Dwarf_Addr	 high_pc;
	uint16_t	 nr_inline_expansions;
	uint16_t	 nr_labels;
	uint16_t	 nr_variables;
	uint16_t	 nr_lexblocks;
	size_t		 size_inline_expansions;
};

static inline struct lexblock *tag__lexblock(const struct tag *self)
{
	return (struct lexblock *)self;
}

/*
 * tag.tag can be DW_TAG_subprogram_type or DW_TAG_subroutine_type.
 */
struct ftype {
	struct tag	 tag;
	struct list_head parms;
	uint16_t	 nr_parms;
	uint8_t		 unspec_parms; /* just one bit is needed */
};

static inline struct ftype *tag__ftype(const struct tag *self)
{
	return (struct ftype *)self;
}

/** 
 * ftype__for_each_parameter - iterate thru all the parameters
 * @self: struct ftype instance to iterate
 * @pos: struct parameter iterator
 */
#define ftype__for_each_parameter(self, pos) \
	list_for_each_entry(pos, &(self)->parms, tag.node)

struct function {
	struct ftype	 proto;
	struct lexblock	 lexblock;
	Dwarf_Off	 abstract_origin;
	Dwarf_Off	 specification;
	strings_t	 name;
	strings_t	 linkage_name;
	size_t		 cu_total_size_inline_expansions;
	uint16_t	 cu_total_nr_inline_expansions;
	uint8_t		 inlined:2;
	uint8_t		 external:1;
	uint8_t		 accessibility:2; /* DW_ACCESS_{public,protected,private} */
	uint8_t		 virtuality:2; /* DW_VIRTUALITY_{none,virtual,pure_virtual} */
	int16_t		 vtable_entry;
	struct list_head vtable_node;
	/* fields used by tools */
	struct list_head tool_node;
	void		 *priv;
};

static inline struct function *tag__function(const struct tag *self)
{
	return (struct function *)self;
}

static inline struct tag *function__tag(const struct function *self)
{
	return (struct tag *)self;
}

/** 
 * function__for_each_parameter - iterate thru all the parameters
 * @self: struct function instance to iterate
 * @pos: struct parameter iterator
 */
#define function__for_each_parameter(self, pos) \
	ftype__for_each_parameter(&self->proto, pos)

struct parameter {
	struct tag	 tag;
	strings_t	 name;
	Dwarf_Off	 abstract_origin;
};

static inline struct parameter *tag__parameter(const struct tag *self)
{
	return (struct parameter *)self;
}

extern Dwarf_Off parameter__type(struct parameter *self, const struct cu *cu);
extern const char *parameter__name(struct parameter *self, const struct cu *cu);

enum vlocation {
	LOCATION_UNKNOWN,
	LOCATION_LOCAL,
	LOCATION_GLOBAL,
	LOCATION_REGISTER,
	LOCATION_OPTIMIZED
};

struct variable {
	struct tag	 tag;
	Dwarf_Off	 abstract_origin;
	strings_t	 name;
	uint8_t		 external:1;
	uint8_t		 declaration:1;
	enum vlocation	 location;
};

static inline struct variable *tag__variable(const struct tag *self)
{
	return (struct variable *)self;
}

struct inline_expansion {
	struct tag	 tag;
	size_t		 size;
	Dwarf_Addr	 low_pc;
	Dwarf_Addr	 high_pc;
};

static inline struct inline_expansion *
				tag__inline_expansion(const struct tag *self)
{
	return (struct inline_expansion *)self;
}

struct label {
	struct tag	 tag;
	strings_t	 name;
	Dwarf_Addr	 low_pc;
};

struct enumerator {
	struct tag	 tag;
	strings_t	 name;
	uint32_t	 value;
};

struct conf_fprintf {
	const char *prefix;
	const char *suffix;
	int32_t	   type_spacing;
	int32_t	   name_spacing;
	uint32_t   base_offset;
	uint8_t	   indent;
	uint8_t	   expand_types:1;
	uint8_t	   expand_pointers:1;
	uint8_t    rel_offset:1;
	uint8_t	   emit_stats:1;
	uint8_t	   suppress_comments:1;
	uint8_t	   suppress_offset_comment:1;
	uint8_t	   show_decl_info:1;
	uint8_t	   show_only_data_members:1;
	uint8_t	   no_semicolon:1;
	uint8_t	   show_first_biggest_size_base_type_member:1;
};

int dwarves__init(size_t user_cacheline_size);

extern void class__find_holes(struct class *self, const struct cu *cu);
extern int class__has_hole_ge(const struct class *self, const uint16_t size);
extern size_t class__fprintf(struct class *self, const struct cu *cu,
			     const struct conf_fprintf *conf, FILE *fp);
void enumeration__add(struct type *self, struct enumerator *enumerator);
extern size_t enumeration__fprintf(const struct tag *tag_self,
				   const struct cu *cu,
				   const struct conf_fprintf *conf, FILE *fp);
extern size_t typedef__fprintf(const struct tag *tag_self, const struct cu *cu,
			       const struct conf_fprintf *conf, FILE *fp);
extern size_t tag__fprintf_decl_info(const struct tag *self, FILE *fp);
extern size_t tag__fprintf(struct tag *self, const struct cu *cu,
			   const struct conf_fprintf *conf, FILE *fp);

extern const char *function__name(struct function *self, const struct cu *cu);
extern size_t function__fprintf_stats(const struct tag *tag_self,
				      const struct cu *cu,
				      FILE *fp);
extern const char *function__prototype(const struct function *self,
				       const struct cu *cu,
				       char *bf, size_t len);

void lexblock__add_inline_expansion(struct lexblock *self,
				    struct inline_expansion *exp);
void lexblock__add_label(struct lexblock *self, struct label *label);
void lexblock__add_lexblock(struct lexblock *self, struct lexblock *child);
void lexblock__add_tag(struct lexblock *self, struct tag *tag);
void lexblock__add_variable(struct lexblock *self, struct variable *var);
extern size_t lexblock__fprintf(const struct lexblock *self,
				const struct cu *cu, struct function *function,
				uint16_t indent, FILE *fp);

extern int cus__loadfl(struct cus *self, char *filenames[]);
extern int cus__load(struct cus *self, const char *filename);
extern int cus__load_dir(struct cus *self, const char *dirname,
			 const char *filename_mask, const int recursive);
void cus__add(struct cus *self, struct cu *cu);
extern void cus__print_error_msg(const char *progname, const struct cus *cus,
				 const char *filename, const int err);
extern struct cu *cus__find_cu_by_name(const struct cus *self,
				       const char *name);
extern struct tag *cu__find_base_type_by_name(const struct cu *self,
					      const char *name);
struct tag *cu__find_base_type_by_name_and_size(const struct cu *self,
						const char *name,
						size_t bit_size);
extern struct tag *cus__find_struct_by_name(const struct cus *self,
					    struct cu **cu,
					    const char *name,
					    const int include_decls);
extern struct tag *cus__find_function_by_name(const struct cus *self,
					      struct cu **cu,
					      const char *name);
extern struct tag *cus__find_tag_by_id(const struct cus *self,
				       struct cu **cu, const Dwarf_Off id);

struct cu *cu__new(const char *name, uint8_t addr_size,
		   const unsigned char *build_id, int build_id_len);
extern void cu__delete(struct cu *self);

void cu__add_tag(struct cu *self, struct tag *tag);
extern struct tag *cu__find_tag_by_id(const struct cu *self,
				      const Dwarf_Off id);
extern struct tag *cu__find_first_typedef_of_type(const struct cu *self,
						  const Dwarf_Off type);
extern struct tag *cu__find_struct_by_name(const struct cu *cu,
					   const char *name,
					   const int include_decls);
extern bool cu__same_build_id(const struct cu *self, const struct cu *other);
extern void	    cu__account_inline_expansions(struct cu *self);
extern int	    cu__for_each_tag(struct cu *self,
				     int (*iterator)(struct tag *tag,
					     	     struct cu *cu,
						     void *cookie),
				     void *cookie,
				     struct tag *(*filter)(struct tag *tag,
							   struct cu *cu,
							   void *cookie));
extern void	    cus__for_each_cu(struct cus *self,
				     int (*iterator)(struct cu *cu,
						     void *cookie),
				     void *cookie,
				     struct cu *(*filter)(struct cu *cu));

extern const struct class_member *
		class__find_bit_hole(const struct class *self,
				     const struct class_member *trailer,
				     const size_t bit_hole_size);

extern struct tag *cu__find_function_by_name(const struct cu *cu,
					     const char *name);

static inline size_t function__size(const struct function *self)
{
	return self->lexblock.high_pc - self->lexblock.low_pc;
}

static inline int function__declared_inline(const struct function *self)
{
	return (self->inlined == DW_INL_declared_inlined ||
	        self->inlined == DW_INL_declared_not_inlined);
}

static inline int function__inlined(const struct function *self)
{
	return (self->inlined == DW_INL_inlined ||
	        self->inlined == DW_INL_declared_inlined);
}

void ftype__add_parameter(struct ftype *self, struct parameter *parm);
extern size_t ftype__fprintf(const struct ftype *self, const struct cu *cu,
			     const char *name, const int inlined,
			     const int is_pointer, const int type_spacing,
			     FILE *fp);
extern int ftype__has_parm_of_type(const struct ftype *self,
				   const struct tag *target,
				   const struct cu *cu);

void type__add_member(struct type *self, struct class_member *member);
extern struct class_member *
	type__find_first_biggest_size_base_type_member(struct type *self,
						       const struct cu *cu);

extern const char *tag__name(const struct tag *self, const struct cu *cu,
			     char *bf, size_t len);
extern void tag__not_found_die(const char *file, int line, const char *func);

#define tag__assert_search_result(tag) \
	do { if (!tag) tag__not_found_die(__FILE__,\
					  __LINE__, __func__); } while (0)

extern size_t tag__size(const struct tag *self, const struct cu *cu);
extern size_t tag__nr_cachelines(const struct tag *self, const struct cu *cu);
extern struct tag *tag__follow_typedef(struct tag *tag, const struct cu *cu);

extern struct class_member *type__find_member_by_name(const struct type *self,
						      const char *name);
extern uint32_t type__nr_members_of_type(const struct type *self,
					 const Dwarf_Off type);
extern struct class_member *type__last_member(struct type *self);

void class__add_vtable_entry(struct class *self, struct function *vtable_entry);
static inline struct class_member *
	class__find_member_by_name(const struct class *self, const char *name)
{
	return type__find_member_by_name(&self->type, name);
}

static inline uint16_t class__nr_members(const struct class *self)
{
	return self->type.nr_members;
}

static inline size_t class__size(const struct class *self)
{
	return self->type.size;
}

static inline int class__is_declaration(const struct class *self)
{
	return self->type.declaration;
}

void namespace__add_tag(struct namespace *self, struct tag *tag);

extern const char *variable__name(const struct variable *self,
				  const struct cu *cu);
extern const char *variable__type_name(const struct variable *self,
				       const struct cu *cu,
				       char *bf, size_t len);

extern const char *dwarf_tag_name(const uint32_t tag);

struct argp_state;

void dwarves_print_version(FILE *fp, struct argp_state *state);

#endif /* _DWARVES_H_ */
