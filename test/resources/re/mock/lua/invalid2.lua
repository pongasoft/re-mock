-- line 1

function level2()
  jbox.invalid{}
end

function level1()
  level2()
end

-- calling function
level1()
