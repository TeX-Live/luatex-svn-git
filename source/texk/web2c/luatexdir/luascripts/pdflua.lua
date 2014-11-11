-- $Id: pdflua.lua 5075 2014-10-24 16:58:43Z oneiros $
-- $URL: https://foundry.supelec.fr/svn/luatex/branches/experimental/source/texk/web2c/luatexdir/luascripts/pdflua.lua $

-- this is early work in progress...

------------------------------------------------------------------------

beginpage = function(a)
end

endpage = function(a)
end

outputpagestree = function()
end

------------------------------------------------------------------------

local pdflua = {
  beginpage = beginpage,
  endpage = endpage,
  outputpagestree = outputpagestree,
}

return pdflua
