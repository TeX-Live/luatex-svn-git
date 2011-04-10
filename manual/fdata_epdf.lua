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
    Catalog = {
      isOK = {
        type = "function",
        shortdesc = "Check if Catalog object is ok.",
        arguments = {
          {type = "Catalog", name = "catalog", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },
      getNumPages = {
        type = "function",
        shortdesc = "Get total number of pages.",
        arguments = {
          {type = "Catalog", name = "catalog", optional = false, },
        },
        returnvalues = {
          {type = "integer", name = "var", optional = false, },
        },
      },
      getPage = {
        type = "function",
        shortdesc = "Get Page.",
        arguments = {
          {type = "Catalog", name = "catalog", optional = false, },
          {type = "integer", name = "integer", optional = false, },
        },
        returnvalues = {
          {type = "Page", name = "var", optional = false, },
        },
      },
      getPageRef = {
        type = "function",
        shortdesc = "Get the reference to a Page object.",
        arguments = {
          {type = "Catalog", name = "catalog", optional = false, },
          {type = "integer", name = "integer", optional = false, },
        },
        returnvalues = {
          {type = "Ref", name = "var", optional = false, },
        },
      },
      getBaseURI = {
        type = "function",
        shortdesc = "Get base URI, if any.",
        arguments = {
          {type = "Catalog", name = "catalog", optional = false, },
        },
        returnvalues = {
          {type = "string", name = "var", optional = false, },
        },
      },
      readMetadata = {
        type = "function",
        shortdesc = "Get the contents of the Metadata stream.",
        arguments = {
          {type = "Catalog", name = "catalog", optional = false, },
        },
        returnvalues = {
          {type = "string", name = "var", optional = false, },
        },
      },
      getStructTreeRoot = {
        type = "function",
        shortdesc = "Get the structure tree root object.",
        arguments = {
          {type = "Catalog", name = "catalog", optional = false, },
        },
        returnvalues = {
          {type = "Object", name = "var", optional = false, },
        },
      },
      findPage = {
        type = "function",
        shortdesc = "Get a Page number by object number and generation.",
        arguments = {
          {type = "Catalog", name = "catalog", optional = false, },
          {type = "integer", name = "number", optional = false, },
          {type = "integer", name = "generation", optional = false, },
        },
        returnvalues = {
          {type = "integer", name = "var", optional = false, },
        },
      },
      findDest = {
        type = "function",
        shortdesc = "Find a named destination.",
        arguments = {
          {type = "Catalog", name = "catalog", optional = false, },
          {type = "string", name = "string", optional = false, },
        },
        returnvalues = {
          {type = "LinkDest", name = "var", optional = false, },
        },
      },
      getDests = {
        type = "function",
        shortdesc = "Get destinations object.",
        arguments = {
          {type = "Catalog", name = "catalog", optional = false, },
        },
        returnvalues = {
          {type = "Object", name = "var", optional = false, },
        },
      },
      numEmbeddedFiles = {
        type = "function",
        shortdesc = "Get number of embedded files.",
        arguments = {
          {type = "Catalog", name = "catalog", optional = false, },
        },
        returnvalues = {
          {type = "integer", name = "var", optional = false, },
        },
      },
      numJS = {
        type = "function",
        shortdesc = "Get number of javascript scripts.",
        arguments = {
          {type = "Catalog", name = "catalog", optional = false, },
        },
        returnvalues = {
          {type = "integer", name = "var", optional = false, },
        },
      },
      getJS = {
        type = "function",
        shortdesc = "Get javascript script.",
        arguments = {
          {type = "Catalog", name = "catalog", optional = false, },
          {type = "integer", name = "integer", optional = false, },
        },
        returnvalues = {
          {type = "string", name = "var", optional = false, },
        },
      },
      getOutline = {
        type = "function",
        shortdesc = "Get Outline object.",
        arguments = {
          {type = "Catalog", name = "catalog", optional = false, },
        },
        returnvalues = {
          {type = "Object", name = "var", optional = false, },
        },
      },
      getAcroForm = {
        type = "function",
        shortdesc = "Get AcroForm object.",
        arguments = {
          {type = "Catalog", name = "catalog", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },
    },
    Dict = {
      incRef = {
        type = "function",
        shortdesc = "Increment reference count to Dict.",
        arguments = {
          {type = "Dict", name = "dict", optional = false, },
        },
        returnvalues = {
        },
      },
      decRef = {
        type = "function",
        shortdesc = "Decrement reference count to Dict.",
        arguments = {
          {type = "Dict", name = "dict", optional = false, },
        },
        returnvalues = {
        },
      },
      getLength = {
        type = "function",
        shortdesc = "Get Dict length.",
        arguments = {
          {type = "Dict", name = "dict", optional = false, },
        },
        returnvalues = {
          {type = "integer", name = "var", optional = false, },
        },
      },
      add = {
        type = "function",
        shortdesc = "Add Object to Dict.",
        arguments = {
          {type = "Dict", name = "dict", optional = false, },
          {type = "string", name = "string", optional = false, },
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
        },
      },
      set = {
        type = "function",
        shortdesc = "Set Object in Dict.",
        arguments = {
          {type = "Dict", name = "dict", optional = false, },
          {type = "string", name = "string", optional = false, },
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
        },
      },
      remove = {
        type = "function",
        shortdesc = "Remove entry from Dict.",
        arguments = {
          {type = "Dict", name = "dict", optional = false, },
          {type = "string", name = "string", optional = false, },
        },
        returnvalues = {
        },
      },
      is = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Dict", name = "dict", optional = false, },
          {type = "string", name = "string", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },
      lookup = {
        type = "function",
        shortdesc = "Look up Dict entry.",
        arguments = {
          {type = "Dict", name = "dict", optional = false, },
          {type = "string", name = "string", optional = false, },
        },
        returnvalues = {
          {type = "Object", name = "var", optional = false, },
        },
      },
      lookupNF = {
        type = "function",
        shortdesc = "Look up Dict entry, not resolving indirection.",
        arguments = {
          {type = "Dict", name = "dict", optional = false, },
          {type = "string", name = "string", optional = false, },
        },
        returnvalues = {
          {type = "Object", name = "var", optional = false, },
        },
      },
      lookupInt = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Dict", name = "dict", optional = false, },
          {type = "string", name = "string", optional = false, },
          {type = "string", name = "string", optional = false, },
        },
        returnvalues = {
          {type = "integer", name = "var", optional = false, },
        },
      },
      getKey = {
        type = "function",
        shortdesc = "Get key from Dict by number.",
        arguments = {
          {type = "Dict", name = "dict", optional = false, },
          {type = "integer", name = "integer", optional = false, },
        },
        returnvalues = {
          {type = "string", name = "var", optional = false, },
        },
      },
      getVal = {
        type = "function",
        shortdesc = "Get value from Dict by number.",
        arguments = {
          {type = "Dict", name = "dict", optional = false, },
          {type = "integer", name = "integer", optional = false, },
        },
        returnvalues = {
          {type = "Object", name = "var", optional = false, },
        },
      },
      getValNF = {
        type = "function",
        shortdesc = "Get value from Dict by number, not resolving indirection.",
        arguments = {
          {type = "Dict", name = "dict", optional = false, },
          {type = "integer", name = "integer", optional = false, },
        },
        returnvalues = {
          {type = "Object", name = "var", optional = false, },
        },
      },
    },
    LinkDest = {
      isOK = {
        type = "function",
        shortdesc = "Check if LinkDest object is ok.",
        arguments = {
          {type = "LinkDest", name = "linkdest", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },
      getKind = {
        type = "function",
        shortdesc = "Get number of LinkDest kind.",
        arguments = {
          {type = "LinkDest", name = "linkdest", optional = false, },
        },
        returnvalues = {
          {type = "integer", name = "var", optional = false, },
        },
      },
      getKindName = {
        type = "function",
        shortdesc = "Get name of LinkDest kind.",
        arguments = {
          {type = "LinkDest", name = "linkdest", optional = false, },
        },
        returnvalues = {
          {type = "string", name = "var", optional = false, },
        },
      },
      isPageRef = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "LinkDest", name = "linkdest", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },
      getPageNum = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "LinkDest", name = "linkdest", optional = false, },
        },
        returnvalues = {
          {type = "integer", name = "var", optional = false, },
        },
      },
      getPageRef = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "LinkDest", name = "linkdest", optional = false, },
        },
        returnvalues = {
          {type = "Ref", name = "var", optional = false, },
        },
      },
      getLeft = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "LinkDest", name = "linkdest", optional = false, },
        },
        returnvalues = {
          {type = "number", name = "var", optional = false, },
        },
      },
      getBottom = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "LinkDest", name = "linkdest", optional = false, },
        },
        returnvalues = {
          {type = "number", name = "var", optional = false, },
        },
      },
      getRight = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "LinkDest", name = "linkdest", optional = false, },
        },
        returnvalues = {
          {type = "number", name = "var", optional = false, },
        },
      },
      getTop = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "LinkDest", name = "linkdest", optional = false, },
        },
        returnvalues = {
          {type = "number", name = "var", optional = false, },
        },
      },
      getZoom = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "LinkDest", name = "linkdest", optional = false, },
        },
        returnvalues = {
          {type = "number", name = "var", optional = false, },
        },
      },
      getChangeLeft = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "LinkDest", name = "linkdest", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },
      getChangeTop = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "LinkDest", name = "linkdest", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },
      getChangeZoom = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "LinkDest", name = "linkdest", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },
    },

------------------------------------------------------------------------

    Object = {
      initBool = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      initInt = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      initReal = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      initString = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      initName = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      initNull = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      initArray = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      initDict = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      initStream = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      initRef = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      initCmd = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      initError = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      initEOF = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      fetch = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getType = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getTypeName = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isBool = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isInt = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isReal = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isNum = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isString = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isName = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isNull = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isArray = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isDict = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isStream = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isRef = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isCmd = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isError = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isEOF = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isNone = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getBool = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getInt = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getReal = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getNum = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getString = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getName = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getArray = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getDict = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getStream = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getRef = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getRefNum = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getRefGen = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getCmd = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      arrayGetLength = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      arrayAdd = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      arrayGet = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      arrayGetNF = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      dictGetLength = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      dictAdd = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      dictSet = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      dictLookup = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      dictLookupNF = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      dictGetKey = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      dictGetVal = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      dictGetValNF = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      streamIs = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      streamReset = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      streamGetChar = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      streamLookChar = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      streamGetPos = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      streamSetPos = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      streamGetDict = {
        type = "function",
        shortdesc = "TODO",
        arguments = {
          {type = "Object", name = "object", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

    },

------------------------------------------------------------------------

    Page = {

      isOK = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getNum = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getMediaBox = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getCropBox = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isCropped = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getMediaWidth = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getMediaHeight = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getCropWidth = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getCropHeight = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getBleedBox = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getTrimBox = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getArtBox = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getRotate = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getLastModified = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getBoxColorInfo = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getGroup = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getMetadata = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getPieceInfo = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getSeparationInfo = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getResourceDict = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getAnnots = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getLinks = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getContents = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

    },

------------------------------------------------------------------------

    PDFDoc = {

      isOK = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getErrorCode = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getErrorCodeName = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getFileName = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getXRef = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getCatalog = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getPageMediaWidth = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getPageMediaHeight = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getPageCropWidth = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getPageCropHeight = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getNumPages = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      readMetadata = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getStructTreeRoot = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      findPage = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getLinks = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      findDest = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isEncrypted = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      okToPrint = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      okToChange = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      okToCopy = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      okToAddNotes = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isLinearized = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getDocInfo = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getDocInfoNF = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getPDFMajorVersion = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getPDFMinorVersion = {
        type = "function",
        shortdesc = "Check if Page object is ok.",
        arguments = {
          {type = "Page", name = "page", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

    },

------------------------------------------------------------------------

    PDFRectangle = {
      isValid = {
        type = "function",
        shortdesc = "Check if PDFRectangle object is valid.",
        arguments = {
          {type = "PDFRectangle", name = "pdfrectangle", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },
    },

------------------------------------------------------------------------

    Stream = {

      getKind = {
        type = "function",
        shortdesc = "Check if Stream object is valid.",
        arguments = {
          {type = "Stream", name = "stream", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getKindName = {
        type = "function",
        shortdesc = "Check if Stream object is valid.",
        arguments = {
          {type = "Stream", name = "stream", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      reset = {
        type = "function",
        shortdesc = "Check if Stream object is valid.",
        arguments = {
          {type = "Stream", name = "stream", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      close = {
        type = "function",
        shortdesc = "Check if Stream object is valid.",
        arguments = {
          {type = "Stream", name = "stream", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getChar = {
        type = "function",
        shortdesc = "Check if Stream object is valid.",
        arguments = {
          {type = "Stream", name = "stream", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      lookChar = {
        type = "function",
        shortdesc = "Check if Stream object is valid.",
        arguments = {
          {type = "Stream", name = "stream", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getRawChar = {
        type = "function",
        shortdesc = "Check if Stream object is valid.",
        arguments = {
          {type = "Stream", name = "stream", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getUnfilteredChar = {
        type = "function",
        shortdesc = "Check if Stream object is valid.",
        arguments = {
          {type = "Stream", name = "stream", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      unfilteredReset = {
        type = "function",
        shortdesc = "Check if Stream object is valid.",
        arguments = {
          {type = "Stream", name = "stream", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getPos = {
        type = "function",
        shortdesc = "Check if Stream object is valid.",
        arguments = {
          {type = "Stream", name = "stream", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isBinary = {
        type = "function",
        shortdesc = "Check if Stream object is valid.",
        arguments = {
          {type = "Stream", name = "stream", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getUndecodedStream = {
        type = "function",
        shortdesc = "Check if Stream object is valid.",
        arguments = {
          {type = "Stream", name = "stream", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getDict = {
        type = "function",
        shortdesc = "Check if Stream object is valid.",
        arguments = {
          {type = "Stream", name = "stream", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

    },

------------------------------------------------------------------------

    XRef = {

      isOK = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getErrorCode = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      isEncrypted = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      okToPrint = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      okToPrintHighRes = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      okToChange = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      okToCopy = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      okToAddNotes = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      okToFillForm = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      okToAccessibility = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      okToAssemble = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getCatalog = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      fetch = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getDocInfo = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getDocInfoNF = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getNumObjects = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getRootNum = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getRootGen = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getSize = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

      getTrailerDict = {
        type = "function",
        shortdesc = "Check if XRef object is ok.",
        arguments = {
          {type = "XRef", name = "xref", optional = false, },
        },
        returnvalues = {
          {type = "boolean", name = "var", optional = false, },
        },
      },

    },

------------------------------------------------------------------------

  }
}

return fdata_epdf
