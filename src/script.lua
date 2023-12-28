

local MyClass = import('MyClass')
local obj = MyClass.new(10)
print(obj:GetValue())  -- 输出：10
obj:SetValue(20)
print(obj:GetValue())  -- 输出：20
print(obj.name)  -- 输出：（空字符串）
obj.name = "Hello"

-- 目前的实现，这个属于无效赋值。
obj.name222 = "sdfsef"
print(obj.name222)  -- 输出：（空字符串）

print(obj.name)  -- 输出：Hello
