local upvaluejoin, getmetatable = debug.upvaluejoin, debug.getmetatable

-- test whether __ipairs works
local function myipairs(x)
  return function(x, i) return i == 0 and 1 or nil end, x, 0
end
local t = setmetatable({}, {__ipairs=myipairs})
local n = 0
for i in ipairs(t) do n = n+1 end

if n ~= 1 then
  local orig_ipairs = ipairs
  function ipairs(t)
    local mt = getmetatable(t)
    if mt.__ipairs then
      local a,b,c = mt.__ipairs(t)
      return a,b,c
    end
    return orig_ipairs(t)
  end
end

-- set an unpvalue in a function without affecting other functions
return function(fn, i, env)
  upvaluejoin(fn, i, function() return env end, 1)
end
