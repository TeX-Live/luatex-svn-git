-- $Id$
  
local fdata_epdf = {
  functions = {
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
  },
  methods = {
    Annot = {
      isOK = {
        type = "function",
        shortdesc = "Check if Annot object is ok.",
        arguments = {
          {type = "Annot", name = "annot", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },
      getAppearance = {
        type = "function",
        shortdesc = "Get Appearance object.",
        arguments = {
          {type = "Annot", name = "annot", optional = false, },
        },
        returnvalues = {
          {type = "Object", name = "var", optional = false, },
        },
      },
      getBorder = {
        type = "function",
        shortdesc = "Get AnnotBorder object.",
        arguments = {
          {type = "Annot", name = "annot", optional = false, },
        },
        returnvalues = {
          {type = "AnnotBorder", name = "var", optional = false, },
        },
      },
      match = {
        type = "function",
        shortdesc = "Check if object number and generation matches Ref.",
        arguments = {
          {type = "Annot", name = "annot", optional = false, },
          {type = "Ref", name = "ref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },
    },
    AnnotBorderStyle = {
      getWidth = {
        type = "function",
        shortdesc = "Get border width.",
        arguments = {
          {type = "AnnotBorderStyle", name = "annotborderstyle", optional = false, },
        },
        returnvalues = {
          {type = "number", name = "var", optional = false, },
        },
      },
    },
    Annots = {
      getNumAnnots = {
        type = "function",
        shortdesc = "Get number of Annots objects.",
        arguments = {
          {type = "Annots", name = "annots", optional = false, },
        },
        returnvalues = {
          {type = "integer", name = "var", optional = false, },
        },
      },
      getAnnot = {
        type = "function",
        shortdesc = "Get Annot object.",
        arguments = {
          {type = "Annots", name = "annots", optional = false, },
          {type = "integer", name = "integer", optional = false, },
        },
        returnvalues = {
          {type = "Annot", name = "var", optional = false, },
        },
      },
    },
    Array = {
      incRef = {
        type = "function",
        shortdesc = "Increment reference count to Array.",
        arguments = {
          {type = "Array", name = "array", optional = false, },
        },
        returnvalues = {
        },
      },
      decRef = {
        type = "function",
        shortdesc = "Decrement reference count to Array.",
        arguments = {
          {type = "Array", name = "array", optional = false, },
        },
        returnvalues = {
        },
      },
      getLength = {
        type = "function",
        shortdesc = "Get Array length.",
        arguments = {
          {type = "Array", name = "array", optional = false, },
        },
        returnvalues = {
          {type = "integer", name = "var", optional = false, },
        },
      },
      add = {
        type = "function",
        shortdesc = "Add Object to Array.",
        arguments = {
          {type = "Array", name = "array", optional = false, },
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
        },
      },
      get = {
        type = "function",
        shortdesc = "Get Object from Array.",
        arguments = {
          {type = "Array", name = "array", optional = false, },
          {type = "integer", name = "integer", optional = false, },
        },
        returnvalues = {
          {type = "Object", name = "var", optional = false, },
        },
      },
      getNF = {
        type = "function",
        shortdesc = "Get Object from Array, not resolving indirection.",
        arguments = {
          {type = "Array", name = "array", optional = false, },
          {type = "integer", name = "integer", optional = false, },
        },
        returnvalues = {
          {type = "Object", name = "var", optional = false, },
        },
      },
      getString = {
        type = "function",
        shortdesc = "Get String from Array.",
        arguments = {
          {type = "Array", name = "array", optional = false, },
          {type = "integer", name = "integer", optional = false, },
        },
        returnvalues = {
          {type = "string", name = "var", optional = false, },
        },
      },
    },
  }
}
  
return fdata_epdf
