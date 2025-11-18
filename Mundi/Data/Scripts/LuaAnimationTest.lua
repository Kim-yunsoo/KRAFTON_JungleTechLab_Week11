
local SkeletalComp
local AnimInstance

local bWalk = true
local bJump = false

local Speed = 0.75


function BeginPlay()
print("Begin")
SkeletalComp = GetComponent(Obj, "USkeletalMeshComponent")
SkeletalComp:AddSequenceInState("Idle", "Data/Animations/Breathing Idle.fbx", 0)
SkeletalComp:AddSequenceInState("Move", "Data/Animations/Standard Walk.fbx", 0.5)
SkeletalComp:AddSequenceInState("Move", "Data/Animations/Standard Run.fbx", 1)
SkeletalComp:AddSequenceInState("Move", "Data/Animations/Walk Backward.fbx", -1)
SkeletalComp:AddSequenceInState("Jump", "Data/Animations/Jumping.fbx", 0)
SkeletalComp:SetStateLoop("Jump", false)    
SkeletalComp:SetStateExitTime("Jump", 0.55)    
SkeletalComp:AddTransition("Idle", "Move", 0.2, function() return bWalk end)
SkeletalComp:AddTransition("Move", "Idle", 0.2, function() return bWalk==false end)
SkeletalComp:AddTransition("Idle", "Jump", 0.2, function() return bJump end)
SkeletalComp:AddTransition("Move", "Jump", 0.2, function() return bJump end)
SkeletalComp:AddTransition("Jump", "Idle", 0.1, nil)
SkeletalComp:SetStartState("Idle")
end

function EndPlay()
    
end


local JumpDelay = 0.5
local CurJumpDelay = 0
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

    if InputManager:IsKeyDown('E') and bJump == false then
    print("Jump")
    bJump = true
    CurJumpDelay = JumpDelay
    end

    if bJump == true then
    CurJumpDelay  = CurJumpDelay - dt
    print("JumpDelay")
    end

    if CurJumpDelay < 0 then
    bJump = false
    print("JumpEnd")

    end


    SkeletalComp:SetBlendValueInState("Move", Speed)

end