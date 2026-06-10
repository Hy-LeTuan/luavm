local function twoSumWithMap(nums, target)
    table.sort(nums)

    local map = {}
    for i, num in ipairs(nums) do
        local complement = target - num
        if map[complement] then
            return map[complement], i
        end
        map[num] = i
    end
    return nil
end

local function twoSumWithTwoPointers(nums, target)
    table.sort(nums)

    local left = 1
    local right = #nums
    while left < right do
        local sum = nums[left] + nums[right]
        if sum == target then
            return left, right
        elseif sum < target then
            left = left + 1
        else
            right = right - 1
        end
    end
    return nil
end

local function twoSum(nums, target, f)
    local i, j = f(nums, target)
    print("The two numbers: ", nums[i], nums[j])
end 


local nums = {2,7,11,15}
local target = 9

twoSum(nums, target, twoSumWithMap)
twoSum(nums, target, twoSumWithTwoPointers)

local nums = {-3, 4, 3, 90}
local target = 0

twoSum(nums, target, twoSumWithMap)
twoSum(nums, target, twoSumWithTwoPointers)
