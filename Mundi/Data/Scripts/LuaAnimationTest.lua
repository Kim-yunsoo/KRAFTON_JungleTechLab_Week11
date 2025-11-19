
local SkeletalComp
local AnimInstance

local bWalk = true
local bJump = false

local Speed = 0.75

local ForwardVector = Vector(1, 0, 0)
local UpVector = Vector(0, 0, 1)
local CameraLocation = Vector(0, 0, 0)

local BackDistance = 7.0
local UpDistance = 2.0

local YawSensitivity = 0.005
local PitchSensitivity = 0.0025

local MovementDelta = 10


function BeginPlay()
print("Begin")
SkeletalComp = GetComponent(Obj, "USkeletalMeshComponent")
SkeletalComp:AddSequenceInState("Idle", "Data/Animations/Breathing Idle.fbx", 0)
SkeletalComp:AddSequenceInState("Move", "Data/Animations/Standard Walk.fbx", 0)
SkeletalComp:AddSequenceInState("Move", "Data/Animations/Standard Run.fbx", 1)
SkeletalComp:AddSequenceInState("Jump", "Data/Animations/Jumping.fbx", 0)
SkeletalComp:SetStateSpeed("Idle", 0.85)
SkeletalComp:SetStateSpeed("Move", 0.85)
SkeletalComp:SetStateLoop("Jump", false)    
SkeletalComp:SetStateExitTime("Jump", 0.55)    
SkeletalComp:AddTransition("Idle", "Move", 0.2, function() return bWalk end)
SkeletalComp:AddTransition("Move", "Idle", 0.2, function() return bWalk==false end)
SkeletalComp:AddTransition("Idle", "Jump", 0.2, function() return bJump end)
SkeletalComp:AddTransition("Move", "Jump", 0.2, function() return bJump end)
SkeletalComp:AddTransition("Jump", "Idle", 0.1, nil)
SkeletalComp:SetStartState("Idle")

-- 카메라 초기화
local Camera = GetCamera()
if Camera then
    Camera:SetCameraForward(ForwardVector)
end
ForwardVector = NormalizeCopy(ForwardVector)




end

function EndPlay()
    
end


local JumpDelay = 0.5
local CurJumpDelay = 0
function Tick(dt)
    if InputManager:IsKeyDown('W') then
    bWalk = true
    MoveForward(MovementDelta * dt) 
    else
    bWalk = false
    end

    if InputManager:IsKeyDown('A') then
    Speed = Speed - dt
    MoveRight(-MovementDelta * dt) 
    end

    if InputManager:IsKeyDown('D') then
    Speed = Speed + dt
    MoveRight(MovementDelta * dt)
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
    if Speed < 0 then
    Speed = 0
elseif Speed > 1 then
    Speed = 1
end

    RotateCamera() -- 마우스로 카메라 회전
    UpdateCamera() -- 카메라 위치/방향 갱신
end

 -- ===== 카메라 관련 함수들 =====

 -- 벡터 정규화 (복사본 반환)
function NormalizeCopy(V)
    local Out = Vector(V.X, V.Y, V.Z)
    Out:Normalize()
    return Out
end

-- 축 기준 회전
function RotateAroundAxis(VectorIn, Axis, Angle)
    local UnitAxis = NormalizeCopy(Axis)
    local CosA, SinA = math.cos(Angle), math.sin(Angle)
    local AxisCrossVector = FVector.Cross(UnitAxis, VectorIn)
    local AxisDotVector   = FVector.Dot(UnitAxis, VectorIn)

    return VectorIn * CosA + AxisCrossVector * SinA + UnitAxis * (AxisDotVector * (1.0 - CosA))
end

-- 마우스 입력으로 카메라(=캐릭터) 회전
function RotateCamera()
    local MouseDelta = InputManager:GetMouseDelta()
    local MouseDeltaX = MouseDelta.X
    local MouseDeltaY = MouseDelta.Y

    -- 좌우 회전 (Yaw)
    local Yaw = MouseDeltaX * YawSensitivity
    ForwardVector = RotateAroundAxis(ForwardVector, UpVector, Yaw)
    ForwardVector = NormalizeCopy(ForwardVector)

    -- 상하 회전 (Pitch)
    local RightVector = FVector.Cross(UpVector, ForwardVector)
    RightVector = NormalizeCopy(RightVector) * -1.0

    local Pitch = MouseDeltaY * -PitchSensitivity
    local Candidate = RotateAroundAxis(ForwardVector, RightVector, Pitch)

    -- 수직 각도 제한
    if (Candidate.Z > 0.4) then
        Candidate.Z = 0.4
    end
    if (Candidate.Z < -0.75) then
        Candidate.Z = -0.75
    end

    ForwardVector = NormalizeCopy(Candidate)

    -- 캐릭터 회전 (지면 기준)
    local LookAt = Vector(ForwardVector.X, ForwardVector.Y, 0)
    SetPlayerForward(Obj, LookAt)
end

-- 캐릭터 앞으로 이동
function MoveForward(Delta)
    Obj.Location = Obj.Location + Vector(ForwardVector.X, ForwardVector.Y, 0) * Delta
end

function MoveRight(Delta)
    local RightVector = FVector.Cross(UpVector, ForwardVector)
    RightVector = NormalizeCopy(RightVector)
    Obj.Location = Obj.Location + Vector(RightVector.X, RightVector.Y, 0) * Delta
end


-- 카메라 위치/방향 갱신
function UpdateCamera()
    -- 1. 카메라를 캐릭터 뒤쪽에 배치
    local Camera = GetCamera()
    if Camera then
        CameraLocation = Obj.Location + (ForwardVector * -BackDistance) + (UpVector * UpDistance)
        Camera:SetLocation(CameraLocation)

        -- 2. 카메라가 캐릭터를 바라보도록 설정
        local Eye = CameraLocation
        local At = Obj.Location
        local Direction = Vector(At.X - Eye.X, At.Y - Eye.Y, At.Z - Eye.Z)
        Camera:SetCameraForward(Direction)
    end
end