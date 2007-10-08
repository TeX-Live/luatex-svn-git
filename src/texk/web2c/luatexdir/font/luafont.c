
#include "luatex-api.h"
#include <ptexlib.h>

#define SAVE_REF 1

char *font_type_strings[]      = {"unknown","virtual","real", NULL};
char *font_format_strings[]    = {"unknown","type1","type3","truetype", "opentype", NULL};
char *font_embedding_strings[] = {"unknown","no","subset", "full", NULL};
char *ligature_type_strings[]  = {"=:", "=:|", "|=:", "|=:|", "", "=:|>", "|=:>", "|=:|>", "", "", "", "|=:|>>", NULL };

void
font_char_to_lua (lua_State *L, internalfontnumber f, charinfo *co) {
  int i;
  liginfo *l;
  kerninfo *ki;
  real_eight_bits *vp;

  lua_createtable(L,0,7);

  lua_pushstring(L,"width");
  lua_pushnumber(L,get_charinfo_width(co));
  lua_rawset(L,-3);

  lua_pushstring(L,"height");
  lua_pushnumber(L,get_charinfo_height(co));
  lua_rawset(L,-3);

  lua_pushstring(L,"depth");
  lua_pushnumber(L,get_charinfo_depth(co));
  lua_rawset(L,-3);

  lua_pushstring(L,"italic");
  lua_pushnumber(L,get_charinfo_italic(co));
  lua_rawset(L,-3);

  if (font_encodingbytes(f) == 2 ) {
	lua_pushstring(L,"index");
    lua_pushnumber(L,get_charinfo_index(co));
    lua_rawset(L,-3);
  }

  if (get_charinfo_name(co)!=NULL) {
	lua_pushstring(L,"name");
    lua_pushstring(L,get_charinfo_name(co));
    lua_rawset(L,-3);
  }

  if (get_charinfo_tounicode(co)!=NULL) {
	lua_pushstring(L,"tounicode");
    lua_pushstring(L,get_charinfo_tounicode(co));
    lua_rawset(L,-3);
  }

  if (get_charinfo_tag(co) == list_tag) {
	lua_pushstring(L,"next");
    lua_pushnumber(L,get_charinfo_remainder(co));
    lua_rawset(L,-3);
  }

  lua_pushstring(L,"used");
  lua_pushboolean(L,(get_charinfo_used(co) ? true : false));
  lua_rawset(L,-3);

  if (get_charinfo_tag(co) == ext_tag) {
	lua_pushstring(L,"extensible");
    lua_createtable(L,0,4);    
    lua_pushnumber(L,get_charinfo_extensible(co,EXT_TOP));
    lua_setfield(L,-2,"top");
    lua_pushnumber(L,get_charinfo_extensible(co,EXT_BOT));
    lua_setfield(L,-2,"bot");
    lua_pushnumber(L,get_charinfo_extensible(co,EXT_MID));
    lua_setfield(L,-2,"mid");
    lua_pushnumber(L,get_charinfo_extensible(co,EXT_REP));
    lua_setfield(L,-2,"rep");
    lua_rawset(L,-3);
  }
  ki = get_charinfo_kerns(co);
  if (ki != NULL) {
	lua_pushstring(L,"kerns");
    lua_createtable(L,10,1);
    for (i=0;!kern_end(ki[i]);i++) {
      if (kern_char(ki[i]) == right_boundarychar) {
		lua_pushstring(L,"right_boundary");
      } else {
		lua_pushnumber(L,kern_char(ki[i]));
      }
      lua_pushnumber(L,kern_kern(ki[i]));
      lua_rawset(L,-3);
    }
    lua_rawset(L,-3);
  }
  l = get_charinfo_ligatures(co);
  if (l!=NULL) {
	lua_pushstring(L,"ligatures");
    lua_createtable(L,10,1);
    for (i=0;!lig_end(l[i]);i++) {
      if (lig_char(l[i]) == right_boundarychar) {
		lua_pushstring(L,"right_boundary");
      } else {
		lua_pushnumber(L,lig_char(l[i]));
      }
      lua_createtable(L,0,2);
	  lua_pushstring(L,"type");
      lua_pushnumber(L,lig_type(l[i]));
      lua_rawset(L,-3);
	  lua_pushstring(L,"char");
      lua_pushnumber(L,lig_replacement(l[i]));
      lua_rawset(L,-3);
      lua_rawset(L,-3);
    }
    lua_rawset(L,-3);
  }
}

static void
write_lua_parameters (lua_State *L, int f) {
  int k;
  lua_newtable(L);
  for (k=1;k<=font_params(f);k++) {
    lua_pushnumber(L,font_param(f,k));
    switch (k) {
    case slant_code:         lua_setfield(L,-2,"slant");         break;
    case space_code:         lua_setfield(L,-2,"space");         break;
    case space_stretch_code: lua_setfield(L,-2,"space_stretch"); break;
    case space_shrink_code:  lua_setfield(L,-2,"space_shrink");  break;
    case x_height_code:      lua_setfield(L,-2,"x_height");      break;
    case quad_code:          lua_setfield(L,-2,"quad");          break;
    case extra_space_code:   lua_setfield(L,-2,"extra_space");   break;
    default:
      lua_rawseti(L,-2,k);
    }
  }
  lua_setfield(L,-2,"parameters");
}


int
font_to_lua (lua_State *L, int f) {
  int k;
  charinfo *co;
  if (font_cache_id(f)) {
    /* fetch the table from the registry if  it was 
       saved there by font_from_lua() */ 
    lua_rawgeti(L,LUA_REGISTRYINDEX,font_cache_id(f));
    /* fontdimens can be changed from tex code */
    write_lua_parameters(L,f);
    return 1;
  }

  lua_newtable(L);
  lua_pushstring(L,font_name(f));
  lua_setfield(L,-2,"name");
  if(font_area(f)!=NULL) {
	lua_pushstring(L,font_area(f));
	lua_setfield(L,-2,"area");
  }
  if(font_filename(f)!=NULL) {
	lua_pushstring(L,font_filename(f));
	lua_setfield(L,-2,"filename");
  }
  if(font_fullname(f)!=NULL) {
	lua_pushstring(L,font_fullname(f));
	lua_setfield(L,-2,"fullname");
  }
  if(font_encodingname(f)!=NULL) {
	lua_pushstring(L,font_encodingname(f));
	lua_setfield(L,-2,"encodingname");
  }

  lua_pushboolean(L,(font_used(f) ? true : false));
  lua_setfield(L,-2,"used");

  
  lua_pushstring(L,font_type_strings[font_type(f)]);
  lua_setfield(L,-2,"type");
  lua_pushstring(L,font_format_strings[font_format(f)]);
  lua_setfield(L,-2,"format");
  lua_pushstring(L,font_embedding_strings[font_embedding(f)]);
  lua_setfield(L,-2,"embedding");
  
  lua_pushnumber(L,font_size(f));
  lua_setfield(L,-2,"size");
  lua_pushnumber(L,font_dsize(f));
  lua_setfield(L,-2,"designsize");
  lua_pushnumber(L,font_checksum(f));
  lua_setfield(L,-2,"checksum");
  lua_pushnumber(L,font_natural_dir(f));
  lua_setfield(L,-2,"direction");
  lua_pushnumber(L,font_encodingbytes(f));
  lua_setfield(L,-2,"encodingbytes");
  lua_pushnumber(L,font_tounicode(f));
  lua_setfield(L,-2,"tounicode");

  /* params */
  write_lua_parameters(L,f);
  
  /* chars */
  lua_createtable(L,font_tables[f]->charinfo_size,0); /* all characters */

  if (has_left_boundary(f)) {
    co = get_charinfo(f,left_boundarychar);
    font_char_to_lua(L,f,co);
    lua_setfield(L,-2,"left_boundary");
  }
  if (has_right_boundary(f)) {
    co = get_charinfo(f,right_boundarychar);
    font_char_to_lua(L,f,co);
    lua_setfield(L,-2,"right_boundary");
  }

  for (k=font_bc(f);k<=font_ec(f);k++) {
    if (char_exists(f,k)) {
      lua_pushnumber(L,k);
      co = get_charinfo(f,k);
      font_char_to_lua(L,f,co);
      lua_rawset(L,-3);
    }
  }
  lua_setfield(L,-2,"characters");
  return 1;
}

static int 
count_hash_items (lua_State *L, char *name){
  int n = -1;
  lua_getfield(L,-1,name);
  if (!lua_isnil(L,-1)) {
    if (lua_istable(L,-1)) {
      n = 0;
      /* now find the number */
      lua_pushnil(L);  /* first key */
      while (lua_next(L, -2) != 0) {
		n++;
		lua_pop(L,1);
      }
    }
  }
  lua_pop(L,1);
  return n;
}

#define streq(a,b) (strcmp(a,b)==0)

#define append_packet(k) { cpackets[np++] = k; }

#define do_store_four(l) {							\
    append_packet((l&0xFF000000)>>24);				\
    append_packet((l&0x00FF0000)>>16);				\
    append_packet((l&0x0000FF00)>>8);				\
    append_packet((l&0x000000FF));  } 

/*
*/

static int
numeric_field (lua_State *L, char *name, int dflt) {
  int i = dflt;
  lua_pushstring(L,name);
  lua_rawget(L,-2);
  if (lua_isnumber(L,-1)) {	
    i = lua_tonumber(L,-1);
  }
  lua_pop(L,1);
  return i;
}

static int
enum_field (lua_State *L, char *name, int dflt, char **values) {
  int k;
  char *s;
  int i = dflt;
  lua_pushstring(L,name);
  lua_rawget(L,-2);
  if (lua_isnumber(L,-1)) {	
    i = lua_tonumber(L,-1);
  } else if (lua_isstring(L,-1)) {
    s = (char *)lua_tostring(L,-1);
    k = 0;
    while (values[k] != NULL) {
      if (strcmp(values[k],s) == 0) {
	i = k;
	break;
      }
      k++;
    }
  }
  lua_pop(L,1);
  return i;
}

static int
boolean_field (lua_State *L, char *name, int dflt) {
  int i = dflt;
  lua_pushstring(L,name);
  lua_rawget(L,-2);
  if (lua_isboolean(L,-1)) {	
    i = lua_toboolean(L,-1);
  }
  lua_pop(L,1);
  return i;
}

static char *
string_field (lua_State *L, char *name, char *dflt) {
  char *i;
  lua_pushstring(L,name);
  lua_rawget(L,-2);
  if (lua_isstring(L,-1)) {	
    i = xstrdup(lua_tostring(L,-1));
  } else if (dflt==NULL) {
    i = NULL;
  } else {
    i = xstrdup(dflt);
  }
  lua_pop(L,1);
  return i;
}

static int
count_char_packet_bytes  (lua_State *L) {
  int i, l, ff;
  size_t len;
  char *s;
  ff = 0;
  l = 0;
  for (i=1;i<=lua_objlen(L,-1);i++) {
    lua_rawgeti(L,-1,i);
    if (lua_istable(L,-1)) {
      lua_rawgeti(L,-1,1);
      if (lua_isstring(L,-1)) {
	s = (char *)lua_tostring(L,-1);
	if      (streq(s,"font"))    { l+= 5; ff =1; }
	else if (streq(s,"char"))    { if (ff==0) { l+=5;  } l += 5; ff = 1;	} 
	else if (streq(s,"slot"))    { l += 10; ff = 1;}
	else if (streq(s,"comment")) { ;     } 
	else if (streq(s,"push"))    { l++;  } 
	else if (streq(s,"pop"))     { l++;  } 
	else if (streq(s,"rule"))    { l+=9; }
	else if (streq(s,"right"))   { l+=5; }
	else if (streq(s,"node"))    { l+=5; }
	else if (streq(s,"down"))    { l+=5; }
	else if (streq(s,"special")) { 
	  lua_rawgeti(L,-2,2);
	  (void)lua_tolstring(L,-1,&len); 
	  lua_pop(L,1);
	  if (len>0) { l = l + 5 + len;  } 
	}
	else { fprintf(stdout,"unknown packet command %s!\n",s); }
      } else {
	fprintf(stdout,"no packet command!\n");
      }
      lua_pop(L,1); /* command name */
    }
    lua_pop(L,1); /* item */
  }
  return l;
}



static void
read_char_packets  (lua_State *L, integer *l_fonts, charinfo *co) {
  int i, n, m;
  size_t l;
  int cmd;
  char *s;
  real_eight_bits *cpackets;
  int ff = 0;
  int np = 0;
  int max_f = 0;
  int pc = count_char_packet_bytes  (L);
  if (pc<=0)
    return;
  assert(l_fonts != NULL);
  assert(l_fonts[1] != 0);
  while (l_fonts[(max_f+1)]!=0) 
    max_f++;

  cpackets = xmalloc(pc+1);
  for (i=1;i<=lua_objlen(L,-1);i++) {
    lua_rawgeti(L,-1,i);
    if (lua_istable(L,-1)) {
      /* fetch the command code */
      lua_rawgeti(L,-1,1);
      if (lua_isstring(L,-1)) {
		s = (char *)lua_tostring(L,-1);
		cmd = 0;
		if      (streq(s,"font"))    {  cmd = packet_font_code;     
		}
		else if (streq(s,"char"))    {  cmd = packet_char_code;     
		  if (ff==0) {
			append_packet(packet_font_code);
			ff = l_fonts[1];
			do_store_four(ff);
		  }
		} 
		else if (streq(s,"slot"))    {  cmd = packet_nop_code;
		  lua_rawgeti(L,-2,2);  n = lua_tointeger(L,-1);
		  ff = (n>max_f ? l_fonts[1] : l_fonts[n]);
		  lua_rawgeti(L,-3,3);  n = lua_tointeger(L,-1);
		  lua_pop(L,2);
		  append_packet(packet_font_code);
		  do_store_four(ff);
		  append_packet(packet_char_code);
		  do_store_four(n);
		} 
		else if (streq(s,"comment")) {  cmd = packet_nop_code;     } 
		else if (streq(s,"node"))    {  cmd = packet_node_code;    }
		else if (streq(s,"push"))    {  cmd = packet_push_code;    } 
		else if (streq(s,"pop"))     {  cmd = packet_pop_code;     } 
		else if (streq(s,"rule"))    {  cmd = packet_rule_code;    }
		else if (streq(s,"right"))   {  cmd = packet_right_code;   }
		else if (streq(s,"down"))    {  cmd = packet_down_code;    }
		else if (streq(s,"special")) {  cmd = packet_special_code; } 

	switch(cmd) {
	case packet_push_code:
	case packet_pop_code:
	  append_packet(cmd);
	  break;
	case packet_font_code:
	  append_packet(cmd);
	  lua_rawgeti(L,-2,2);
	  n = lua_tointeger(L,-1);
	  ff = (n>max_f ? l_fonts[1] : l_fonts[n]);
	  do_store_four(ff);
	  lua_pop(L,1);
	  break;
	case packet_node_code:
	  append_packet(cmd);
	  lua_rawgeti(L,-2,2);
	  n = copy_node_list(nodelist_from_lua(L));
	  do_store_four(n);
	  lua_pop(L,1);
	  break;
	case packet_char_code:
	  append_packet(cmd);
	  lua_rawgeti(L,-2,2);
	  n = lua_tointeger(L,-1);
	  do_store_four(n);
	  lua_pop(L,1);
	  break;
	case packet_right_code:
	case packet_down_code:
	  append_packet(cmd);
	  lua_rawgeti(L,-2,2);
	  n = lua_tointeger(L,-1);
	  /* TODO this multiplier relates to the font size, apparently */
	  do_store_four(((n<<4)/10));
	  lua_pop(L,1);
	  break;
	case packet_rule_code:
	  append_packet(cmd);
	  lua_rawgeti(L,-2,2);
	  n = lua_tointeger(L,-1);
	  /* here too, twice */
	  do_store_four(((n<<4)/10));
	  lua_rawgeti(L,-3,3);
	  n = lua_tointeger(L,-1);
	  do_store_four(((n<<4)/10));
	  lua_pop(L,2);
	  break;
	case packet_special_code:
	  append_packet(cmd);
	  lua_rawgeti(L,-2,2);
	  s = (char *)lua_tolstring(L,-1,&l);
	  if (l>0) {
	    do_store_four(l);
	    m = (int)l;
	    while(m>0) {
	      n = *s++;	  m--;
	      append_packet(n);
	    }
	  }
	  lua_pop(L,1);
	  break;
	case packet_nop_code:
	  break;
	default:
	  fprintf(stdout,"Unknown char packet code %s (char %d in font %s)\n",s,c,font_name(f));
	}
      }
      lua_pop(L,1); /* command code */
    } else {
      fprintf(stdout,"Found a `commands' item that is not a table (char %d in font %s)\n",c,font_name(f));
    }
    lua_pop(L,1); /* command table */
  }
  append_packet(packet_end_code);
  set_charinfo_packets(co,cpackets);
  return;
}


static void
read_lua_cidinfo (lua_State *L, int f) {
  int i;
  char *s;
  lua_getfield(L,-1,"cidinfo");
  if (lua_istable(L,-1)) {	
    i = numeric_field(L,"version",0);
    set_font_cidversion(f,i);    
    i = numeric_field(L,"supplement",0);
    set_font_cidsupplement(f,i);
    s = string_field(L,"registry","Adobe"); /* Adobe-Identity-0 */
    set_font_cidregistry(f,s);
    s = string_field(L,"ordering","Identity");
    set_font_cidordering(f,s);
  }
  lua_pop(L,1);
}


static void
read_lua_parameters (lua_State *L, int f) {
  int i, n;
  char *s;
  lua_getfield(L,-1,"parameters");
  if (lua_istable(L,-1)) {	
    /* the number of parameters is the max(IntegerKeys(L)),7) */
    n = 7;
    lua_pushnil(L);  /* first key */
    while (lua_next(L, -2) != 0) {
      if (lua_isnumber(L,-2)) {
	i = lua_tonumber(L,-2);
	if (i > n)  n = i;
      }
      lua_pop(L,1); /* pop value */
    }

    if (n>7) set_font_params(f,n);

    /* sometimes it is handy to have all integer keys */
    for (i=1;i<=7;i++) {
      lua_rawgeti(L,-1,i);
      if (lua_isnumber(L,-1)) {
	n = lua_tointeger(L,-1);
	set_font_param(f,i, n);
      }
      lua_pop(L,1); 
    }

    lua_pushnil(L);  /* first key */
    while (lua_next(L, -2) != 0) {
      if (lua_isnumber(L,-2)) {
	i = lua_tointeger(L,-2);
	if (i>=8) {
	  n = (lua_isnumber(L,-1) ? lua_tointeger(L,-1) : 0);
	  set_font_param(f,i, n);
	}
      } else if (lua_isstring(L,-2)) {
	s = (char *)lua_tostring(L,-2);
	n = (lua_isnumber(L,-1) ? lua_tointeger(L,-1) : 0);

	if       (strcmp("slant",s)== 0)         {  set_font_param(f,slant_code,n); }
	else if  (strcmp("space",s)== 0)         {  set_font_param(f,space_code,n); }
	else if  (strcmp("space_stretch",s)== 0) {  set_font_param(f,space_stretch_code,n); }
	else if  (strcmp("space_shrink",s)== 0)  {  set_font_param(f,space_shrink_code,n); }
	else if  (strcmp("x_height",s)== 0)      {  set_font_param(f,x_height_code,n); }
	else if  (strcmp("quad",s)== 0)          {  set_font_param(f,quad_code,n); }
	else if  (strcmp("extra_space",s)== 0)   {  set_font_param(f,extra_space_code,n); }

      }
      lua_pop(L,1); 
    }
  }
  lua_pop(L,1);

}

void
font_char_from_lua (lua_State *L, internal_font_number f, integer i, integer *l_fonts) {
  int k,r,t;
  charinfo *co;
  kerninfo *ckerns;
  liginfo *cligs;
  real_eight_bits *cpackets;
  scaled j;
  char *s;
  int nc; /* number of character info items */
  int nl; /* number of ligature table items */
  int nk; /* number of kern table items */
  int np; /* number of virtual packet bytes */
  int ctr;
  nl = 1; nk = 1; np = 1; ctr = 0;
  if (lua_istable(L,-1)) {
    co = get_charinfo(f,i); 
    set_charinfo_tag       (co,0);
    j = numeric_field(L,"width",0);        set_charinfo_width (co,j);
    j = numeric_field(L,"height",0);       set_charinfo_height (co,j);
    j = numeric_field(L,"depth",0);        set_charinfo_depth (co,j);
    j = numeric_field(L,"italic",0);       set_charinfo_italic (co,j);	      
    k = boolean_field(L,"used",0);         set_charinfo_used(co,k);
    j = numeric_field(L,"index",0);        set_charinfo_index(co,j);
    s = string_field (L,"name",NULL);      set_charinfo_name(co,s);
    s = string_field (L,"tounicode",NULL); set_charinfo_tounicode(co,s);
      
    k = numeric_field(L,"next",-1); 
    if (k>=0) {
      set_charinfo_tag       (co,list_tag);
      set_charinfo_remainder (co,k);
    }
    
    lua_getfield(L,-1,"extensible");
    if (lua_istable(L,-1)){ 
      int top, bot,mid, rep;
      top = numeric_field(L,"top",0);
      bot = numeric_field(L,"bot",0);
      mid = numeric_field(L,"mid",0);
      rep = numeric_field(L,"rep",0);
      if (top != 0 || bot != 0 || mid != 0 || rep != 0) {
	set_charinfo_tag       (co,ext_tag);
	set_charinfo_extensible (co,top,bot,mid,rep);
      } else {
	pdftex_warn("lua-loaded font %s char [%d] has an invalid extensible field!",font_name(f),i);
      }
    }
    lua_pop(L,1);
      
    nk = count_hash_items(L,"kerns");
    if (nk>0) {
      ckerns = xcalloc((nk+1),sizeof(kerninfo));
      lua_getfield(L,-1,"kerns");
      if (lua_istable(L,-1)) {  /* there are kerns */
	ctr = 0;
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
	  k = non_boundarychar;
	  if (lua_isnumber(L,-2)) {
	    k = lua_tonumber(L,-2); /* adjacent char */
	    if (k<0)
	      k = non_boundarychar;
	  } else if (lua_isstring(L,-2)) {
	    s = (char *)lua_tostring(L,-2);
	    if (strcmp(s,"right_boundary")==0) {
	      k = right_boundarychar;
	      if (!has_right_boundary(f))
		set_right_boundary(f,get_charinfo(f,right_boundarychar));
	    }
	  }
	  j = lua_tonumber(L,-1); /* movement */
	  if (k!=non_boundarychar) {
	    set_kern_item(ckerns[ctr],k,j);
	    ctr++;
	  } else {
	    pdftex_warn("lua-loaded font %s char [%d] has an invalid kern field!",font_name(f),i);
	  }
	  lua_pop(L,1);
	}
	/* guard against empty tables */
	if (ctr>0) {
	  set_kern_item(ckerns[ctr],end_kern,0);
	  set_charinfo_kerns(co,ckerns);
	} else {
	  pdftex_warn("lua-loaded font %s char [%d] has an invalid kerns field!",font_name(f),i);
	}
      }
      lua_pop(L,1);
    }
      
    /* packet commands */
    np = count_hash_items(L,"commands");
    if (np>0) {
      lua_getfield(L,-1,"commands");
      if (lua_istable(L,-1)){ 
	read_char_packets(L,(integer *)l_fonts,co);
      }
      lua_pop(L,1);
    }
    
    /* ligatures */
    nl = count_hash_items(L,"ligatures");
      
    if (nl>0) {
      cligs = xcalloc((nl+1),sizeof(liginfo));
      lua_getfield(L,-1,"ligatures");
      if (lua_istable(L,-1)){/* do ligs */
	ctr = 0;
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
	  k = non_boundarychar;
	  if (lua_isnumber(L,-2)) {
	    k = lua_tonumber(L,-2); /* adjacent char */
	    if (k<0) {
	      k = non_boundarychar;
	    }
	  } else if (lua_isstring(L,-2)) {
	    s = (char *)lua_tostring(L,-2);
	    if (strcmp(s,"right_boundary")==0) {
	      k = right_boundarychar;
	      if (!has_right_boundary(f))
		set_right_boundary(f,get_charinfo(f,right_boundarychar));
	    }
	  }
	  r = -1;
	  if (lua_istable(L,-1)) { 
	    r = numeric_field(L,"char",-1); /* ligature */
	  }
	  if (r != -1 && k != non_boundarychar) {		
	    t = enum_field(L,"type",0,ligature_type_strings);
	    set_ligature_item(cligs[ctr],(t*2)+1,k,r);
	    ctr++;
	  } else {
	    pdftex_warn("lua-loaded font %s char [%d] has an invalid ligature field!",font_name(f),i);
	  }
	  lua_pop(L,1); /* iterator value */
	}		  
	/* guard against empty tables */
	if (ctr>0) {
	  set_ligature_item(cligs[ctr],0,end_ligature,0);
	  set_charinfo_ligatures(co,cligs);
	} else {
	  pdftex_warn("lua-loaded font %s char [%d] has an invalid ligatures field!",font_name(f),i);
	}
      }
      lua_pop(L,1); /* ligatures table */
    }
  }
}



/* The caller has to fix the state of the lua stack when there is an error! */


int
font_from_lua (lua_State *L, int f) {
  int i,k,n,r,t;
  int s_top; /* lua stack top */
  int bc; /* first char index */
  int ec; /* last char index */
  int ctr;
  char *s;
  integer *l_fonts = NULL;
  /* the table is at stack index -1 */

  s = string_field(L,"area","");                 set_font_area(f,s);
  s = string_field(L,"filename",NULL);           set_font_filename(f,s);
  s = string_field(L,"encodingname",NULL);       set_font_encodingname(f,s);

  s = string_field(L,"name",NULL);               set_font_name(f,s);
  s = string_field(L,"fullname",font_name(f));   set_font_fullname(f,s);

  if (s==NULL) {
    pdftex_fail("lua-loaded font [%d] has no name!",f);
    return false;
  }

  i = numeric_field(L,"designsize",655360);      set_font_dsize(f,i);
  i = numeric_field(L,"size",font_dsize(f));     set_font_size(f,i);
  i = numeric_field(L,"checksum",0);             set_font_checksum(f,i);
  i = numeric_field(L,"direction",0);            set_font_natural_dir(f,i);
  i = numeric_field(L,"encodingbytes",0);        set_font_encodingbytes(f,i);
  i = numeric_field(L,"tounicode",0);            set_font_tounicode(f,i);
  i = numeric_field(L,"hyphenchar",get_default_hyphen_char()); set_hyphen_char(f,i);
  i = numeric_field(L,"skewchar",get_default_skew_char());     set_skew_char(f,i);
  i = boolean_field(L,"used",0);                 set_font_used(f,i);

  i = enum_field(L,"type",     unknown_font_type,font_type_strings);      set_font_type(f,i);
  i = enum_field(L,"format",   unknown_format,   font_format_strings);    set_font_format(f,i);
  i = enum_field(L,"embedding",unknown_embedding,font_embedding_strings); set_font_embedding(f,i);
  if (font_encodingbytes(f)==0 && 
      (font_format(f)==opentype_format || font_format(f)==truetype_format)) {
    set_font_encodingbytes(f,2);
  }
  
  /* now fetch the base fonts, if needed */
  n = count_hash_items(L,"fonts");
  if (n>0) {
    l_fonts = xmalloc((n+2)*sizeof(integer));
    memset (l_fonts,0,(n+2)*sizeof(integer));
    lua_getfield(L,-1,"fonts");
    for (i=1;i<=n;i++) {
      lua_rawgeti(L,-1,i);
      if (lua_istable(L,-1)) {
		lua_getfield(L,-1,"id");
		if (lua_isnumber(L,-1)) {
		  l_fonts[i] = lua_tonumber(L,-1);
		  lua_pop(L,2); /* pop id  and entry */
		  continue; 
		}
		lua_pop(L,1); /* pop id */
	  };
      s = NULL;
      if (lua_istable(L,-1)) {
		lua_getfield(L,-1,"name");
		if (lua_isstring(L,-1)) {
		  s = (char *)lua_tostring(L,-1);
		}
		lua_pop(L,1); /* pop name */
      }
      if (s!= NULL) {
		lua_getfield(L,-1,"size");
		t = (lua_isnumber(L,-1) ? lua_tonumber(L,-1) : -1000);
		lua_pop(L,1);
		
		/* TODO: the stack is messed up, otherwise this 
		 * explicit resizing would not be needed 
		 */
		s_top = lua_gettop(L);
		l_fonts[i] = find_font_id(s,"",t);
		lua_settop(L,s_top);
      } else {
		pdftex_fail("Invalid local font in font %s!\n", font_name(f));
      }
      lua_pop(L,1); /* pop list entry */
    }
    lua_pop(L,1); /* pop list entry */
  } else {
    if(font_type(f) == virtual_font_type) {
      pdftex_fail("Invalid local fonts in font %s!\n", font_name(f));
    } else {
      l_fonts = xmalloc(3*sizeof(integer));
      l_fonts[0] = 0;
      l_fonts[1] = f;
      l_fonts[2] = 0;
    }
  }

  /* parameters */
  read_lua_parameters(L,f);

  read_lua_cidinfo(L,f);

  /* characters */
  lua_getfield(L,-1,"characters");
  if (lua_istable(L,-1)) {	
    /* find the array size values */
    ec = 0; bc = -1; 
    lua_pushnil(L);  /* first key */
    while (lua_next(L, -2) != 0) {
      if (lua_isnumber(L,-2)) {
	i = lua_tointeger(L,-2);
	if (i>=0) {
	  if (lua_istable(L,-1)) {
	    if (i>ec) ec = i;
	    if (bc<0) bc = i;
	    if (bc>=0 && i<bc) bc = i;
	  }
	}
      }
      lua_pop(L, 1);
    }

    if (bc != -1) {
      set_font_bc(f,bc);
      set_font_ec(f,ec);
      lua_pushnil(L);  /* first key */
      while (lua_next(L, -2) != 0) {
	if (lua_isnumber(L,-2)) {
	  i = lua_tonumber(L,-2);
	  if (i>=0) {
	    font_char_from_lua(L,f, i, l_fonts);
	  }
	} else if (lua_isstring(L,-2)) {
	  s = (char *)lua_tostring(L,-2);
	  if (strcmp(s,"left_boundary")==0) {
	    font_char_from_lua(L,f, left_boundarychar, l_fonts);
	  } else if (strcmp(s,"right_boundary")==0) {
	    font_char_from_lua(L,f, right_boundarychar, l_fonts);
	  }
	}
	lua_pop(L, 1);
      }
      lua_pop(L, 1);
      
    } else { /* jikes, no characters */
      pdftex_warn("lua-loaded font [%d] has no characters!",f);
    }

#if SAVE_REF
    r = luaL_ref(Luas[0],LUA_REGISTRYINDEX);    /* pops the table */
    set_font_cache_id(f,r);
#else
    lua_pop(Luas[0],1);
#endif
  } else { /* jikes, no characters */
    pdftex_warn("lua-loaded font [%d] has no character table!",f);
  }
  if (l_fonts!=NULL) 
    free(l_fonts);
  return true;
}

 
/*
if disable_lig=0 and has_lig(main_f,cur_l) then begin
  lig:=get_ligature(main_f,cur_l,cur_r);
  if is_ligature(lig) then 
    @<Do ligature command and goto somewhere@>;
  end;
if disable_kern=0 and has_kern(main_f,cur_l) then begin
  main_k:=get_kern(main_f,cur_l,cur_r);
  if main_k<>0 then begin 
    wrapup(rt_hit);
    tail_append(new_kern(main_k));
    if new_left_ghost or right_ghost then begin
      subtype(tail):=explicit;
      end;
    goto main_loop_move;
    end;
  end;
*/

/*

@ When a ligature or kern instruction matches a character, we know from
|read_font_info| that the character exists in the font, even though we
haven't verified its existence in the normal way.

This section could be made into a subroutine, if the code inside
|main_control| needs to be shortened.

\chardef\?='174 % vertical line to indicate character retention

@<Do ligature command...@>=
begin if new_left_ghost or right_ghost then goto main_loop_wrapup;
if cur_l=non_boundarychar then lft_hit:=true
else if lig_stack=null then rt_hit:=true;
check_interrupt; {allow a way out in case there's an infinite ligature loop}
case lig_type(lig) of
qi(1),qi(5):begin cur_l:=lig_replacement(lig); {\.{=:\?}, \.{=:\?>}}
  ligature_present:=true;
  end;
qi(2),qi(6):begin cur_r:=lig_replacement(lig); {\.{\?=:}, \.{\?=:>}}
  if lig_stack=null then {right boundary character is being consumed}
    begin lig_stack:=new_lig_item(cur_r); bchar:=non_boundarychar;
    end
  else if is_char_node(lig_stack) then {|vlink(lig_stack)=null|}
    begin main_p:=lig_stack; lig_stack:=new_lig_item(cur_r);
    lig_ptr(lig_stack):=main_p;
    end
  else character(lig_stack):=cur_r;
  end;
qi(3):begin cur_r:=lig_replacement(lig); {\.{\?=:\?}}
  main_p:=lig_stack; lig_stack:=new_lig_item(cur_r);
  vlink(lig_stack):=main_p;
  end;
qi(7),qi(11):begin wrapup(false); {\.{\?=:\?>}, \.{\?=:\?>>}}
  cur_q:=tail; cur_l:=lig_replacement(lig);
  ligature_present:=true;
  end;
othercases begin cur_l:=lig_replacement(lig); 
 ligature_present:=true; {\.{=:}}
  if lig_stack=null then goto main_loop_wrapup
  else goto main_loop_move+1;
  end
endcases;
if lig_type(lig)>qi(4) then
  if lig_type(lig)<>qi(7) then goto main_loop_wrapup;
if cur_l<>non_boundarychar then goto main_lig_loop;
if has_left_boundary(main_f) then main_k:=left_boundarychar
else main_k:=non_boundarychar; goto main_lig_loop+1;
end

*/

void 
handle_ligkern(halfword head, halfword tail, int dir) {
  halfword r = head, save_tail = vlink(tail);
  vlink(tail) = null;
  while (r!=null) {
	switch (type(r)) {
	case glyph_node:
#ifdef VERBOSE
	  fprintf(stderr,"%c",character(r));
#endif
	  break;
	case glue_node:
#ifdef VERBOSE
	  fprintf(stderr," ");
#endif
	  break;
	default:
	  break;
	}
	r = vlink(r);
  }
#ifdef VERBOSE
  fprintf(stderr,"\n");
#endif
  vlink(tail) = save_tail;
}



void 
new_ligkern(halfword head, halfword tail, int dir) {
  int callback_id = 0;
  lua_State *L = Luas[0];
  /*  fprintf(stderr,"new_ligkern(%d, %d, %d)",vlink(head),tail,dir);*/
  callback_id = callback_defined(ligkern_callback);
  if (head==null || vlink(head)==null)
    return;
  if (callback_id>0) {
    /* */
    lua_rawgeti(L,LUA_REGISTRYINDEX,callback_callbacks_id);
    lua_rawgeti(L,-1, callback_id);
    if (!lua_isfunction(L,-1)) {
      lua_pop(L,2);
      return;
    }
    nodelist_to_lua(L,head);
    nodelist_to_lua(L,tail);
    lua_pushnumber(L,dir);
    if (lua_pcall(L,3,0,0) != 0) {
      fprintf(stdout,"error: %s\n",lua_tostring(L,-1));
      lua_pop(L,2);
      lua_error(L);
      return;
    } 
    lua_pop(L,1);
  }  else {
    handle_ligkern(head,tail,dir);
  }
}

/* 

If a hyphen may be inserted between |hc[j]| and |hc[j+1]|, the hyphenation
procedure will set |hyf[j]| to some small odd number. But before we look
at \TeX's hyphenation procedure, which is independent of the rest of the
line-breaking algorithm, let us consider what we will do with the hyphens
it finds, since it is better to work on this part of the program before
forgetting what |ha| and |hb|, etc., are all about.

@<Glob...@>=
@!hyf:array [0..64] of 0..9; {odd values indicate discretionary hyphens}
@!init_list:pointer; {list of punctuation characters preceding the word}
@!init_lig:boolean; {does |init_list| represent a ligature?}
@!init_lft:boolean; {if so, did the ligature involve a left boundary?}

@ @<Local variables for hyphenation@>=
@!i,@!j,@!l:0..65; {indices into |hc| or |hu|}
@!q,@!r,@!s:pointer; {temporary registers for list manipulation}
@!bchar:integer; {right boundary character of hyphenated word, or |non_boundarychar|}

@ \TeX\ will never insert a hyphen that has fewer than
\.{\\lefthyphenmin} letters before it or fewer than
\.{\\righthyphenmin} after it; hence, a short word has
comparatively little chance of being hyphenated. If no hyphens have
been found, we can save time by not having to make any changes to the
paragraph.

@<If no hyphens were found, |return|@>=
for j:=l_hyf to hn-r_hyf do if odd(hyf[j]) then goto found1;
return;
found1:

@ We must now face the fact that the battle is not over, even though the
{\def\!{\kern-1pt}%
hyphens have been found: The process of reconstituting a word can be nontrivial
because ligatures might change when a hyphen is present. {\sl The \TeX book\/}
discusses the difficulties of the word ``difficult'', and
the discretionary material surrounding a
hyphen can be considerably more complex than that. Suppose
\.{abcdef} is a word in a font for which the only ligatures are \.{b\!c},
\.{c\!d}, \.{d\!e}, and \.{e\!f}. If this word permits hyphenation
between \.b and \.c, the two patterns with and without hyphenation are
$\.a\,\.b\,\.-\,\.{c\!d}\,\.{e\!f}$ and $\.a\,\.{b\!c}\,\.{d\!e}\,\.f$.
Thus the insertion of a hyphen might cause effects to ripple arbitrarily
far into the rest of the word. A further complication arises if additional
hyphens appear together with such rippling, e.g., if the word in the
example just given could also be hyphenated between \.c and \.d; \TeX\
avoids this by simply ignoring the additional hyphens in such weird cases.}

Still further complications arise in the presence of ligatures that do not
delete the original characters. When punctuation precedes the word being
hyphenated, \TeX's method is not perfect under all possible scenarios,
because punctuation marks and letters can propagate information back and forth.
For example, suppose the original pre-hyphenation pair
\.{*a} changes to \.{*y} via a \.{\?=:} ligature, which changes to \.{xy}
via a \.{=:\?} ligature; if $p_{a-1}=\.x$ and $p_a=\.y$, the reconstitution
procedure isn't smart enough to obtain \.{xy} again. In such cases the
font designer should include a ligature that goes from \.{xa} to \.{xy}.

@ The processing is facilitated by a subroutine called |reconstitute|. Given
a string of characters $x_j\ldots x_n$, there is a smallest index $m\ge j$
such that the ``translation'' of $x_j\ldots x_n$ by ligatures and kerning
has the form $y_1\ldots y_t$ followed by the translation of $x_{m+1}\ldots x_n$,
where $y_1\ldots y_t$ is some nonempty sequence of character, ligature, and
kern nodes. We call $x_j\ldots x_m$ a ``cut prefix'' of $x_j\ldots x_n$.
For example, if $x_1x_2x_3=\.{fly}$, and if the font contains `fl' as a
ligature and a kern between `fl' and `y', then $m=2$, $t=2$, and $y_1$ will
be a ligature node for `fl' followed by an appropriate kern node~$y_2$.
In the most common case, $x_j$~forms no ligature with $x_{j+1}$ and we
simply have $m=j$, $y_1=x_j$. If $m<n$ we can repeat the procedure on
$x_{m+1}\ldots x_n$ until the entire translation has been found.

The |reconstitute| function returns the integer $m$ and puts the nodes
$y_1\ldots y_t$ into a vlinked list starting at |vlink(hold_head)|,
getting the input $x_j\ldots x_n$ from the |hu| array. If $x_j=256$,
we consider $x_j$ to be an implicit left boundary character; in this
case |j| must be strictly less than~|n|. There is a
parameter |bchar|, which is either 256 or an implicit right boundary character
assumed to be present just following~$x_n$. (The value |hu[n+1]| is never
explicitly examined, but the algorithm imagines that |bchar| is there.)

If there exists an index |k| in the range $j\le k\le m$ such that |hyf[k]|
is odd and such that the result of |reconstitute| would have been different
if $x_{k+1}$ had been |hchar|, then |reconstitute| sets |hyphen_passed|
to the smallest such~|k|. Otherwise it sets |hyphen_passed| to zero.

A special convention is used in the case |j=0|: Then we assume that the
translation of |hu[0]| appears in a special list of charnodes starting at
|init_list|; moreover, if |init_lig| is |true|, then |hu[0]| will be
a ligature character, involving a left boundary if |init_lft| is |true|.
This facility is provided for cases when a hyphenated
word is preceded by punctuation (like single or double quotes) that might
affect the translation of the beginning of the word.

@<Glob...@>=
@!hyphen_passed:small_number; {first hyphen in a ligature, if any}

@ @<Declare the function called |reconstitute|@>=
function reconstitute(@!j,@!n:small_number;@!bchar,@!hchar:integer):
  small_number;
label continue,done;
var @!p:pointer; {temporary register for list manipulation}
@!t:pointer; {a node being appended to}
@!q:four_quarters; {character information or a lig/kern instruction}
@!cur_rh:integer; {hyphen character for ligature testing}
@!test_char:integer; {hyphen or other character for ligature testing}
@!w:scaled; {amount of kerning}
@!k:integer; {position of current lig/kern instruction}
@!lig:liginfo; {a ligature structure}
begin hyphen_passed:=0; t:=hold_head; w:=0; vlink(hold_head):=null;
 {at this point |ligature_present=lft_hit=rt_hit=false|}
@<Set up data structures with the cursor following position |j|@>;
continue:@<If there's a ligature or kern at the cursor position, update the data
  structures, possibly advancing~|j|; continue until the cursor moves@>;
@<Append a ligature and/or kern to the translation;
  |goto continue| if the stack of inserted ligatures is nonempty@>;
reconstitute:=j;
end;

@ The reconstitution procedure shares many of the global data structures
by which \TeX\ has processed the words before they were hyphenated.
There is an implied ``cursor'' between characters |cur_l| and |cur_r|;
these characters will be tested for possible ligature activity. If
|ligature_present| then |cur_l| is a ligature character formed from the
original characters following |cur_q| in the current translation list.
There is a ``ligature stack'' between the cursor and character |j+1|,
consisting of pseudo-ligature nodes linked together by their |vlink| fields.
This stack is normally empty unless a ligature command has created a new
character that will need to be processed later. A pseudo-ligature is
a special node having a |character| field that represents a potential
ligature and a |lig_ptr| field that points to a |char_node| or is |null|.
We have
$$|cur_r|=\cases{|character(lig_stack)|,&if |lig_stack>null|;\cr
  |qi(hu[j+1])|,&if |lig_stack=null| and |j<n|;\cr
  bchar,&if |lig_stack=null| and |j=n|.\cr}$$

@<Glob...@>=
@!cur_l,@!cur_r:integer; {characters before and after the cursor}
@!cur_q:pointer; {where a ligature should be detached}
@!lig_stack:pointer; {unfinished business to the right of the cursor}
@!ligature_present:boolean; {should a ligature node be made for |cur_l|?}
@!lft_hit,@!rt_hit:boolean; {did we hit a ligature with a boundary character?}
@!charnode_to_t_tmp:pointer;

@ @d append_charnode_to_t(#)== begin charnode_to_t_tmp:=new_glyph(hf,#); 
    vlink(t):=charnode_to_t_tmp; t:=vlink(t); 
    node_attr(t):=hattr; add_node_attr_ref(hattr);
   end
@d set_cur_r==begin if j<n then cur_r:=qi(hu[j+1])@+else cur_r:=bchar;
    if odd(hyf[j]) then cur_rh:=hchar@+else cur_rh:=non_boundarychar;
    end

@<Set up data structures with the cursor following position |j|@>=
cur_l:=qi(hu[j]); cur_q:=t;
if j=0 then
  begin ligature_present:=init_lig; p:=init_list;
  if ligature_present then lft_hit:=init_lft;
  while p>null do
    begin append_charnode_to_t(character(p)); p:=vlink(p);
    end;
  end
else if cur_l<>non_boundarychar then append_charnode_to_t(cur_l);
lig_stack:=null; set_cur_r

@ We may want to look at the lig/kern program twice, once for a hyphen
and once for a normal letter. (The hyphen might appear after the letter
in the program, so we'd better not try to look for both at once.)

@<If there's a ligature or kern at the cursor position, update...@>=
if cur_l=non_boundarychar then
  begin k:= non_boundarychar;
  if has_left_boundary(hf) then k:=left_boundarychar;
  if k=non_boundarychar then goto done;
  end
else
  k:=cur_l;
begin
  if cur_rh<>non_boundarychar then test_char:=cur_rh@+else test_char:=cur_r;
  if disable_lig=0 and has_lig(hf,k) then begin
	lig:=get_ligature(hf,k,test_char);
    if is_ligature(lig) then begin
	  @<Update |hyphen_passed| and |hchar|, maybe goto continue@>;
      @<Carry out a ligature replacement, updating the cursor structure
        and possibly advancing~|j|; |goto continue| if the cursor doesn't
        advance, otherwise |goto done|@>;
      end;
    end
  else begin if disable_kern=0 and has_kern(hf,k) then
	  {TH: this 'Update ..' should not be needed ? }
	  @<Update |hyphen_passed| and |hchar|, maybe goto continue@>;
	  w:=get_kern(hf,k,test_char);
    end;
  end;
done:

@ @<Update |hyphen_passed| and |hchar|, maybe goto continue@>=
if cur_rh<>non_boundarychar then begin 
  hyphen_passed:=j; hchar:=non_boundarychar; cur_rh:=non_boundarychar; 
  goto continue;
  end
else begin
  if hchar<>non_boundarychar then if odd(hyf[j]) then
    begin hyphen_passed:=j; hchar:=non_boundarychar;
    end;
  end


@ @d wrap_lig(#)==if ligature_present then
    begin p:=new_ligature(hf,cur_l,vlink(cur_q));
    delete_attribute_ref(node_attr(p));
    node_attr(p):=hattr; add_node_attr_ref(hattr);
    if lft_hit then
      begin subtype(p):=3; lft_hit:=false;
      end;
    if # then if lig_stack=null then
      begin incr(subtype(p)); rt_hit:=false;
      end;
    vlink(cur_q):=p; t:=p; ligature_present:=false;
    end
@d pop_lig_stack==begin if lig_ptr(lig_stack)>null then
    begin vlink(t):=lig_ptr(lig_stack); {this is a charnode for |hu[j+1]|}
    t:=vlink(t); incr(j);
    end;
  p:=lig_stack; lig_stack:=vlink(p); ext_free_node(p,glyph_node_size);
  if lig_stack=null then set_cur_r@+else cur_r:=character(lig_stack);
  end {if |lig_stack| isn't |null| we have |cur_rh=non_boundarychar|}

@<Append a ligature and/or kern to the translation...@>=
wrap_lig(rt_hit);
if w<>0 then
  begin charnode_to_t_tmp:=new_kern(w); vlink(t):= charnode_to_t_tmp; t:=vlink(t); w:=0;
  end;
if lig_stack>null then
  begin cur_q:=t; cur_l:=character(lig_stack); ligature_present:=true;
  pop_lig_stack; goto continue;
  end

@ @<Carry out a ligature replacement, updating the cursor structure...@>=
begin if cur_l=non_boundarychar then lft_hit:=true;
if j=n then if lig_stack=null then rt_hit:=true;
check_interrupt; {allow a way out in case there's an infinite ligature loop}
case lig_type(lig) of
qi(1),qi(5):begin cur_l:=lig_replacement(lig); {\.{=:\?}, \.{=:\?>}}
  ligature_present:=true;
  end;
qi(2),qi(6):begin cur_r:=lig_replacement(lig); {\.{\?=:}, \.{\?=:>}}
  if lig_stack>null then character(lig_stack):=cur_r
  else begin lig_stack:=new_lig_item(cur_r);
    if j=n then bchar:=non_boundarychar
    else begin p:=new_glyph(hf,qi(hu[j+1])); lig_ptr(lig_stack):=p;
      delete_attribute_ref(node_attr(p));
      node_attr(p):=hattr; add_node_attr_ref(hattr); end;
    end;
  end;
qi(3):begin cur_r:=lig_replacement(lig); {\.{\?=:\?}}
  p:=lig_stack; lig_stack:=new_lig_item(cur_r); vlink(lig_stack):=p;
  end;
qi(7),qi(11):begin wrap_lig(false); {\.{\?=:\?>}, \.{\?=:\?>>}}
  cur_q:=t; cur_l:=lig_replacement(lig); ligature_present:=true;
  end;
othercases begin cur_l:=lig_replacement(lig); ligature_present:=true; {\.{=:}}
  if lig_stack>null then pop_lig_stack
  else if j=n then goto done
  else begin append_charnode_to_t(cur_r); incr(j); set_cur_r;
    end;
  end
endcases;
if lig_type(lig)>qi(4) then if lig_type(lig)<>qi(7) then goto done;
goto continue;
end
*/
