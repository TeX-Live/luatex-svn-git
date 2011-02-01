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

local pagelist = {}

------------------------------------------------------------------------

local makepagedict = function()
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
  pagelist[pagenum] = pagedict
end

------------------------------------------------------------------------

local make_pageslist = function(pxlist) -- px is page or pages
  local kids_max = 4
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

outputpagestree = function()
  local pageslist = make_pageslist(pagelist)
  output_pxlist(pagelist)
  while #pageslist > 1 do
    local tmplist = make_pageslist(pageslist)
    output_pxlist(pageslist)
    pageslist = tmplist
  end
  output_pxlist(pageslist)
  local rootobjnum = pageslist[1]:getobjnum()
  return rootobjnum
end

------------------------------------------------------------------------

local makecatalog = function()
  local catalog_tbl = {
    Type = pdfobj.newName("Catalog"),
    Pages = pdfobj.newRef(pdfobj.get_last_pages()),
  }
  local threads = pdfobj.get_threads()
  local outlines = pdfobj.get_outlines()
  local names = pdfobj.get_names()
  local openaction = pdfobj.get_catalog_openaction()
  if threads ~= 0 then
    catalog_tbl.Threads = pdfobj.newRef(threads)
  end
  if outlines ~= 0 then
    catalog_tbl.Outlines = pdfobj.newRef(outlines)
  end
  if names ~= 0 then
    catalog_tbl.Names = pdfobj.newRef(names)
  end
  if openaction ~= 0 then
    catalog_tbl.OpenAction = pdfobj.newRef(openaction)
  end
  local catalog = pdfobj.newDict(catalog_tbl)
  local catalog_objnum = pdf.reserveobj()
  catalog:setobjnum(catalog_objnum)

  -- instead of \pdfcatalogtoks
  -- better use PDF objects as true dict entries

  local catalogtoks = pdfobj.get_catalogtoks() -- a string
  if string.len(catalogtoks) > 0 then
    catalog:setattr(catalogtoks)
  end
  --
  catalog:pdfout()
  return catalog_objnum
end

------------------------------------------------------------------------

local pdflua = {
  makecatalog = makecatalog,
  makepagedict = makepagedict,
  outputpagestree = outputpagestree,
}

return pdflua
