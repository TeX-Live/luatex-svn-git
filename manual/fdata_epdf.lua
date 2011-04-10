-- $Id$

local fdata_epdf = {
  open = {
    type = "function",
    shortdesc = "Open a PDF document.",
    arguments = {
      {type = "string", name = "filename", optional = false, },
    },
    returnvalues = {
      {type = "PDFDoc", name = "var", optional = false, },
    },
  },
  Annot = {
    type = "function",
    shortdesc = "Construct an Annot object.",
    arguments = {
      {type = "XRef", name = "xref", optional = false, },
      {type = "Dict", name = "dict", optional = false, },
      {type = "Catalog", name = "catalog", optional = false, },
      {type = "Ref", name = "ref", optional = false, },
    },
    returnvalues = {
      {type = "Annot", name = "var", optional = false, },
    },
  },
  Annots = {
    type = "function",
    shortdesc = "Construct an Annots object.",
    arguments = {
      {type = "XRef", name = "xref", optional = false, },
      {type = "Catalog", name = "catalog", optional = false, },
      {type = "Object", name = "object", optional = false, },
    },
    returnvalues = {
      {type = "Annots", name = "var", optional = false, },
    },
  },
  Array = {
    type = "function",
    shortdesc = "Construct an Array object.",
    arguments = {
      {type = "XRef", name = "xref", optional = false, },
    },
    returnvalues = {
      {type = "Array", name = "var", optional = false, },
    },
  },
  Dict = {
    type = "function",
    shortdesc = "Construct a Dict object.",
    arguments = {
      {type = "XRef", name = "xref", optional = false, },
    },
    returnvalues = {
      {type = "Dict", name = "var", optional = false, },
    },
  },
  Object = {
    type = "function",
    shortdesc = "Construct an Object object.",
    arguments = {
    },
    returnvalues = {
      {type = "Object", name = "var", optional = false, },
    },
  },
  PDFRectangle = {
    type = "function",
    shortdesc = "Construct a PDFRectangle object.",
    arguments = {
    },
    returnvalues = {
      {type = "PDFRectangle", name = "var", optional = false, },
    },
  },
}

return fdata_epdf
