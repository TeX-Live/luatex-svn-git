/* kpathsea.c: creating and freeing library instances

   Copyright 2009 Taco Hoekwater.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this library; if not, see <http://www.gnu.org/licenses/>.  */

/* One big global struct, and a variable that points to it */

#include <kpathsea/config.h>

kpathsea
kpathsea_new (void) 
{
    kpathsea ret;
    ret = xcalloc(1, sizeof(kpathsea_instance));
    return ret;
}

#define string_free(a) if((a)!=NULL) free((char *)(a))

void
str_llist_free (str_llist_type p)
{
    str_llist_type q;
    while (p!=NULL) {
        q = p->next;
        free (p->str);
        free (p);
        p = q;
    }
}

void
cache_free (cache_entry *the_cache, int cache_size) 
{
    int f ;
    for (f=0;f<cache_size;f++) {
        string_free (the_cache[f].key);
        str_llist_free (the_cache[f].value[0]);
    }
    free (the_cache);
}

/* Sadly, quite a lot of the freeing is not safe: 
   it seems there are literals used all over. */
void
kpathsea_finish (kpathsea kpse)
{
    int i;
    kpse_format_info_type f;
    if (kpse==NULL)
        return;
#if 0
    /* free internal stuff */
    hash_free (kpse->cnf_hash);
    hash_free (kpse->db);
    hash_free (kpse->alias_db);
    str_list_free(&kpse->db_dir_list); 
    hash_free (kpse->link_table);
    cache_free (kpse->the_cache, kpse->cache_length);
    hash_free (kpse->map);
    string_free (kpse->map_path);
    string_free (kpse->elt);
    /*string_free (kpse->path);*/
    if (kpse->log_file != (FILE *)NULL)
        fclose(kpse->log_file);
    string_free (kpse->invocation_name);
    string_free (kpse->invocation_short_name);
    string_free (kpse->program_name);
    string_free (kpse->fallback_font);
    string_free (kpse->fallback_resolutions_string);
    if(kpse->fallback_resolutions != NULL)
        free(kpse->fallback_resolutions);
    for (i = 0; i != kpse_last_format; ++i) {
        f = kpse->format_info[i];
        /*string_free ((string)f.path);*/
        string_free ((string)f.override_path);
        string_free ((string)f.client_path);
        /*string_free ((string)f.cnf_path);*/
    }

    if (kpse->missfont != (FILE *)NULL)
        fclose(kpse->missfont);

    for (i = 0; i < (int)kpse->expansion_len; i++) {
        string_free (kpse->expansions[i].var);
    }
    free (kpse->expansions);
    if (kpse->saved_env!=NULL) {
        for (i = 0; i != kpse->saved_count; ++i)
            string_free (kpse->saved_env[i]);
        free (kpse->saved_env);
    }    
#endif /* zero */
#if defined (KPSE_COMPAT_API)
    if (kpse==kpse_def)
        return;
#endif
    free(kpse);
}


#if defined (KPSE_COMPAT_API)

kpathsea_instance kpse_def_inst = {
    NULL,      /*p_record_input record_input;*/
    NULL,      /*p_record_output record_output;*/
    {NULL, 0}, /*hash_table_type cnf_hash;*/
    false,     /*boolean doing_cnf_init;*/
    {NULL, 0}, /*hash_table_type db;*/
    {NULL, 0}, /*hash_table_type alias_db;*/
    {0, NULL}, /*str_list_type db_dir_list;*/
    0,         /*unsigned debug;*/
    {NULL, 0}, /*hash_table_type link_table;*/
    NULL,      /*cache_entry *the_cache;*/
    0,         /*unsigned cache_length;*/
    {NULL, 0}, /*hash_table_type map;*/
    NULL,      /*const_string map_path;*/
    false,     /*boolean debug_hash_lookup_int;*/
    NULL,      /*string elt;*/
    0,         /*unsigned elt_alloc;*/
    NULL,      /*const_string path;*/
    false,     /*boolean followup_search;*/
    NULL,      /*FILE *log_file;*/
    false,     /*boolean log_opened;*/
    NULL,      /*string invocation_name;*/
    NULL,      /*string invocation_short_name;*/
    NULL,      /*string program_name;*/
    0,         /*int ll_verbose;*/
    NULL,      /*const_string fallback_font;*/
    NULL,      /*const_string fallback_resolutions_string;*/
    0,         /*unsigned *fallback_resolutions;*/
    {          /*kpse_format_info_type format_info[kpse_last_format];*/
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, 0, NULL, false, 0, false },
    },
    false,     /*boolean make_tex_discard_errors;*/
    NULL,      /*FILE *missfont;*/
    NULL,      /*expansion_type *expansions;*/
    0,         /*unsigned expansion_len ;*/
    NULL,      /*char **saved_env;*/
   0           /*int saved_count;*/
};

kpathsea kpse_def = &kpse_def_inst;

#endif
