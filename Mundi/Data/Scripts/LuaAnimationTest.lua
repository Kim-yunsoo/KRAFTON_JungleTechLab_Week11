
local SkeletalComp
local AnimInstance

function BeginPlay()
print("Begin")
SkeletalComp = GetComponent(Obj, "USkeletalMeshComponent")
SkeletalComp:PlayAnimation("Data/Animations/Breathing Idle.fbx", true)
AnimInstance = SkeletalComp:GetAnimInstance()
if AnimInstance == nil then
print("없")
else
print("잇")
end
--AnimInstance:SetStartState("Test")

--SkeletalComp:PlayAnimation("Data/Animations/Standard Walk.fbx", true)
--SkeletalComp.AnimInstance:AddState("Walk", "Data/Animations/Standard Walk.fbx")

end

function EndPlay()
    
end

function Tick(dt)

end