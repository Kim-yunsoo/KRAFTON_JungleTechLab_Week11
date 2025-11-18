
local SkeletalComp
local AnimInstance

local bWalk = true

local Speed = 0.75


function BeginPlay()
print("Begin")
SkeletalComp = GetComponent(Obj, "USkeletalMeshComponent")
SkeletalComp:AddSequenceInState("Idle", "Data/Animations/Breathing Idle.fbx", 0)
SkeletalComp:AddSequenceInState("Move", "Data/Animations/Standard Walk.fbx", 0.5)
SkeletalComp:AddSequenceInState("Move", "Data/Animations/Standard Run.fbx", 1)
SkeletalComp:AddSequenceInState("Move", "Data/Animations/Walk Backward.fbx", -1)
SkeletalComp:AddTransition("Idle","Move", 0.2, function() return bWalk end)
SkeletalComp:AddTransition("Move","Idle", 0.2, function() return bWalk==false end)
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

    if InputManager:IsKeyDown('A') then
    Speed = Speed - dt
    end

    if InputManager:IsKeyDown('D') then
    Speed = Speed + dt
    end

    SkeletalComp:SetBlendValueInState("Move", Speed)

end