
local SkeletalComp
local AnimInstance

local bWalk = true



function BeginPlay()
print("Begin")
SkeletalComp = GetComponent(Obj, "USkeletalMeshComponent")
SkeletalComp:AddState("Idle", "Data/Animations/Breathing Idle.fbx")
SkeletalComp:AddState("Walk", "Data/Animations/Standard Walk.fbx")
SkeletalComp:AddTransition("Idle","Walk", 0.2, function() return bWalk end)
SkeletalComp:AddTransition("Walk","Idle", 0.2, function() return bWalk==false end)
SkeletalComp:SetStartState("Idle")
end

function EndPlay()
    
end

function Tick(dt)
    if InputManager:IsKeyDown('W') then
    bWalk = true
    else
    bWalk = false
    end

end