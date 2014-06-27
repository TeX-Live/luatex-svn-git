-- $Id: pdflua.lua 5022 2014-06-06 19:22:31Z oneiros $
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
