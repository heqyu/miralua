

local MyClass = import('MyClass')
local obj = MyClass.new(10)
print("obj", obj)  
print("obj GetValue", obj:GetValue())  -- 输出：10
obj:SetValue(20)
print("obj GetValue", obj:GetValue())  -- 输出：20
print("obj name", obj.name)  -- 输出：nil
print("set obj name to Hello")
obj.name = "Hello" 
print("obj name", obj.name)  -- 输出：Hello

-- 对于c++变量的赋值，如果类型不匹配，会报错。
-- obj.name = true
-- bad argument #3 to '__newindex' (string expected, got boolean)

-- lua侧新增的变量会存储到c++的元表中
obj.name222 = "aaaa"
print("obj name222", obj.name222)  -- 输出：aaaa


local obj2 = MyClass.new(20)
print("obj2 GetValue", obj2:GetValue())  -- 输出：20
-- print "obj2 name222
print("obj2 name222", obj2.name222)  -- 输出：aaaa


