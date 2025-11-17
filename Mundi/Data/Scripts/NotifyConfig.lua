-- NotifyConfig.lua
-- Key: animation file path (as used by UAnimSequenceBase::GetFilePath())
-- Value: table mapping NotifyName -> function(event)

NotifyConfig = {
    ["Data/Animations/Standard Walk.fbx"] = {
        TEST = function(event)
            print(string.format("[Lua] TEST Notify triggered at %.3f sec", event.TriggerTime))
        end,

        Footstep = function(event)
            if PlayFootstep ~= nil then
                PlayFootstep()
            else
                print("[Lua] Footstep (stub)")
            end
        end,
    },

    -- Example: add another sequence with same notify names, different behavior
    ["Data/Animations/Run.fbx"] = {
        Footstep = function(event)
            if PlayRunFootstep ~= nil then
                PlayRunFootstep()
            else
                print("[Lua] Run Footstep (stub)")
            end
        end,
    },
}

