local function createNode(v)
    return { value = v, left = nil, right = nil }
end

local function buildTree(nums)
    table.sort(nums)

    local function buildTreeHelper(left, right)
        if left > right then
            return nil
        end

        local mid = math.floor((left + right) / 2)
        local node = createNode(nums[mid])
        node.left = buildTreeHelper(left, mid - 1)
        node.right = buildTreeHelper(mid + 1, right)

        return node
    end 

    return buildTreeHelper(1, #nums)
end

local function inorderTraversal(root)
    if root == nil then
        return
    end

    inorderTraversal(root.left)
    print(root.value)
    inorderTraversal(root.right)
end 

local nums = {1, 4, 2, 3, 6, 8, 9, 10, 5, 7}
local root = buildTree(nums)
inorderTraversal(root)