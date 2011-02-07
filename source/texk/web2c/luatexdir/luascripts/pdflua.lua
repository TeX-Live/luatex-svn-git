-- pdflua.lua

-- Copyright 2010-2011 Taco Hoekwater <taco@@luatex.org>
-- Copyright 2010-2011 Hartmut Henkel <hartmut@@luatex.org>

-- This file is part of LuaTeX.

-- LuaTeX is free software; you can redistribute it and/or modify it under
-- the terms of the GNU General Public License as published by the Free
-- Software Foundation; either version 2 of the License, or (at your
-- option) any later version.

-- LuaTeX is distributed in the hope that it will be useful, but WITHOUT
-- ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
-- FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
-- License for more details.

-- You should have received a copy of the GNU General Public License along
-- with LuaTeX; if not, see <http://www.gnu.org/licenses/>.

-- $Id$
-- $URL$

-- this is early work in progress...

local doc = {
  pagelist = {},
  catalog = pdfobj.newDict(),
  info = pdfobj.newDict(),
}

------------------------------------------------------------------------

local get_pagelist = function()
  return doc.pagelist
end

------------------------------------------------------------------------

local make_pagedict = function()
  local pagedict_tbl = {
    Type = pdfobj.newName("Page"),
    Contents = pdfobj.newRef(pdfobj.get_last_stream()),
    Resources = pdfobj.newRef(pdfobj.get_last_resources()),
    MediaBox = pdfobj.newArray({
      pdfobj.newReal(0, 0),
      pdfobj.newReal(0, 0),
      pdfobj.newReal(pdfobj.get_cur_page_size_h()),
      pdfobj.newReal(pdfobj.get_cur_page_size_v()),
    })
  }
  local annots = pdfobj.get_annots()
  local beads = pdfobj.get_beads()
  local group = pdfobj.get_group()
  if annots ~= 0 then
    pagedict_tbl.Annots = pdfobj.newRef(annots)
  end
  if beads ~= 0 then
    pagedict_tbl.Beads = pdfobj.newRef(beads)
  end
  if group ~= 0 then
    pagedict_tbl.Group = pdfobj.newRef(group)
  end
  local pagedict = pdfobj.newDict(pagedict_tbl)
  pagedict:setobjnum(pdfobj.get_last_page())

  -- instead of \pdfpageattr and pdf.pageattributes maybe
  -- better use PDF objects as true dict entries

  local pageattr = pdfobj.get_pdf_page_attr() -- a string
  if pdf.pageattributes then
    assert(type(pdf.pageattributes) == "string")
    pageattr = pageattr .. "\n" .. pdf.pageattributes
  end
  if string.len(pageattr) > 0 then
    pagedict:setattr(pageattr)
  end
  --
  local pagenum = pdfobj.get_total_pages()
  doc.pagelist[pagenum] = pagedict
end

------------------------------------------------------------------------

local make_pageslist = function(pxlist) -- px is page or pages
  local kids_max = 8
  assert(#pxlist > 0)
  local pageslist
  local ispagelist = pxlist[1]:get().Type:get() == "Page"
  if ispagelist or (#pxlist > 1) then
    local kids, pages_tbl, parent_objnum, pages
    pageslist = {} -- new list of /Pages objects
    for _, px in ipairs(pxlist) do
      local pxdict_tbl = px:get()
      local px_objnum = px:getobjnum()
      if kids == nil or #kids == kids_max then
        parent_objnum = pdf.reserveobj()
        pages_tbl = {}
        kids = {}
        pages_tbl.Kids = pdfobj.newArray(kids)
        pages_tbl.Type = pdfobj.newName("Pages")
        pages_tbl.Count = pdfobj.newInt()
        pages = pdfobj.newDict(pages_tbl)
        pages:setobjnum(parent_objnum)
        pageslist[#pageslist + 1] = pages
      end
      kids[#kids + 1] = pdfobj.newRef(px_objnum)
      if ispagelist then
        pages_tbl.Count:incr()
      else
        local c = pages_tbl.Count:get()
        local k = pxdict_tbl.Count:get()
        pages_tbl.Count:set(c + k)
      end
      pxdict_tbl.Parent = pdfobj.newRef(parent_objnum)
    end
  else
    pageslist = pxlist
  end
  return pageslist
end

local output_pxlist = function(pxlist)
  for k, v in ipairs(pxlist) do
    v:pdfout()
  end
end

local output_pagestree = function()
  local pagelist = doc.pagelist
  local pageslist = make_pageslist(pagelist)
  output_pxlist(pagelist)
  while #pageslist > 1 do
    local tmplist = make_pageslist(pageslist)
    output_pxlist(pageslist)
    pageslist = tmplist
  end
  output_pxlist(pageslist)
  local pagesrootobjnum = pageslist[1]:getobjnum()
  return pagesrootobjnum
end

------------------------------------------------------------------------

local make_catalog = function()
  local catalog = doc.catalog
  local catalog_tbl = catalog:get()
  if catalog_tbl == nil then
    catalog_tbl = {}
    catalog:set(catalog_tbl)
  end
  assert(type(catalog_tbl) == "table")
  local catalog_objnum = catalog:getobjnum()
  if catalog_objnum == 0 then
    catalog_objnum = pdf.reserveobj()
    catalog:setobjnum(catalog_objnum)
  end

  local catalogtoks = pdfobj.get_catalogtoks() -- a string
  if string.len(catalogtoks) > 0 then
    catalog:setattr(catalogtoks)
  end

  catalog_tbl.Type = pdfobj.newName("Catalog")

  catalog_tbl.Pages = pdfobj.newRef()

  local threads = pdfobj.get_threads()
  local outlines = pdfobj.get_outlines()
  local names = pdfobj.get_names()
  local openaction = pdfobj.get_catalog_openaction()
  if catalog_tbl.Threads == nil and threads ~= 0 then
    catalog_tbl.Threads = pdfobj.newRef(threads)
  end
  if catalog_tbl.Outlines == nil and outlines ~= 0 then
    catalog_tbl.Outlines = pdfobj.newRef(outlines)
  end
  if catalog_tbl.Names == nil and names ~= 0 then
    catalog_tbl.Names = pdfobj.newRef(names)
  end
  if catalog_tbl.OpenAction == nil and openaction ~= 0 then
    catalog_tbl.OpenAction = pdfobj.newRef(openaction)
  end

  return catalog_objnum
end

------------------------------------------------------------------------

local make_info = function()
  local info = doc.info
  local info_tbl = info:get()
  if info_tbl == nil then
    info_tbl = {}
    info:set(info_tbl)
  end
  assert(type(info_tbl) == "table")
  local info_objnum = info:getobjnum()
  if info_objnum == 0 then
    info_objnum = pdf.reserveobj()
    info:setobjnum(info_objnum)
  end

  local infotoks = pdfobj.get_infotoks() -- a string
  if string.len(infotoks) > 0 then
    info:setattr(infotoks)
  end

  info_tbl.Type = pdfobj.newName("Info")

  if string.find(infotoks, "/Producer") == nil then
    local lver = pdfobj.get_luatex_version()
    local luatex_version = math.floor(lver / 100) .. "." .. math.floor(lver % 100)
    local luatex_revision = pdfobj.get_luatex_revision()
    local ps = "LuaTeX-" .. luatex_version .. "." .. luatex_revision
    info_tbl.Producer = pdfobj.newString(ps)
  end

  if info_tbl.Creator == nil and string.find(infotoks, "/Creator") == nil then
    info_tbl.Creator = pdfobj.newString("TeX")
  end

  if info_tbl.CreationDate == nil and string.find(infotoks, "/CreationDate") == nil then
    info_tbl.CreationDate = pdfobj.newString(pdfobj.get_creationdate())
  end

  if info_tbl.ModDate == nil and string.find(infotoks, "/ModDate") == nil then
    info_tbl.ModDate = pdfobj.newString(pdfobj.get_creationdate())
  end

  if info_tbl.Trapped == nil and string.find(infotoks, "/Trapped") == nil then
    info_tbl.Trapped = pdfobj.newName("False")
  end

  if info_tbl["PTEX.Fullbanner"] == nil then
    info_tbl["PTEX.Fullbanner"] = pdfobj.newString(pdfobj.get_pdftex_banner())
  end

  return info_objnum
end

------------------------------------------------------------------------

local write_pending_objects = function()
  local pagesrootobjnum = output_pagestree()
  doc.catalog:get().Pages:set(pagesrootobjnum) -- well...
  doc.catalog:pdfout()
  doc.info:pdfout()
end

------------------------------------------------------------------------

local pdflua = {
  get_pagelist = get_pagelist,
  make_pagedict = make_pagedict,
  make_catalog = make_catalog,
  make_info = make_info,
  write_pending_objects = write_pending_objects,
}

return pdflua
